#include "codegen/RecordEmitter.h"
#include "codegen/ArchiveEmitter.h"
#include "codegen/EnumEmitter.h"
#include "codegen/GraphEmitter.h"
#include "codegen/TypeScriptImports.h"
#include "generated/TypeGraphTypes.h"
#include "model/IntegerLiteral.h"
#include "model/TypeName.h"

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
    bool checkDefaultConstruction{};
    std::string_view tsClear;
};

std::string cppName(const TypeGraph& graph, const TypeNode& type, const TypeModule& module) {
    std::string scope;
    std::string name = type.qualifiedName;
    for (const auto& ns : module.namespaces) {
        scope += ns + "::";
        if (type.qualifiedName.starts_with(scope)) name = type.qualifiedName.substr(scope.size());
    }
    return qualifyIfShadowed(graph, type, module, std::move(name));
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
    return formatTypeName(graph, id, [&](const TypeNode& type) {
        if (!type.cppHeader.empty()) deps.headers.insert(type.cppHeader);
        return cppName(graph, type, module);
    });
}

bool hasPmrStorage(const TypeGraph& graph, TypeId id, std::set<uint32_t>& visited) {
    const auto& data = graph.node(id).data;
    if (const auto* external = std::get_if<ExternalTypeNode>(&data)) return external->isPmr;
    if (const auto* instance = std::get_if<TemplateInstanceType>(&data))
        return std::get<ExternalTypeNode>(graph.node(instance->declaration).data).isPmr;
    const auto* record = std::get_if<RecordType>(&data);
    return record && visited.insert(id.value).second && std::ranges::any_of(record->fields, [&](const FieldInfo& field) {
        return hasPmrStorage(graph, field.type.type, visited);
    });
}

