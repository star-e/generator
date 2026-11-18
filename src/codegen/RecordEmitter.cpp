#include "codegen/RecordEmitter.h"
#include "codegen/EnumEmitter.h"
#include "codegen/TypeScriptImports.h"
#include "generated/TypeGraphTypes.h"
#include "model/IntegerLiteral.h"

#include <algorithm>
#include <iterator>
#include <locale>
#include <numeric>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string_view>

namespace cc::codegen {
namespace {

struct Dependencies {
    std::set<std::string> headers;
    std::set<std::string> forwardDeclarations;
    TypeImports typeImports;
    TypeImports valueImports;
};

struct Member {
    std::string name;
    std::string cpp;
    std::string ts;
    std::string cppDefault;
    std::string tsDefault;
    std::string comment;
    bool move{};
    bool readonly{};
    bool usesAllocator{};
};

std::string cppName(const TypeGraph& graph, const TypeNode& type, const TypeModule& module) {
    std::string scope;
    std::string name = type.qualifiedName;
    for (const auto& ns : module.namespaces) {
        scope += ns + "::";
        if (type.qualifiedName.starts_with(scope)) name = type.qualifiedName.substr(scope.size());
    }
    // Qualify from the root if a closer declaration shadows the shortened name.
    scope.clear();
    for (auto it = module.namespaces.begin(); it != module.namespaces.end(); ++it) {
        scope += *it + "::";
        const auto other = graph.findType(scope + name);
        if (other != TypeId{} && graph.node(other).qualifiedName != type.qualifiedName) return "::" + type.qualifiedName;
    }
    return name;
}

[[noreturn]] void invalid(const FieldInfo& field, const std::string& reason) {
    throw std::runtime_error(field.type.location.describe() + ": " + field.name + ": " + reason);
}

bool numericDefault(const std::string& type, const std::string& literal) {
    if (type != "float" && type != "double") return validUnsignedFieldDefault(type, literal);
    if (literal.empty()) return false;
    const auto first = literal.front() == '-' ? 1U : 0U;
    if (first == literal.size() || literal[first] < '0' || literal[first] > '9') return false;
    if (literal[first] == '0' && first + 1 < literal.size() && literal[first + 1] != '.') return false;
    std::istringstream in(literal);
    in.imbue(std::locale::classic());
    if (type == "float") {
        float value{};
        return (in >> value) && in.eof();
    }
    double value{};
    return (in >> value) && in.eof();
}

std::string nativeType(const TypeGraph& graph, const TypeModule& module, TypeId id, Dependencies& deps) {
    const auto& node = graph.node(id);
    if (const auto* ptr = std::get_if<PointerType>(&node.data)) return nativeType(graph, module, ptr->pointee, deps) + "*";
    if (const auto* instance = std::get_if<TemplateInstanceType>(&node.data)) {
        std::string name = nativeType(graph, module, instance->declaration, deps) + "<";
        for (size_t i = 0; i < instance->arguments.size(); ++i) {
            if (i) name += ", ";
            const auto& arg = instance->arguments[i];
            if (const auto* type = std::get_if<TypeId>(&arg)) name += nativeType(graph, module, *type, deps);
            else name += std::get<std::string>(arg);
        }
        return name + ">";
    }
    if (!node.cppHeader.empty()) deps.headers.insert(node.cppHeader);
    return cppName(graph, node, module);
}

Member projectMember(const TypeGraph& graph, ModuleId owner, const FieldInfo& field, Dependencies& deps,
                     bool emitTs) {
    const auto& module = graph.modules.at(owner.value);
    const auto& node = graph.node(field.type.type);
    Member m{.name = field.name, .cppDefault = field.initializer, .readonly = field.isReadonly};
    if (node.qualifiedName == "ccstd::pmr::string") {
        if (field.isNullable || !field.initializer.empty()) invalid(field, "expected a default-constructed PMR string");
        if (node.cppHeader.empty()) invalid(field, "missing native header for " + node.qualifiedName);
        m.cpp = nativeType(graph, module, field.type.type, deps);
        if (emitTs) {
            if (node.typescriptName != "string") invalid(field, "expected a TypeScript string mapping for " + node.qualifiedName);
            m.ts = *node.typescriptName;
            m.tsDefault = "''";
        }
        m.move = true;
        m.usesAllocator = true;
        return m;
    }
    const auto* instance = std::get_if<TemplateInstanceType>(&node.data);
    if (instance && graph.node(instance->declaration).qualifiedName == "ccstd::vector" &&
        instance->arguments.size() == 1 && instance->arguments.front() == TemplateArgument{graph.findType("uint8_t")}) {
        if (field.isNullable || (emitTs && !field.initializer.empty())) invalid(field, "expected a default-constructed byte vector");
        m.cpp = nativeType(graph, module, field.type.type, deps);
        deps.headers.insert("utility");
        m.move = true;
        m.ts = "Uint8Array";
        m.tsDefault = "new Uint8Array(0)";
        return m;
    }
    if (!emitTs && !field.isNullable) {
        m.cpp = nativeType(graph, module, field.type.type, deps);
        return m;
    }
    const auto* ptr = std::get_if<PointerType>(&node.data);
    if (ptr || instance) {
        if (!field.isNullable || (!field.initializer.empty() && field.initializer != "nullptr" && field.initializer != "null")) {
            invalid(field, "expected a nullable resource with a null default");
        }
        TypeId target;
        if (ptr) {
            target = ptr->pointee;
        } else {
            const auto& wrapper = graph.node(instance->declaration);
            if (wrapper.qualifiedName != "cc::IntrusivePtr" || instance->arguments.size() != 1 ||
                !std::holds_alternative<TypeId>(instance->arguments[0])) invalid(field, "unsupported resource type");
            if (wrapper.cppHeader.empty()) invalid(field, "missing native header for " + wrapper.qualifiedName);
            deps.headers.insert(wrapper.cppHeader);
            deps.headers.insert("utility");
            target = std::get<TypeId>(instance->arguments[0]);
        }
        const auto& resource = graph.node(target);
        const auto* external = std::get_if<ExternalTypeNode>(&resource.data);
        if (!external) invalid(field, "expected an external resource");
        if (emitTs) {
            if (!resource.typescriptName || resource.typescriptModule.empty()) invalid(field, "missing external resource mapping");
            deps.typeImports[resource.typescriptModule].insert(*resource.typescriptName);
            m.ts = *resource.typescriptName + " | null";
            m.tsDefault = "null";
        }
        const auto name = cppName(graph, resource, module);
        if (ptr) {
            const auto ns = resource.qualifiedName.rfind("::");
            std::string decl = std::string(external->isStruct ? "struct " : "class ") + resource.name + ";";
            if (ns != std::string::npos) decl = "namespace " + resource.qualifiedName.substr(0, ns) + " { " + decl + " }";
            deps.forwardDeclarations.insert(decl);
            m.cpp = name + "*";
            m.cppDefault = "nullptr";
            m.comment = "/*pointer*/ ";
        } else {
            if (resource.cppHeader.empty()) invalid(field, "missing native header for " + resource.qualifiedName);
            deps.headers.insert(resource.cppHeader);
            m.cpp = cppName(graph, graph.node(instance->declaration), module) + "<" + name + ">";
            m.cppDefault.clear();
            m.comment = "/*refcount*/ ";
            m.move = true;
        }
        return m;
    }
    if (field.isNullable || !node.typescriptName) invalid(field, "unsupported scalar type " + node.qualifiedName);
    if (!node.cppHeader.empty()) deps.headers.insert(node.cppHeader);
    m.cpp = cppName(graph, node, module);
    m.ts = *node.typescriptName;
    m.tsDefault = field.initializer;
    if (std::holds_alternative<EnumType>(node.data)) {
        const auto pos = field.initializer.rfind("::");
        if (pos == std::string::npos) invalid(field, "expected a named enum default");
        const auto member = field.initializer.substr(pos + 2);
        m.cppDefault = m.cpp + "::" + member;
        m.tsDefault = m.ts + "." + member;
        if (node.module != owner) {
            if (node.cppHeader.empty() || node.typescriptModule.empty()) invalid(field, "missing external enum mapping");
            deps.valueImports[node.typescriptModule].insert(m.ts);
        }
        return m;
    }
    if (m.ts == "boolean" && (m.tsDefault == "false" || m.tsDefault == "true")) return m;
    if (m.ts == "number" && numericDefault(node.qualifiedName, m.tsDefault)) return m;
    invalid(field, "unsupported scalar default " + field.initializer);
}

using Order = std::vector<uint32_t>;

void cppFlagOperators(std::ostream& out, const std::string& name, const std::string& base) {
    for (const char op : {'|', '&'}) {
        out << "\nconstexpr " << name << " operator" << op << "(const " << name << " lhs, const " << name << " rhs) noexcept {\n"
            << "    return static_cast<" << name << ">(static_cast<" << base << ">(lhs) " << op
            << " static_cast<" << base << ">(rhs));\n}\n";
    }
    for (const char op : {'|', '&'}) {
        out << "\nconstexpr " << name << "& operator" << op << "=(" << name << "& lhs, const " << name << " rhs) noexcept {\n"
            << "    return lhs = lhs " << op << " rhs;\n}\n";
    }
    out << "\nconstexpr bool operator!(" << name << " e) noexcept {\n"
        << "    return e == static_cast<" << name << ">(0);\n}\n"
        << "\nconstexpr " << name << " operator~(" << name << " e) noexcept {\n"
        << "    return static_cast<" << name << ">(~static_cast<std::underlying_type_t<" << name << ">>(e));\n}\n"
        << "\nconstexpr bool any(" << name << " e) noexcept {\n    return !!e;\n}\n";
}

void writeEnumNames(std::ostream& out, const std::string& name, const EnumType& enumeration, EnumTarget target) {
    const bool cpp = target == EnumTarget::cpp;
    if (cpp) out << "inline const char* getName(" << name << " e) noexcept {\n";
    else out << "\nexport function get" << name << "Name (e: " << name << "): string {\n";
    out << "    switch (e) {\n";
    std::set<decltype(Enumerator::value)> seen;
    for (const auto& value : enumeration.values) {
        if (!seen.insert(value.value).second) continue;
        if (cpp) out << "        case " << name << "::" << value.name << ": return \"" << value.name << "\";\n";
        else out << "    case " << name << "." << value.name << ":\n        return '" << value.name << "';\n";
    }
    if (cpp) out << "    }\n    return \"\";\n}\n";
    else out << "    default:\n        return '';\n    }\n}\n";
}

void inlineConstructors(std::ostream& out, const std::string& name, const RecordType& record,
                        const std::vector<Member>& members) {
    if (!record.constructors.empty() || record.moveOnly) out << "    " << name << "() = default;\n";
    for (const auto& ctor : record.constructors) {
        if (ctor.empty()) continue;
        out << "    " << name << "(";
        for (size_t i = 0; i < ctor.size(); ++i) {
            const auto& m = members.at(ctor[i]);
            if (i) out << ", ";
            out << m.cpp << " " << m.name << "In";
        }
        out << (ctor.size() == 1 ? ") noexcept // NOLINT\n" : ") noexcept\n");
        bool first = true;
        for (uint32_t i = 0; i < members.size(); ++i) {
            if (std::ranges::find(ctor, i) == ctor.end()) continue;
            const auto& m = members[i];
            out << (first ? "    : " : ",\n      ") << m.name << "(";
            if (m.move) out << "std::move(";
            out << m.name << "In" << (m.move ? "))" : ")");
            first = false;
        }
        out << " {}\n";
    }
    if (!record.constructors.empty() || record.moveOnly) out << '\n';
}

enum class CtorArgs { defaults, fields, move, copy };

void pmrInitializers(std::ostream& source, const std::vector<Member>& members, CtorArgs args, const Order& fields = {}) {
    bool first = true;
    for (uint32_t i = 0; i < members.size(); ++i) {
        const auto& m = members[i];
        const bool hasArg = args == CtorArgs::move || args == CtorArgs::copy ||
                            (args == CtorArgs::fields && std::ranges::find(fields, i) != fields.end());
        if (!m.usesAllocator && !hasArg) continue;
        source << (first ? "\n: " : ",\n  ") << m.name << "(";
        if (hasArg) {
            const bool move = m.move && args != CtorArgs::copy;
            if (move) source << "std::move(";
            source << (args == CtorArgs::fields ? m.name + "In" : "rhs." + m.name);
            if (move) source << ")";
            if (m.usesAllocator) source << ", ";
        }
        if (m.usesAllocator) source << "alloc";
        source << ")";
        first = false;
    }
    source << " {}\n";
}

void pmrConstructors(std::ostream& header, std::ostream& source, const std::string& name,
                     const RecordType& record, const std::vector<Member>& members, const Member& allocatorMember) {
    const std::string alloc = "const allocator_type& alloc";
    const auto defaultAlloc = alloc + " = boost::container::pmr::get_default_resource()";
    const auto constructor = "\n" + name + "::" + name + "(";
    header << "    using allocator_type = boost::container::pmr::polymorphic_allocator<char>;\n"
           << "    allocator_type get_allocator() const noexcept { // NOLINT\n"
           << "        return {" << allocatorMember.name << ".get_allocator().resource()};\n    }\n\n"
           << "    " << name << "(" << defaultAlloc << ") noexcept; // NOLINT\n";
    source << constructor << alloc << ") noexcept";
    pmrInitializers(source, members, CtorArgs::defaults);
    for (const auto& order : record.constructors) {
        if (order.empty()) continue;
        std::string params;
        for (auto index : order) {
            const auto& m = members.at(index);
            params += m.cpp + " " + m.name + "In, ";
        }
        header << "    " << name << "(" << params << defaultAlloc << ");\n";
        source << constructor << params << alloc << ") // NOLINT";
        pmrInitializers(source, members, CtorArgs::fields, order);
    }
    for (auto args : {CtorArgs::move, CtorArgs::copy}) {
        if (args == CtorArgs::copy && record.moveOnly) continue;
        const auto rhs = name + (args == CtorArgs::move ? "&& rhs" : " const& rhs");
        header << "    " << name << "(" << rhs << ", " << alloc << ");\n";
        source << constructor << rhs << ", " << alloc << ")";
        pmrInitializers(source, members, args);
    }
    header << '\n';
}

void cppEquality(std::ostream& out, const std::string& name, const std::vector<Member>& members) {
    const auto params = "(const " + name + "& lhs, const " + name + "& rhs) noexcept {\n";
    out << "\ninline bool operator==" << params;
    auto writeTuple = [&](std::string_view object) {
        out << "std::forward_as_tuple(";
        for (size_t i = 0; i < members.size(); ++i) {
            if (i) out << ", ";
            out << object << "." << members[i].name;
        }
        out << ")";
    };
    out << "    return ";
    writeTuple("lhs");
    out << " ==\n           ";
    writeTuple("rhs");
    out << ";\n}\n\ninline bool operator!=" << params << "    return !(lhs == rhs);\n}\n";
}

void cppHash(std::ostream& declarations, std::ostream& definitions, const TypeNode& node, const std::vector<Member>& members) {
    const auto& type = node.qualifiedName;
    const auto signature = "operator()(const " + type + "& val) const noexcept";
    declarations << "\ntemplate <>\nstruct hash<" << type << "> {\n    hash_t " << signature << ";\n};\n";
    definitions << "\ninline hash_t hash<" << type << ">::" << signature << " {\n    hash_t seed = 0;\n";
    for (const auto& m : members) definitions << "    hash_combine(seed, val." << m.name << ");\n";
    definitions << "    return seed;\n}\n";
}

void cppIncludes(std::ostream& out, const std::set<std::string>& includes) {
    for (const auto& include : includes) {
        if (include.find('/') == std::string::npos) out << "#include <" << include << ">\n";
        else out << "#include \"" << include << "\"\n";
    }
}

void writeNamespace(std::ostream& out, const TypeModule& module, const std::ostringstream& body) {
    for (const auto& ns : module.namespaces) out << "\nnamespace " << ns << " {\n";
    out << body.str();
    for (auto it = module.namespaces.rbegin(); it != module.namespaces.rend(); ++it) out << "\n} // namespace " << *it << "\n";
}

std::string forwardHeader(const TypeGraph& graph, const TypeModule& module, const std::ostringstream& hashes,
                          const std::string& preamble) {
    std::set<std::string> includes;
    std::ostringstream header, body;
    std::optional<bool> previousEnum;
    for (auto id : module.declarations) {
        const auto& node = graph.node(id);
        const auto* enumeration = std::get_if<EnumType>(&node.data);
        const bool isEnum = enumeration != nullptr;
        if (previousEnum != isEnum) body << '\n';
        previousEnum = isEnum;
        body << (isEnum ? "enum class " : "struct ") << node.name;
        if (enumeration && enumeration->underlying != TypeId{}) {
            const auto& base = graph.node(enumeration->underlying);
            if (!base.cppHeader.empty()) includes.insert(base.cppHeader);
            body << " : " << cppName(graph, base, module);
        }
        body << ";\n";
    }
    if (!hashes.view().empty()) {
        includes.insert("cocos/base/std/hash/hash.h");
        includes.erase("cstdint"); // The engine hash header already supplies the fixed-width integer types.
    }
    header << preamble << "#pragma once\n// clang-format off\n";
    cppIncludes(header, includes);
    writeNamespace(header, module, body);
    if (!hashes.view().empty()) header << "\nnamespace ccstd {\n" << hashes.str() << "\n} // namespace ccstd\n";
    header << "\n// clang-format on\n";
    return header.str();
}

std::string namesHeader(const TypeGraph& graph, const TypeModule& module, const std::filesystem::path& typesHeader,
                        const std::string& preamble) {
    std::ostringstream header, body;
    header << preamble << "// clang-format off\n#pragma once\n#include \"" << typesHeader.generic_string() << "\"\n";
    body << '\n';
    for (auto id : module.declarations) {
        const auto& node = graph.node(id);
        if (const auto* enumeration = std::get_if<EnumType>(&node.data)) {
            if (!enumeration->isFlags) writeEnumNames(body, node.name, *enumeration, EnumTarget::cpp);
            continue;
        }
        std::string_view name = node.name;
        if (std::holds_alternative<TagType>(node.data)) {
            if (name.size() > 3 && name.ends_with("Tag")) name.remove_suffix(3);
            else if (name.size() > 1 && name.ends_with('_')) name.remove_suffix(1);
        }
        body << "inline const char* getName(const " << node.name << "& /*v*/) noexcept { return \"" << name << "\"; }\n";
    }
    writeNamespace(header, module, body);
    header << "\n// clang-format on\n";
    return header.str();
}

void cppRecord(std::ostream& out, std::ostream& source, const TypeGraph& graph, const TypeModule& module,
               const TypeNode& node, const std::vector<Member>& members, Dependencies& deps) {
    const auto& record = std::get<RecordType>(node.data);
    out << "struct " << node.name << " {\n";
    const auto allocator = std::ranges::find(members, true, &Member::usesAllocator);
    if (allocator == members.end()) {
        inlineConstructors(out, node.name, record, members);
    } else {
        pmrConstructors(out, source, node.name, record, members, *allocator);
    }
    if (allocator != members.end() || record.moveOnly) {
        const auto& name = node.name;
        const auto moveSpec = allocator != members.end() ? " noexcept" : "";
        out << "    " << name << "(" << name << "&& rhs)" << moveSpec << " = default;\n"
            << "    " << name << "(" << name << " const& rhs) = delete;\n"
            << "    " << name << "& operator=(" << name << "&& rhs)" << moveSpec << " = default;\n"
            << "    " << name << "& operator=(" << name << " const& rhs) = " << (record.moveOnly ? "delete" : "default") << ";\n\n";
    }
    for (const auto& m : members) {
        out << "    " << m.cpp << " " << m.name;
        if (!m.cppDefault.empty()) out << "{" << m.cppDefault << "}";
        out << ";\n";
    }
    for (const auto& method : record.methods) {
        out << "    " << nativeType(graph, module, method.result.type, deps) << " " << method.name << "(";
        for (size_t i = 0; i < method.parameters.size(); ++i) {
            if (i) out << ", ";
            const auto& p = method.parameters[i];
            out << nativeType(graph, module, p.type.type, deps) << " " << p.name;
        }
        out << ")";
        if (method.isConst) out << " const";
        if (method.isNoexcept) out << " noexcept";
        out << ";\n";
    }
    out << "};\n";
}

bool inferTsType(const Member& m) {
    return m.ts == "number" || m.ts == "boolean" || m.ts == "string";
}

std::string tsParams(const std::vector<Member>& members, const Order& order, bool defaults, const std::string& sep) {
    std::string params;
    for (const auto index : order) {
        const auto& m = members.at(index);
        if (!params.empty()) params += sep;
        params += m.name;
        if (!defaults || !inferTsType(m)) params += ": " + m.ts;
        if (defaults) params += " = " + m.tsDefault;
    }
    return params;
}

Order resetParameters(const std::string& name, const std::vector<Member>& members, const Order& order,
                      const std::set<std::string>& keepOnReset, bool resetToDefaults) {
    if (resetToDefaults) {
        if (!keepOnReset.empty()) throw std::runtime_error(name + ": resetToDefaults conflicts with keepOnReset");
        return {};
    }
    auto retained = keepOnReset;
    for (const auto& m : members) {
        if (m.name == "reset") throw std::runtime_error(name + ".reset: field conflicts with reset method");
        if (retained.erase(m.name)) continue;
        if (m.readonly) throw std::runtime_error(name + "." + m.name + ": reset requires a writable field");
    }
    if (!retained.empty()) throw std::runtime_error(name + ": unknown keepOnReset field " + *retained.begin());
    auto reset = order;
    std::erase_if(reset, [&](auto index) { return keepOnReset.contains(members[index].name); });
    return reset;
}

void tsConstructorAndReset(std::ostream& ts, const std::vector<Member>& members, const Order& order,
                           const Order* reset, const std::set<std::string>& keepOnReset) {
    const auto params = tsParams(members, order, true, ", ");
    const bool multiline = params.size() + std::string_view("    constructor () {").size() > 120;
    auto signature = [&](const Order& fields, bool defaults) {
        if (multiline && !fields.empty()) return "\n        " + tsParams(members, fields, defaults, ",\n        ") + ",\n    ";
        return defaults ? params : tsParams(members, fields, false, ", ");
    };
    ts << "    constructor (" << signature(order, true) << ") {\n";
    for (uint32_t i = 0; i < members.size(); ++i) {
        if (std::ranges::find(order, i) != order.end()) ts << "        this." << members[i].name << " = " << members[i].name << ";\n";
    }
    ts << "    }\n";
    if (reset) {
        ts << "    reset (" << signature(*reset, false) << "): void {\n";
        for (uint32_t i = 0; i < members.size(); ++i) {
            const auto& m = members[i];
            if (keepOnReset.contains(m.name)) {
                if (m.ts == "Uint8Array") ts << "        // " << m.name << ": Uint8Array size unchanged\n";
                continue;
            }
            const auto& value = std::ranges::find(order, i) != order.end() ? m.name : m.tsDefault;
            ts << "        this." << m.name << " = " << value << ";\n";
        }
        ts << "    }\n";
    }
}

void tsRecord(std::ostream& ts, const TypeNode& node, const std::vector<Member>& members, const Order& order,
              const Order* reset, bool resetToDefaults, const std::set<std::string>& keepOnReset) {
    const bool initializeFields = resetToDefaults && std::get<RecordType>(node.data).constructors.empty();
    ts << "export class " << node.name << " {\n";
    if (!initializeFields) tsConstructorAndReset(ts, members, order, resetToDefaults ? nullptr : reset, keepOnReset);
    if (resetToDefaults) {
        ts << "    reset (): void {\n";
        for (const auto& m : members) {
            if (m.readonly) throw std::runtime_error(node.name + "." + m.name + ": reset requires a writable field");
            if (m.name == "reset") throw std::runtime_error(node.name + ".reset: field conflicts with reset method");
            ts << "        this." << m.name << " = " << m.tsDefault << ";\n";
        }
        ts << "    }\n";
    }
    for (uint32_t i = 0; i < members.size(); ++i) {
        const auto& m = members[i];
        if (initializeFields || std::ranges::find(order, i) == order.end()) {
            ts << "    " << (m.readonly ? "readonly " : "") << m.comment << m.name;
            if (!inferTsType(m)) ts << ": " << m.ts;
            ts << " = " << m.tsDefault << ";\n";
        } else {
            ts << "    declare " << (m.readonly ? "readonly " : "") << m.comment << m.name << ": " << m.ts << ";\n";
        }
    }
    ts << "}\n";
}

// Both writers consume this sequence in declaration order, independently of constructor and pool order.
struct ArchiveField {
    std::string name;
    std::string primitive;
    std::string cppDescription;
    std::string tsDescription;
};

std::vector<ArchiveField> archiveFields(const TypeGraph& graph, const RecordType& record, const std::vector<Member>& members) {
    std::vector<ArchiveField> fields;
    for (size_t i = 0; i < record.fields.size(); ++i) {
        const auto& field = record.fields[i];
        const auto& node = graph.node(field.type.type);
        ArchiveField entry{.name = field.name};
        if (field.transient) {
            std::string_view cpp = members[i].cpp, ts = members[i].ts;
            if (cpp.ends_with('*')) cpp.remove_suffix(1);
            if (ts.ends_with(" | null")) ts.remove_suffix(7);
            entry.cppDescription = cpp;
            entry.tsDescription = ts;
        } else {
            if (field.isReadonly) invalid(field, "archive load requires a writable field");
            std::string_view type = node.qualifiedName;
            if (const auto* enumeration = std::get_if<EnumType>(&node.data))
                type = enumeration->underlying == TypeId{} ? std::string_view{"int"} : graph.node(enumeration->underlying).qualifiedName;
            static constexpr std::string_view numbers[]{"int", "unsigned", "short", "char", "bool", "int8_t", "uint8_t",
                "int16_t", "uint16_t", "int32_t", "uint32_t", "float", "double"};
            if (node.qualifiedName == "bool") entry.primitive = "Bool";
            else if (node.qualifiedName == "ccstd::pmr::string") entry.primitive = "String";
            else if (std::ranges::find(numbers, type) != std::end(numbers)) entry.primitive = "Number";
            else invalid(field, "unsupported archive field type " + node.qualifiedName);
        }
        fields.push_back(std::move(entry));
    }
    return fields;
}

const TypeNode& archiveInterface(const TypeGraph& graph, const TypeModule& module, std::string_view name) {
    for (auto id : module.declarations) {
        const auto& type = graph.node(id);
        if (type.name == name && std::holds_alternative<InterfaceType>(type.data)) return type;
    }
    throw std::runtime_error(module.name + ": missing archive interface " + std::string(name));
}

const std::string& archiveMethod(const TypeNode& interfaceType, const std::string& method) {
    const auto& methods = std::get<InterfaceType>(interfaceType.data).methods;
    const auto found = std::ranges::find(methods, method, &MethodInfo::name);
    if (found != methods.end() && found->typescriptName) return *found->typescriptName;
    throw std::runtime_error("missing TypeScript projection for " + interfaceType.qualifiedName + "." + method);
}

struct RecordArchives {
    std::ostringstream declarations, definitions, script;

