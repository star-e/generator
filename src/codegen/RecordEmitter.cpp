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
    if (type == "uint64_t") return safeUnsignedNumber(literal);
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

std::string pointerType(const TypeGraph& graph, const TypeModule& module, const TypeNode& resource, Dependencies& deps) {
    const auto ns = resource.qualifiedName.rfind("::");
    std::string decl = std::get<ExternalTypeNode>(resource.data).isStruct ? "struct " : "class ";
    decl += resource.name + ";";
    if (ns != std::string::npos) decl = "namespace " + resource.qualifiedName.substr(0, ns) + " { " + decl + " }";
    deps.forwardDeclarations.insert(decl);
    return cppName(graph, resource, module) + "*";
}

std::string methodType(const TypeGraph& graph, const TypeModule& module, TypeId id, Dependencies& deps) {
    if (const auto* ptr = std::get_if<PointerType>(&graph.node(id).data)) {
        const auto& target = graph.node(ptr->pointee);
        if (std::holds_alternative<ExternalTypeNode>(target.data) && target.module != ModuleId{})
            return pointerType(graph, module, target, deps);
    }
    return nativeType(graph, module, id, deps);
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

BoundField bindMember(const TypeGraph& graph, ModuleId owner, const FieldInfo& field, Dependencies& deps,
                     bool emitTs) {
    const auto& module = graph.modules.at(owner.value);
    const auto& node = graph.node(field.type.type);
    BoundField m{.name = field.name, .cppDefault = field.initializer, .readonly = field.isReadonly, .transient = field.transient};
    auto scalarType = field.type.type;
    while (const auto* alias = std::get_if<AliasType>(&graph.node(scalarType).data)) scalarType = alias->target;
    const auto& scalar = graph.node(scalarType).qualifiedName;
    if (scalar == "uint64_t" && !m.cppDefault.empty()) m.cppDefault += "ULL";
    m.scalar = std::holds_alternative<BuiltinType>(node.data) || std::holds_alternative<EnumType>(node.data) ||
               std::holds_alternative<AliasType>(node.data) || std::holds_alternative<PointerType>(node.data) ||
               (std::holds_alternative<ExternalTypeNode>(node.data) && (scalar == "uint64_t" || archivePrimitive(graph, field.type.type) == "Number"));
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
        if (node.qualifiedName == "ccstd::pmr::string") {
            m.emptyPmrArgument = "\"\"";
            m.checkDefaultConstruction = true;
            deps.headers.insert("cocos/base/Pmr.h");
        }
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
        if (graph.node(instance->declaration).qualifiedName == "ccstd::pmr::vector") {
            m.emptyPmrArgument = "0U";
            m.checkDefaultConstruction = true;
            deps.headers.insert("cocos/base/Pmr.h");
        }
        if (emitTs) {
            m.ts = boundTypeScriptName(graph, field.type, owner, deps.typeImports);
            m.tsDefault = typeScriptContainerInitializer(node);
            if (node.typescriptContainer == ContainerKind::Array) m.tsReset = "this." + m.name + ".length = 0";
            if (isAssociativeContainer(node.typescriptContainer)) m.tsReset = "this." + m.name + ".clear()";
        }
        return m;
    }
    if (const auto* record = std::get_if<RecordType>(&node.data); record && !field.isNullable && field.initializer.empty()) {
        m.cpp = nativeType(graph, module, field.type.type, deps);
        std::set<uint32_t> visited;
        m.usesAllocator = hasPmrStorage(graph, field.type.type, visited);
        m.checkDefaultConstruction = true;
        // Pass copyable, single-scalar value records by reference in native constructors.
        if (!record->moveOnly && record->fields.size() == 1) {
            const auto inner = record->fields.front().type.type;
            const auto primitive = archivePrimitive(graph, inner);
            m.byConstReference = !std::holds_alternative<RecordType>(graph.node(inner).data) && (primitive == "Number" || primitive == "Bool");
        }
        m.move = !m.byConstReference;
        if (emitTs) {
            if (node.typescriptNumber) {
                m.ts = "number";
                m.tsDefault = record->fields.front().initializer;
                return m;
            }
            if (node.module != owner && node.typescriptModule.empty()) invalid(field, "missing external record mapping");
            m.ts = boundTypeScriptName(graph, field.type, owner, deps.valueImports);
            m.tsDefault = "new " + m.ts + "()";
        }
        return m;
    }
    if (const auto* ext = std::get_if<ExternalTypeNode>(&node.data); ext && !field.isNullable &&
        (ext->constructorConstRef || !node.typescriptArchiveModule.empty() || (ext->isStruct && !node.typescriptModule.empty()))) {
        if (!field.initializer.empty()) invalid(field, "expected a default-constructed external value");
        m.cpp = nativeType(graph, module, field.type.type, deps);
        m.byConstReference = ext->constructorConstRef;
        m.move = true;
        m.usesAllocator = ext->isPmr;
        m.checkDefaultConstruction = true;
        if (emitTs) {
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
            m.cpp = pointerType(graph, module, resource, deps);
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
    m.safeUnsignedInteger = scalar == "uint64_t";
    if (m.safeUnsignedInteger) {
        if (!field.typescriptInitializer.empty()) m.tsDefault = field.typescriptInitializer;
        if (!safeUnsignedNumber(m.tsDefault))
            invalid(field, "uint64_t default exceeds the TypeScript safe integer range: " + m.tsDefault);
        return m;
    }
    if (m.ts == "number" && numericDefault(scalar, m.tsDefault)) return m;
    invalid(field, "unsupported scalar default " + field.initializer);
}

std::vector<BoundField> bindFields(const TypeGraph& graph, ModuleId owner, const std::vector<FieldInfo>& fields,
                                   Dependencies& deps, bool emitTs, const std::map<uint32_t, std::string>& nestedResets) {
    std::vector<BoundField> result;
    for (const auto& field : fields) {
        auto member = bindMember(graph, owner, field, deps, emitTs && !field.cppOnly);
        if (field.isReadonly) {
            const auto childReset = nestedResets.find(field.type.type.value);
            if (childReset != nestedResets.end()) member.tsReset = "this." + member.name + childReset->second;
        }
        if (!field.typescriptInitializer.empty()) {
            auto id = field.type.type;
            if (graph.node(id).typescriptNumber) id = std::get<RecordType>(graph.node(id).data).fields.front().type.type;
            if (const auto* alias = std::get_if<AliasType>(&graph.node(id).data)) id = alias->target;
            if (member.ts != "number" || !numericDefault(graph.node(id).qualifiedName, field.typescriptInitializer))
                invalid(field, "invalid TypeScript numeric default " + field.typescriptInitializer);
            member.tsDefault = field.typescriptInitializer;
        }
        result.push_back(std::move(member));
    }
    return result;
}

using Order = std::vector<uint32_t>;

Order bindTypeScriptConstructor(const TypeNode& node, const RecordType& record, std::vector<BoundField>& fields,
                                const RecordOutput& output) {
    auto constructors = record.constructors;
    if (std::ranges::any_of(record.fields, &FieldInfo::cppOnly)) {
        Order indices(fields.size());
        std::vector<BoundField> visible;
        for (uint32_t i = 0; i < fields.size(); ++i) {
            if (record.fields[i].cppOnly) continue;
            indices[i] = static_cast<uint32_t>(visible.size());
            visible.push_back(std::move(fields[i]));
        }
        fields = std::move(visible);
        for (auto& ctor : constructors) {
            std::erase_if(ctor, [&](auto index) { return record.fields[index].cppOnly; });
            for (auto& index : ctor) index = indices[index];
        }
    }
    if (constructors.empty()) {
        Order order(fields.size());
        std::iota(order.begin(), order.end(), 0U);
        constructors.push_back(std::move(order));
    }
    if (const auto selected = output.constructors.find(node.name); selected != output.constructors.end()) {
        for (const auto& candidate : constructors) {
            if (std::ranges::equal(candidate, selected->second, {}, [&](auto i) -> std::string_view { return fields[i].name; }))
                return candidate;
        }
        throw std::runtime_error(node.qualifiedName + ": TypeScript constructor does not match a declared constructor");
    }
    const auto& order = *std::ranges::max_element(constructors, {}, &Order::size);
    if (order.size() < fields.size() && std::ranges::any_of(constructors, [&](const Order& other) {
            return other.size() == order.size() && other != order;
        })) throw std::runtime_error(node.qualifiedName + ": ambiguous partial TypeScript constructor");
    return order;
}

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
                        const std::vector<BoundField>& members, std::set<std::string>& headers) {
    if (!record.constructors.empty() || record.moveOnly) out << "    " << name << "() = default;\n";
    for (const auto& ctor : record.constructors) {
        if (ctor.empty()) continue;
        out << "    " << name << "(";
        for (size_t i = 0; i < ctor.size(); ++i) {
            const auto& m = members.at(ctor[i]);
            if (i) out << ", ";
            out << m.parameterType() << " " << m.name << "In";
        }
        out << (ctor.size() == 1 ? ") noexcept // NOLINT\n" : ") noexcept\n");
        bool first = true;
        for (uint32_t i = 0; i < members.size(); ++i) {
            if (std::ranges::find(ctor, i) == ctor.end()) continue;
            const auto& m = members[i];
            out << (first ? "    : " : ",\n      ") << m.name << "(";
            const bool moveInput = m.move && !m.byConstReference;
            if (moveInput) {
                headers.insert("utility");
                out << "std::move(";
            }
            out << m.name << "In" << (moveInput ? "))" : ")");
            first = false;
        }
        out << " {}\n";
    }
    if (!record.constructors.empty() || record.moveOnly) out << '\n';
}

enum class CtorArgs { defaults, fields, move, copy };

std::string pmrArguments(const BoundField& m, std::string value, bool move) {
    if (value.empty() && m.usesAllocator)
        return m.allocatorArguments();
    if (!value.empty() && m.move && move) value = "std::move(" + value + ")";
    if (m.usesAllocator) value += value.empty() ? "alloc" : ", alloc";
    return value;
}

void pmrInitializers(std::ostream& source, const std::vector<BoundField>& members, CtorArgs args, const Order& fields = {}) {
    bool first = true;
    for (uint32_t i = 0; i < members.size(); ++i) {
        const auto& m = members[i];
        const bool hasArg = args == CtorArgs::move || args == CtorArgs::copy ||
                            (args == CtorArgs::fields && std::ranges::find(fields, i) != fields.end());
        if (!m.usesAllocator && !hasArg) continue;
        std::string value;
        if (hasArg) value = args == CtorArgs::fields ? m.name + "In" : "rhs." + m.name;
        const bool moveValue = args == CtorArgs::move || (args == CtorArgs::fields && !m.byConstReference);
        source << (first ? "\n: " : ",\n  ") << m.name << "(" << pmrArguments(m, value, moveValue) << ")";
        first = false;
    }
    source << " {}\n";
}

std::string pmrExceptionSpec(const std::string& recordName, const std::vector<BoundField>& members, const Order& fields) {
    std::string checks;
    for (size_t i = 0; i < members.size(); ++i) {
        const auto& m = members[i];
        if (fields.empty() ? !m.checkDefaultConstruction : m.scalar) continue;
        const bool supplied = std::ranges::find(fields, i) != fields.end();
        const auto args = pmrArguments(m, supplied ? m.name + "In" : "", !m.byConstReference);
        const auto init = args.empty() && !m.cppDefault.empty() ? "{" + m.cppDefault + "}" : "(" + args + ")";
        if (!checks.empty()) checks += " && ";
        if (!supplied && !m.emptyPmrArgument.empty()) checks += "noexcept(" + args + ")";
        else checks += "noexcept(decltype(" + recordName + "::" + m.name + ")" + init + ")";
    }
    return checks.empty() ? " noexcept" : " noexcept(" + checks + ")";
}

void pmrConstructors(std::ostream& header, std::ostream& source, const std::string& name,
                     const RecordType& record, const std::vector<BoundField>& members, const BoundField& allocatorMember,
                     std::set<std::string>& headers) {
    const std::string alloc = "const allocator_type& alloc";
    const auto allocParameter = record.requireAllocator ? alloc : alloc + " = boost::container::pmr::get_default_resource()";
    const auto constructor = "\n" + name + "::" + name + "(";
    const auto exceptionSpec = pmrExceptionSpec(name, members, {});
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
            if (m.move) headers.insert("utility");
            params += m.parameterType() + " " + m.name + "In, ";
        }
        const auto fieldSpec = pmrExceptionSpec(name, members, order);
        header << "    " << name << "(" << params << allocParameter << ")" << fieldSpec << ";\n";
        source << constructor << params << alloc << ")" << fieldSpec << " // NOLINT";
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

void cppComparisons(std::ostream& out, const std::string& name, const std::vector<std::string_view>& elements, const RecordType& record) {
    const auto params = "(const " + name + "& lhs, const " + name + "& rhs) noexcept {\n";
    auto writeTuple = [&](std::string_view object) {
        out << "std::forward_as_tuple(";
        for (size_t i = 0; i < elements.size(); ++i) {
            if (i) out << ", ";
            out << object << "." << elements[i];
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

void cppHash(std::ostream& declarations, std::ostream& definitions, const TypeNode& node, const std::vector<std::string_view>& elements) {
    const auto& type = node.qualifiedName;
    const auto signature = "operator()(const " + type + "& val) const noexcept";
    declarations << "\ntemplate <>\nstruct hash<" << type << "> {\n    hash_t " << signature << ";\n};\n";
    definitions << "\ninline hash_t hash<" << type << ">::" << signature << " {\n    hash_t seed = 0;\n";
    for (const auto name : elements) definitions << "    hash_combine(seed, val." << name << ");\n";
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

void writeScalarAliases(std::ostream& out, const TypeGraph& graph, const TypeModule& module, std::set<std::string>& includes) {
    for (auto id : module.declarations) {
        const auto& node = graph.node(id);
        const auto* alias = std::get_if<AliasType>(&node.data);
        if (!alias) continue;
        const auto& target = graph.node(alias->target);
        if (!target.cppHeader.empty()) includes.insert(target.cppHeader);
        out << "\nusing " << node.name << " = " << cppName(graph, target, module) << ";\n";
    }
}

std::string forwardHeader(const TypeGraph& graph, const TypeModule& module, const std::ostringstream& hashes,
                          const std::string& preamble) {
    std::set<std::string> includes;
    std::ostringstream header, body;
    std::optional<bool> previousEnum;
    writeScalarAliases(body, graph, module, includes);
    for (auto id : module.declarations) {
        const auto& node = graph.node(id);
        if (std::holds_alternative<AliasType>(node.data)) continue;
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

std::string namesHeader(const TypeGraph& graph, const TypeModule& module, const RecordOutput& output,
                        const std::string& preamble) {
    std::ostringstream header, body;
    header << preamble << "// clang-format off\n#pragma once\n#include \"" << output.header.generic_string() << "\"\n";
    for (const auto& path : output.importedNamesHeaders) header << "#include \"" << path.generic_string() << "\"\n";
    body << '\n';
    for (auto id : module.declarations) {
        const auto& node = graph.node(id);
        if (std::holds_alternative<AliasType>(node.data)) continue;
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
               const TypeNode& node, const std::vector<BoundField>& members, Dependencies& deps) {
    const auto& record = std::get<RecordType>(node.data);
    out << "struct " << node.name << " {\n";
    const auto allocator = std::ranges::find(members, true, &BoundField::usesAllocator);
    if (allocator == members.end()) {
        if (record.requireAllocator) throw std::runtime_error(node.qualifiedName + ": @requireAllocator requires a PMR field");
        inlineConstructors(out, node.name, record, members, deps.headers);
    } else {
        pmrConstructors(out, source, node.name, record, members, *allocator, deps.headers);
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
        out << "    " << (method.isStatic ? "static " : "") << methodType(graph, module, method.result.type, deps) << " " << method.name << "(";
        for (size_t i = 0; i < method.parameters.size(); ++i) {
            if (i) out << ", ";
            const auto& p = method.parameters[i];
            if (p.byConstReference) out << "const ";
            out << methodType(graph, module, p.type.type, deps) << (p.byConstReference ? "& " : " ") << p.name;
        }
        out << ")";
        if (method.isConst) out << " const";
        if (method.isNoexcept) out << " noexcept";
        if (method.cppExpression.empty()) out << ";\n";
        else out << " { return " << method.cppExpression << "; }\n";
    }
    out << "};\n";
}

bool inferTsType(const BoundField& m) {
    return m.ts == "number" || m.ts == "boolean" || m.ts == "string";
}

std::string tsParams(const std::vector<BoundField>& members, const Order& order, bool defaults, const std::string& sep) {
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

Order resetParameters(const std::string& name, const std::vector<BoundField>& members, const Order& constructor,
                      const std::set<std::string>& keepOnReset, const ResetBinding& binding) {
    if (binding.kind == ResetBinding::Kind::defaults) {
        if (!keepOnReset.empty()) throw std::runtime_error(name + ": resetToDefaults conflicts with keepOnReset");
        return {};
    }
    Order order;
    if (binding.kind == ResetBinding::Kind::fields) {
        for (const auto& field : binding.fields) {
            const auto it = std::ranges::find(members, field, &BoundField::name);
            if (it == members.end()) throw std::runtime_error(name + ": unknown reset field " + field);
            if (keepOnReset.contains(field)) throw std::runtime_error(name + "." + field + ": reset parameter conflicts with keepOnReset");
            order.push_back(static_cast<uint32_t>(it - members.begin()));
        }
    } else {
        order = constructor;
    }
    auto retained = keepOnReset;
    for (uint32_t i = 0; i < members.size(); ++i) {
        const auto& m = members[i];
        if (m.name == "reset") throw std::runtime_error(name + ".reset: field conflicts with reset method");
        if (retained.erase(m.name)) continue;
        if (m.readonly && (m.tsReset.empty() || std::ranges::find(order, i) != order.end()))
            throw std::runtime_error(name + "." + m.name + ": reset requires a writable field");
    }
    if (!retained.empty()) throw std::runtime_error(name + ": unknown keepOnReset field " + *retained.begin());
    std::erase_if(order, [&](auto index) { return keepOnReset.contains(members[index].name); });
    return order;
}

void tsConstructorAndReset(std::ostream& ts, const std::vector<BoundField>& members, const Order& order,
                           const Order* reset, const std::set<std::string>& keepOnReset, bool hasBase) {
    const auto params = tsParams(members, order, true, ", ");
    const bool multiline = params.size() + std::string_view("    constructor () {").size() > 120;
    auto signature = [&](const Order& fields, bool defaults) {
        if (multiline && !fields.empty()) return "\n        " + tsParams(members, fields, defaults, ",\n        ") + ",\n    ";
        return defaults ? params : tsParams(members, fields, false, ", ");
    };
    ts << "    constructor (" << signature(order, true) << ") {\n";
    if (hasBase) ts << "        super();\n";
    for (uint32_t i = 0; i < members.size(); ++i) {
        const auto& m = members[i];
        const bool parameter = std::ranges::find(order, i) != order.end();
        if (m.safeUnsignedInteger) {
            ts << "        " << m.tsUint64Initializer(parameter ? m.name : m.tsDefault) << ";\n";
        } else if (parameter) {
            ts << "        this." << m.name << " = " << m.name << ";\n";
        }
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
            const bool parameter = std::ranges::find(*reset, i) != reset->end();
            if (!m.tsReset.empty() && !parameter) {
                ts << "        " << m.tsReset << ";\n";
                continue;
            }
            const auto& value = parameter ? m.name : m.tsDefault;
            ts << "        this." << m.name << " = " << value << ";\n";
        }
        ts << "    }\n";
    }
}

void tsRecord(std::ostream& ts, const TypeNode& node, const std::vector<BoundField>& members, const Order& order,
              const Order* reset, bool resetToDefaults, const std::set<std::string>& keepOnReset, const std::string& base) {
    const bool initializeFields = resetToDefaults && std::get<RecordType>(node.data).constructors.empty();
    ts << "export class " << node.name;
    if (!base.empty()) ts << " extends " << base;
    ts << " {\n";
    if (!initializeFields) tsConstructorAndReset(ts, members, order, resetToDefaults ? nullptr : reset, keepOnReset, !base.empty());
    else if (std::ranges::any_of(members, &BoundField::safeUnsignedInteger))
        tsConstructorAndReset(ts, members, {}, nullptr, keepOnReset, !base.empty());
    if (resetToDefaults) {
        ts << "    reset (): void {\n";
        for (const auto& m : members) {
            if (m.name == "reset") throw std::runtime_error(node.name + ".reset: field conflicts with reset method");
            if (!m.tsReset.empty()) {
                ts << "        " << m.tsReset << ";\n";
                continue;
            }
            if (m.readonly) throw std::runtime_error(node.name + "." + m.name + ": reset requires a writable field");
            ts << "        this." << m.name << " = " << m.tsDefault << ";\n";
        }
        ts << "    }\n";
    }
    for (uint32_t i = 0; i < members.size(); ++i) {
        const auto& m = members[i];
        if (m.safeUnsignedInteger) {
            ts << "    declare " << (m.readonly ? "readonly " : "") << m.name << ": number;\n";
            continue;
        }
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
                                        const std::vector<BoundField>& members, ArchiveRequirements& requirements) {
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
                    if (emitTs && !field.tsDescription.empty()) script << "    // skip, " << member << ": " << field.tsDescription << '\n';
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
    enum class Reuse { reset, assign, clear, unchanged };
    std::ostringstream resets, factories, fields;
    size_t count{};

    void add(const std::string& name, const std::string& pool, Reuse reuse,
             const std::vector<BoundField>& members = {}, const Order& order = {}) {
        ++count;
        resets << "        this." << pool << ".reset(); // " << name << "\n";
        factories << "    create" << name << " (";
        if (!order.empty()) factories << "\n        " << tsParams(members, order, true, ",\n        ") << ",\n    ";
        factories << "): " << name << " {\n        const v = this." << pool << ".add(); // " << name << "\n";
        if (reuse == Reuse::clear) factories << "        v.clear();\n";
        if (reuse == Reuse::assign) {
            for (const auto i : order) factories << "        v." << members.at(i).name << " = " << members.at(i).name << ";\n";
        }
        if (reuse == Reuse::reset) {
            factories << "        v.reset(";
            for (size_t i = 0; i < order.size(); ++i) {
                if (i) factories << ", ";
                factories << members.at(order[i]).name;
            }
            factories << ");\n";
        }
        factories << "        return v;\n    }\n";
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
    auto pendingResets = output.resets;
    // By-value member types are defined before the record that contains them.
    std::map<uint32_t, std::string> nestedResets;
    std::string enumImports;
    bool previousTag = false;
    bool usesGraphRuntime = false;
    bool usesUint64Field = false;
    const std::set<std::string> keepNone;
    if (!output.forwardHeader) writeScalarAliases(cpp, graph, module, deps.headers);
    for (auto id : module.declarations) {
        const auto& node = graph.node(id);
        if (std::holds_alternative<AliasType>(node.data)) continue;
        if (std::holds_alternative<TagType>(node.data)) {
            if (!previousTag) cpp << '\n';
            cpp << "struct " << node.name << " {};\n";
            previousTag = true;
            continue;
        }
        previousTag = false;
        const auto entry = output.pools.find(node.name);
        if (const auto* model = std::get_if<GraphType>(&node.data)) {
            const auto fields = bindFields(graph, owner, model->fields, deps, output.typescript.has_value(), nestedResets);
            std::string inputArchive;
            if (model->serializable && (!fields.empty() || model->nameIndex) && archive) {
                deps.headers.insert((archive->layout.includeDir / (archive->layout.prefix + "Fwd.h")).generic_string());
                inputArchive = "::" + archiveInterface(graph, archive->module, "InputArchive").qualifiedName;
            }
            emitCppGraph(graph, id, cpp, definitions, deps.headers, deps.forwardDeclarations, fields, inputArchive);
            if (output.typescript) {
                emitTypeScriptGraph(graph, id, ts, deps.typeImports, fields);
                usesUint64Field |= std::ranges::any_of(fields, &BoundField::safeUnsignedInteger);
                if (entry != output.pools.end()) {
                    if (!model->mutableGraph) throw std::runtime_error(node.qualifiedName + ": graph pool requires mutable_graph");
                    pool.add(node.name, entry->second.member, Pool::Reuse::clear);
                }
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
        auto members = bindFields(graph, owner, record->fields, deps, output.typescript.has_value(), nestedResets);
        cppRecord(cpp, definitions, graph, module, node, members, deps);
        if (record->serializable) {
            if (!archive) throw std::runtime_error(node.qualifiedName + ": @serialize requires serializationHeader");
            const auto fields = archiveFields(graph, owner, *record, members, archives.requirements);
            archives.add(graph, archive->module, node, fields, output.typescript.has_value() && !node.typescriptNumber, deps.valueImports);
        }
        if (record->hashable || record->compareEqual || record->compareLess) {
            std::vector<std::string_view> elements;
            for (const auto& field : record->fields) {
                if (!field.notElement) elements.push_back(field.name);
            }
            if (record->hashable) cppHash(hashDeclarations, hashDefinitions, node, elements);
            if (record->compareEqual || record->compareLess) {
                deps.headers.insert("tuple");
                cppComparisons(cpp, node.name, elements, *record);
            }
        }
        if (!output.typescript || node.typescriptNumber) continue;
        if (const auto impl = output.externalClasses.find(node.name); impl != output.externalClasses.end()) {
            deps.valueImports[impl->second.generic_string()].insert(node.name);
            ts << "export { " << node.name << " };\n";
            if (entry != output.pools.end()) {
                if (!std::ranges::all_of(record->fields, &FieldInfo::cppOnly))
                    throw std::runtime_error(node.qualifiedName + ": externally implemented pool record must be empty");
                pool.add(node.name, entry->second.member, Pool::Reuse::unchanged);
            }
            continue;
        }
        const Order order = bindTypeScriptConstructor(node, *record, members, output);
        const auto rule = pendingResets.extract(node.name);
        const ResetBinding constructorReset;
        const auto& binding = rule.empty() ? constructorReset : rule.mapped();
        const bool resetToDefaults = binding.kind == ResetBinding::Kind::defaults;
        const auto& retained = entry == output.pools.end() ? keepNone : entry->second.keepOnReset;
        const auto hooks = output.fieldResets.find(node.name);
        if (hooks != output.fieldResets.end()) {
            for (const auto& [field, hook] : hooks->second) {
                const auto member = std::ranges::find(members, field, &BoundField::name);
                if (member == members.end()) throw std::runtime_error(node.qualifiedName + ": unknown reset field " + field);
                if (retained.contains(field)) throw std::runtime_error(node.qualifiedName + "." + field + ": reset conflicts with keepOnReset");
                if (hook.from.empty()) {
                    member->tsReset = "this." + field + "." + hook.name + "()";
                } else {
                    deps.valueImports[hook.from.generic_string()].insert(hook.name);
                    member->tsReset = hook.name + "(this." + field + ")";
                }
            }
        }
        std::optional<Order> reset;
        if (entry != output.pools.end() && entry->second.assignFields) {
            if (order.size() != members.size() || std::ranges::any_of(members, &BoundField::readonly))
                throw std::runtime_error(node.qualifiedName + ": pool assign requires constructor parameters for all fields and writable fields");
            pool.add(node.name, entry->second.member, Pool::Reuse::assign, members, order);
        } else if (entry != output.pools.end() || !rule.empty()) {
            reset = resetParameters(node.name, members, order, retained, binding);
            if (entry != output.pools.end()) pool.add(node.name, entry->second.member, Pool::Reuse::reset, members, *reset);
        }
        if (reset) {
            std::string args;
            for (auto index : *reset) {
                const auto& member = members[index];
                if (hooks != output.fieldResets.end() && hooks->second.contains(member.name))
                    throw std::runtime_error(node.qualifiedName + "." + member.name + ": reset parameter also has a reset function");
                if (!args.empty()) args += ", ";
                args += member.tsDefault;
            }
            nestedResets.emplace(id.value, ".reset(" + args + ")");
        }
        const auto base = output.bases.find(node.name);
        if (base != output.bases.end()) deps.valueImports[base->second.from.generic_string()].insert(base->second.name);
        usesUint64Field |= std::ranges::any_of(members, &BoundField::safeUnsignedInteger);
        tsRecord(ts, node, members, order, reset ? &*reset : nullptr, resetToDefaults, retained,
                 base == output.bases.end() ? "" : base->second.name);
    }
    if (pool.count != output.pools.size()) throw std::runtime_error("unknown pool type in " + module.name);
    if (!pendingResets.empty()) throw std::runtime_error("unknown reset record " + pendingResets.begin()->first + " in " + module.name);
    if (!pendingEnums.empty()) throw std::runtime_error("unknown enum " + *pendingEnums.begin() + " in " + module.name);
    if (!pendingConstEnums.empty()) throw std::runtime_error("unknown const enum " + *pendingConstEnums.begin() + " in " + module.name);

    std::ostringstream header, script;
    header << preamble << "#pragma once\n";
    if (!output.forwardHeader && !hashDefinitions.view().empty()) deps.headers.insert("cocos/base/std/hash/hash.h");
    deps.headers.erase(output.header.generic_string());
    if (output.forwardHeader) deps.headers.erase(output.forwardHeader->generic_string());
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
    if (output.namesHeader) files.emplace(output.nativeRoot / *output.namesHeader, namesHeader(graph, module, output, preamble));
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
    for (const auto& [member, dependency] : output.poolDependencies)
        deps.typeImports[dependency.from.generic_string()].insert(dependency.name);
    writeImports(script, deps.valueImports, *output.typescript, false);
    for (const auto& [modulePath, names] : deps.valueImports) {
        auto it = deps.typeImports.find(modulePath);
        if (it == deps.typeImports.end()) continue;
        for (const auto& name : names) it->second.erase(name);
        if (it->second.empty()) deps.typeImports.erase(it);
    }
    writeImports(script, deps.typeImports, *output.typescript, true);
    if (pool.count) {
        script << "import { RecyclePool } from '" << modulePath(output.poolModule, *output.typescript) << "';\n";
        ts << "\nfunction createPool<T> (Constructor: new() => T): RecyclePool<T> {\n"
           << "    return new RecyclePool<T>(() => new Constructor(), 16);\n}\n"
           << "\nexport class " << module.name << "ObjectPool {\n    constructor (";
        std::string params;
        for (const auto& [member, dependency] : output.poolDependencies) {
            if (!params.empty()) params += ", ";
            params += member + ": " + dependency.name;
        }
        ts << params << ") {\n";
        for (const auto& [member, dependency] : output.poolDependencies) ts << "        this." << member << " = " << member << ";\n";
        ts << "    }\n    reset (): void {\n" << pool.resets.str() << "    }\n" << pool.factories.str();
        for (const auto& [member, dependency] : output.poolDependencies) ts << "    public readonly " << member << ": " << dependency.name << ";\n";
        ts << pool.fields.str() << "}\n";
    }
    if (!archives.script.view().empty()) {
        auto path = archive->layout.typescript;
        script << "import type { OutputArchive, InputArchive } from '" << modulePath(path.replace_extension(), *output.typescript) << "';\n";
    }
    script << ts.str();
    if (usesUint64Field) {
        script << R"(
function cc$defineUint64 (target: object, key: string, initial: number): void {
    let value: number;
    const set = (next: number): void => {
        if (!Number.isSafeInteger(next) || next < 0) throw new RangeError(key + ' requires a safe unsigned integer');
        value = next;
    };
    set(initial);
    Object.defineProperty(target, key, { enumerable: true, configurable: true, get: () => value, set });
}
)";
    }
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
