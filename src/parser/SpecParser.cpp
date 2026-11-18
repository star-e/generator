#include "parser/SpecParser.h"

#include <cctype>
#include <set>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace cc::codegen {

std::string SourceLocation::describe() const {
    return file.string() + ":" + std::to_string(line) + ":" + std::to_string(column);
}

namespace {

bool identifierStart(char ch) {
    return std::isalpha(static_cast<unsigned char>(ch)) || ch == '_';
}

class Parser {
public:
    Parser(const std::filesystem::path& file, std::string_view source)
    : remaining(source), position{file, 1, 1}, location(position) {
        advance();
    }

    Module parse() {
        Module module;
        const bool external = accept("extern");
        expect("module");
        module.name = identifier();
        expect(";");
        while (accept("import")) {
            module.imports.push_back({location, identifier()});
            expect(";");
        }
        expect("namespace");
        module.namespaces = qualifiedName();
        expect("{");
        std::set<std::string> names;
        while (!accept("}")) {
            if (external) {
                const bool isStruct = accept("struct");
                if (!isStruct) expect("class");
                const auto start = location;
                module.declarations.push_back({uniqueName(names), start, ExternalDeclaration{isStruct}});
                expect(";");
            } else {
                module.declarations.push_back(declaration(names));
            }
        }
        if (!token.empty()) {
            fail("unexpected token after namespace");
        }
        return module;
    }

private:
    std::string_view remaining;
    SourceLocation position;
    SourceLocation location;
    std::string_view token;

    void consume(size_t count) {
        for (char ch : remaining.substr(0, count)) {
            if (ch == '\n') {
                ++position.line;
                position.column = 1;
            } else {
                ++position.column;
            }
        }
        remaining.remove_prefix(count);
    }

    void advance() {
        while (!remaining.empty()) {
            if (std::isspace(static_cast<unsigned char>(remaining.front()))) {
                consume(1);
            } else if (remaining.starts_with("//")) {
                consume(remaining.substr(0, remaining.find('\n')).size());
            } else {
                break;
            }
        }
        location = position;
        size_t length = 0;
        if (!remaining.empty()) {
            length = remaining.starts_with("::") || remaining.starts_with("<<") ? 2 : 1;
            if (remaining.starts_with("0x") || remaining.starts_with("0X")) {
                length = 2;
                while (length < remaining.size() && std::isalnum(static_cast<unsigned char>(remaining[length]))) ++length;
            } else if (std::isdigit(static_cast<unsigned char>(remaining.front()))) {
                while (length < remaining.size() && std::isdigit(static_cast<unsigned char>(remaining[length]))) ++length;
                if (length + 1 < remaining.size() && remaining[length] == '.' &&
                    std::isdigit(static_cast<unsigned char>(remaining[length + 1]))) {
                    ++length;
                    while (length < remaining.size() && std::isdigit(static_cast<unsigned char>(remaining[length]))) ++length;
                }
            } else if (identifierStart(remaining.front())) {
                while (length < remaining.size() &&
                       (identifierStart(remaining[length]) ||
                        std::isdigit(static_cast<unsigned char>(remaining[length])))) {
                    ++length;
                }
            }
        }
        token = remaining.substr(0, length);
        consume(length);
    }

    [[noreturn]] void fail(const std::string& message) const {
        throw std::runtime_error(location.describe() + ": " + message);
    }

    bool accept(std::string_view expected) {
        if (token != expected) return false;
        advance();
        return true;
    }

    void expect(std::string_view expected) {
        if (!accept(expected)) fail("expected '" + std::string(expected) + "'");
    }

    std::string identifier() {
        if (token.empty() || !identifierStart(token.front())) fail("expected identifier");
        std::string result(token);
        advance();
        return result;
    }

    std::string uniqueName(std::set<std::string>& names) {
        if (!names.emplace(token).second) fail("duplicate name '" + std::string(token) + "'");
        return identifier();
    }

    std::vector<std::string> qualifiedName() {
        std::vector<std::string> parts{identifier()};
        while (accept("::")) parts.push_back(identifier());
        return parts;
    }

