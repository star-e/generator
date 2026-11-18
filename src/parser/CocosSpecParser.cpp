#include "parser/CocosSpecParser.h"

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
        expect("module");
        module.name = identifier();
        expect(";");
        expect("namespace");
        module.namespaces = qualifiedName();
        expect("{");
        std::set<std::string> names;
        while (!accept("}")) {
            expect("interface");
            Interface declaration;
            declaration.name = uniqueName(names);
            expect("{");
            std::set<std::string> methods;
            while (!accept("}")) {
                declaration.methods.push_back(method(methods));
            }
            module.interfaces.push_back(std::move(declaration));
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
            length = remaining.starts_with("::") ? 2 : 1;
            if (identifierStart(remaining.front())) {
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

    TypeRef type() {
        TypeRef result{location, identifier()};
        while (accept("::")) result.name += "::" + identifier();
        result.pointer = accept("*");
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