    void add(const TypeGraph& graph, const TypeModule& archive, const TypeNode& record, const std::vector<ArchiveField>& fields, bool emitTs) {
        const auto& name = record.name;
        declarations << '\n';
        for (bool save : {true, false}) {
            const std::string op = save ? "save" : "load";
            const auto& api = archiveInterface(graph, archive, save ? "OutputArchive" : "InputArchive");
            const auto archiveName = cppName(graph, api, graph.modules.at(record.module.value));
            const auto signature = "void " + op + "(" + archiveName + "& ar, " + (save ? "const " : "") + name + "& v)";
            declarations << signature << ";\n";
            definitions << '\n' << signature << " {\n";
            if (emitTs) script << "\nexport function " << op << name << " (a: " << api.name << ", v: " << name << "): void {\n";
            for (const auto& field : fields) {
                const auto member = "v." + field.name;
                if (field.primitive.empty()) {
                    definitions << "    // skip, " << field.name << ": " << field.cppDescription << '\n';
                    if (emitTs) script << "    // skip, " << member << ": " << field.tsDescription << '\n';
                    continue;
                }
                definitions << "    " << op << "(ar, " << member << ");\n";
                if (emitTs) {
                    const auto& method = archiveMethod(api, (save ? "write" : "read") + field.primitive);
                    if (save) script << "    a." << method << "(" << member << ");\n";
                    else script << "    " << member << " = a." << method << "();\n";
                }
            }
            definitions << "}\n";
            if (emitTs) script << "}\n";
        }
    }

