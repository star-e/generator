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
        if (accept("@")) {
            expect("pool");
            module.pool = true;
        }
        module.external = accept("extern");
        if (module.pool && module.external) fail("@pool requires a generated module");
        expect("module");
        module.name = identifier();
        expect(";");
        while (accept("import")) {
            module.imports.push_back({location, identifier()});
            expect(";");
        }
        if (accept("cpp")) {
            if (!module.external) fail("cpp.header requires an extern module");
            expect("{");
            expect("header");
            module.cppHeader = headerPath(false);
            expect(";");
            expect("}");
        }
        bool hasNamespace = false;
        while (!token.empty()) {
            if (accept("binding")) {
                binding(module);
                continue;
            }
            if ((hasNamespace && !module.external) || !accept("namespace")) fail("expected namespace or binding block");
            const auto parts = qualifiedName();
            if (!hasNamespace) module.namespaces = parts;
            else if (module.namespaces != parts) module.namespaces.clear();
            hasNamespace = true;
            expect("{");
            std::set<std::string> names;
            if (module.external) {
                std::string scope;
                for (const auto& part : parts) scope += (scope.empty() ? "" : "::") + part;
                parseExternalDeclarations(module, scope, names);
            } else {
                while (!accept("}")) {
                    if (token == "constexpr") module.cppConstants.push_back(cppBoolConstant(names));
                    else module.declarations.push_back(declaration(names));
                }
            }
        }
        return module;
    }

