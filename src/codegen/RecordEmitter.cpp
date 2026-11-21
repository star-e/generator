#include "codegen/Documentation.h"
#include "codegen/RecordEmitter.h"
#include "codegen/ArchiveEmitter.h"
#include "codegen/EnumEmitter.h"
#include "codegen/GraphEmitter.h"
#include "codegen/TypeScriptImports.h"
#include "generated/TypeGraphTypes.h"
#include "model/NumericLiteral.h"
#include "model/TypeName.h"

#include <algorithm>
#include <iterator>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string_view>

namespace cc::codegen {
namespace {

struct Dependencies {
    bool emitCpp{};
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
    return validFloatingDefault(type, literal);
}

std::string nativeType(const TypeGraph& graph, const TypeModule& module, TypeId id, Dependencies& deps) {
    return formatTypeName(graph, id, [&](const TypeNode& type) {
        if (deps.emitCpp) collectCppHeader(graph, module, type, deps.headers);
        return cppName(graph, type, module);
    });
}

std::string pointerType(const TypeGraph& graph, const TypeModule& module, TypeId id, const TypeNode& resource, Dependencies& deps) {
    const auto ns = resource.qualifiedName.rfind("::");
    std::string decl = std::get<ExternalTypeNode>(resource.data).isStruct ? "struct " : "class ";
    decl += resource.name + ";";
    if (ns != std::string::npos) decl = "namespace " + resource.qualifiedName.substr(0, ns) + " { " + decl + " }";
    deps.forwardDeclarations.insert(decl);
    return formatTypeName(graph, id, [&](const TypeNode& type) { return cppName(graph, type, module); });
}

std::string methodType(const TypeGraph& graph, const TypeModule& module, TypeId id, Dependencies& deps) {
    if (const auto* ptr = std::get_if<PointerType>(&graph.node(id).data)) {
        const auto* q = std::get_if<QualifiedType>(&graph.node(ptr->pointee).data);
        const auto& target = graph.node(q ? q->base : ptr->pointee);
        if (std::holds_alternative<ExternalTypeNode>(target.data) && target.module != ModuleId{})
            return pointerType(graph, module, id, target, deps);
    }
    return nativeType(graph, module, id, deps);
}

struct RecordStorage {
    bool usesAllocator{};
    bool containsGraph{};
};

bool hasAllocatorConstructor(const RecordType& record) {
    return record.constructors.empty() || std::ranges::any_of(record.constructors, [](const ConstructorInfo& ctor) {
        return ctor.fields.empty() || ctor.defaults;
    });
}

bool hasDefaultAllocator(const RecordType& record) {
    return std::ranges::any_of(record.constructors, [](const ConstructorInfo& ctor) {
        return ctor.fields.empty() && ctor.defaults;
    });
}

auto emptyConstructor(const RecordType& record) {
    auto ctor = std::ranges::find_if(record.constructors, [](const auto& c) { return c.fields.empty(); });
    return ctor != record.constructors.end() ? ctor : std::ranges::find(record.constructors, true, &ConstructorInfo::defaults);
}

bool accessibleWithoutFields(const ConstructorInfo& ctor) {
    return !ctor.cppPrivate && (ctor.fields.empty() || ctor.defaults);
}

template<class Predicate>
bool recordHierarchySatisfies(const TypeGraph& graph, const RecordType& record, const Predicate& predicate) {
    if (!predicate(record)) return false;
    return std::ranges::all_of(record.bases, [&](TypeId id) {
        const auto* base = std::get_if<RecordType>(&graph.node(id).data);
        return !base || recordHierarchySatisfies(graph, *base, predicate);
    });
}

bool hasAccessibleEmptyConstructor(const TypeGraph& graph, const RecordType& record) {
    const auto ctor = emptyConstructor(record);
    if (ctor != record.constructors.end() && ctor->cppPrivate) return false;
    return recordHierarchySatisfies(graph, record, [](const RecordType& r) {
        return !r.destructorPrivate && (r.constructors.empty() || std::ranges::any_of(r.constructors, accessibleWithoutFields));
    });
}

RecordStorage recordStorage(const TypeGraph& graph, TypeId id, std::set<uint32_t>& visited) {
    const auto& data = graph.node(id).data;
    if (const auto* alias = std::get_if<AliasType>(&data)) return recordStorage(graph, alias->target, visited);
    if (const auto* graphType = std::get_if<GraphType>(&data)) return {!graphType->indexedStorage, true};
    if (const auto* external = std::get_if<ExternalTypeNode>(&data)) return {external->isPmr};
    if (const auto* instance = std::get_if<TemplateInstanceType>(&data))
        return {std::get<ExternalTypeNode>(graph.node(instance->declaration).data).isPmr};
    const auto* record = std::get_if<RecordType>(&data);
    RecordStorage result;
    if (record && visited.insert(id.value).second) {
        auto include = [&](TypeId type) {
            const auto child = recordStorage(graph, type, visited);
            result.usesAllocator |= child.usesAllocator;
            result.containsGraph |= child.containsGraph;
        };
        for (auto base : record->bases) include(base);
        for (const auto& field : record->fields) include(field.type.type);
    }
    return result;
}

BoundField bindMember(const TypeGraph& graph, ModuleId owner, const FieldInfo& field, Dependencies& deps,
                     bool emitTs, bool typeOnly) {
    const auto& module = graph.modules.at(owner.value);
    const auto& node = graph.node(field.type.type);
    BoundField m{.name = field.name, .cppDefault = field.initializer, .documentation = field.documentation,
                 .readonly = field.isReadonly, .transient = field.transient};
    auto scalarType = field.type.type;
    while (const auto* alias = std::get_if<AliasType>(&graph.node(scalarType).data)) scalarType = alias->target;
    if (scalarType != field.type.type && std::holds_alternative<TemplateInstanceType>(graph.node(scalarType).data)) {
        auto target = field;
        target.type.type = scalarType;
        auto bound = bindMember(graph, owner, target, deps, emitTs, typeOnly);
        bound.cpp = nativeType(graph, module, field.type.type, deps);
        return bound;
    }
    if (std::holds_alternative<ReferenceType>(graph.node(scalarType).data)) {
        if (emitTs) invalid(field, "borrowed reference fields require a native record or @cppOnly");
        m.cpp = nativeType(graph, module, field.type.type, deps);
        m.referenceMember = true;
        return m;
    }
    const auto& scalar = graph.node(scalarType).qualifiedName;
    if (scalar == "uint64_t" && !m.cppDefault.empty()) m.cppDefault += "ULL";
    const bool isScalar = std::holds_alternative<BuiltinType>(node.data) || std::holds_alternative<EnumType>(node.data) ||
                          std::holds_alternative<AliasType>(node.data) || std::holds_alternative<PointerType>(node.data) ||
                          (std::holds_alternative<ExternalTypeNode>(node.data) && (scalar == "uint64_t" || archivePrimitive(graph, field.type.type) == "Number"));
    if (const auto* external = std::get_if<ExternalTypeNode>(&node.data); external && external->isString) {
        if (field.isNullable || !field.initializer.empty()) invalid(field, "expected a default-constructed owning string");
        if (deps.emitCpp && node.cppHeader.empty()) invalid(field, "missing native header for " + node.qualifiedName);
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
            deps.headers.insert("cocos/base/Pmr.h");
        }
        return m;
    }
    const auto* instance = std::get_if<TemplateInstanceType>(&node.data);
    if (instance && (node.typescriptName || node.typescriptContainer != ContainerKind::None ||
                     (!emitTs && std::get<ExternalTypeNode>(graph.node(instance->declaration).data).container != ContainerKind::None))) {
        if (field.isNullable || (emitTs && !field.initializer.empty())) invalid(field, "expected a default-constructed container field");
        m.cpp = nativeType(graph, module, field.type.type, deps);
        if (node.typescriptContainer == ContainerKind::None) deps.headers.insert("utility");
        m.move = true;
        const auto& container = std::get<ExternalTypeNode>(graph.node(instance->declaration).data);
        m.usesAllocator = container.isPmr;
        if (graph.node(instance->declaration).qualifiedName == "ccstd::pmr::vector") {
            m.emptyPmrArgument = "0U";
            deps.headers.insert("cocos/base/Pmr.h");
        }
        if (emitTs) {
            m.ts = boundTypeScriptName(graph, field.type, owner, deps.typeImports);
            m.tsDefault = typeScriptContainerInitializer(node);
            m.tsKeepOnReset = node.typescriptContainer == ContainerKind::None && isUnclampedIntegerTypedArrayName(m.ts);
            if (node.typescriptContainer == ContainerKind::Array) m.tsReset = "this." + m.name + ".length = 0";
            if (isAssociativeContainer(node.typescriptContainer)) m.tsReset = "this." + m.name + ".clear()";
        }
        return m;
    }
    const auto* record = std::get_if<RecordType>(&node.data);
    const auto* nestedGraph = std::get_if<GraphType>(&node.data);
    const auto bindObject = [&] {
        const bool nullable = field.typescriptInitializer == "null";
        const bool needsArgs = record && record->typescriptConstructor && !record->typescriptConstructor->defaults &&
                               !record->typescriptConstructor->fields.empty();
        const bool canConstruct = !node.typescriptTypeOnly && !needsArgs;
        m.ts = boundTypeScriptName(graph, field.type, owner, nullable || typeOnly || !canConstruct ? deps.typeImports : deps.valueImports);
        if (nullable) m.tsDefault = "null";
        else if (canConstruct) m.tsDefault = "new " + m.ts + "()";
        else if (!typeOnly) invalid(field, "no TypeScript default for " + m.ts);
        if (nullable) m.ts += " | null";
    };
    if ((record || nestedGraph) && !field.isNullable && field.initializer.empty()) {
        m.cpp = nativeType(graph, module, field.type.type, deps);
        std::set<uint32_t> visited;
        const auto storage = recordStorage(graph, field.type.type, visited);
        m.usesAllocator = storage.usesAllocator;
        m.defaultNeedsAllocator = record && storage.usesAllocator && hasAccessibleEmptyConstructor(graph, *record) &&
                                  !hasDefaultAllocator(*record);
        m.containsGraph = storage.containsGraph;
        // Pass copyable, single-scalar value records by reference in native constructors.
        if (record && record->bases.empty() && !record->moveOnly && record->fields.size() == 1) {
            const auto inner = record->fields.front().type.type;
            const auto primitive = archivePrimitive(graph, inner);
            m.byConstReference = !std::holds_alternative<RecordType>(graph.node(inner).data) && (primitive == "Number" || primitive == "Bool");
        }
        m.move = !m.byConstReference;
        if (emitTs) {
            if (record && node.typescriptNumber) {
                m.ts = "number";
                const auto& value = record->fields.front();
                m.tsDefault = value.typescriptInitializer.empty() ? value.initializer : value.typescriptInitializer;
                if (m.tsDefault.empty()) m.tsDefault = "0";
                return m;
            }
            if (node.module != owner && node.typescriptModule.empty()) invalid(field, "missing external record mapping");
            bindObject();
            if (nestedGraph && nestedGraph->mutableGraph && m.tsDefault != "null") m.tsReset = "this." + m.name + ".clear()";
        }
        return m;
    }
    if (const auto* ext = std::get_if<ExternalTypeNode>(&node.data); ext && !field.isNullable &&
        (ext->constructorConstRef || (!isScalar && !node.typescriptModule.empty()) || !node.typescriptArchiveModule.empty())) {
        if (!field.initializer.empty()) invalid(field, "expected a default-constructed external value");
        m.cpp = nativeType(graph, module, field.type.type, deps);
        m.byConstReference = ext->constructorConstRef;
        m.move = true;
        m.usesAllocator = ext->isPmr;
        if (emitTs) {
            bindObject();
            if (field.typescriptInitializer != "null") {
                m.tsResetsObjectInPlace = true;
                m.tsReset = "this." + m.name + ".reset()";
            }
        }
        return m;
    }
    if (!emitTs && !field.isNullable) {
        m.cpp = nativeType(graph, module, field.type.type, deps);
        m.move = !isScalar;
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
            if (deps.emitCpp && wrapper.cppHeader.empty()) invalid(field, "missing native header for " + wrapper.qualifiedName);
            deps.headers.insert(wrapper.cppHeader);
            deps.headers.insert("utility");
            target = std::get<TypeId>(instance->arguments[0]);
        }
        const auto* qualifier = std::get_if<QualifiedType>(&graph.node(target).data);
        const auto& resource = graph.node(qualifier ? qualifier->base : target);
        const auto* external = std::get_if<ExternalTypeNode>(&resource.data);
        if (!external && !emitTs && ptr) {
            m.cpp = nativeType(graph, module, field.type.type, deps);
            m.cppDefault = "nullptr";
            return m;
        }
        if (!external) invalid(field, "expected an external resource");
        if (emitTs) {
            if (!resource.typescriptName || resource.typescriptModule.empty()) invalid(field, "missing external resource mapping");
            deps.typeImports[resource.typescriptModule].insert(*resource.typescriptName);
            m.ts = *resource.typescriptName + " | null";
            m.tsDefault = "null";
        }
        const auto name = cppName(graph, resource, module);
        if (ptr) {
            m.cpp = pointerType(graph, module, field.type.type, resource, deps);
            m.cppDefault = "nullptr";
            m.comment = "/*pointer*/ ";
        } else {
            if (deps.emitCpp && resource.cppHeader.empty()) invalid(field, "missing native header for " + resource.qualifiedName);
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
    if (m.tsDefault.empty() && m.ts == "boolean") m.tsDefault = "false";
    if (m.tsDefault.empty() && m.ts == "number") m.tsDefault = "0";
    if (const auto* enumeration = std::get_if<EnumType>(&node.data)) {
        if (node.module != owner) {
            if ((deps.emitCpp && node.cppHeader.empty()) || node.typescriptModule.empty()) invalid(field, "missing external enum mapping");
            (typeOnly ? deps.typeImports : deps.valueImports)[node.typescriptModule].insert(m.ts);
        }
        if (field.initializer.empty()) {
            const auto zero = std::ranges::find_if(enumeration->values, [](const Enumerator& e) {
                return std::visit([](auto value) { return value == 0; }, e.value);
            });
            if (zero != enumeration->values.end()) m.tsDefault = m.ts + "." + zero->name;
            else if (!typeOnly) invalid(field, "enum needs a named default");
        } else {
            const auto pos = field.initializer.rfind("::");
            if (pos == std::string::npos) invalid(field, "expected a named enum default");
            const auto member = field.initializer.substr(pos + 2);
            m.cppDefault = m.cpp + "::" + member;
            m.tsDefault = m.ts + "." + member;
        }
        return m;
    }
    if (m.ts == "boolean" && (m.tsDefault == "false" || m.tsDefault == "true")) return m;
    m.safeUnsignedInteger = scalar == "uint64_t";
    if (m.safeUnsignedInteger) {
        if (!field.typescriptInitializer.empty()) m.tsDefault = field.typescriptInitializer;
        if (!safeUnsignedNumber(m.tsDefault)) {
            if (!typeOnly) invalid(field, "uint64_t default exceeds the TypeScript safe integer range: " + m.tsDefault);
            m.tsDefault.clear();
        }
        return m;
    }
    if (m.ts == "number" && numericDefault(scalar, m.tsDefault)) return m;
    invalid(field, "unsupported scalar default " + field.initializer);
}

std::vector<BoundField> bindFields(const TypeGraph& graph, const TypeNode& owner, const std::vector<FieldInfo>& fields,
                                   Dependencies& deps, bool emitTs, const NestedResets& nestedResets,
                                   const std::vector<uint32_t>& requiredFields = {}) {
    const auto* ownerRecord = std::get_if<RecordType>(&owner.data);
    const bool immovable = ownerRecord ? ownerRecord->immovable : std::get<GraphType>(owner.data).immovable;
    std::vector<BoundField> result;
    for (const auto& field : fields) {
        const auto& data = graph.node(field.type.type).data;
        const auto* nestedGraph = std::get_if<GraphType>(&data);
        const auto* nestedRecord = std::get_if<RecordType>(&data);
        if ((nestedGraph && nestedGraph->immovable) || (nestedRecord && nestedRecord->immovable)) {
            if (!immovable)
                invalid(field, std::string("cannot use an @immovable ") + (nestedGraph ? "graph" : "record") + " as a value field of a movable owner");
            if (ownerRecord && std::ranges::any_of(ownerRecord->constructors, [&](const ConstructorInfo& ctor) {
                    return std::ranges::find(ctor.fields, result.size()) != ctor.fields.end();
                })) invalid(field, "an @immovable value field must be constructed in place");
        }
        auto member = bindMember(graph, owner.module, field, deps, emitTs && !field.cppOnly,
                                 std::ranges::find(requiredFields, result.size()) != requiredFields.end());
        if (emitTs && !field.cppOnly && member.tsDefault != "null") {
            const auto childReset = nestedResets.find(field.type.type.value);
            if (childReset != nestedResets.end()) member.tsResetsObjectInPlace = true;
            if (childReset != nestedResets.end() && field.isReadonly) {
                std::string args;
                for (const auto& arg : childReset->second) {
                    if (arg.expression.empty()) invalid(field, "nested reset argument has no TypeScript default");
                    const auto& type = graph.node(arg.type);
                    if (arg.expression.starts_with("new ") || std::holds_alternative<EnumType>(type.data)) {
                        auto& imports = type.typescriptContainer == ContainerKind::None ? deps.valueImports : deps.typeImports;
                        boundTypeScriptName(graph, arg.type, owner.module, imports, field.name);
                    }
                    if (!args.empty()) args += ", ";
                    args += arg.expression;
                }
                member.tsReset = "this." + member.name + ".reset(" + args + ")";
            }
        }
        if (emitTs && field.typescriptInitializer == "null") {
            if (field.cppOnly || member.tsDefault != "null" || field.isNullable || field.isReadonly)
                invalid(field, "TypeScript default null requires a writable value object field");
        } else if (emitTs && !field.typescriptInitializer.empty()) {
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

void validateObjectResets(const TypeNode& owner, const std::vector<FieldInfo>& fields,
                          const std::vector<BoundField>& members, const std::set<std::string>& keepOnReset) {
    for (const auto& member : members) {
        if (!member.tsResetsObjectInPlace || member.readonly || keepOnReset.contains(member.name)) continue;
        const auto field = std::ranges::find(fields, member.name, &FieldInfo::name);
        throw std::runtime_error(field->type.location.describe() + ": " + owner.qualifiedName + "." + member.name +
                                 ": in-place reset requires a readonly field");
    }
}

void removeCppOnlyFields(const RecordType& record, std::vector<BoundField>& members, Order& order) {
    if (std::ranges::none_of(record.fields, &FieldInfo::cppOnly)) return;
    Order indices(members.size());
    std::vector<BoundField> visible;
    for (uint32_t i = 0; i < members.size(); ++i) {
        if (record.fields[i].cppOnly) continue;
        indices[i] = static_cast<uint32_t>(visible.size());
        visible.push_back(std::move(members[i]));
    }
    members = std::move(visible);
    for (auto& i : order) i = indices[i];
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

TypeId constructorFieldType(const TypeGraph& graph, TypeId type) {
    while (const auto* alias = std::get_if<AliasType>(&graph.node(type).data)) type = alias->target;
    return type;
}

bool isExternalValue(const TypeData& data) {
    return std::holds_alternative<ExternalTypeNode>(data) || std::holds_alternative<TemplateInstanceType>(data);
}

bool hasNativeValueDefault(const TypeGraph& graph, TypeId type, std::map<uint32_t, bool>& knownDefaults) {
    type = constructorFieldType(graph, type);
    const auto& node = graph.node(type);
    if (std::holds_alternative<BuiltinType>(node.data) || std::holds_alternative<EnumType>(node.data) ||
        std::holds_alternative<PointerType>(node.data) || isExternalValue(node.data)) return true;
    if (const auto* record = std::get_if<RecordType>(&node.data)) {
        if (const auto it = knownDefaults.find(type.value); it != knownDefaults.end()) return it->second;
        std::set<uint32_t> visited;
        const bool pmr = recordStorage(graph, type, visited).usesAllocator;
        auto fieldHasDefault = [&](const FieldInfo& field) {
            return !field.initializer.empty() || hasNativeValueDefault(graph, field.type.type, knownDefaults);
        };
        bool available;
        if (!pmr) {
            available = std::ranges::all_of(record->fields, fieldHasDefault);
        } else {
            available = hasDefaultAllocator(*record);
        }
        available = available && hasAccessibleEmptyConstructor(graph, *record);
        knownDefaults.emplace(type.value, available);
        return available;
    }
    return false;
}

std::string constructorSignature(const ConstructorInfo& ctor, const RecordType& record) {
    std::string result = "constructor(";
    for (size_t i = 0; i < ctor.fields.size(); ++i) {
        if (i) result += ", ";
        result += record.fields[ctor.fields[i]].name;
    }
    return result + (ctor.defaults ? ") defaults" : ")");
}

void validateNativeConstructorDefaults(const TypeGraph& graph, const TypeNode& node, const std::vector<BoundField>& members, bool pmr) {
    const auto& record = std::get<RecordType>(node.data);
    std::map<uint32_t, bool> knownDefaults;
    auto type = [&](uint32_t i) { return constructorFieldType(graph, record.fields[i].type.type); };
    for (const auto& ctor : record.constructors) {
        if (!ctor.defaults) continue;
        for (const auto i : ctor.fields) {
            const auto& field = record.fields[i];
            if (field.initializer.empty() && !members[i].defaultNeedsAllocator && !hasNativeValueDefault(graph, type(i), knownDefaults))
                invalid(field, node.qualifiedName + " " + constructorSignature(ctor, record) +
                               ": no C++ default for " + qualifiedTypeName(graph, type(i)));
        }
        for (const auto& other : record.constructors) {
            if (&ctor == &other) continue;
            // Defaulted PMR field overloads all accept (allocator_arg, alloc).
            // An explicit empty PMR declaration shares the allocator-only overload.
            const bool sharedDefaults = other.defaults && (!pmr || (!ctor.fields.empty() && !other.fields.empty()));
            const bool prefix = !pmr && other.fields.size() <= ctor.fields.size() &&
                std::equal(other.fields.begin(), other.fields.end(), ctor.fields.begin(),
                           [&](auto lhs, auto rhs) { return type(lhs) == type(rhs); });
            if (sharedDefaults || prefix) {
                const auto call = pmr ? " for " + node.name + "(std::allocator_arg, alloc)" :
                    " at " + std::to_string(other.defaults ? 0 : other.fields.size()) + " arguments";
                throw std::runtime_error(node.qualifiedName + ": " + constructorSignature(ctor, record) +
                                         " conflicts with " + constructorSignature(other, record) + call);
            }
        }
    }
}

std::string nativeDefaultArgument(const TypeGraph& graph, TypeId type, const BoundField& field) {
    if (!field.cppDefault.empty()) return " = " + field.cppDefault;
    const auto& node = graph.node(constructorFieldType(graph, type));
    return isExternalValue(node.data) ? " = " + field.cpp + "{}" : " = {}";
}

struct BaseCall {
    TypeId type;
    std::vector<const FieldInfo*> fields;
    bool nothrow{};
    bool allocatorTagged{};
};

using BaseCalls = std::vector<BaseCall>;

// Each combination calls one constructor per direct record base, in inheritance order.
std::vector<BaseCalls> baseConstructors(const TypeGraph& graph, const RecordType& record) {
    std::vector<BaseCalls> combinations(1);
    for (auto id : record.bases) {
        const auto* base = std::get_if<RecordType>(&graph.node(id).data);
        if (!base) continue;
        const auto empty = std::ranges::find_if(base->constructors, accessibleWithoutFields);
        std::vector<BaseCalls> next;
        for (const auto& ancestors : baseConstructors(graph, *base)) {
            BaseCall call{id, {}, true};
            for (const auto& ancestor : ancestors) {
                call.fields.insert(call.fields.end(), ancestor.fields.begin(), ancestor.fields.end());
                call.nothrow &= ancestor.nothrow;
            }
            auto add = [&](const ConstructorInfo* ctor) {
                auto choice = call;
                if (ctor) {
                    if (!ctor->defaults) {
                        for (auto index : ctor->fields) choice.fields.push_back(&base->fields.at(index));
                    }
                    choice.nothrow &= !ctor->custom || ctor->isNoexcept;
                    // A private explicit (alloc) can coexist with public allocator-tagged defaults.
                    if (ctor->defaults && !ctor->fields.empty()) choice.allocatorTagged = emptyConstructor(*base)->cppPrivate;
                }
                // An implicit or explicitly defaulted base constructor may run throwing field initializers.
                if (choice.fields.empty() && (!ctor || (!ctor->custom && ctor->fields.empty())) && !base->fields.empty())
                    choice.nothrow = false;
                for (auto combination : combinations) {
                    combination.push_back(choice);
                    next.push_back(std::move(combination));
                }
            };
            if (base->constructors.empty()) add(nullptr);
            else if (empty != base->constructors.end()) add(&*empty);
            else for (const auto& ctor : base->constructors) {
                if (!ctor.cppPrivate) add(&ctor);
            }
        }
        if (next.empty()) throw std::runtime_error(graph.node(id).qualifiedName + ": no accessible base constructor");
        combinations = std::move(next);
    }
    return combinations;
}

void appendArgument(std::string& list, const std::string& argument) {
    if (!list.empty()) list += ", ";
    list += argument;
}

std::string inputArgument(const BoundField& field, std::set<std::string>& headers) {
    const auto argument = field.name + "In";
    if (!field.move || field.byConstReference) return argument;
    headers.insert("utility");
    return "std::move(" + argument + ")";
}

struct BaseInitializer {
    std::string name;
    std::string arguments;
    bool usesAllocator{};
    bool containsGraph{};
    bool allocatorTagged{};
};

struct ConstructorPrefix {
    std::string parameters;
    std::vector<BaseInitializer> bases;
    bool nothrow{true};
    Order types;
};

ConstructorPrefix bindBaseArguments(const TypeGraph& graph, const TypeNode& node, const BaseCalls& calls, Dependencies& deps) {
    ConstructorPrefix prefix;
    std::set<std::string> names;
    for (const auto& field : std::get<RecordType>(node.data).fields) names.insert(field.name);
    for (const auto& call : calls) {
        const auto& base = graph.node(call.type);
        std::set<uint32_t> visited;
        const auto storage = recordStorage(graph, call.type, visited);
        BaseInitializer init{cppName(graph, base, graph.modules.at(node.module.value)), {}, storage.usesAllocator, storage.containsGraph, call.allocatorTagged};
        if (call.allocatorTagged) deps.headers.insert("memory");
        for (const auto* field : call.fields) {
            auto member = bindMember(graph, node.module, *field, deps, false, false);
            if (names.contains(member.name)) member.name = base.name + "_" + member.name;
            while (!names.insert(member.name).second) member.name += "_";
            appendArgument(prefix.parameters, member.parameterType() + " " + member.name + "In");
            appendArgument(init.arguments, inputArgument(member, deps.headers));
            prefix.types.push_back(constructorFieldType(graph, field->type.type).value);
        }
        prefix.bases.push_back(std::move(init));
        prefix.nothrow &= call.nothrow;
    }
    return prefix;
}

void validateInheritedOverloads(const TypeGraph& graph, const TypeNode& node, const std::vector<ConstructorPrefix>& prefixes, bool pmr) {
    if (prefixes.size() == 1) return;
    const auto& record = std::get<RecordType>(node.data);
    std::set<std::pair<bool, Order>> signatures;
    auto add = [&](bool allocatorTagged, const Order& types) {
        if (!signatures.emplace(allocatorTagged, types).second)
            throw std::runtime_error(node.qualifiedName + ": inherited constructor overloads conflict at " +
                                     std::to_string(types.size()) + " field arguments");
    };
    for (const auto& prefix : prefixes) {
        if (record.constructors.empty() || (pmr && hasAllocatorConstructor(record))) add(false, prefix.types);
        for (const auto& ctor : record.constructors) {
            if (pmr && ctor.fields.empty()) continue;
            auto types = prefix.types;
            const bool tagged = pmr && ctor.defaults;
            for (auto i : ctor.fields) {
                if (ctor.defaults) add(tagged, types);
                types.push_back(constructorFieldType(graph, record.fields[i].type.type).value);
            }
            add(tagged, types);
        }
    }
}

// The emitter groups special members and fields, so each declaration carries its source access.
void memberAccess(std::ostream& out, bool& currentPrivate, bool nextPrivate) {
    if (currentPrivate == nextPrivate) return;
    out << (nextPrivate ? "private:\n" : "public:\n");
    currentPrivate = nextPrivate;
}

void inlineConstructors(std::ostream& out, const TypeGraph& graph, const std::string& name, const RecordType& record,
                        const std::vector<BoundField>& members, const ConstructorPrefix& prefix, bool specialMembers,
                        std::set<std::string>& headers, bool& currentPrivate) {
    const auto empty = std::ranges::find_if(record.constructors, [](const auto& c) { return c.fields.empty(); });
    if (prefix.types.empty() && (empty != record.constructors.end() || (record.constructors.empty() && specialMembers))) {
        memberAccess(out, currentPrivate, empty != record.constructors.end() && empty->cppPrivate);
        if (empty != record.constructors.end()) writeDocumentation(out, empty->documentation, "    ");
        if (empty != record.constructors.end() && empty->custom)
            out << "    " << name << "()" << (empty->isNoexcept ? " noexcept" : "") << ";\n";
        else out << "    " << name << "() = default;\n";
    }
    auto write = [&](const ConstructorInfo& declaration) {
        const auto& ctor = declaration.fields;
        if (ctor.empty() && prefix.types.empty()) return;
        memberAccess(out, currentPrivate, declaration.cppPrivate);
        writeDocumentation(out, declaration.documentation, "    ");
        out << "    " << name << "(" << prefix.parameters;
        for (size_t i = 0; i < ctor.size(); ++i) {
            const auto& m = members.at(ctor[i]);
            if (i || !prefix.types.empty()) out << ", ";
            out << m.parameterType() << " " << m.name << "In";
            if (declaration.defaults) out << nativeDefaultArgument(graph, record.fields[ctor[i]].type.type, m);
        }
        if (declaration.custom) {
            out << ')' << (declaration.isNoexcept ? " noexcept" : "") << ";\n";
            return;
        }
        out << ')' << (prefix.nothrow ? " noexcept" : "") << (prefix.types.size() + ctor.size() == 1 ? " // NOLINT\n" : "\n");
        bool first = true;
        for (const auto& base : prefix.bases) {
            if (base.arguments.empty()) continue;
            out << (first ? "    : " : ",\n      ") << base.name << "(" << base.arguments << ")";
            first = false;
        }
        for (uint32_t i = 0; i < members.size(); ++i) {
            if (std::ranges::find(ctor, i) == ctor.end()) continue;
            const auto& m = members[i];
            out << (first ? "    : " : ",\n      ") << m.name << "(";
            out << inputArgument(m, headers) << ")";
            first = false;
        }
        out << " {}\n";
    };
    if (record.constructors.empty()) write({});
    else for (const auto& declaration : record.constructors) write(declaration);
    if (!prefix.types.empty() || !record.constructors.empty() || record.moveOnly) out << '\n';
}

enum class CtorArgs { fields, move, copy };

std::string pmrArguments(const BoundField& m, std::string value, bool move) {
    if (value.empty() && m.usesAllocator)
        return m.allocatorArguments();
    if (!value.empty() && m.move && move) value = "std::move(" + value + ")";
    if (m.usesAllocator) value += value.empty() ? "alloc" : ", alloc";
    return value;
}

void pmrInitializers(std::ostream& source, const ConstructorPrefix& prefix, const std::vector<BoundField>& members,
                     CtorArgs args, const Order& fields = {}) {
    bool first = true;
    for (const auto& base : prefix.bases) {
        auto arguments = base.arguments;
        if (args == CtorArgs::move) arguments = "std::move(static_cast<" + base.name + "&>(rhs))";
        if (args == CtorArgs::copy) arguments = "static_cast<const " + base.name + "&>(rhs)";
        if (args == CtorArgs::fields && base.allocatorTagged) {
            arguments = arguments.empty() ? "std::allocator_arg, alloc" : "std::allocator_arg, alloc, " + arguments;
        } else if (base.usesAllocator) appendArgument(arguments, "alloc");
        if (arguments.empty()) continue;
        source << (first ? "\n: " : ",\n  ") << base.name << '(' << arguments << ')';
        first = false;
    }
    for (uint32_t i = 0; i < members.size(); ++i) {
        const auto& m = members[i];
        const bool hasArg = args == CtorArgs::move || args == CtorArgs::copy ||
                            (args == CtorArgs::fields && std::ranges::find(fields, i) != fields.end());
        if (!m.usesAllocator && !hasArg && !m.cppDefault.empty()) continue;
        std::string value;
        if (hasArg) value = args == CtorArgs::fields ? m.name + "In" : "rhs." + m.name;
        const bool moveValue = args == CtorArgs::move || (args == CtorArgs::fields && !m.byConstReference);
        source << (first ? "\n: " : ",\n  ") << m.name << "(" << pmrArguments(m, value, moveValue) << ")";
        first = false;
    }
    source << " {}\n";
}

void pmrFieldConstructors(std::ostream& header, std::ostream& source, const TypeGraph& graph, const std::string& name,
                          const RecordType& record, const std::vector<BoundField>& members, const ConstructorPrefix& prefix,
                          std::set<std::string>& headers, bool& currentPrivate) {
    const std::string alloc = "const allocator_type& alloc";
    const auto constructor = "\n" + name + "::" + name + "(";
    auto writeConstructor = [&](const Order& order, const std::string& declaration, const std::string& params, const ConstructorInfo* ctor = nullptr) {
        memberAccess(header, currentPrivate, ctor && ctor->cppPrivate);
        if (ctor) writeDocumentation(header, ctor->documentation, "    ");
        header << "    " << name << "(" << declaration << ")" << (ctor && ctor->isNoexcept ? " noexcept" : "")
               << ";" << (params == alloc ? " // NOLINT\n" : "\n");
        if (ctor && ctor->custom) return;
        source << constructor << params << ")";
        if (!order.empty()) source << " // NOLINT";
        pmrInitializers(source, prefix, members, CtorArgs::fields, order);
    };
    if (hasAllocatorConstructor(record)) {
        const auto empty = emptyConstructor(record);
        auto params = prefix.parameters;
        appendArgument(params, alloc);
        auto declaration = params;
        if (hasDefaultAllocator(record)) declaration += " = boost::container::pmr::get_default_resource()";
        writeConstructor({}, declaration, params, empty == record.constructors.end() ? nullptr : &*empty);
    }
    for (const auto& ctor : record.constructors) {
        if (ctor.fields.empty()) continue;
        std::string params;
        if (ctor.defaults) {
            headers.insert("memory");
            params = "const std::allocator_arg_t&, " + alloc;
        }
        if (!prefix.parameters.empty()) appendArgument(params, prefix.parameters);
        auto declaration = params;
        Order supplied;
        for (const auto index : ctor.fields) {
            const auto& m = members.at(index);
            // A default argument cannot refer to alloc. End the preceding overload
            // here so its member initializers can construct the omitted field with alloc.
            if (ctor.defaults && m.defaultNeedsAllocator) {
                writeConstructor(supplied, declaration, params, &ctor);
                declaration = params;
            }
            if (m.move) headers.insert("utility");
            const auto param = m.parameterType() + " " + m.name + "In";
            if (!params.empty()) {
                params += ", ";
                declaration += ", ";
            }
            params += param;
            declaration += param;
            if (ctor.defaults && !m.defaultNeedsAllocator) declaration += nativeDefaultArgument(graph, record.fields[index].type.type, m);
            supplied.push_back(index);
        }
        if (!ctor.defaults) {
            params += ", " + alloc;
            declaration = params;
        }
        writeConstructor(supplied, declaration, params, &ctor);
    }
}

void pmrSpecialConstructors(std::ostream& header, std::ostream& source, const std::string& name, bool moveOnly,
                            const std::vector<BoundField>& members, const ConstructorPrefix& prefix) {
    const std::string alloc = "const allocator_type& alloc";
    for (auto args : {CtorArgs::move, CtorArgs::copy}) {
        if (args == CtorArgs::copy && moveOnly) continue;
        const auto rhs = name + (args == CtorArgs::move ? "&& rhs" : " const& rhs");
        header << "    " << name << "(" << rhs << ", " << alloc << ");\n";
        source << "\n" << name << "::" << name << "(" << rhs << ", " << alloc << ")";
        pmrInitializers(source, prefix, members, args);
    }
    header << '\n';
}

void cppComparisons(std::ostream& out, const std::string& name, const std::vector<std::string_view>& elements, const auto& record) {
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
        if (std::holds_alternative<TemplateInstanceType>(target.data)) continue;
        collectCppHeader(graph, module, target, includes);
        out << '\n';
        writeDocumentation(out, node.documentation);
        out << "using " << node.name << " = " << cppName(graph, target, module) << ";\n";
    }
}

void writeTemplateAliases(std::ostream& out, const TypeGraph& graph, ModuleId owner, Dependencies& deps) {
    std::set<uint32_t> visiting, emitted;
    const auto& module = graph.modules.at(owner.value);
    const auto write = [&](auto& self, TypeId id) -> void {
        const auto& node = graph.node(id);
        const auto* alias = std::get_if<AliasType>(&node.data);
        if (!alias || node.module != owner || emitted.contains(id.value) ||
            !std::holds_alternative<TemplateInstanceType>(graph.node(alias->target).data)) return;
        if (!visiting.insert(id.value).second) throw std::runtime_error(node.qualifiedName + ": cyclic type alias");
        const auto target = formatTypeName(graph, alias->target, [&](const TypeNode& part) {
            if (std::holds_alternative<AliasType>(part.data)) self(self, graph.findType(part.qualifiedName));
            collectCppHeader(graph, module, part, deps.headers);
            return cppName(graph, part, module);
        });
        visiting.erase(id.value);
        emitted.insert(id.value);
        out << '\n';
        writeDocumentation(out, node.documentation);
        out << "using " << node.name << " = " << target << ";\n";
    };
    for (auto id : module.declarations) write(write, id);
}

std::string forwardHeader(const TypeGraph& graph, const TypeModule& module, const std::ostringstream& hashes,
                          const std::string& preamble, const std::set<std::string>& forwardIncludes) {
    std::ostringstream header, body;
    std::set<std::string> includes;
    std::optional<bool> previousEnum;
    writeScalarAliases(body, graph, module, includes);
    for (auto id : module.declarations) {
        const auto& node = graph.node(id);
        if (std::holds_alternative<AliasType>(node.data)) continue;
        const auto* enumeration = std::get_if<EnumType>(&node.data);
        const bool isEnum = enumeration != nullptr;
        if (previousEnum != isEnum) body << '\n';
        previousEnum = isEnum;
        const auto* record = std::get_if<RecordType>(&node.data);
        const bool isClass = std::holds_alternative<InterfaceType>(node.data) || (record && record->isClass);
        body << (isEnum ? "enum class " : isClass ? "class " : "struct ") << node.name;
        if (enumeration && enumeration->underlying != TypeId{}) {
            const auto& base = graph.node(enumeration->underlying);
            collectCppHeader(graph, module, base, includes);
            body << " : " << cppName(graph, base, module);
        }
        body << ";\n";
    }
    if (!hashes.view().empty()) {
        includes.insert("cocos/base/std/hash/hash.h");
        includes.erase("cstdint"); // The engine hash header already supplies the fixed-width integer types.
    }
    header << preamble << "#pragma once\n// clang-format off\n";
    for (const auto& include : forwardIncludes) {
        header << "#include \"" << include << "\"\n";
        includes.erase(include);
    }
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

std::string cppMemberFunctions(const TypeGraph& graph, const TypeModule& module, const std::vector<MethodInfo>& methods,
                               Dependencies& deps, bool currentPrivate = false) {
    std::ostringstream out;
    for (const auto& method : methods) {
        memberAccess(out, currentPrivate, method.cppPrivate);
        writeDocumentation(out, method.documentation, "    ");
        const auto type = methodType(graph, module, method.result.type, deps);
        if (method.isExplicitConversion) out << "    explicit operator " << type;
        else out << "    " << (method.isStatic ? "static " : "") << type << " " << method.name;
        out << "(";
        for (size_t i = 0; i < method.parameters.size(); ++i) {
            if (i) out << ", ";
            const auto& p = method.parameters[i];
            if (p.byConstReference) out << "const ";
            out << methodType(graph, module, p.type.type, deps) << (p.byConstReference ? "& " : " ") << p.name;
            if (!p.defaultValue.empty()) out << " = " << p.defaultValue;
        }
        out << ")";
        if (method.isConst) out << " const";
        if (method.isNoexcept) out << " noexcept";
        if (method.cppBody.empty()) out << ";\n";
        else out << ' ' << method.cppBody << '\n';
    }
    return out.str();
}

struct StaticField {
    BoundField member;
    bool forwardEnum;
};

void cppRecordFriends(std::ostream& out, const TypeGraph& graph, const TypeModule& module, const TypeNode& node,
                      const TypeModule* archive, Dependencies& deps) {
    const auto& record = std::get<RecordType>(node.data);
    const auto& name = node.name;
    if (std::ranges::any_of(record.fields, [](const auto& f) { return f.cppPrivate && !f.notElement; })) {
        if (record.compareEqual) out << "    friend bool operator==(const " << name << "&, const " << name << "&) noexcept;\n";
        if (record.compareLess) out << "    friend bool operator<(const " << name << "&, const " << name << "&) noexcept;\n";
        if (record.hashable) out << "    friend struct ::ccstd::hash<" << name << ">;\n";
    }
    if (archive && record.serializable && std::ranges::any_of(record.fields, [](const auto& f) { return f.cppPrivate && !f.transient; })) {
        out << "    friend void save(" << nativeType(graph, module, archive->outputArchive, deps) << "&, const " << name << "&);\n"
            << "    friend void load(" << nativeType(graph, module, archive->inputArchive, deps) << "&, " << name << "&);\n";
    }
}

void cppRecord(std::ostream& out, std::ostream& source, const TypeGraph& graph, const TypeModule& module,
               const TypeNode& node, const std::vector<BoundField>& members, const std::vector<StaticField>& statics,
               const TypeModule* archive, Dependencies& deps) {
    const auto& record = std::get<RecordType>(node.data);
    const bool immovable = !recordHierarchySatisfies(graph, record, [](const RecordType& r) { return !r.immovable; });
    const bool moveOnly = record.moveOnly || std::ranges::any_of(record.bases, [&](TypeId id) { return hasMoveOnlyValue(graph, id); });
    for (size_t i = 0; i < members.size(); ++i) {
        const auto& field = record.fields[i];
        if (!immovable && !moveOnly && members[i].containsGraph && hasMoveOnlyValue(graph, field.type.type))
            invalid(field, "a record containing a move-only graph must declare @moveOnly");
    }
    writeDocumentation(out, node.documentation);
    out << (record.isClass ? "class " : "struct ") << node.name;
    if (record.isFinal) out << " final";
    for (auto base : record.bases)
        out << (base == record.bases.front() ? " : public " : ", public ") << nativeType(graph, module, base, deps);
    out << " {\n";
    if (record.isClass) out << "public:\n";
    cppRecordFriends(out, graph, module, node, archive, deps);
    bool currentPrivate = false;
    std::map<uint32_t, bool> staticDefaults;
    for (size_t i = 0; i < statics.size(); ++i) {
        const auto& [member, forwardEnum] = statics[i];
        const auto& field = record.staticFields[i];
        memberAccess(out, currentPrivate, field.cppPrivate);
        writeDocumentation(out, field.documentation, "    ");
        const auto& type = graph.node(field.type.type);
        if (member.cppDefault.empty() && !hasNativeValueDefault(graph, field.type.type, staticDefaults))
            invalid(field, "no C++ default for static field of type " + type.qualifiedName);
        const auto* alias = std::get_if<AliasType>(&type.data);
        const bool scalarAlias = alias && !std::holds_alternative<TemplateInstanceType>(graph.node(alias->target).data);
        if (std::holds_alternative<BuiltinType>(type.data) || scalarAlias ||
            (std::holds_alternative<EnumType>(type.data) && !forwardEnum) || std::holds_alternative<PointerType>(type.data)) {
            out << "    inline static " << member.cpp << " " << member.name << "{" << member.cppDefault << "};\n";
        } else {
            out << "    static " << member.cpp << " " << member.name << ";\n";
            source << "\n" << member.cpp << " " << node.name << "::" << member.name << "{" << member.cppDefault << "};\n";
        }
    }
    const auto allocator = std::ranges::find(members, true, &BoundField::usesAllocator);
    std::vector<ConstructorPrefix> prefixes;
    for (const auto& calls : baseConstructors(graph, record)) prefixes.push_back(bindBaseArguments(graph, node, calls, deps));
    const auto& bases = prefixes.front().bases;
    const auto allocatorBase = std::ranges::find(bases, true, &BaseInitializer::usesAllocator);
    const bool usesAllocator = allocator != members.end() || allocatorBase != bases.end();
    validateNativeConstructorDefaults(graph, node, members, usesAllocator);
    validateInheritedOverloads(graph, node, prefixes, usesAllocator);
    if (!usesAllocator) {
        for (const auto& prefix : prefixes)
            inlineConstructors(out, graph, node.name, record, members, prefix, immovable || moveOnly, deps.headers, currentPrivate);
    } else {
        memberAccess(out, currentPrivate, false);
        const auto accessor = allocator != members.end() ? allocator->name + "." : allocatorBase->name + "::";
        out << "    using allocator_type = boost::container::pmr::polymorphic_allocator<char>;\n"
            << "    allocator_type get_allocator() const noexcept { // NOLINT\n"
            << "        return {" << accessor << "get_allocator().resource()};\n    }\n\n";
        for (const auto& prefix : prefixes)
            pmrFieldConstructors(out, source, graph, node.name, record, members, prefix, deps.headers, currentPrivate);
        if (!immovable) {
            memberAccess(out, currentPrivate, false);
            if (!bases.empty()) deps.headers.insert("utility");
            pmrSpecialConstructors(out, source, node.name, moveOnly, members, prefixes.front());
        }
    }
    if (immovable || moveOnly || usesAllocator) memberAccess(out, currentPrivate, false);
    if (immovable) {
        const auto& name = node.name;
        out << "    " << name << "(" << name << "&&) = delete;\n"
            << "    " << name << "(const " << name << "&) = delete;\n"
            << "    " << name << "& operator=(" << name << "&&) = delete;\n"
            << "    " << name << "& operator=(const " << name << "&) = delete;\n\n";
    } else if (usesAllocator || moveOnly) {
        const auto& name = node.name;
        // Move assignment of an embedded graph can allocate storage for its columns.
        const bool containsGraph = std::ranges::any_of(members, &BoundField::containsGraph) || std::ranges::any_of(bases, &BaseInitializer::containsGraph);
        const auto assignmentSpec = usesAllocator && !containsGraph ? " noexcept" : "";
        const bool assignable = !std::ranges::any_of(members, &BoundField::referenceMember);
        out << "    " << name << "(" << name << "&& rhs)" << (usesAllocator ? " noexcept" : "") << " = default;\n"
            << "    " << name << "(" << name << " const& rhs) = delete;\n"
            << "    " << name << "& operator=(" << name << "&& rhs)" << assignmentSpec << " = " << (assignable ? "default" : "delete") << ";\n"
            << "    " << name << "& operator=(" << name << " const& rhs) = " << (moveOnly || !assignable ? "delete" : "default") << ";\n\n";
    }
    if (record.destructorNoexcept.has_value()) {
        memberAccess(out, currentPrivate, record.destructorPrivate);
        writeDocumentation(out, record.destructorDocumentation, "    ");
        out << "    ~" << node.name << "()" << (*record.destructorNoexcept ? " noexcept" : "") << ";\n";
    }
    if (!record.bases.empty() || record.implements) {
        memberAccess(out, currentPrivate, false);
        emitImplementationMethods(out, graph, node, deps.headers);
    }
    if (!record.cppMembers.empty()) memberAccess(out, currentPrivate, false);
    out << record.cppMembers;
    for (size_t i = 0; i < members.size(); ++i) {
        const auto& m = members[i];
        memberAccess(out, currentPrivate, record.fields[i].cppPrivate);
        writeDocumentation(out, m.documentation, "    ");
        out << "    ";
        if (record.fields[i].isMutable) out << "mutable ";
        out << m.cpp << " " << m.name;
        if (!m.cppDefault.empty()) out << "{" << m.cppDefault << "}";
        out << ";\n";
    }
    out << cppMemberFunctions(graph, module, record.methods, deps, currentPrivate) << "};\n";
}

std::string tsClassMethods(const TypeScriptClassMethods& methods) {
    std::ostringstream out;
    for (const auto& entry : methods) {
        if (const auto* raw = std::get_if<std::string>(&entry)) {
            out << *raw;
            if (!raw->ends_with('\n')) out << '\n';
            continue;
        }
        const auto& method = std::get<TypeScriptClassMethod>(entry);
        out << '\n';
        writeDocumentation(out, method.documentation, "    ");
        out << "    " << method.name << " (";
        for (size_t i = 0; i < method.parameters.size(); ++i) {
            const auto& param = method.parameters[i];
            if (i) out << ", ";
            out << param.name << (param.optional ? "?: " : ": ") << param.type;
        }
        out << "): " << method.result << ' ' << method.body << '\n';
    }
    return out.str();
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
                      const std::set<std::string>& keepOnReset) {
    Order order;
    for (auto index : constructor) {
        const auto& member = members[index];
        if (!member.readonly && !keepOnReset.contains(member.name)) order.push_back(index);
    }
    auto retained = keepOnReset;
    for (const auto& m : members) {
        if (m.name == "reset") throw std::runtime_error(name + ".reset: field conflicts with reset method");
        if (retained.erase(m.name)) continue;
        if (m.readonly && m.tsReset.empty())
            throw std::runtime_error(name + "." + m.name + ": reset requires a writable field");
    }
    if (!retained.empty()) throw std::runtime_error(name + ": unknown keepOnReset field " + *retained.begin());
    return order;
}

void tsConstructorAndReset(std::ostream& ts, const std::vector<BoundField>& members, const Order& order,
                           const Order* reset, const std::set<std::string>& keepOnReset, bool hasBase, bool emitConstructor, bool defaults) {
    const auto params = tsParams(members, order, defaults, ", ");
    const bool multiline = params.size() + std::string_view("    constructor () {").size() > 120;
    auto signature = [&](const Order& fields, bool withDefaults, const std::string& singleLine) {
        if (multiline && !fields.empty()) return "\n        " + tsParams(members, fields, withDefaults, ",\n        ") + ",\n    ";
        return singleLine;
    };
    if (emitConstructor) {
        ts << "    constructor (" << signature(order, defaults, params) << ") {\n";
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
    }
    if (reset) {
        ts << "    reset (" << signature(*reset, false, tsParams(members, *reset, false, ", ")) << "): void {\n";
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

void tsRecord(std::ostream& ts, std::ostream& staticInitializers, const TypeNode& node, const std::vector<BoundField>& members,
              const std::vector<StaticField>& statics, const Order& order,
              const Order* reset, const std::set<std::string>& keepOnReset, const std::string& base,
              const std::optional<ConstructorInfo>& constructor, std::string_view classMethods) {
    writeDocumentation(ts, node.documentation);
    ts << "export class " << node.name;
    if (!base.empty()) ts << " extends " << base;
    ts << " {\n";
    for (const auto& [field, forwardEnum] : statics) {
        if (field.ts.empty()) continue; // Native-only field.
        writeDocumentation(ts, field.documentation, "    ");
        const auto declaration = std::string(field.readonly ? "static readonly " : "static ") + field.name + ": " + field.ts;
        if (field.safeUnsignedInteger) {
            ts << "    declare " << declaration << ";\n";
            staticInitializers << "cc$defineUint64(" << node.name << ", '" << field.name << "', " << field.tsDefault << ");\n";
        } else if (forwardEnum || field.tsDefault.starts_with("new ")) {
            // Initialize after local classes and enums are available.
            ts << "    declare " << declaration << ";\n";
            staticInitializers << "Object.assign(" << node.name << ", { " << field.name << ": " << field.tsDefault << " });\n";
        } else {
            ts << "    " << declaration << " = " << field.tsDefault << ";\n";
        }
    }
    const bool emitConstructor = constructor.has_value() || std::ranges::any_of(members, &BoundField::safeUnsignedInteger);
    if (constructor) writeDocumentation(ts, constructor->documentation, "    ");
    tsConstructorAndReset(ts, members, order, reset, keepOnReset, !base.empty(), emitConstructor, constructor && constructor->defaults);
    ts << classMethods;
    for (uint32_t i = 0; i < members.size(); ++i) {
        const auto& m = members[i];
        writeDocumentation(ts, m.documentation, "    ");
        if (m.safeUnsignedInteger) {
            ts << "    declare " << (m.readonly ? "readonly " : "") << m.name << ": number;\n";
            continue;
        }
        if (std::ranges::find(order, i) == order.end()) {
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
                                        const std::vector<BoundField>& members, ArchiveRequirements& requirements, bool emitTs) {
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
            if (field.typescriptInitializer == "null") invalid(field, "native value archive cannot encode TypeScript null");
            if (field.isReadonly && !archivePrimitive(graph, field.type.type).empty())
                invalid(field, "archive load requires a writable field");
            requirements.checkType(graph, owner, field.type.type, field.type.location.describe() + ": " + field.name, emitTs);
        }
        fields.push_back(std::move(entry));
    }
    return fields;
}

struct RecordArchives {
    std::ostringstream declarations, definitions, script;
    ArchiveRequirements requirements;

    void add(const TypeGraph& graph, const TypeModule& archive, const TypeNode& record, const std::vector<ArchiveField>& fields, bool emitTs, ArchiveImports& imports) {
        const auto& name = record.name;
        declarations << '\n';
        for (bool save : {true, false}) {
            const std::string op = save ? "save" : "load";
            const auto& api = graph.node(save ? archive.outputArchive : archive.inputArchive);
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

    void emitNative(Files& files, const TypeGraph& graph, const TypeModule& module, const RecordOutput& output,
                    const ArchiveOutput& archive, const std::string& preamble) const {
        const auto& path = *output.serializationHeader;
        const auto archiveHeader = archive.output.header.generic_string();
        std::ostringstream header, source;
        header << preamble << "#pragma once\n#include \"" << archiveHeader << "\"\n#include \""
               << output.forwardHeader.generic_string() << "\"\n";
        writeNamespace(header, module, declarations);
        source << preamble << "// NOLINTBEGIN(misc-include-cleaner)\n// clang-format off\n";
        cppIncludes(source, {archiveHeader, path.generic_string(), output.header.generic_string(),
                             (archive.output.header.parent_path() / "details/SerializationUtils.h").generic_string()});
        source << "// clang-format on\n";
        // The engine's generic vector loader needs a T&. Packed bools expose a proxy.
        if (requirements.needsBoolVectorLoad) {
            std::ostringstream packed;
            packed << "\ntemplate<class Allocator>\ninline void load(" << graph.node(archive.module.inputArchive).name
                   << R"(& ar, std::vector<bool, Allocator>& bits) {
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

void writeRecordImports(std::ostream& script, const TypeImports& values, TypeImports& types, const std::filesystem::path& from) {
    writeImports(script, values, from, false);
    for (const auto& [source, names] : values) {
        const auto it = types.find(source);
        if (it == types.end()) continue;
        for (const auto& name : names) it->second.erase(name);
        if (it->second.empty()) types.erase(it);
    }
    writeImports(script, types, from, true);
}

} // namespace

Files emitRecords(const TypeGraph& graph, ModuleId owner, const RecordOutput& output, std::optional<ArchiveOutput> archive,
                  const std::string& preamble, NestedResets& nestedResets) {
    const auto& module = graph.modules.at(owner.value);
    const bool standaloneInterfaces = !module.declarations.empty() && std::ranges::all_of(module.declarations, [&](TypeId id) {
        const auto* interfaceType = std::get_if<InterfaceType>(&graph.node(id).data);
        return interfaceType && interfaceType->bases.empty();
    });
    Dependencies deps{.emitCpp = !output.header.empty()};
    ArchiveImports archiveImports{deps.valueImports, output.typescript ? (output.typescript->parent_path() / "serialization").generic_string() : ""};
    Pool pool;
    RecordArchives archives;
    std::ostringstream cpp, definitions, hashDeclarations, hashDefinitions, ts, staticInitializers, nameFunctions;
    if (deps.emitCpp) writeTemplateAliases(cpp, graph, owner, deps);
    auto pendingEnums = output.namedEnums;
    std::string enumImports;
    bool previousTag = false;
    bool usesUint64Field = false;
    const std::set<std::string> keepNone;
    for (auto id : module.declarations) {
        const auto& node = graph.node(id);
        const bool emitTs = output.typescript && node.typescriptName;
        if (!deps.emitCpp && !emitTs) continue;
        if (std::holds_alternative<AliasType>(node.data)) continue;
        if (std::holds_alternative<TagType>(node.data)) {
            if (!previousTag) cpp << '\n';
            writeDocumentation(cpp, node.documentation);
            cpp << "struct " << node.name << " {};\n";
            previousTag = true;
            continue;
        }
        previousTag = false;
        const auto entry = output.pools.find(node.name);
        const auto methods = output.classMethods.find(node.name);
        const auto classMethods = methods == output.classMethods.end() ? std::string{} : tsClassMethods(methods->second);
        if (std::holds_alternative<InterfaceType>(node.data)) {
            if (emitTs && standaloneInterfaces && !ts.view().empty()) ts << '\n';
            emitInterface(graph, node, deps.emitCpp ? &cpp : nullptr, deps.headers, emitTs ? &ts : nullptr, deps.typeImports, output.interfaceBindings);
            continue;
        }
        if (const auto* model = std::get_if<GraphType>(&node.data)) {
            const auto fields = bindFields(graph, node, model->fields, deps, emitTs, nestedResets);
            if (deps.emitCpp) {
                std::string inputArchive;
                if (output.serializationHeader && model->serializable && (!fields.empty() || model->nameIndex || model->keyComponentIndex) && archive) {
                    deps.headers.insert(archive->output.forwardHeader.generic_string());
                    inputArchive = "::" + graph.node(archive->module.inputArchive).qualifiedName;
                }
                const auto methods = cppMemberFunctions(graph, module, model->methods, deps);
                emitCppGraph(graph, id, cpp, definitions, deps.headers, deps.forwardDeclarations, fields, methods, inputArchive);
                if (model->compareEqual || model->compareLess || model->hashable) {
                    std::vector<std::string_view> elements;
                    for (const auto& component : model->components) elements.push_back(component.name);
                    for (const auto& field : model->fields) {
                        if (!field.notElement) elements.push_back(field.name);
                    }
                    if (model->hashable) cppHash(hashDeclarations, hashDefinitions, node, elements);
                    if (model->compareEqual || model->compareLess) {
                        deps.headers.insert("tuple");
                        cppComparisons(cpp, node.name, elements, *model);
                    }
                }
            }
            if (emitTs) {
                if (model->mutableGraph) validateObjectResets(node, model->fields, fields, keepNone);
                emitTypeScriptGraph(graph, id, ts, deps.valueImports, deps.typeImports, fields, classMethods);
                usesUint64Field |= std::ranges::any_of(fields, &BoundField::safeUnsignedInteger);
                if (entry != output.pools.end()) {
                    if (!model->mutableGraph) throw std::runtime_error(node.qualifiedName + ": graph pool requires mutable_graph");
                    pool.add(node.name, entry->second.member, Pool::Reuse::clear);
                }
            }
            if (model->serializable && (output.serializationHeader || emitTs)) {
                emitGraphArchive(graph, node, archive->module, archives.declarations, archives.definitions,
                                 emitTs ? &archives.script : nullptr, archiveImports, archives.requirements);
            }
            continue;
        }
        cpp << '\n';
        if (emitTs) ts << '\n';
        if (const auto* enumeration = std::get_if<EnumType>(&node.data)) {
            writeDocumentation(cpp, node.documentation);
            cpp << "enum class " << node.name;
            std::string baseName = "int";
            if (enumeration->underlying != TypeId{}) {
                const auto& base = graph.node(enumeration->underlying);
                if (deps.emitCpp) collectCppHeader(graph, module, base, deps.headers);
                baseName = cppName(graph, base, module);
                cpp << " : " << baseName;
            }
            cpp << (enumeration->isFlags ? " { // NOLINT(performance-enum-size)\n" : " {\n");
            writeEnumMembers(cpp, *enumeration, EnumTarget::cpp);
            if (emitTs) {
                writeDocumentation(ts, node.documentation);
                writeTypeScriptEnum(ts, node.name, *enumeration, output.runtimeEnums.contains(node.name));
            }
            cpp << "};\n";
            if (enumeration->isFlags) {
                deps.headers.insert("type_traits");
                cppFlagOperators(cpp, node.name, baseName);
            }
            if (pendingEnums.erase(node.name)) {
                if (!enumImports.empty()) enumImports += ", ";
                enumImports += node.name;
                writeEnumNames(nameFunctions, node.name, *enumeration, EnumTarget::typescript);
            }
            continue;
        }
        const auto* record = std::get_if<RecordType>(&node.data);
        if (!record) throw std::runtime_error("expected record or enum: " + node.qualifiedName);
        const auto& selected = record->typescriptConstructor;
        auto constructor = selected.value_or(ConstructorInfo{});
        if (!constructor.defaults && !constructor.fields.empty() && entry != output.pools.end())
            throw std::runtime_error(node.qualifiedName + ": pool requires a TypeScript constructor callable without arguments");
        const auto requiredFields = constructor.defaults ? Order{} : constructor.fields;
        auto members = bindFields(graph, node, record->fields, deps, emitTs, nestedResets, requiredFields);
        std::vector<StaticField> statics;
        for (const auto& field : record->staticFields) {
            if (emitTs && !field.cppOnly && (field.name == "prototype" || field.name == "name" || field.name == "length" ||
                field.name == "arguments" || field.name == "caller" || field.name == "constructor"))
                invalid(field, "reserved TypeScript static member name");
            auto member = bindMember(graph, owner, field, deps, emitTs && !field.cppOnly, false);
            if (field.cppOnly) member.ts.clear();
            const auto& type = graph.node(field.type.type);
            const bool forwardEnum = std::holds_alternative<EnumType>(type.data) && type.module == owner && field.type.type.value > id.value;
            usesUint64Field |= emitTs && !field.cppOnly && member.safeUnsignedInteger;
            statics.push_back({std::move(member), forwardEnum});
        }
        if (deps.emitCpp) cppRecord(cpp, definitions, graph, module, node, members, statics,
                                    output.serializationHeader ? &archive->module : nullptr, deps);
        if (record->serializable && (output.serializationHeader || emitTs)) {
            const bool emitTsArchive = emitTs && !node.typescriptNumber;
            const auto fields = archiveFields(graph, owner, *record, members, archives.requirements, emitTsArchive);
            archives.add(graph, archive->module, node, fields, emitTsArchive, archiveImports);
        }
        if (deps.emitCpp && (record->hashable || record->compareEqual || record->compareLess)) {
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
        if (!emitTs || node.typescriptNumber) continue;
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
        auto& order = constructor.fields;
        removeCppOnlyFields(*record, members, order);
        std::set<std::string> retained;
        std::optional<Order> reset;
        if (entry != output.pools.end() && entry->second.assignFields) {
            if (order.size() != members.size() || std::ranges::any_of(members, &BoundField::readonly))
                throw std::runtime_error(node.qualifiedName + ": pool assign requires constructor parameters for all fields and writable fields");
            pool.add(node.name, entry->second.member, Pool::Reuse::assign, members, order);
        } else if (entry != output.pools.end() && members.empty() && !statics.empty() && entry->second.keepOnReset.empty()) {
            pool.add(node.name, entry->second.member, Pool::Reuse::unchanged);
        } else if (entry != output.pools.end()) {
            retained = entry->second.keepOnReset;
            for (const auto& member : members) {
                if (member.tsKeepOnReset) retained.insert(member.name);
            }
            validateObjectResets(node, record->fields, members, retained);
            reset = resetParameters(node.name, members, order, retained);
            pool.add(node.name, entry->second.member, Pool::Reuse::reset, members, *reset);
            auto& args = nestedResets[id.value];
            for (auto index : *reset) {
                const auto& member = members[index];
                const auto field = std::ranges::find(record->fields, member.name, &FieldInfo::name);
                args.push_back({field->type.type, member.tsDefault});
            }
        }
        const auto base = output.bases.find(node.name);
        if (base != output.bases.end()) deps.valueImports[base->second.from.generic_string()].insert(base->second.name);
        usesUint64Field |= std::ranges::any_of(members, &BoundField::safeUnsignedInteger);
        tsRecord(ts, staticInitializers, node, members, statics, order, reset ? &*reset : nullptr, retained,
                 base == output.bases.end() ? "" : base->second.name, selected, classMethods);
    }
    if (pool.count != output.pools.size()) throw std::runtime_error("unknown pool type in " + module.name);
    if (!pendingEnums.empty()) throw std::runtime_error("unknown enum " + *pendingEnums.begin() + " in " + module.name);

    Files files;
    if (deps.emitCpp) {
        deps.headers.erase(output.header.generic_string());
        deps.headers.erase(output.forwardHeader.generic_string());
        if (standaloneInterfaces && output.forwardIncludes.empty()) {
            emitInterfaceHeaders(files, graph, module, output, cpp.view(), deps.headers, preamble);
        } else {
            std::ostringstream header;
            header << preamble << "#pragma once\n";
            cppIncludes(header, deps.headers);
            const auto include = output.forwardHeader.lexically_relative(output.header.parent_path());
            header << "#include \"" << include.generic_string() << "\"\n";
            for (const auto& decl : deps.forwardDeclarations) header << "\n" << decl << "\n";
            writeNamespace(header, module, cpp);
            if (!hashDefinitions.view().empty()) {
                header << "\nnamespace ccstd {\n" << hashDefinitions.str() << "\n} // namespace ccstd\n";
            }
            files.emplace(output.nativeRoot / output.header, header.str());
            files.emplace(output.nativeRoot / output.forwardHeader, forwardHeader(graph, module, hashDeclarations, preamble, output.forwardIncludes));
        }
        auto path = output.header;
        path.replace_extension(".cpp");
        if (definitions.view().empty()) {
            files.emplace(output.nativeRoot / path, emitHeaderSource(module, output.header, preamble));
        } else {
            std::ostringstream source;
            source << preamble << "#include \"" << output.header.filename().generic_string() << "\"\n#include <utility>\n";
            writeNamespace(source, module, definitions);
            files.emplace(output.nativeRoot / path, source.str());
        }
    }
    if (output.jsbHeader) emitRecordJsb(files, graph, module, output, preamble);
    if (output.serializationHeader) archives.emitNative(files, graph, module, output, *archive, preamble);
    if (output.namesHeader) files.emplace(output.nativeRoot / *output.namesHeader, namesHeader(graph, module, output, preamble));
    if (!output.typescript) return files;
    std::ostringstream script;
    script << preamble << "/* eslint-disable max-len */\n";
    for (const auto& [member, dependency] : output.poolDependencies)
        deps.typeImports[dependency.from.generic_string()].insert(dependency.name);
    TypeImports packageValues, packageTypes;
    for (const auto& item : output.typescriptImports) {
        if (item.from.starts_with("./") || item.from.starts_with("../")) {
            const auto source = (output.typescript->parent_path() / item.from).lexically_normal().generic_string();
            auto& names = (item.typeOnly ? deps.typeImports : deps.valueImports)[source];
            names.insert(item.names.begin(), item.names.end());
        } else {
            auto& names = (item.typeOnly ? packageTypes : packageValues)[item.from];
            names.insert(item.names.begin(), item.names.end());
        }
    }
    std::ostringstream runtimeImports;
    const auto runtimeImport = [&](const std::filesystem::path& path, std::initializer_list<std::string> names, bool typeOnly) {
        const auto source = path.generic_string();
        if (deps.valueImports.contains(source) || deps.typeImports.contains(source)) {
            (typeOnly ? deps.typeImports : deps.valueImports)[source].insert(names);
        } else {
            writeNamedImport(runtimeImports, names, modulePath(path, *output.typescript), typeOnly);
        }
    };
    if (pool.count) runtimeImport(output.poolModule, {"RecyclePool"}, false);
    if (!archives.script.view().empty()) {
        if (!archive->output.typescript) throw std::runtime_error(module.name + ": serialization requires TypeScript output from " + archive->module.name);
        const auto& writer = graph.node(archive->module.outputArchive);
        const auto& reader = graph.node(archive->module.inputArchive);
        for (const auto* api : {&writer, &reader}) {
            if (!api->typescriptName)
                throw std::runtime_error(module.name + ": serialization requires TypeScript export " + api->qualifiedName);
        }
        auto path = *archive->output.typescript;
        runtimeImport(path.replace_extension(), {writer.name, reader.name}, true);
    }
    writeRecordImports(script, packageValues, packageTypes, {});
    writeRecordImports(script, deps.valueImports, deps.typeImports, *output.typescript);
    if (standaloneInterfaces && !deps.typeImports.empty()) script << '\n';
    script << runtimeImports.str();
    if (pool.count) {
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
    script << ts.str();
    if (!staticInitializers.view().empty()) script << '\n' << staticInitializers.str();
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
        const auto& input = graph.node(archive->module.inputArchive);
        script << "\nfunction cc$readSize (a: " << input.name << "): number {\n"
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