Member bindMember(const TypeGraph& graph, ModuleId owner, const FieldInfo& field, Dependencies& deps,
                     bool emitTs) {
    const auto& module = graph.modules.at(owner.value);
    const auto& node = graph.node(field.type.type);
    Member m{.name = field.name, .cppDefault = field.initializer, .readonly = field.isReadonly};
    if (const auto* external = std::get_if<ExternalTypeNode>(&node.data); external && external->isString) {
        if (field.isNullable || !field.initializer.empty()) invalid(field, "expected a default-constructed owning string");
        if (node.cppHeader.empty()) invalid(field, "missing native header for " + node.qualifiedName);
        m.cpp = nativeType(graph, module, field.type.type, deps);
        if (emitTs) {
            m.ts = node.typescriptName.value();
            if (!node.typescriptModule.empty()) deps.typeImports[node.typescriptModule].insert(m.ts);
            m.tsDefault = "''";
        }
        m.move = true;
        m.usesAllocator = external->isPmr;
        return m;
    }
    const auto* instance = std::get_if<TemplateInstanceType>(&node.data);
    if (instance && (node.typescriptName || node.typescriptContainer != ContainerKind::None)) {
        if (field.isNullable || (emitTs && !field.initializer.empty())) invalid(field, "expected a default-constructed container field");
        m.cpp = nativeType(graph, module, field.type.type, deps);
        if (node.typescriptContainer == ContainerKind::None) deps.headers.insert("utility");
        m.move = true;
        const auto& container = std::get<ExternalTypeNode>(graph.node(instance->declaration).data);
        m.usesAllocator = container.isPmr;
        m.checkDefaultConstruction = isAssociativeContainer(container.container);
        if (emitTs) {
            m.ts = boundTypeScriptName(graph, field.type, owner, deps.typeImports);
            m.tsDefault = typeScriptContainerInitializer(node);
            if (node.typescriptContainer == ContainerKind::Array) m.tsClear = ".length = 0";
            if (isAssociativeContainer(node.typescriptContainer)) m.tsClear = ".clear()";
        }
        return m;
    }
    if (std::holds_alternative<RecordType>(node.data) && !field.isNullable && field.initializer.empty()) {
        m.cpp = nativeType(graph, module, field.type.type, deps);
        std::set<uint32_t> visited;
        m.usesAllocator = hasPmrStorage(graph, field.type.type, visited);
        m.checkDefaultConstruction = true;
        m.move = true;
        if (emitTs) {
            if (node.module != owner && node.typescriptModule.empty()) invalid(field, "missing external record mapping");
            m.ts = boundTypeScriptName(graph, field.type, owner, deps.valueImports);
            m.tsDefault = "new " + m.ts + "()";
        }
        return m;
    }
    if (!emitTs && !field.isNullable) {
        m.cpp = nativeType(graph, module, field.type.type, deps);
        return m;
    }
    const auto* ptr = std::get_if<PointerType>(&node.data);
    if (ptr || instance) {
        if (instance && !field.isNullable)
            invalid(field, "missing TypeScript binding for " + qualifiedTypeName(graph, field.type.type));
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
                        const std::vector<Member>& members, std::set<std::string>& headers) {
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
            if (m.move) {
                headers.insert("utility");
                out << "std::move(";
            }
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
    const auto allocParameter = record.requireAllocator ? alloc : alloc + " = boost::container::pmr::get_default_resource()";
    const auto constructor = "\n" + name + "::" + name + "(";
    std::string checks;
    for (const auto& member : members) {
        if (!member.checkDefaultConstruction) continue;
        if (!checks.empty()) checks += " && ";
        checks += "noexcept(decltype(" + member.name + ")(" + (member.usesAllocator ? "alloc" : "") + "))";
    }
    const auto exceptionSpec = checks.empty() ? " noexcept" : " noexcept(" + checks + ")";
    header << "    using allocator_type = boost::container::pmr::polymorphic_allocator<char>;\n"
           << "    allocator_type get_allocator() const noexcept { // NOLINT\n"
           << "        return {" << allocatorMember.name << ".get_allocator().resource()};\n    }\n\n"
           << "    " << name << "(" << allocParameter << ")" << exceptionSpec << "; // NOLINT\n";
    source << constructor << alloc << ")" << exceptionSpec;
    pmrInitializers(source, members, CtorArgs::defaults);
    for (const auto& order : record.constructors) {
        if (order.empty()) continue;
        std::string params;
        for (auto index : order) {
            const auto& m = members.at(index);
            params += m.cpp + " " + m.name + "In, ";
        }
        header << "    " << name << "(" << params << allocParameter << ");\n";
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

void cppComparisons(std::ostream& out, const std::string& name, const std::vector<Member>& members, const RecordType& record) {
    const auto params = "(const " + name + "& lhs, const " + name + "& rhs) noexcept {\n";
    auto writeTuple = [&](std::string_view object) {
        out << "std::forward_as_tuple(";
        for (size_t i = 0; i < members.size(); ++i) {
            if (i) out << ", ";
            out << object << "." << members[i].name;
        }
        out << ")";
    };
    auto writeComparison = [&](std::string_view op) {
        out << "\ninline bool operator" << op << params << "    return ";
        writeTuple("lhs");
        out << ' ' << op << "\n           ";
        writeTuple("rhs");
        out << ";\n}\n";
    };
    if (record.compareEqual) {
        writeComparison("==");
        out << "\ninline bool operator!=" << params << "    return !(lhs == rhs);\n}\n";
    }
    if (record.compareLess) writeComparison("<");
    if (record.compareGreater) out << "\ninline bool operator>" << params << "    return rhs < lhs;\n}\n";
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
        if (record.requireAllocator) throw std::runtime_error(node.qualifiedName + ": @requireAllocator requires a PMR field");
        inlineConstructors(out, node.name, record, members, deps.headers);
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
    for (uint32_t i = 0; i < members.size(); ++i) {
        const auto& m = members[i];
        if (m.name == "reset") throw std::runtime_error(name + ".reset: field conflicts with reset method");
        if (retained.erase(m.name)) continue;
        if (m.readonly && (m.tsClear.empty() || std::ranges::find(order, i) != order.end()))
            throw std::runtime_error(name + "." + m.name + ": reset requires a writable field");
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
                if (isTypedArrayName(m.ts)) ts << "        // " << m.name << ": " << m.ts << " size unchanged\n";
                continue;
            }
            const bool parameter = std::ranges::find(order, i) != order.end();
            if (!m.tsClear.empty() && !parameter) {
                ts << "        this." << m.name << m.tsClear << ";\n";
                continue;
            }
            const auto& value = parameter ? m.name : m.tsDefault;
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
            if (m.name == "reset") throw std::runtime_error(node.name + ".reset: field conflicts with reset method");
            if (!m.tsClear.empty()) {
                ts << "        this." << m.name << m.tsClear << ";\n";
                continue;
            }
            if (m.readonly) throw std::runtime_error(node.name + "." + m.name + ": reset requires a writable field");
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
    TypeId type;
    bool transient{};
    std::string cppDescription;
    std::string tsDescription;
};

std::vector<ArchiveField> archiveFields(const TypeGraph& graph, ModuleId owner, const RecordType& record,
                                        const std::vector<Member>& members, ArchiveRequirements& requirements) {
    std::vector<ArchiveField> fields;
    for (size_t i = 0; i < record.fields.size(); ++i) {
        const auto& field = record.fields[i];
        ArchiveField entry{.name = field.name, .type = field.type.type, .transient = field.transient};
        if (field.transient) {
            std::string_view cpp = members[i].cpp, ts = members[i].ts;
            if (cpp.ends_with('*')) cpp.remove_suffix(1);
            if (ts.ends_with(" | null")) ts.remove_suffix(7);
            entry.cppDescription = cpp;
            entry.tsDescription = ts;
        } else {
            if (field.isReadonly && !archivePrimitive(graph, field.type.type).empty())
                invalid(field, "archive load requires a writable field");
            requirements.checkType(graph, owner, field.type.type, field.type.location.describe() + ": " + field.name);
        }
        fields.push_back(std::move(entry));
    }
    return fields;
}

struct RecordArchives {
    std::ostringstream declarations, definitions, script;
    ArchiveRequirements requirements;

    void add(const TypeGraph& graph, const TypeModule& archive, const TypeNode& record, const std::vector<ArchiveField>& fields, bool emitTs, TypeImports& imports) {
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
                if (field.transient) {
                    definitions << "    // skip, " << field.name << ": " << field.cppDescription << '\n';
                    if (emitTs) script << "    // skip, " << member << ": " << field.tsDescription << '\n';
                    continue;
                }
                definitions << "    " << op << "(ar, " << member << ");\n";
                if (emitTs) ArchiveScript{graph, api, imports, save}.value(script, field.type, member, "    ");
            }
            definitions << "}\n";
            if (emitTs) script << "}\n";
        }
    }

    void emitNative(Files& files, const TypeModule& module, const RecordOutput& output,
                    const ArchiveOutput& archive, const std::string& preamble) const {
        const auto& path = *output.serializationHeader;
        const auto archiveHeader = (archive.layout.includeDir / (archive.layout.prefix + "Types.h")).generic_string();
        std::ostringstream header, source;
        header << preamble << "#pragma once\n#include \"" << archiveHeader << "\"\n#include \""
               << output.forwardHeader.value_or(output.header).generic_string() << "\"\n";
        writeNamespace(header, module, declarations);
        source << preamble << "// NOLINTBEGIN(misc-include-cleaner)\n// clang-format off\n";
        cppIncludes(source, {archiveHeader, path.generic_string(), output.header.generic_string(),
                             (archive.layout.includeDir / "details/SerializationUtils.h").generic_string()});
        source << "// clang-format on\n";
        // The engine's generic vector loader needs a T&. Packed bools expose a proxy.
        if (requirements.needsBoolVectorLoad) {
            std::ostringstream packed;
            packed << R"(
template<class Allocator>
inline void load(InputArchive& ar, std::vector<bool, Allocator>& bits) {
    uint32_t size{};
    load(ar, size);
    bits.resize(size);
    for (uint32_t i = 0; i < size; ++i) bits[i] = ar.readBool();
}
)";
            writeNamespace(source, archive.module, packed);
        }
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
    auto pendingReset = output.resetToDefaults;
    auto pendingCtorReset = output.resetFromConstructor;
    std::string enumImports;
    size_t poolCount{};
    bool previousTag = false;
    bool usesGraphRuntime = false;
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
        if (const auto* model = std::get_if<GraphType>(&node.data)) {
            emitCppGraph(graph, id, cpp, definitions, deps.headers, deps.forwardDeclarations);
            if (output.typescript) {
                emitTypeScriptGraph(graph, id, ts, deps.typeImports);
                if (model->hierarchy) usesGraphRuntime = true;
            }
            if (model->serializable) {
                if (!archive) throw std::runtime_error(node.qualifiedName + ": @serialize requires serializationHeader");
                emitGraphArchive(graph, node, archive->module, archives.declarations, archives.definitions,
                                 output.typescript ? &archives.script : nullptr, deps.valueImports, archives.requirements);
            }
            continue;
        }
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
        for (const auto& field : record->fields) members.push_back(bindMember(graph, owner, field, deps, output.typescript.has_value()));
        cppRecord(cpp, definitions, graph, module, node, members, deps);
        if (record->serializable) {
            if (!archive) throw std::runtime_error(node.qualifiedName + ": @serialize requires serializationHeader");
            const auto fields = archiveFields(graph, owner, *record, members, archives.requirements);
            archives.add(graph, archive->module, node, fields, output.typescript.has_value(), deps.valueImports);
        }
        if (record->hashable) cppHash(hashDeclarations, hashDefinitions, node, members);
        if (record->compareEqual || record->compareLess) {
            deps.headers.insert("tuple");
            cppComparisons(cpp, node.name, members, *record);
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
        if (resetToDefaults && resetFromConstructor) throw std::runtime_error(node.qualifiedName + ": conflicting reset bindings");
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
    if (!pendingEnums.empty()) throw std::runtime_error("unknown enum " + *pendingEnums.begin() + " in " + module.name);
    if (!pendingConstEnums.empty()) throw std::runtime_error("unknown const enum " + *pendingConstEnums.begin() + " in " + module.name);

    std::ostringstream header, script;
    header << preamble << "#pragma once\n";
    if (!output.forwardHeader && !hashDefinitions.view().empty()) deps.headers.insert("cocos/base/std/hash/hash.h");
    deps.headers.erase(output.header.generic_string());
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
    if (archive) archives.emitNative(files, module, output, *archive, preamble);
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
    // '$' cannot occur in a spec identifier, including declarations named ED, OutE or OutEI.
    if (usesGraphRuntime) script << "import * as cc$graph from '" << modulePath("cocos/rendering/custom/graph", *output.typescript) << "';\n";
    writeImports(script, deps.valueImports, *output.typescript, false);
    for (const auto& [modulePath, names] : deps.valueImports) {
        auto it = deps.typeImports.find(modulePath);
        if (it == deps.typeImports.end()) continue;
        for (const auto& name : names) it->second.erase(name);
        if (it->second.empty()) deps.typeImports.erase(it);
    }
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
    script << ts.str();
    if (archives.requirements.needsReadSize) {
        const auto& input = archiveInterface(graph, archive->module, "InputArchive");
        script << "\nfunction cc$readSize (a: InputArchive): number {\n"
               << "    const n = a." << archiveMethod(input, "readNumber") << "();\n"
               << "    if (!Number.isInteger(n) || n < 0 || n > 0xFFFFFFFF) throw new RangeError('Invalid archive size');\n"
               << "    return n;\n}\n";
    }
    script << archives.script.str();
    files.emplace(*output.typescript, script.str());
    if (!enumImports.empty()) {
        const auto stem = output.typescript->stem().generic_string();
        const auto namesPath = output.typescript->parent_path() / (stem + "-names.ts");
        files.emplace(namesPath, preamble + "import { " + enumImports + " } from './" + stem + "';\n" + nameFunctions.str());
    }
    return files;
}

} // namespace cc::codegen