    void emitNative(Files& files, const TypeModule& module, const RecordOutput& output,
                    const OutputLayout& archive, const std::string& preamble) const {
        const auto& path = *output.serializationHeader;
        const auto archiveHeader = (archive.includeDir / (archive.prefix + "Types.h")).generic_string();
        std::ostringstream header, source;
        header << preamble << "#pragma once\n#include \"" << archiveHeader << "\"\n#include \""
               << output.forwardHeader.value_or(output.header).generic_string() << "\"\n";
        writeNamespace(header, module, declarations);
        source << preamble << "// NOLINTBEGIN(misc-include-cleaner)\n// clang-format off\n";
        cppIncludes(source, {archiveHeader, path.generic_string(), output.header.generic_string(),
                             (archive.includeDir / "details/SerializationUtils.h").generic_string()});
        source << "// clang-format on\n";
        writeNamespace(source, module, definitions);
        source << "// NOLINTEND(misc-include-cleaner)\n";
        files.emplace(output.nativeRoot / path, header.str());
        auto cpp = output.nativeRoot / path;
        files.emplace(cpp.replace_extension(".cpp"), source.str());
    }
};

struct Pool {
    std::ostringstream resets, factories, fields;

    void add(const std::string& name, const std::string& pool, const std::vector<Member>& members, const Order& order) {
        resets << "        this." << pool << ".reset(); // " << name << "\n";
        factories << "    create" << name << " (";
        if (!order.empty()) factories << "\n        " << tsParams(members, order, true, ",\n        ") << ",\n    ";
        factories << "): " << name << " {\n        const v = this." << pool << ".add(); // " << name << "\n        v.reset(";
        for (size_t i = 0; i < order.size(); ++i) {
            if (i) factories << ", ";
            factories << members.at(order[i]).name;
        }
        factories << ");\n        return v;\n    }\n";
        fields << "    private readonly " << pool << ": RecyclePool<" << name << "> = createPool(" << name << ");\n";
    }
};

} // namespace

Files emitRecords(const TypeGraph& graph, ModuleId owner, const RecordOutput& output, std::optional<ArchiveOutput> archive,
                  const std::string& preamble) {
    const auto& module = graph.modules.at(owner.value);
    Dependencies deps;
    Pool pool;
    RecordArchives archives;
    std::ostringstream cpp, definitions, hashDeclarations, hashDefinitions, ts, nameFunctions;
    auto pendingEnums = output.namedEnums;
    auto pendingConstEnums = output.constEnums;
    auto pendingComparable = output.comparable;
    auto pendingHashes = output.hashable;
    auto pendingReset = output.resetToDefaults;
    auto pendingCtorReset = output.resetFromConstructor;
    std::string enumImports;
    size_t poolCount{};
    bool previousTag = false;
    const std::set<std::string> keepNone;
    for (auto id : module.declarations) {
        const auto& node = graph.node(id);
        if (std::holds_alternative<TagType>(node.data)) {
            if (!previousTag) cpp << '\n';
            cpp << "struct " << node.name << " {};\n";
            previousTag = true;
            continue;
        }
        previousTag = false;
        cpp << '\n';
        ts << '\n';
        if (const auto* enumeration = std::get_if<EnumType>(&node.data)) {
            cpp << "enum class " << node.name;
            std::string baseName = "int";
            if (enumeration->underlying != TypeId{}) {
                const auto& base = graph.node(enumeration->underlying);
                if (!base.cppHeader.empty()) deps.headers.insert(base.cppHeader);
                baseName = cppName(graph, base, module);
                cpp << " : " << baseName;
            }
            cpp << (enumeration->isFlags ? " { // NOLINT(performance-enum-size)\n" : " {\n");
            ts << (pendingConstEnums.erase(node.name) ? "export const enum " : "export enum ") << node.name << " {\n";
            writeEnumMembers(cpp, *enumeration, EnumTarget::cpp);
            if (output.typescript) writeEnumMembers(ts, *enumeration, EnumTarget::typescript);
            cpp << "};\n";
            if (enumeration->isFlags) {
                deps.headers.insert("type_traits");
                cppFlagOperators(cpp, node.name, baseName);
            }
            ts << "}\n";
            if (pendingEnums.erase(node.name)) {
                if (!enumImports.empty()) enumImports += ", ";
                enumImports += node.name;
                writeEnumNames(nameFunctions, node.name, *enumeration, EnumTarget::typescript);
            }
            continue;
        }
        const auto* record = std::get_if<RecordType>(&node.data);
        if (!record) throw std::runtime_error("expected record or enum: " + node.qualifiedName);
        std::vector<Member> members;
        for (const auto& field : record->fields) members.push_back(projectMember(graph, owner, field, deps, output.typescript.has_value()));
        cppRecord(cpp, definitions, graph, module, node, members, deps);
        if (record->serializable) {
            if (!archive) throw std::runtime_error(node.qualifiedName + ": @serialize requires serializationHeader");
            archives.add(graph, archive->module, node, archiveFields(graph, *record, members), output.typescript.has_value());
        }
        if (pendingHashes.erase(node.name)) cppHash(hashDeclarations, hashDefinitions, node, members);
        if (pendingComparable.erase(node.name)) {
            deps.headers.insert("tuple");
            cppEquality(cpp, node.name, members);
        }
        if (!output.typescript) continue;
        Order order(members.size());
        std::iota(order.begin(), order.end(), 0U);
        if (!record->constructors.empty()) {
            const auto ctor = std::ranges::max_element(record->constructors, {}, &Order::size);
            if (ctor->size() < members.size() && std::ranges::count(record->constructors, ctor->size(), &Order::size) > 1)
                throw std::runtime_error(node.qualifiedName + ": ambiguous partial TypeScript constructor");
            order = *ctor;
        }
        const auto entry = output.pools.find(node.name);
        const bool resetToDefaults = pendingReset.erase(node.name) != 0;
        const bool resetFromConstructor = pendingCtorReset.erase(node.name) != 0;
        if (resetToDefaults && resetFromConstructor) throw std::runtime_error(node.qualifiedName + ": conflicting reset projections");
        const auto& retained = entry == output.pools.end() ? keepNone : entry->second.keepOnReset;
        std::optional<Order> reset;
        if (entry != output.pools.end() || resetFromConstructor)
            reset = resetParameters(node.name, members, order, retained, resetToDefaults);
        if (entry != output.pools.end()) {
            ++poolCount;
            pool.add(node.name, entry->second.member, members, *reset);
        }
        tsRecord(ts, node, members, order, reset ? &*reset : nullptr, resetToDefaults, retained);
    }
    if (poolCount != output.pools.size()) throw std::runtime_error("unknown pool record in " + module.name);
    if (!pendingReset.empty()) throw std::runtime_error("unknown resetToDefaults record " + *pendingReset.begin() + " in " + module.name);
    if (!pendingCtorReset.empty()) throw std::runtime_error("unknown resetFromConstructor record " + *pendingCtorReset.begin() + " in " + module.name);
    if (!pendingComparable.empty()) throw std::runtime_error("unknown comparable record " + *pendingComparable.begin() + " in " + module.name);
    if (!pendingHashes.empty()) throw std::runtime_error("unknown hashable record " + *pendingHashes.begin() + " in " + module.name);
    if (!pendingEnums.empty()) throw std::runtime_error("unknown enum " + *pendingEnums.begin() + " in " + module.name);
    if (!pendingConstEnums.empty()) throw std::runtime_error("unknown const enum " + *pendingConstEnums.begin() + " in " + module.name);

    std::ostringstream header, script;
    header << preamble << "#pragma once\n";
    if (!output.forwardHeader && !hashDefinitions.view().empty()) deps.headers.insert("cocos/base/std/hash/hash.h");
    cppIncludes(header, deps.headers);
    if (output.forwardHeader) {
        const auto include = output.forwardHeader->lexically_relative(output.header.parent_path());
        header << "#include \"" << include.generic_string() << "\"\n";
    }
    for (const auto& decl : deps.forwardDeclarations) header << "\n" << decl << "\n";
    writeNamespace(header, module, cpp);
    if (!hashDefinitions.view().empty()) {
        header << "\nnamespace ccstd {\n";
        if (!output.forwardHeader) header << hashDeclarations.str();
        header << hashDefinitions.str() << "\n} // namespace ccstd\n";
    }

    Files files{{output.nativeRoot / output.header, header.str()}};
    if (output.jsbHeader) emitRecordJsb(files, graph, module, output, preamble);
    if (archive) archives.emitNative(files, module, output, archive->layout, preamble);
    if (output.forwardHeader) files.emplace(output.nativeRoot / *output.forwardHeader, forwardHeader(graph, module, hashDeclarations, preamble));
    if (output.namesHeader) files.emplace(output.nativeRoot / *output.namesHeader, namesHeader(graph, module, output.header, preamble));
    if (!definitions.view().empty()) {
        auto path = output.header;
        path.replace_extension(".cpp");
        std::ostringstream source;
        source << preamble << "#include \"" << output.header.filename().generic_string() << "\"\n#include <utility>\n";
        writeNamespace(source, module, definitions);
        files.emplace(output.nativeRoot / path, source.str());
    }
    if (!output.typescript) return files;
    script << preamble << "/* eslint-disable max-len */\n";
    writeImports(script, deps.valueImports, *output.typescript, false);
    writeImports(script, deps.typeImports, *output.typescript, true);
    if (poolCount) {
        script << "import { RecyclePool } from '" << modulePath(output.poolModule, *output.typescript) << "';\n";
        ts << "\nfunction createPool<T> (Constructor: new() => T): RecyclePool<T> {\n"
           << "    return new RecyclePool<T>(() => new Constructor(), 16);\n}\n"
           << "\nexport class " << module.name << "ObjectPool {\n    constructor () {\n    }\n    reset (): void {\n"
           << pool.resets.str() << "    }\n" << pool.factories.str() << pool.fields.str() << "}\n";
    }
    if (!archives.script.view().empty()) {
        auto path = archive->layout.typescript;
        script << "import type { OutputArchive, InputArchive } from '" << modulePath(path.replace_extension(), *output.typescript) << "';\n";
    }
    script << ts.str() << archives.script.str();
    files.emplace(*output.typescript, script.str());
    if (!enumImports.empty()) {
        const auto stem = output.typescript->stem().generic_string();
        const auto namesPath = output.typescript->parent_path() / (stem + "-names.ts");
        files.emplace(namesPath, preamble + "import { " + enumImports + " } from './" + stem + "';\n" + nameFunctions.str());
    }
    return files;
}

} // namespace cc::codegen