private:
    CppBoolConstant cppBoolConstant(std::set<std::string>& names) {
        CppBoolConstant result{.location = location, .documentation = documentation()};
        expect("constexpr");
        expect("bool");
        result.name = uniqueName(names);
        expect("=");
        if (accept("true")) result.value = true;
        else expect("false");
        expect(";");
        return result;
    }

    static std::string expandHeader(std::string path, std::string_view name) {
        constexpr std::string_view placeholder = "{name}";
        for (size_t pos = 0; (pos = path.find(placeholder, pos)) != std::string::npos; pos += name.size())
            path.replace(pos, placeholder.size(), name);
        return path;
    }

    std::string headerPath(bool allowName) {
        const auto at = location;
        auto spelling = quotedString("C++ header");
        const auto sample = allowName ? expandHeader(spelling, "Type") : spelling;
        if (sample.find_first_of("{}") != std::string::npos)
            throw std::runtime_error(at.describe() + (allowName ? ": expected {name} as the header placeholder"
                                                              : ": module cpp.header requires a fixed path"));
        const std::filesystem::path path(sample);
        if (sample.find_first_of("\\:\"<>") != std::string::npos || path.has_root_path() || !path.has_filename() ||
            std::ranges::any_of(sample, [](unsigned char ch) { return std::iscntrl(ch); }) ||
            std::ranges::any_of(path, [](const auto& part) { return part == "." || part == ".."; }))
            throw std::runtime_error(at.describe() + ": expected a relative C++ include path");
        return spelling;
    }

    void parseExternalDeclarations(Module& module, const std::string& scope, std::set<std::string>& names) {
        const auto append = [&](const std::string& header) {
            auto decl = (token == "enum" || token == "flags") ? declaration(names) : externalDeclaration(names);
            decl.externalScope = scope;
            decl.cppHeader = expandHeader(header, decl.name);
            module.declarations.push_back(std::move(decl));
        };
        while (!accept("}")) {
            if (accept("header")) {
                const auto header = headerPath(true);
                expect("{");
                while (!accept("}")) {
                    if (token == "header") fail("nested header blocks are not supported");
                    append(header);
                }
            } else {
                append(module.cppHeader);
            }
        }
    }

    Declaration externalDeclaration(std::set<std::string>& names) {
        auto doc = documentation();
        ExternalDeclaration decl;
        while (accept("@")) {
            const auto annotation = identifier();
            if (annotation == "string" && !decl.isString) decl.isString = true;
            else if (annotation == "pmr" && !decl.isPmr) decl.isPmr = true;
            else if (annotation == "constRef" && !decl.constructorConstRef) decl.constructorConstRef = true;
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
            do {
                decl.templateParameters.push_back(uniqueName(parameters));
                if (accept("...")) {
                    decl.variadicTemplate = true;
                    break;
                }
            } while (accept(","));
            expect(">");
        }
        if (decl.variadicTemplate && decl.container != ContainerKind::None)
            fail("a container binding requires fixed template arity");
        if (decl.container == ContainerKind::Array && decl.templateParameters.size() != 1)
            fail("@container(\"array\") requires exactly one type parameter");
        if (decl.container == ContainerKind::Map && decl.templateParameters.size() != 2)
            fail("@container(\"map\") requires exactly two type parameters");
        if (decl.container == ContainerKind::Set && decl.templateParameters.size() != 1)
            fail("@container(\"set\") requires exactly one type parameter");
        if (decl.isString && !decl.templateParameters.empty()) fail("@string requires a non-template declaration");
        decl.isStruct = accept("struct");
        decl.isAlias = !decl.isStruct && accept("type");
        if (decl.constructorConstRef && (!decl.isStruct || decl.isString || !decl.templateParameters.empty()))
            fail("@constRef requires an external value struct");
        if (!decl.isStruct && !decl.isAlias) expect("class");
        const auto start = location;
        auto name = uniqueName(names);
        if (accept("{")) {
            if (decl.isAlias) fail("an imported type alias cannot declare members");
            if (accept("public") || accept("protected")) expect(":");
            expect("virtual");
            expect("~");
            expect(name);
            expect("(");
            expect(")");
            accept("noexcept");
            expect(";");
            expect("}");
            decl.hasVirtualDestructor = true;
        }
        expect(";");
        return {.name = std::move(name), .location = start, .data = std::move(decl), .documentation = std::move(doc)};
    }

    std::string_view remaining;
    SourceLocation position;
    SourceLocation location;
    std::string_view token;
    // Trivia belongs to the next token, including when a lookahead parser is copied.
    std::string_view leadingDoc;
    size_t docIndent{};

    std::string documentation() const {
        auto rest = leadingDoc;
        std::string text;
        bool first = true;
        while (!rest.empty()) {
            const auto end = rest.find('\n');
            auto line = rest.substr(0, end);
            const auto margin = line.substr(0, line.find_first_not_of(" \t")).size();
            if (!first && line.substr(margin).starts_with('*')) {
                line.remove_prefix(margin + 1);
                if (line.starts_with(' ')) line.remove_prefix(1);
            } else {
                line.remove_prefix(std::min(margin, first ? size_t{1} : docIndent));
            }
            line = line.substr(0, line.find_last_not_of(" \t\r") + 1);
            if (!text.empty()) text += '\n';
            text += line;
            if (end == rest.npos) break;
            rest.remove_prefix(end + 1);
            first = false;
        }
        text.erase(text.find_last_not_of('\n') + 1);
        return text;
    }

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
        leadingDoc = {};
        while (!remaining.empty()) {
            if (std::isspace(static_cast<unsigned char>(remaining.front()))) {
                consume(1);
            } else if (remaining.starts_with("//")) {
                consume(remaining.substr(0, remaining.find('\n')).size());
            } else if (remaining.starts_with("/*")) {
                const auto end = remaining.find("*/", 2);
                if (end == remaining.npos)
                    throw std::runtime_error(position.describe() + ": unterminated block comment");
                if (remaining.starts_with("/**")) {
                    leadingDoc = end > 2 ? remaining.substr(3, end - 3) : std::string_view{};
                    docIndent = position.column - 1;
                }
                consume(end + 2);
            } else {
                break;
            }
        }
        location = position;
        size_t length = 0;
        if (!remaining.empty()) {
            length = remaining.starts_with("::") || remaining.starts_with("<<") || remaining.starts_with("=>") ? 2 : 1;
            if (remaining.starts_with("...")) {
                length = 3;
            } else if (remaining.starts_with("0x") || remaining.starts_with("0X")) {
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
        const auto at = location;
        return parseBindingPath(quotedString("binding path"), at.describe());
    }

    TypeScriptImport bindingImport() {
        auto name = identifier();
        expect("from");
        return {std::move(name), bindingPath()};
    }

    std::vector<std::string> bindingFields() {
        std::vector<std::string> result;
        std::set<std::string> names;
        expect("(");
        if (!accept(")")) {
            do { result.push_back(uniqueName(names)); } while (accept(","));
            expect(")");
        }
        return result;
    }

    Callback callback() {
        Callback result;
        result.name = identifier();
        result.optional = accept("?");
        expect("(");
        std::set<std::string> parameters;
        if (!accept(")")) {
            do {
                CallbackParameter parameter{uniqueName(parameters), accept("?")};
                if (!parameter.optional && !result.parameters.empty() && result.parameters.back().optional)
                    fail("required callback parameter follows an optional parameter");
                expect(":");
                parameter.type = type();
                result.parameters.push_back(std::move(parameter));
            } while (accept(","));
            expect(")");
        }
        expect(":");
        result.result = type();
        if (accept("|")) {
            expect("undefined");
            result.optionalResult = true;
        }
        return result;
    }

    // These names belong to TypeScript, including names supplied by binding imports.
    std::string tsMethodType() {
        std::string type;
        for (;;) {
            if (accept("(")) {
                type += "(" + tsMethodType() + ")";
                expect(")");
            } else {
                type += identifier();
                while (accept(".")) type += "." + identifier();
                if (accept("<")) {
                    type += "<" + tsMethodType();
                    while (accept(",")) type += ", " + tsMethodType();
                    expect(">");
                    type += '>';
                }
            }
            while (accept("[")) {
                expect("]");
                type += "[]";
            }
            if (token != "|" && token != "&") return type;
            type += token == "|" ? " | " : " & ";
            advance();
        }
    }

    TypeScriptClassMethod tsClassMethod(std::string name, std::string_view indent) {
        TypeScriptClassMethod method{.name = std::move(name)};
        expect("(");
        std::set<std::string> parameters;
        if (!accept(")")) {
            do {
                auto paramName = uniqueName(parameters);
                const bool optional = accept("?");
                if (!optional && !method.parameters.empty() && method.parameters.back().optional)
                    fail("required TypeScript parameter after an optional parameter");
                expect(":");
                method.parameters.push_back({std::move(paramName), tsMethodType(), optional});
            } while (accept(",") && token != ")");
            expect(")");
        }
        expect(":");
        method.result = tsMethodType();
        if (token != "{") fail("expected a TypeScript method body");
        method.body = tsMethodBody(indent);
        accept(";");
        return method;
    }

    bool atTsMethodSignature() const {
        auto next = *this;
        if (!next.accept("(")) return false;
        if (next.accept(")")) return next.token == ":";
        // A field-list option such as keep(source) has no parameter types.
        return !next.token.empty() && identifierStart(next.token.front()) && (next.followedBy(":") || next.followedBy("?"));
    }

    void bindingOptions(TypeScriptBinding& rule) {
        std::set<std::string> options;
        std::set<std::string> methodNames;
        while (!accept("}")) {
            const auto indent = std::string_view(token.data() - (location.column - 1), location.column - 1);
            auto doc = documentation();
            const auto key = identifier();
            if (rule.kind == BindingTarget::classType && key != "constructor" && atTsMethodSignature()) {
                if (!methodNames.insert(key).second) fail("duplicate TypeScript method " + key);
                auto method = tsClassMethod(key, indent);
                method.documentation = std::move(doc);
                rule.classMethods.emplace_back(std::move(method));
                continue;
            }
            if (key != "method" && key != "getter" && key != "field" && key != "callback" && !options.insert(key).second) fail("duplicate binding option " + key);
            if (rule.kind == BindingTarget::interfaceType && (key == "method" || key == "getter")) {
                const auto name = identifier();
                const auto [it, inserted] = rule.methods.try_emplace(name);
                if (!inserted) fail("duplicate method binding " + name);
                auto& method = it->second;
                if (accept("=>")) method.target = identifier();
                if (key == "getter") {
                    method.accessor = AccessorKind::Getter;
                    if (method.target.empty()) method.target = name;
                    if (accept("setter")) {
                        const auto setter = identifier();
                        if (!rule.methods.emplace(setter, InterfaceMethodBinding{.target = method.target, .accessor = AccessorKind::Setter}).second)
                            fail("duplicate method binding " + setter);
                    }
                }
                if (accept("optional")) method.optional = bindingFields();
                method.nullable = accept("nullable");
            } else if (rule.kind == BindingTarget::interfaceType && key == "callback") {
                rule.callbacks.push_back(callback());
                rule.callbacks.back().documentation = std::move(doc);
            } else if (rule.kind == BindingTarget::interfaceType && key == "helpers") {
                rule.helpers = true;
            } else if (rule.kind == BindingTarget::interfaceType && key == "field") {
                auto name = identifier();
                expect("renamedFrom");
                if (!rule.renamedFrom.emplace(name, identifier()).second) fail("duplicate field binding " + name);
            } else if (rule.kind == BindingTarget::classType && key == "field") {
                const auto field = identifier();
                expect("default");
                const bool negative = accept("-");
                if ((negative || token != "null") && (token.empty() || !std::isdigit(static_cast<unsigned char>(token.front()))))
                    fail("expected a numeric default or null");
                if (!rule.fieldDefaults.emplace(field, (negative ? "-" : "") + std::string(token)).second)
                    fail("duplicate field default " + field);
                advance();
            } else if (rule.kind == BindingTarget::classType && key == "extends") {
                rule.base = bindingImport();
            } else if (rule.kind == BindingTarget::classType && key == "methods") {
                if (token != "R" || !remaining.starts_with('"')) fail("expected a raw string for TypeScript methods");
                auto raw = rawStringBody("TypeScript");
                if (!raw.empty()) rule.classMethods.emplace_back(std::string(raw));
                advance();
            } else if (rule.kind == BindingTarget::classType && key == "mapKey") {
                if (accept("json")) rule.mapKey = MapKeyKind::Json;
                else if (accept("string")) rule.mapKey = MapKeyKind::String;
                else fail("expected 'json' or 'string'");
            } else if (rule.kind == BindingTarget::classType && key == "constructor") {
                rule.constructor = bindingFields();
                rule.requiredConstructor = accept("required");
            } else if (rule.kind == BindingTarget::classType && (key == "pool" || key == "assign" || key == "keep")) {
                if (!rule.pool) rule.pool.emplace();
                auto& pool = *rule.pool;
                auto reuse = key;
                if (key == "pool") {
                    pool.member = identifier();
                    if (token == "assign" || token == "keep") reuse = identifier();
                }
                if (reuse != "pool" && (pool.assignFields || !pool.keepOnReset.empty()))
                    fail("duplicate or conflicting pool reuse option");
                if (reuse == "assign") pool.assignFields = true;
                else if (reuse == "keep") {
                    expect("(");
                    do { uniqueName(pool.keepOnReset); } while (accept(","));
                    expect(")");
                }
            } else if (rule.kind == BindingTarget::enumeration || rule.kind == BindingTarget::constEnum) {
                if (key == "names") rule.names = true;
                else if (key == "register") {
                    if (rule.kind == BindingTarget::constEnum) fail("register requires a runtime enum");
                    rule.registerEnum = true;
                } else fail("unknown enum binding option " + key);
            } else fail("invalid binding option " + key);
            expect(";");
        }
    }

    TypeScriptBinding typeScriptBinding() {
        TypeScriptBinding rule{.source = type()};
        if (accept("archive")) {
            expect("from");
            rule.archiveFrom = bindingPath();
            expect(";");
            return rule;
        }
        expect("=>");
        if (accept("interface")) rule.kind = BindingTarget::interfaceType;
        else if (accept("class")) {
            rule.kind = BindingTarget::classType;
            if (accept("from")) rule.from = bindingPath();
        } else if (accept("const")) { expect("enum"); rule.kind = BindingTarget::constEnum; }
        else if (accept("enum")) rule.kind = BindingTarget::enumeration;
        else {
            rule.externalTarget = accept("extern");
            if (rule.externalTarget) {
                if (!rule.source.arguments.empty()) fail("expected a built-in TypedArray target for a template instance");
                rule.externalInterface = accept("interface");
                if (!rule.externalInterface && !accept("class") && !accept("enum"))
                    fail("expected an external class, interface or enum binding");
            }
            rule.target = token == "\"" ? quotedString("TypeScript type") : identifier();
            if (rule.target.empty()) fail("empty TypeScript type binding");
            if (rule.target == "number" && accept("(")) {
                rule.kind = BindingTarget::numberField;
                rule.numberField = identifier();
                expect(")");
            } else if (rule.externalTarget || token == "from") {
                if (!identifierStart(rule.target.front()) || !std::ranges::all_of(rule.target, [](char c) {
                        return identifierStart(c) || std::isdigit(static_cast<unsigned char>(c));
                    })) fail("expected a named TypeScript import");
                if (accept("from")) rule.from = bindingPath();
            }
            if (rule.kind == BindingTarget::type && accept("archive")) {
                expect("from");
                rule.archiveFrom = bindingPath();
            }
        }
        if (rule.kind != BindingTarget::type && accept("{")) bindingOptions(rule);
        expect(";");
        return rule;
    }

    void binding(Module& module) {
        auto& rules = module.bindings;
        if (accept("typescript")) {
            expect("{");
            while (!accept("}")) {
                if (accept("import")) {
                    if (module.external) fail("TypeScript imports require a generated module");
                    auto& item = rules.typescriptImports.emplace_back();
                    item.typeOnly = accept("type");
                    expect("{");
                    do { uniqueName(item.names); } while (accept(",") && token != "}");
                    expect("}");
                    expect("from");
                    item.from = quotedString("TypeScript module specifier");
                    if (item.from.empty() || item.from.find_first_of("'\\") != std::string::npos ||
                        std::ranges::any_of(item.from, [](unsigned char ch) { return std::iscntrl(ch); }))
                        fail("invalid TypeScript module specifier");
                    expect(";");
                } else if (accept("module")) {
                    if (!module.external || token != module.name) fail("expected the declaring extern module " + module.name);
                    if (!rules.typescriptFrom.empty()) fail("duplicate TypeScript module source for " + module.name);
                    advance();
                    expect("from");
                    rules.typescriptFrom = bindingPath();
                    expect(";");
                } else if (accept("export")) {
                    expect("{");
                    do {
                        const auto at = location;
                        const auto name = identifier();
                        if (!rules.typescriptExports.emplace(name, at).second)
                            throw std::runtime_error(at.describe() + ": duplicate TypeScript export " + name);
                    } while (accept(","));
                    expect("}");
                    expect(";");
                } else if (accept("poolModule")) {
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
        } else if (accept("swig")) {
            if (module.external || rules.swig) fail("expected one SWIG binding block on a generated module");
            auto& swig = rules.swig.emplace();
            expect("{");
            while (!accept("}")) {
                if (accept("include")) {
                    auto path = bindingPath();
                    if (std::ranges::find(swig.includes, path) != swig.includes.end()) fail("duplicate SWIG include");
                    swig.includes.push_back(std::move(path));
                } else {
                    const bool macro = accept("macro");
                    if (!macro) expect("releaseInGC");
                    auto condition = macro ? identifier() : std::string{};
                    SwigMember member{location, identifier()};
                    expect(".");
                    member.name = identifier();
                    if (macro) swig.macros.emplace_back(std::move(member), std::move(condition));
                    else swig.releaseInGC.push_back(std::move(member));
                }
                expect(";");
            }
        } else fail("expected binding target typescript, jsb or swig");
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
        result.documentation = documentation();
        if (token == "@" && followedBy("cppOnly")) {
            advance();
            expect("cppOnly");
            result.cppOnly = true;
        }
        if (accept("using")) {
            if (result.cppOnly) fail("@cppOnly requires a record, interface, graph or enum");
            result.location = location;
            result.name = uniqueName(names);
            expect("=");
            result.data = AliasDeclaration{type()};
            expect(";");
            return result;
        }
        if (accept("@")) {
            if (accept("tag")) {
                if (result.cppOnly) fail("@tag already declares a native-only type");
                expect("struct");
                result.data = TagDeclaration{};
            } else if (accept("archive")) {
                expect("(");
                auto& role = std::get<Interface>(result.data).archiveRole;
                if (accept("input")) role = ArchiveRole::input;
                else if (accept("output")) role = ArchiveRole::output;
                else fail("expected input or output archive role");
                expect(")");
                expect("interface");
            } else {
                Record record;
                do {
                    const auto annotation = identifier();
                    if (annotation == "moveOnly" && !record.moveOnly) record.moveOnly = true;
                    else if (annotation == "immovable" && !record.immovable) record.immovable = true;
                    else if (annotation == "serialize" && !record.serializable) record.serializable = true;
                    else if (annotation == "comparison" && record.comparisons.empty()) comparison(record);
                    else if (annotation == "hash" && !record.hashable) record.hashable = true;
                    else if (annotation == "cppOnly" && !result.cppOnly) result.cppOnly = true;
                    else fail("unknown or duplicate record annotation @" + annotation);
                    if (token != "@" && token != "struct" && token != "class" && token != "graph")
                        fail("@" + annotation + " requires a struct");
                } while (accept("@"));
                if (accept("graph")) {
                    if (record.moveOnly) fail("@moveOnly requires a struct");
                    if (record.immovable && record.serializable) fail("@serialize requires a replaceable graph");
                    result.data = GraphDeclaration{.immovable = record.immovable, .serializable = record.serializable,
                                                   .comparisons = std::move(record.comparisons), .hashable = record.hashable};
                } else {
                    if (record.immovable && record.moveOnly) fail("@immovable and @moveOnly cannot be combined");
                    record.isClass = accept("class");
                    if (!record.isClass) expect("struct");
                    result.data = std::move(record);
                }
            }
        } else if (accept("struct")) result.data = Record{};
        else if (accept("class")) result.data = Record{.isClass = true};
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
        } else if (auto* iface = std::get_if<Interface>(&result.data); iface && accept(":")) {
            iface->bases.push_back(type());
        } else if (auto* record = std::get_if<Record>(&result.data)) {
            record->isFinal = accept("final");
            if (accept(":")) {
                do { record->bases.push_back(type()); } while (accept(","));
            }
            if (accept("implements")) record->implements = type();
        }
        expect("{");
        std::set<std::string> members;
        if (std::holds_alternative<TagDeclaration>(result.data)) {
            expect("}");
        } else if (auto* graph = std::get_if<GraphDeclaration>(&result.data)) {
            parseGraph(*graph);
        } else if (auto* record = std::get_if<Record>(&result.data)) {
            bool cppPrivate = false;
            while (!accept("}")) {
                const auto start = location;
                if (token == "public" || token == "private") {
                    cppPrivate = token == "private";
                    advance();
                    expect(":");
                } else if (atConstructor()) {
                    Constructor ctor{start};
                    ctor.documentation = documentation();
                    ctor.cppPrivate = cppPrivate;
                    if (accept("@")) {
                        expect("cppOnly");
                        ctor.cppOnly = true;
                    }
                    expect("constructor");
                    expect("(");
                    if (!accept(")")) {
                        do { ctor.fields.push_back(identifier()); } while (accept(","));
                        expect(")");
                    }
                    ctor.defaults = accept("defaults");
                    if (!ctor.defaults) accept("required"); // Legacy spelling for mandatory parameters.
                    ctor.isNoexcept = accept("noexcept");
                    if (accept("=")) { expect("custom"); ctor.custom = true; }
                    if (ctor.isNoexcept && !ctor.custom) fail("explicit noexcept requires a custom constructor");
                    if (ctor.custom && ctor.defaults) fail("custom constructors cannot use defaults");
                    expect(";");
                    record->constructors.push_back(std::move(ctor));
                } else if (token == "~") {
                    record->destructorDocumentation = documentation();
                    advance();
                    if (record->destructorNoexcept.has_value()) fail("duplicate destructor for " + result.name);
                    if (token != result.name) fail("expected destructor name '" + result.name + "'");
                    advance();
                    expect("(");
                    expect(")");
                    record->destructorNoexcept = accept("noexcept");
                    record->destructorPrivate = cppPrivate;
                    expect(";");
                } else if (token == "cpp" && followedBy("{")) {
                    advance();
                    auto body = cppBody();
                    auto membersText = std::string_view(body).substr(1, body.size() - 2);
                    const auto end = membersText.find_last_not_of(" \t\r\n");
                    if (end != membersText.npos) {
                        membersText = membersText.substr(0, end + 1);
                        if (cppPrivate) record->cppMembers += "private:\n";
                        if (membersText.find('\n') == membersText.npos) {
                            record->cppMembers += "    ";
                            membersText.remove_prefix(membersText.find_first_not_of(" \t"));
                        } else if (membersText.front() == '\n') membersText.remove_prefix(1);
                        record->cppMembers.append(membersText);
                        record->cppMembers += '\n';
                        if (cppPrivate) record->cppMembers += "public:\n";
                    }
                } else if (token == "mutable") {
                    auto doc = documentation();
                    advance();
                    auto member = field(members, true);
                    member.documentation = std::move(doc);
                    member.isMutable = true;
                    member.cppPrivate = cppPrivate;
                    record->fields.push_back(std::move(member));
                } else if (atField()) {
                    bool isStatic = false;
                    auto member = field(members, true, &isStatic);
                    member.cppPrivate = cppPrivate;
                    (isStatic ? record->staticFields : record->fields).push_back(std::move(member));
                } else {
                    record->methods.push_back(method(members, record->methods));
                    record->methods.back().cppPrivate = cppPrivate;
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
            while (!accept("}")) {
                methods.push_back(method(members, methods, true));
            }
        }
        return result;
    }

    GraphComponent graphColumn(std::set<std::string>& names, bool alternative) {
        auto tag = type();
        auto kindName = alternative && accept("as") ? identifier() : std::string{};
        expect(":");
        auto value = type();
        expect("{");
        expect("member");
        expect(":");
        auto member = uniqueName(names);
        expect(";");
        expect("}");
        return {std::move(tag), std::move(value), std::move(member), std::move(kindName)};
    }

    void parseGraph(GraphDeclaration& graph) {
        std::set<std::string> names;
        while (!accept("}")) {
            if (atVertexDescriptorDeclaration()) {
                advance();
                if (graph.vertexType) fail("duplicate vertex descriptor");
                if (token == "=") {
                    graph.vertexAlias = "vertex_descriptor";
                    if (!names.insert(graph.vertexAlias).second) fail("duplicate name 'vertex_descriptor'");
                } else {
                    graph.vertexAlias = uniqueName(names);
                }
                expect("=");
                graph.vertexType = type();
                expect(";");
            } else if (accept("vertex_storage")) {
                if (graph.indexed) fail("duplicate vertex storage");
                expect(":");
                expect("indexed");
                if (accept("(")) {
                    graph.indexedStorage = type();
                    expect(")");
                }
                expect(";");
                graph.indexed = true;
            } else if (accept("vertex_list_graph")) {
                if (graph.vertexList) fail("duplicate vertex_list_graph");
                expect(";");
                graph.vertexList = true;
            } else if (accept("edge_list_graph")) {
                if (graph.edgeList) fail("duplicate edge_list_graph");
                expect(";");
                graph.edgeList = true;
            } else if (accept("mutable_graph")) {
                if (graph.mutableGraph) fail("duplicate mutable_graph");
                graph.mutableGraph = true;
                if (accept("{")) {
                    expect("remove_vertex");
                    expect(";");
                    expect("}");
                    graph.removeVertex = true;
                } else expect(";");
            } else if (token == "incidence_graph" || token == "bidirectional_graph") {
                const bool incoming = token == "bidirectional_graph";
                auto& enabled = incoming ? graph.bidirectional : graph.incidence;
                if (enabled) fail("duplicate " + std::string(token));
                enabled = true;
                advance();
                expect("{");
                expect(incoming ? "in_edges" : "out_edges");
                expect(":");
                expect("vector");
                expect(";");
                expect("}");
            } else if (accept("hierarchy_graph")) {
                if (graph.hierarchy) fail("duplicate hierarchy_graph");
                expect("{");
                expect("storage");
                expect(":");
                graph.separateEdges = accept("separate_edges");
                if (!graph.separateEdges && !accept("shared_edges")) fail("expected 'shared_edges' or 'separate_edges'");
                expect(";");
                expect("}");
                graph.hierarchy = true;
            } else if (token == "named_graph" || (token == "component_index" && !followedBy(":"))) {
                auto& selected = token == "named_graph" ? graph.nameComponent : graph.indexedComponent;
                if (selected) fail("duplicate " + std::string(token));
                advance();
                expect("{");
                expect("component");
                expect(":");
                selected = type();
                expect(";");
                expect("}");
            } else if (token == "name_index" && !followedBy(":")) {
                if (graph.nameIndex) fail("duplicate name_index");
                graph.nameIndex = location;
                advance();
                expect(";");
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
                graph.components.push_back(graphColumn(names, false));
            } else if (accept("polymorphic_graph")) {
                if (!graph.alternatives.empty()) fail("duplicate polymorphic_graph");
                expect("{");
                if (token == "}") fail("polymorphic_graph requires at least one alternative");
                do {
                    graph.alternatives.push_back(graphColumn(names, true));
                } while (!accept("}"));
            } else if (token == "@" && followedBy("tag")) {
                fail("declare graph tags outside the graph");
            } else if (atField()) {
                graph.fields.push_back(field(names));
            } else {
                graph.methods.push_back(method(names, graph.methods));
            }
        }
        if (!graph.vertexType) fail("graph requires a vertex descriptor");
        if (graph.bidirectional && !graph.incidence) fail("bidirectional_graph requires incidence_graph");
        if (graph.incidence && !graph.indexed) fail("incidence_graph requires indexed vertex storage");
        if (graph.incidence && graph.hierarchy && !graph.separateEdges) fail("explicit incidence storage conflicts with shared hierarchy edges");
        if (graph.edgeList && !graph.incidence && !(graph.hierarchy && !graph.separateEdges))
            fail("edge_list_graph requires incidence_graph or shared hierarchy edges");
        if (!graph.alternatives.empty() && (graph.hashable || !graph.comparisons.empty()))
            fail("polymorphic graph comparison and hashing are unsupported");
        if (graph.hierarchy && !graph.indexed) fail("hierarchy edges require indexed vertex storage");
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

    EnumMember enumMember(std::set<std::string>& members) {
        auto doc = documentation();
        const auto start = location;
        EnumMember member{.name = uniqueName(members), .initializerLocation = start, .documentation = std::move(doc)};
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

    bool atConstructor() const {
        if (token != "@") return token == "constructor" && followedBy("(");
        auto next = *this;
        next.advance();
        return next.accept("cppOnly") && next.accept("constructor") && next.token == "(";
    }

    bool atVertexDescriptorDeclaration() const {
        if (token != "vertex_descriptor") return false;
        auto next = *this;
        next.advance();
        return next.token == "=" || (!next.token.empty() && identifierStart(next.token.front()) && next.followedBy("="));
    }

    bool followedBy(std::string_view expected) const {
        auto next = *this;
        next.advance();
        return next.token == expected;
    }

    Field field(std::set<std::string>& names, bool record = false, bool* isStatic = nullptr) {
        Field result;
        result.documentation = documentation();
        while (accept("@")) {
            if (record && accept("cppOnly")) {
                if (result.cppOnly) fail("duplicate @cppOnly");
                result.cppOnly = true;
            } else if (accept("notElement")) {
                if (result.notElement) fail("duplicate @notElement");
                result.notElement = true;
            } else {
                expect("transient");
                if (result.transient) fail("duplicate @transient");
                result.transient = true;
            }
        }
        if (isStatic) *isStatic = accept("static");
        result.isReadonly = accept("readonly");
        if (token == "nullable" && !followedBy(":")) {
            result.isNullable = true;
            advance();
        }
        result.name = uniqueName(names);
        expect(":");
        result.type = type(nullptr, record);
        if (accept("=")) result.initializer = defaultValue();
        expect(";");
        return result;
    }

    std::string defaultValue() {
        const bool negative = accept("-");
        if (!token.empty() && std::isdigit(static_cast<unsigned char>(token.front()))) {
            std::string literal = negative ? "-" : "";
            literal += token;
            advance();
            return literal;
        }
        if (negative) fail("expected numeric default");
        return qualifiedTypeName();
    }

    std::string parameterDefault() {
        if (accept("{")) {
            expect("}");
            return "{}";
        }
        if (token == "\"") {
            const char* begin = token.data();
            finishLiteral('"', "unterminated C++ literal");
            std::string literal(begin, remaining.data());
            advance();
            return literal;
        }
        auto value = defaultValue();
        while (accept("|")) value += " | " + qualifiedTypeName();
        return value;
    }

    TypeRef type(bool* byConstReference = nullptr, bool allowReference = false) {
        const auto at = location;
        const bool isConst = accept("const");
        TypeRef result{at, qualifiedTypeName()};
        if (accept("<")) {
            result.hasTemplateArguments = true;
            if (token != ">") {
                do { result.arguments.push_back(type()); } while (accept(","));
            }
            expect(">");
        }
        result.pointer = accept("*");
        result.reference = allowReference && accept("&");
        if (isConst && !result.pointer && byConstReference && result.reference) {
            *byConstReference = true;
            result.reference = false;
        } else if (isConst) {
            if (!result.pointer && !result.reference) fail("expected '*' after const pointee type");
            result.constPointee = true;
        }
        return result;
    }

    bool atField() const {
        auto next = *this;
        while (next.accept("@")) {
            if (next.token != "cppOnly" && next.token != "transient" && next.token != "notElement") return false;
            next.advance();
        }
        next.accept("static");
        return next.token == "readonly" || next.token == "nullable" || next.followedBy(":");
    }

    void skipCppWord() {
        const bool number = std::isdigit(static_cast<unsigned char>(remaining.front()));
        size_t count = 1;
        while (count < remaining.size()) {
            const char ch = remaining[count];
            if (!std::isalnum(static_cast<unsigned char>(ch)) && ch != '_' && !(number && (ch == '.' || ch == '\''))) break;
            ++count;
        }
        const auto word = remaining.substr(0, count);
        consume(count);
        if (!remaining.starts_with('"') || (word != "R" && word != "u8R" && word != "uR" && word != "UR" && word != "LR")) return;

        rawStringBody("C++");
    }

    std::string_view rawStringBody(const std::string& language) {
        const auto open = remaining.find('(', 1);
        if (open == remaining.npos || open > 17) fail("invalid " + language + " raw string delimiter");
        const auto delimiter = remaining.substr(1, open - 1);
        if (delimiter.find_first_of(" )\\\t\v\f\r\n") != delimiter.npos) fail("invalid " + language + " raw string delimiter");
        const auto end = remaining.find(")" + std::string(delimiter) + "\"", open + 1);
        if (end == remaining.npos) fail("unterminated " + language + " raw string");
        const auto body = remaining.substr(open + 1, end - open - 1);
        consume(end + delimiter.size() + 2);
        return body;
    }

    void finishLiteral(char quote, const char* diagnostic) {
        while (!remaining.empty() && remaining.front() != quote) {
            if (remaining.front() == '\n' || remaining.front() == '\r') fail(diagnostic);
            if (remaining.front() == '\\') {
                consume(1);
                if (remaining.empty()) fail(diagnostic);
                if (remaining.starts_with("\r\n")) consume(1);
            }
            consume(1);
        }
        if (remaining.empty()) fail(diagnostic);
        consume(1);
    }

    // Keep the native body verbatim. Braces in literals and comments do not
    // participate in matching the brace that returns control to the spec lexer.
    std::string cppBody() {
        const char* start = token.data();
        const auto opening = location;
        size_t depth = 1;
        while (!remaining.empty()) {
            location = position;
            const char ch = remaining.front();
            if (remaining.starts_with("//")) {
                size_t end = remaining.find('\n');
                while (end != remaining.npos && end > 0 &&
                       (remaining[end - 1] == '\\' || (end > 1 && remaining.substr(end - 2, 2) == "\\\r")))
                    end = remaining.find('\n', end + 1);
                consume(remaining.substr(0, end).size());
            } else if (remaining.starts_with("/*")) {
                const auto end = remaining.find("*/", 2);
                if (end == remaining.npos) fail("unterminated C++ block comment");
                consume(end + 2);
            } else if (identifierStart(ch) || std::isdigit(static_cast<unsigned char>(ch))) {
                skipCppWord();
            } else if (ch == '"' || ch == '\'') {
                consume(1);
                finishLiteral(ch, "unterminated C++ literal");
            } else {
                consume(1);
                if (ch == '{') ++depth;
                if (ch == '}' && --depth == 0) {
                    std::string body(start, remaining.data());
                    advance();
                    return body;
                }
            }
        }
        location = opening;
        fail("unterminated C++ method body");
    }

    void tsRegexLiteral() {
        consume(1);
        bool inClass = false;
        while (!remaining.empty()) {
            const char ch = remaining.front();
            if (ch == '\n' || ch == '\r') break;
            consume(1);
            if (ch == '\\' && !remaining.empty()) consume(1);
            else if (ch == '[') inClass = true;
            else if (ch == ']') inClass = false;
            else if (ch == '/' && !inClass) {
                while (!remaining.empty() && identifierStart(remaining.front())) consume(1);
                return;
            }
        }
        fail("unterminated TypeScript regular expression");
    }

    void tsTemplateLiteral() {
        consume(1);
        while (!remaining.empty()) {
            const char ch = remaining.front();
            consume(1);
            if (ch == '`') return;
            if (ch == '\\' && !remaining.empty()) consume(1);
            else if (ch == '$' && remaining.starts_with('{')) {
                consume(1);
                tsBracedBody(nullptr);
            }
        }
        fail("unterminated TypeScript template literal");
    }

    // Locate the closing brace without interpreting statements. Only code lines are
    // reindented when moving the method from its binding block into a generated class.
    void tsBracedBody(std::vector<const char*>* codeLines) {
        const auto opening = location;
        bool expressionStart = true;
        bool statementStart = codeLines != nullptr; // A template substitution starts with an expression.
        bool controlKeyword = false;
        bool propertyName = false;
        std::vector<bool> controlParens;
        std::vector<bool> statementBraces;
        while (!remaining.empty()) {
            location = position;
            const char ch = remaining.front();
            if (std::isspace(static_cast<unsigned char>(ch))) {
                consume(1);
                if (ch == '\n' && codeLines) codeLines->push_back(remaining.data());
                continue;
            }
            if (remaining.starts_with("//")) {
                consume(remaining.substr(0, remaining.find('\n')).size());
                continue;
            }
            if (remaining.starts_with("/*")) {
                const auto end = remaining.find("*/", 2);
                if (end == remaining.npos) fail("unterminated TypeScript block comment");
                consume(end + 2);
                continue;
            }
            if (identifierStart(ch) || ch == '$' || std::isdigit(static_cast<unsigned char>(ch))) {
                size_t length = 1;
                while (length < remaining.size() && (identifierStart(remaining[length]) || remaining[length] == '$' ||
                       std::isdigit(static_cast<unsigned char>(remaining[length])))) ++length;
                const auto word = remaining.substr(0, length);
                consume(length);
                controlKeyword = !propertyName && (word == "if" || word == "while" || word == "for" || word == "with" || word == "switch" || word == "catch");
                statementStart = !propertyName && (word == "else" || word == "do" || word == "try" || word == "finally" || word == "catch");
                expressionStart = statementStart || (!propertyName && (word == "return" || word == "throw" || word == "case" ||
                    word == "delete" || word == "typeof" || word == "void" || word == "yield" || word == "await" ||
                    word == "in" || word == "instanceof" || word == "of"));
                propertyName = false;
                continue;
            }
            const bool startsStatementBlock = statementStart;
            statementStart = false;
            propertyName = false;
            if (ch == '\'' || ch == '"') {
                consume(1);
                finishLiteral(ch, "unterminated TypeScript string literal");
                expressionStart = false;
            } else if (ch == '`') {
                tsTemplateLiteral();
                expressionStart = false;
            } else if (ch == '/' && expressionStart) {
                tsRegexLiteral();
                expressionStart = false;
            } else if (ch == '!' && !remaining.starts_with("!=")) {
                consume(1); // Both prefix negation and a postfix non-null assertion preserve the expression context.
            } else if (remaining.starts_with("++") || remaining.starts_with("--")) {
                consume(2); // Prefix and postfix operators preserve the prior expression context.
            } else if (remaining.starts_with("...")) {
                consume(3);
                expressionStart = true;
            } else {
                consume(1);
                if (ch == '(') controlParens.push_back(controlKeyword);
                if (ch == '{') {
                    statementBraces.push_back(startsStatementBlock);
                    statementStart = expressionStart = true;
                } else if (ch == '}') {
                    if (statementBraces.empty()) return;
                    statementStart = expressionStart = statementBraces.back();
                    statementBraces.pop_back();
                } else if (ch == ')' && !controlParens.empty()) {
                    statementStart = expressionStart = controlParens.back();
                    controlParens.pop_back();
                } else {
                    propertyName = ch == '.';
                    statementStart = ch == ';';
                    expressionStart = ch != ')' && ch != ']' && !propertyName;
                }
            }
            controlKeyword = false;
        }
        location = opening;
        fail("unterminated TypeScript method body");
    }

    std::string tsMethodBody(std::string_view indent) {
        const char* start = token.data();
        std::vector<const char*> codeLines;
        tsBracedBody(&codeLines);
        std::string body;
        if (indent.find_first_not_of(" \t") == indent.npos) {
            for (const auto* line : codeLines) {
                const std::string_view rest(line, remaining.data());
                if (!rest.starts_with(indent) || rest.empty() || rest.front() == '\n' || rest.front() == '\r') continue;
                body.append(start, line);
                body += "    ";
                start = line + indent.size();
            }
        }
        body.append(start, remaining.data());
        advance();
        return body;
    }

    Method method(std::set<std::string>& names, const std::vector<Method>& methods, bool inInterface = false) {
        Method result;
        result.documentation = documentation();
        bool cppOnly = false;
        while (accept("@")) {
            const auto annotation = identifier();
            if (annotation == "cppOnly" && !cppOnly) cppOnly = true;
            else if (annotation == "getter" && result.accessor == AccessorKind::None) result.accessor = AccessorKind::Getter;
            else if (annotation == "setter" && result.accessor == AccessorKind::None) result.accessor = AccessorKind::Setter;
            else if (annotation == "optionalMethod" && !result.optionalMethod) result.optionalMethod = true;
            else if (annotation == "nullable" && result.resultPresence == ResultPresence::Required) result.resultPresence = ResultPresence::Nullable;
            else if (annotation == "optionalResult" && result.resultPresence == ResultPresence::Required) result.resultPresence = ResultPresence::Optional;
            else fail("unknown or duplicate method annotation @" + annotation);
        }
        result.explicitCppOnly = cppOnly;
        result.isPureVirtual = accept("virtual");
        result.isStatic = accept("static");
        if (result.isPureVirtual && !inInterface) fail("pure virtual methods require an interface");
        if ((result.isPureVirtual && result.isStatic) || (result.isStatic && token == "virtual"))
            fail("static method cannot be virtual");
        result.isExplicitConversion = accept("explicit");
        if (result.isExplicitConversion) {
            if (result.isStatic) fail("a conversion operator cannot be static");
            expect("operator");
        }
        result.result = type(nullptr, true);
        if (!result.isExplicitConversion) {
            if (accept("operator")) {
                if (result.isStatic) fail("a call operator cannot be static in C++17");
                expect("(");
                expect(")");
                result.name = "operator()";
                cppOnly = true;
            } else {
                result.name = std::ranges::find(methods, token, &Method::name) == methods.end() ? uniqueName(names) : identifier();
            }
        }
        result.cppOnly = cppOnly || result.isExplicitConversion || !result.isPureVirtual;
        if (result.accessor != AccessorKind::None && result.cppOnly)
            fail("@getter and @setter require a TypeScript-visible pure virtual method");
        if ((result.optionalMethod || result.resultPresence != ResultPresence::Required) && result.cppOnly)
            fail("TypeScript optionality requires a visible pure virtual method");
        if (result.optionalMethod && result.accessor != AccessorKind::None)
            fail("@optionalMethod requires a regular method");
        expect("(");
        std::set<std::string> parameters;
        if (!accept(")")) {
            if (result.isExplicitConversion) fail("a conversion operator cannot take parameters");
            do {
                bool byConstReference = false;
                Parameter parameter{type(&byConstReference, true), uniqueName(parameters), byConstReference};
                if (accept("=")) parameter.defaultValue = parameterDefault();
                else if (!result.parameters.empty() && !result.parameters.back().defaultValue.empty())
                    fail("parameters after a default argument must also have defaults");
                result.parameters.push_back(std::move(parameter));
            } while (accept(","));
            expect(")");
        }
        result.isConst = accept("const");
        if (result.isStatic && result.isConst) fail("static method cannot be const");
        result.isNoexcept = accept("noexcept");
        if (result.isPureVirtual) {
            expect("=");
            expect("0");
            expect(";");
        } else if (token == "{") {
            result.cppBody = cppBody();
            accept(";");
        } else {
            if (token == "=") fail("pure virtual method requires 'virtual'");
            expect(";");
        }
        return result;
    }
};

} // namespace

std::filesystem::path parseBindingPath(const std::string& text, const std::string& context) {
    const std::filesystem::path path(text);
    if (text.find_first_of("'\"\\:") != std::string::npos ||
        std::ranges::any_of(text, [](unsigned char ch) { return std::iscntrl(ch); }) ||
        path.has_root_path() || !path.has_filename() || path.filename() == ".")
        throw std::runtime_error(context + ": expected relative binding path");
    for (const auto& part : path)
        if (part == "..") throw std::runtime_error(context + ": binding path escapes its root");
    return path.lexically_normal();
}

Module parseModule(const std::filesystem::path& file, const std::string& source) {
    return Parser(file, source).parse();
}

} // namespace cc::codegen
