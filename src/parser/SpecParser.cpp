#include "parser/SpecParser.h"
#include "generated/TypeGraphTypes.h"

#include <algorithm>
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
        bool hasNamespace = false;
        std::set<std::string> names;
        while (!token.empty()) {
            if (accept("binding")) {
                binding(module.bindings);
                continue;
            }
            if (hasNamespace || !accept("namespace")) fail("expected namespace or binding block");
            hasNamespace = true;
            module.namespaces = qualifiedName();
            parseDeclarations(module, external, names);
        }
        return module;
    }

private:
    void parseDeclarations(Module& module, bool external, std::set<std::string>& names) {
        expect("{");
        while (!accept("}")) {
            if (external) {
                ExternalDeclaration decl;
                while (accept("@")) {
                    const auto annotation = identifier();
                    if (annotation == "string" && !decl.isString) decl.isString = true;
                    else if (annotation == "pmr" && !decl.isPmr) decl.isPmr = true;
                    else if (annotation == "container" && decl.container == ContainerKind::None) {
                        expect("(");
                        const auto kind = quotedString("container kind");
                        if (kind == "array") decl.container = ContainerKind::Array;
                        else if (kind == "map") decl.container = ContainerKind::Map;
                        else if (kind == "set") decl.container = ContainerKind::Set;
                        else fail("unsupported container kind '" + kind + "'");
                        expect(")");
                        continue;
                    } else fail("unknown or duplicate external type annotation @" + annotation);
                    if (token == "(") fail("@" + annotation + " takes no arguments");
                }
                if (accept("template")) {
                    expect("<");
                    std::set<std::string> parameters;
                    do { decl.templateParameters.push_back(uniqueName(parameters)); } while (accept(","));
                    expect(">");
                }
                if (decl.container == ContainerKind::Array && decl.templateParameters.size() != 1)
                    fail("@container(\"array\") requires exactly one type parameter");
                if (decl.container == ContainerKind::Map && decl.templateParameters.size() != 2)
                    fail("@container(\"map\") requires exactly two type parameters");
                if (decl.container == ContainerKind::Set && decl.templateParameters.size() != 1)
                    fail("@container(\"set\") requires exactly one type parameter");
                if (decl.isString && !decl.templateParameters.empty()) fail("@string requires a non-template declaration");
                decl.isStruct = accept("struct");
                if (!decl.isStruct) expect("class");
                const auto start = location;
                module.declarations.push_back({uniqueName(names), start, std::move(decl)});
                expect(";");
            } else {
                module.declarations.push_back(declaration(names));
            }
        }
    }

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
            length = remaining.starts_with("::") || remaining.starts_with("<<") || remaining.starts_with("=>") ? 2 : 1;
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

    std::string qualifiedTypeName() {
        std::string name = accept("::") ? "::" : "";
        name += identifier();
        while (accept("::")) name += "::" + identifier();
        return name;
    }

    std::filesystem::path bindingPath() {
        const auto text = quotedString("binding path");
        const std::filesystem::path path(text);
        if (text.find_first_of("'\"\\:") != std::string::npos || path.empty() || path.has_root_path() ||
            path.filename().empty() || path.filename() == ".") fail("expected relative binding path");
        for (const auto& part : path) if (part == "..") fail("binding path escapes its root");
        return path.lexically_normal();
    }

    TypeScriptImport bindingImport() {
        auto name = identifier();
        expect("from");
        return {std::move(name), bindingPath()};
    }

    void bindingOptions(TypeScriptBinding& rule) {
        std::set<std::string> options;
        while (!accept("}")) {
            const auto key = identifier();
            if (key != "method" && key != "field" && !options.insert(key).second) fail("duplicate binding option " + key);
            if (rule.kind == BindingTarget::interfaceType && key == "method") {
                auto name = identifier();
                expect("=>");
                if (!rule.methods.emplace(name, identifier()).second) fail("duplicate method binding " + name);
            } else if (rule.kind == BindingTarget::interfaceType && key == "helpers") {
                rule.helpers = true;
            } else if (rule.kind == BindingTarget::interfaceType && key == "field") {
                auto name = identifier();
                expect("renamedFrom");
                if (!rule.renamedFrom.emplace(name, identifier()).second) fail("duplicate field binding " + name);
            } else if (rule.kind == BindingTarget::classType && key == "field") {
                const auto field = identifier();
                if (accept("reset")) {
                    expect("with");
                    if (!rule.fieldResets.emplace(field, bindingImport()).second) fail("duplicate field reset " + field);
                } else {
                    expect("default");
                    const bool negative = accept("-");
                    if (token.empty() || !std::isdigit(static_cast<unsigned char>(token.front()))) fail("expected a numeric default");
                    if (!rule.fieldDefaults.emplace(field, (negative ? "-" : "") + std::string(token)).second)
                        fail("duplicate field default " + field);
                    advance();
                }
            } else if (rule.kind == BindingTarget::classType && key == "extends") {
                rule.base = bindingImport();
            } else if (rule.kind == BindingTarget::classType && key == "mapKey") {
                expect("json");
                rule.jsonMapKey = true;
            } else if (rule.kind == BindingTarget::classType && key == "reset") {
                auto& reset = rule.reset.emplace();
                if (accept("(")) {
                    reset.kind = ResetBinding::Kind::fields;
                    if (!accept(")")) {
                        std::set<std::string> fields;
                        do { reset.fields.push_back(uniqueName(fields)); } while (accept(","));
                        expect(")");
                    }
                } else if (accept("defaults")) {
                    reset.kind = ResetBinding::Kind::defaults;
                } else {
                    expect("constructor");
                }
            } else if (rule.kind == BindingTarget::classType && key == "pool") {
                auto& pool = rule.pool.emplace();
                pool.member = identifier();
                if (accept("assign")) pool.assignFields = true;
                else if (accept("keep")) {
                    expect("(");
                    do { uniqueName(pool.keepOnReset); } while (accept(","));
                    expect(")");
                }
            } else if (rule.kind == BindingTarget::enumeration || rule.kind == BindingTarget::constEnum) {
                if (key == "names") rule.names = true;
                else if (key == "register") rule.registerEnum = true;
                else fail("unknown enum binding option " + key);
            } else fail("invalid binding option " + key);
            expect(";");
        }
    }

    TypeScriptBinding typeScriptBinding() {
        TypeScriptBinding rule{.source = type()};
        expect("=>");
        if (accept("interface")) rule.kind = BindingTarget::interfaceType;
        else if (accept("class")) {
            rule.kind = BindingTarget::classType;
            if (accept("from")) rule.from = bindingPath();
        } else if (accept("const")) { expect("enum"); rule.kind = BindingTarget::constEnum; }
        else if (accept("enum")) rule.kind = BindingTarget::enumeration;
        else {
            if (accept("extern")) {
                if (!rule.source.arguments.empty()) fail("expected a built-in TypedArray target for a template instance");
                if (!accept("class") && !accept("interface") && !accept("enum"))
                    fail("expected an external class, interface or enum binding");
            }
            rule.target = token == "\"" ? quotedString("TypeScript type") : identifier();
            if (rule.target.empty()) fail("empty TypeScript type binding");
            if (rule.target == "number" && accept("(")) {
                rule.kind = BindingTarget::numberField;
                rule.numberField = identifier();
                expect(")");
            } else if (accept("from")) {
                if (!identifierStart(rule.target.front()) || !std::ranges::all_of(rule.target, [](char c) {
                        return identifierStart(c) || std::isdigit(static_cast<unsigned char>(c));
                    })) fail("expected a named TypeScript import");
                rule.from = bindingPath();
                if (accept("archive")) {
                    expect("from");
                    rule.archiveFrom = bindingPath();
                }
            }
        }
        if (rule.kind != BindingTarget::type && accept("{")) bindingOptions(rule);
        expect(";");
        return rule;
    }

    void binding(ModuleBindings& rules) {
        if (accept("typescript")) {
            expect("{");
            while (!accept("}")) {
                if (accept("poolModule")) {
                    if (!rules.poolModule.empty()) fail("duplicate poolModule binding");
                    rules.poolModule = bindingPath();
                    expect(";");
                } else if (accept("poolDependency")) {
                    const auto member = identifier();
                    expect(":");
                    rules.poolDependencies.push_back({member, identifier()});
                    expect(";");
                } else rules.typescript.push_back(typeScriptBinding());
            }
        } else if (token == "jsb") {
            rules.jsb = location;
            advance();
            expect("{");
            while (!accept("}")) {
                if (accept("include")) {
                    if (!rules.jsbIncludes.insert(bindingPath()).second) fail("duplicate JSB include");
                } else {
                    expect("record");
                    const auto start = location;
                    const auto name = qualifiedTypeName();
                    if (!rules.jsbRecords.emplace(name, start).second) fail("duplicate JSB record " + name);
                }
                expect(";");
            }
        } else fail("expected binding target typescript or jsb");
        accept(";");
    }

    void comparison(Record& record) {
        expect("(");
        do {
            const auto op = quotedString("comparison operator");
            if (op != "==" && op != "<" && op != ">") fail("unsupported comparison operator '" + op + "'");
            if (!record.comparisons.insert(op).second) fail("duplicate comparison operator '" + op + "'");
        } while (accept(","));
        expect(")");
    }

    Declaration declaration(std::set<std::string>& names) {
        Declaration result;
        if (accept("using")) {
            result.location = location;
            result.name = uniqueName(names);
            expect("=");
            result.data = AliasDeclaration{type()};
            expect(";");
            return result;
        }
        if (accept("@")) {
            if (accept("tag")) {
                expect("struct");
                result.data = TagDeclaration{};
            } else {
                Record record;
                do {
                    const auto annotation = identifier();
                    if (annotation == "moveOnly" && !record.moveOnly) record.moveOnly = true;
                    else if (annotation == "requireAllocator" && !record.requireAllocator) record.requireAllocator = true;
                    else if (annotation == "serialize" && !record.serializable) record.serializable = true;
                    else if (annotation == "comparison" && record.comparisons.empty()) comparison(record);
                    else if (annotation == "hash" && !record.hashable) record.hashable = true;
                    else fail("unknown or duplicate record annotation @" + annotation);
                    if (token != "@" && token != "struct" && !(annotation == "serialize" && token == "graph"))
                        fail("@" + annotation + " requires a struct");
                } while (accept("@"));
                if (accept("graph")) {
                    if (record.moveOnly || record.requireAllocator || record.hashable || !record.comparisons.empty())
                        fail("only @serialize applies to a graph");
                    result.data = GraphDeclaration{.serializable = true};
                } else {
                    expect("struct");
                    result.data = std::move(record);
                }
            }
        } else if (accept("struct")) result.data = Record{};
        else if (accept("graph")) result.data = GraphDeclaration{};
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
        if (std::holds_alternative<TagDeclaration>(result.data)) {
            expect("}");
        } else if (auto* graph = std::get_if<GraphDeclaration>(&result.data)) {
            parseGraph(*graph);
        } else if (auto* record = std::get_if<Record>(&result.data)) {
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

    GraphComponent graphColumn(std::set<std::string>& names) {
        auto tag = type();
        expect(":");
        auto value = type();
        expect("{");
        expect("member");
        expect(":");
        auto member = uniqueName(names);
        expect(";");
        expect("}");
        return {std::move(tag), std::move(value), std::move(member)};
    }

    void parseGraph(GraphDeclaration& graph) {
        std::set<std::string> names;
        while (!accept("}")) {
            if (accept("vertex_descriptor")) {
                if (graph.vertexType) fail("duplicate vertex descriptor");
                graph.vertexAlias = uniqueName(names);
                expect("=");
                graph.vertexType = type();
                expect(";");
            } else if (accept("vertex_storage")) {
                if (graph.indexed) fail("duplicate vertex storage");
                expect(":");
                expect("indexed");
                expect(";");
                graph.indexed = true;
            } else if (accept("vertex_list_graph")) {
                if (graph.vertexList) fail("duplicate vertex_list_graph");
                expect(";");
                graph.vertexList = true;
            } else if (accept("mutable_graph")) {
                if (graph.mutableGraph) fail("duplicate mutable_graph");
                expect(";");
                graph.mutableGraph = true;
            } else if (accept("hierarchy_graph")) {
                if (graph.hierarchy) fail("duplicate hierarchy_graph");
                expect("{");
                expect("storage");
                expect(":");
                expect("shared_edges");
                expect(";");
                expect("}");
                graph.hierarchy = true;
            } else if (accept("named_graph")) {
                if (graph.nameComponent) fail("duplicate named_graph");
                expect("{");
                expect("component");
                expect(":");
                graph.nameComponent = type();
                expect(";");
                expect("}");
            } else if (token == "parent_graph") {
                if (graph.parentGraph) fail("duplicate parent_graph");
                graph.parentGraph = location;
                advance();
                expect(";");
            } else if (token == "addressable_graph") {
                if (graph.addressableGraph) fail("duplicate addressable_graph");
                graph.addressableGraph = location;
                advance();
                expect(";");
            } else if (accept("component")) {
                graph.components.push_back(graphColumn(names));
            } else if (accept("polymorphic_graph")) {
                if (!graph.alternatives.empty()) fail("duplicate polymorphic_graph");
                expect("{");
                if (token == "}") fail("polymorphic_graph requires at least one alternative");
                do {
                    graph.alternatives.push_back(graphColumn(names));
                } while (!accept("}"));
            } else if (token == "@" && followedBy("tag")) {
                fail("declare graph tags outside the graph");
            } else if (token == "@" || token == "readonly" || token == "nullable" || followedBy(":")) {
                graph.fields.push_back(field(names));
            } else {
                fail("expected graph descriptor, storage, capability, component reference or field");
            }
        }
        if (!graph.vertexType) fail("graph requires a vertex descriptor");
        if (graph.hierarchy && !graph.indexed) fail("shared hierarchy edges require indexed vertex storage");
        if (!graph.components.empty() && !graph.indexed) fail("components require indexed vertex storage");
        if (!graph.alternatives.empty() && !graph.indexed) fail("polymorphic_graph requires indexed vertex storage");
    }

    std::string quotedString(const std::string& description) {
        if (token != "\"") fail("expected quoted " + description);
        // The opening quote is the current token. Read its contents before skipping whitespace or comments.
        std::string message;
        while (!remaining.empty()) {
            char ch = remaining.front();
            if (ch == '\r' || ch == '\n') break;
            consume(1);
            if (ch == '"') {
                advance();
                return message;
            }
            if (ch == '\\') {
                if (remaining.empty() || (remaining.front() != '"' && remaining.front() != '\\')) {
                    fail("invalid escape in " + description);
                }
                ch = remaining.front();
                consume(1);
            }
            message.push_back(ch);
        }
        fail("unterminated " + description);
    }

    std::optional<std::string> deprecationMessage() {
        if (!accept("@")) return std::nullopt;
        expect("deprecated");
        expect("(");
        auto message = quotedString("deprecation message");
        expect(")");
        return message;
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
        if (accept("@")) {
            expect("transient");
            result.transient = true;
        }
        result.isReadonly = accept("readonly");
        result.isNullable = accept("nullable");
        result.name = uniqueName(names);
        expect(":");
        result.type = type();
        if (accept("=")) {
            const bool negative = accept("-");
            if (!token.empty() && std::isdigit(static_cast<unsigned char>(token.front()))) {
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
        TypeRef result{location, qualifiedTypeName()};
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
        const bool cppOnly = accept("@");
        if (cppOnly) expect("cppOnly");
        Method result{type(), uniqueName(names)};
        result.cppOnly = cppOnly;
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