    Declaration declaration(std::set<std::string>& names) {
        Declaration result;
        if (accept("struct")) result.data = Record{};
        else if (accept("flags")) result.data = Enum{.isFlags = true};
        else if (accept("enum")) {
            expect("class");
            result.data = Enum{};
        } else {
            expect("interface");
        }
        result.location = location;
        result.name = uniqueName(names);
        if (auto* enumeration = std::get_if<Enum>(&result.data); enumeration && accept(":")) {
            enumeration->underlying = type();
        }
        expect("{");
        std::set<std::string> members;
        if (auto* record = std::get_if<Record>(&result.data)) {
            while (!accept("}")) {
                const auto start = location;
                if (token == "constructor" && followedBy("(")) {
                    advance();
                    Constructor ctor{start};
                    expect("(");
                    if (!accept(")")) {
                        do { ctor.fields.push_back(identifier()); } while (accept(","));
                        expect(")");
                    }
                    expect(";");
                    record->constructors.push_back(std::move(ctor));
                } else if (token == "fn" && !followedBy(":")) {
                    advance();
                    record->methods.push_back(recordMethod(members));
                } else {
                    record->fields.push_back(field(members));
                }
            }
        } else if (auto* enumeration = std::get_if<Enum>(&result.data)) {
            while (!accept("}")) {
                enumeration->values.push_back(enumMember(members));
                if (!accept(",")) {
                    expect("}");
                    break;
                }
            }
        } else {
            auto& methods = std::get<Interface>(result.data).methods;
            while (!accept("}")) methods.push_back(method(members));
        }
        return result;
    }

    std::optional<std::string> deprecationMessage() {
        if (!accept("@")) return std::nullopt;
        expect("deprecated");
        expect("(");
        if (token != "\"") fail("expected quoted deprecation message");
        // The opening quote is the current token. Read its contents before skipping whitespace or comments.
        std::string message;
        while (!remaining.empty()) {
            char ch = remaining.front();
            if (ch == '\r' || ch == '\n') break;
            consume(1);
            if (ch == '"') {
                advance();
                expect(")");
                return message;
            }
            if (ch == '\\') {
                if (remaining.empty() || (remaining.front() != '"' && remaining.front() != '\\')) {
                    fail("invalid escape in deprecation message");
                }
                ch = remaining.front();
                consume(1);
            }
            message.push_back(ch);
        }
        fail("unterminated deprecation message");
    }

    EnumMember enumMember(std::set<std::string>& members) {
        auto message = deprecationMessage();
        const auto start = location;
        EnumMember member{.name = uniqueName(members), .initializerLocation = start, .deprecationMessage = std::move(message)};
        if (!accept("=")) return member;
        member.initializerLocation = location;
        const bool negative = accept("-");
        if (token.empty() || !std::isdigit(static_cast<unsigned char>(token.front()))) {
            if (negative) fail("expected integer literal");
            member.initializer = identifier();
            if (token == "<<") fail("expected an integer literal as the left shift operand");
            return member;
        }
        if (negative) member.initializer = "-";
        member.initializer += token;
        advance();
        if (accept("<<")) {
            if (token.empty() || !std::isdigit(static_cast<unsigned char>(token.front()))) {
                fail("expected nonnegative integer shift count");
            }
            member.leftShift = LeftShift{std::string(token), location};
            advance();
        }
        return member;
    }

    bool followedBy(std::string_view expected) const {
        auto next = *this;
        next.advance();
        return next.token == expected;
    }

    Field field(std::set<std::string>& names) {
        Field result;
        result.isReadonly = accept("readonly");
        result.isNullable = accept("nullable");
        result.name = uniqueName(names);
        expect(":");
        result.type = type();
        if (accept("=")) {
            const bool negative = accept("-");
            if (!token.empty() && std::isdigit(static_cast<unsigned char>(token.front()))) {
                if (token.starts_with("0x") || token.starts_with("0X")) fail("expected decimal field default");
                if (negative) result.initializer = "-";
                result.initializer += token;
                advance();
            } else {
                if (negative) fail("expected numeric default");
                result.initializer = identifier();
                while (accept("::")) result.initializer += "::" + identifier();
            }
        }
        expect(";");
        return result;
    }

    TypeRef type() {
        TypeRef result{location, identifier()};
        while (accept("::")) result.name += "::" + identifier();
        if (accept("<")) {
            do {
                result.arguments.push_back(type());
            } while (accept(","));
            expect(">");
        }
        result.pointer = accept("*");
        return result;
    }

    Method recordMethod(std::set<std::string>& names) {
        Method result;
        result.name = uniqueName(names);
        expect("(");
        std::set<std::string> parameters;
        if (!accept(")")) {
            do {
                auto name = uniqueName(parameters);
                expect(":");
                result.parameters.push_back({type(), std::move(name)});
            } while (accept(","));
            expect(")");
        }
        result.isConst = accept("const");
        result.isNoexcept = accept("noexcept");
        expect("-");
        expect(">");
        result.result = type();
        expect(";");
        return result;
    }

    Method method(std::set<std::string>& names) {
        Method result{type(), uniqueName(names)};
        expect("(");
        std::set<std::string> parameters;
        if (!accept(")")) {
            do {
                result.parameters.push_back({type(), uniqueName(parameters)});
            } while (accept(","));
            expect(")");
        }
        result.isConst = accept("const");
        result.isNoexcept = accept("noexcept");
        expect(";");
        return result;
    }
};

} // namespace

Module parseModule(const std::filesystem::path& file, const std::string& source) {
    return Parser(file, source).parse();
}

} // namespace cc::codegen
