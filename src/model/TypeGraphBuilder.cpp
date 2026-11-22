#include "model/TypeGraphBuilder.h"
#include "model/Binding.h"
#include "model/NumericLiteral.h"
#include "model/TypeName.h"

#include <algorithm>
#include <bit>
#include <limits>
#include <optional>
#include <set>
#include <stdexcept>
#include <string_view>
#include <type_traits>

namespace cc::codegen {

std::string componentAccessorSuffix(std::string tagName) {
    if (tagName.ends_with('_')) tagName.pop_back();
    if (!tagName.empty() && tagName.front() >= 'a' && tagName.front() <= 'z') tagName.front() -= 'a' - 'A';
    return tagName;
}

namespace {

std::string qualify(const std::string& scope, const std::string& name) {
    return scope.empty() ? name : scope + "::" + name;
}

// A namespace can be reopened by several modules, so its vertex has no single owner.
TypeId ensureNamespace(TypeGraph& graph, const std::string& scope) {
    if (scope.empty()) return {};
    if (TypeId existing = graph.findType(scope); existing != TypeId{}) {
        if (!std::holds_alternative<NamespaceType>(graph.node(existing).data)) {
            throw std::invalid_argument("expected namespace " + scope);
        }
        return existing;
    }
    const auto pos = scope.rfind("::");
    const auto name = pos == std::string::npos ? scope : scope.substr(pos + 2);
    const auto parent = pos == std::string::npos ? TypeId{} : ensureNamespace(graph, scope.substr(0, pos));
    return graph.appendNode(TypeNode{.name = name, .qualifiedName = scope, .data = NamespaceType{}}, parent);
}

void registerScalars(TypeGraph& graph) {
    const auto addScalar = [&](const char* name, const char* ts, const std::string& header = {}) {
        graph.appendNode(TypeNode{.name = name, .qualifiedName = name,
                                  .data = header.empty() ? TypeData{BuiltinType{}} : TypeData{ExternalTypeNode{}},
                                  .cppHeader = header, .typescriptName = ts}, {});
    };
    // Keep the scalar vertex IDs in name order.
    addScalar("bool", "boolean");
    for (const auto* name : {"char", "double", "float", "int"}) addScalar(name, "number");
    for (const auto* name : {"int32_t", "uint16_t", "uint32_t", "uint64_t", "uint8_t"}) addScalar(name, "number", "cstdint");
    addScalar("void", "void");
}

TypeId resolveName(const TypeGraph& graph, const std::string& name, const SourceLocation& location,
                   std::string scope, ModuleId owner) {
    if (name.starts_with("::")) return resolveName(graph, name.substr(2), location, {}, owner);
    for (;;) {
        const auto id = graph.findType(qualify(scope, name));
        if (id != TypeId{}) {
            const auto& declaration = graph.node(id);
            if (std::holds_alternative<NamespaceType>(declaration.data)) {
                throw std::runtime_error(location.describe() + ": namespace used as type " + name);
            }
            const auto& imports = graph.modules.at(owner.value).imports;
            if (declaration.module != ModuleId{} && declaration.module != owner &&
                std::ranges::find(imports, declaration.module) == imports.end()) {
                throw std::runtime_error(location.describe() + ": " + declaration.qualifiedName + " requires import " +
                                         graph.modules.at(declaration.module.value).name + " in module " +
                                         graph.modules.at(owner.value).name);
            }
            return id;
        }
        if (scope.empty()) throw std::runtime_error(location.describe() + ": unknown type " + name);
        const auto parent = scope.rfind("::");
        scope = parent == std::string::npos ? std::string{} : scope.substr(0, parent);
    }
}

TypeUse resolve(TypeGraph& graph, const TypeRef& use, const std::string& scope, ModuleId owner) {
    auto id = resolveName(graph, use.name, use.location, scope, owner);
    const auto& declaration = graph.node(id);
    if (!acceptsTemplateArity(declaration, use.arguments.size()) ||
        (!declaration.templateParameters.empty() && use.arguments.empty() && !use.hasTemplateArguments))
        throw std::runtime_error(use.location.describe() + ": wrong number of template arguments for " + declaration.qualifiedName);
    if (use.hasTemplateArguments || !use.arguments.empty()) {
        if (!std::holds_alternative<ExternalTypeNode>(graph.node(id).data))
            throw std::runtime_error(use.location.describe() + ": expected an external template for " + graph.node(id).qualifiedName);
        std::vector<TemplateArgument> arguments;
        for (const auto& argument : use.arguments) arguments.emplace_back(resolve(graph, argument, scope, owner).type);
        id = graph.makeTemplateInstance(id, std::move(arguments));
    }
    if (use.constPointee) id = graph.makeQualified(id, true, false);
    if (use.pointer) id = graph.makePointer(id);
    if (use.reference) id = graph.makeReference(id, false);
    return {id, use.location};
}

std::string parameterDefault(const TypeGraph& graph, const Parameter& parameter, TypeUse type, const std::string& scope, ModuleId owner);

MethodInfo resolveMethod(TypeGraph& graph, const Method& method, const std::string& scope, ModuleId owner) {
    MethodInfo member{.result = resolve(graph, method.result, scope, owner), .name = method.name,
                      .isConst = method.isConst, .isNoexcept = method.isNoexcept, .cppOnly = method.cppOnly,
                      .cppBody = method.cppBody, .isStatic = method.isStatic, .isPureVirtual = method.isPureVirtual,
                      .isExplicitConversion = method.isExplicitConversion, .explicitCppOnly = method.explicitCppOnly,
                      .accessor = method.accessor,
                      .optionalMethod = method.optionalMethod, .resultPresence = method.resultPresence, .cppPrivate = method.cppPrivate, .documentation = method.documentation};
    if (member.isExplicitConversion) {
        // Match overloads by their resolved target, including aliases inside references and pointers.
        member.name = "operator " + formatTypeName(graph, member.result.type, [&](const TypeNode& part) {
            if (const auto* alias = std::get_if<AliasType>(&part.data))
                return graph.node(canonicalType(graph, {alias->target, member.result.location})).qualifiedName;
            return part.qualifiedName;
        });
    }
    for (const auto& parameter : method.parameters) {
        const auto type = resolve(graph, parameter.type, scope, owner);
        member.parameters.push_back({type, parameter.name, parameter.byConstReference, parameterDefault(graph, parameter, type, scope, owner)});
    }
    return member;
}

std::vector<MethodInfo> resolveMemberFunctions(TypeGraph& graph, const std::vector<Method>& methods, const std::string& scope, ModuleId owner) {
    std::vector<MethodInfo> result;
    const auto generatesOverloads = [](const MethodInfo& method) {
        return method.isPureVirtual && !method.parameters.empty() && !method.parameters.back().defaultValue.empty();
    };
    for (const auto& method : methods) {
        auto resolved = resolveMethod(graph, method, scope, owner);
        for (const auto& other : result) {
            if (other.name != resolved.name || (!other.isStatic && !resolved.isStatic && other.isConst != resolved.isConst)) continue;
            size_t count = resolved.parameters.size();
            if (generatesOverloads(other) || generatesOverloads(resolved)) {
                // Each defaulted suffix creates a callable prefix, including the zero-argument overload.
                count = std::max(requiredParameterCount(other), requiredParameterCount(resolved));
                if (count > std::min(other.parameters.size(), resolved.parameters.size())) continue;
            } else if (count != other.parameters.size()) continue;
            if (std::equal(other.parameters.begin(), other.parameters.begin() + count, resolved.parameters.begin(), [&](const auto& a, const auto& b) {
                    return a.byConstReference == b.byConstReference && canonicalType(graph, a.type) == canonicalType(graph, b.type);
                })) throw std::runtime_error(method.result.location.describe() + ": duplicate method signature " + resolved.name);
        }
        result.push_back(std::move(resolved));
    }
    return result;
}

TypeId enumDefaultType(const TypeGraph& graph, TypeUse use, const std::string& literal, const std::string& scope, ModuleId owner) {
    const auto pos = literal.rfind("::");
    const auto type = resolveName(graph, literal.substr(0, pos), use.location, scope, owner);
    const auto& data = graph.node(type).data;
    const auto* enumeration = std::get_if<EnumType>(&data);
    if (type != use.type || (!enumeration && !std::holds_alternative<ExternalTypeNode>(data)) ||
        (enumeration && std::ranges::find(enumeration->values, literal.substr(pos + 2), &Enumerator::name) == enumeration->values.end()))
        throw std::runtime_error(use.location.describe() + ": invalid enum default " + literal);
    return type;
}

std::vector<FieldInfo> resolveFields(TypeGraph& graph, const std::vector<Field>& fields, const std::string& scope, ModuleId owner) {
    std::vector<FieldInfo> result;
    for (const auto& field : fields) {
        auto use = resolve(graph, field.type, scope, owner);
        std::string_view magnitude = field.initializer;
        if (magnitude.starts_with('-')) magnitude.remove_prefix(1);
        const auto& typeName = graph.node(canonicalType(graph, use)).qualifiedName;
        if ((hasHexPrefix(magnitude) || (typeName == "uint64_t" && !field.initializer.empty())) &&
            !validUnsignedFieldDefault(typeName, field.initializer)) {
            throw std::runtime_error(use.location.describe() + ": " + field.name + ": invalid " + typeName +
                                     " default " + field.initializer);
        }
        if (field.initializer.find("::") != std::string::npos) enumDefaultType(graph, use, field.initializer, scope, owner);
        if (field.isMutable && (std::holds_alternative<ReferenceType>(graph.node(use.type).data) ||
            std::holds_alternative<QualifiedType>(graph.node(use.type).data)))
            throw std::runtime_error(use.location.describe() + ": mutable requires a non-const value field");
        result.push_back({std::move(use), field.name, field.initializer, field.isReadonly, field.isNullable, field.transient, {}, field.cppOnly, field.notElement, field.isMutable, field.cppPrivate, field.documentation});
    }
    return result;
}

std::optional<ConstructorInfo> selectTypeScriptConstructor(const TypeNode& node, const RecordType& record, const TypeScriptBinding& rule) {
    auto constructors = record.constructors;
    std::erase_if(constructors, [](const ConstructorInfo& ctor) { return ctor.cppOnly; });
    for (auto& ctor : constructors)
        std::erase_if(ctor.fields, [&](auto i) { return record.fields[i].cppOnly; });

    if (const auto& selected = rule.constructor) {
        if (record.constructors.empty()) {
            // Older TS bindings could explicitly request either zero or all visible fields.
            auto& ctor = constructors.emplace_back();
            ctor.defaults = true;
            if (!selected->empty()) {
                for (uint32_t i = 0; i < record.fields.size(); ++i)
                    if (!record.fields[i].cppOnly) ctor.fields.push_back(i);
            }
        }
        std::erase_if(constructors, [&](const ConstructorInfo& ctor) {
            return !std::ranges::equal(ctor.fields, *selected, {}, [&](auto i) -> std::string_view { return record.fields[i].name; });
        });
        if (constructors.empty()) throw std::runtime_error(node.qualifiedName + ": TypeScript constructor does not match a declared constructor");
    }
    if (constructors.empty()) return std::nullopt;
    if (constructors.size() != 1)
        throw std::runtime_error(node.qualifiedName + ": ambiguous TypeScript constructor; mark native overloads @cppOnly");
    auto ctor = std::move(constructors.front());
    if (rule.requiredConstructor) {
        if (!record.constructors.empty() && ctor.defaults)
            throw std::runtime_error(node.qualifiedName + ": required TypeScript constructor conflicts with declared defaults");
        ctor.defaults = false;
    }
    return ctor;
}

RecordType resolveRecord(TypeGraph& graph, TypeId id, const Record& syntax, const std::string& scope, ModuleId owner) {
    RecordType result{.moveOnly = syntax.moveOnly, .immovable = syntax.immovable, .serializable = syntax.serializable,
                      .compareEqual = syntax.comparisons.contains("=="),
                      .compareLess = syntax.comparisons.contains("<") || syntax.comparisons.contains(">"),
                      .compareGreater = syntax.comparisons.contains(">"), .hashable = syntax.hashable};
    result.fields = resolveFields(graph, syntax.fields, scope, owner);
    result.isClass = syntax.isClass;
    result.isFinal = syntax.isFinal;
    result.destructorNoexcept = syntax.destructorNoexcept;
    result.destructorPrivate = syntax.destructorPrivate;
    result.destructorDocumentation = syntax.destructorDocumentation;
    result.cppMembers = syntax.cppMembers;
    for (const auto& base : syntax.bases) {
        const auto target = resolve(graph, base, scope, owner).type;
        const auto& node = graph.node(target);
        const auto* record = std::get_if<RecordType>(&node.data);
        if ((!record && !std::holds_alternative<InterfaceType>(node.data)) || (record && record->isFinal) ||
            (node.module == owner && target.value >= id.value) || std::ranges::find(result.bases, target) != result.bases.end())
            throw std::runtime_error(base.location.describe() + ": expected a distinct, earlier non-final base");
        result.bases.push_back(target);
    }
    if (syntax.implements) {
        const auto target = resolve(graph, *syntax.implements, scope, owner).type;
        if (!std::holds_alternative<InterfaceType>(graph.node(target).data))
            throw std::runtime_error(syntax.implements->location.describe() + ": implements requires an interface");
        result.implements = target;
    }
    result.staticFields = resolveFields(graph, syntax.staticFields, scope, owner);
    for (const auto& field : result.staticFields) {
        if (std::holds_alternative<ReferenceType>(graph.node(canonicalType(graph, field.type)).data))
            throw std::runtime_error(field.type.location.describe() + ": static reference field requires a reference initializer");
    }
    std::set<std::vector<uint32_t>> signatures;
    for (const auto& ctor : syntax.constructors) {
        std::vector<uint32_t> members, signature;
        for (const auto& name : ctor.fields) {
            const auto field = std::ranges::find(result.fields, name, &FieldInfo::name);
            if (field == result.fields.end() && std::ranges::find(result.staticFields, name, &FieldInfo::name) != result.staticFields.end())
                throw std::runtime_error(ctor.location.describe() + ": constructor cannot initialize static field " + name);
            if (field == result.fields.end()) throw std::runtime_error(ctor.location.describe() + ": unknown constructor field " + name);
            const auto index = static_cast<uint32_t>(field - result.fields.begin());
            if (std::ranges::find(members, index) != members.end()) {
                throw std::runtime_error(ctor.location.describe() + ": repeated constructor field " + name);
            }
            members.push_back(index);
            signature.push_back(canonicalType(graph, field->type).value);
        }
        if (!signatures.insert(std::move(signature)).second) {
            throw std::runtime_error(ctor.location.describe() + ": duplicate constructor signature");
        }
        result.constructors.push_back({std::move(members), ctor.defaults, ctor.cppOnly, ctor.custom, ctor.isNoexcept, ctor.cppPrivate, ctor.documentation});
    }
    for (uint32_t i = 0; i < result.fields.size(); ++i) {
        const auto& field = result.fields[i];
        if (std::holds_alternative<ReferenceType>(graph.node(canonicalType(graph, field.type)).data)) {
            const auto context = field.type.location.describe() + ": " + field.name;
            if (field.isNullable || !field.initializer.empty())
                throw std::runtime_error(context + ": reference fields cannot have a null or default initializer");
            if (result.constructors.empty() || std::ranges::any_of(result.constructors, [i](const ConstructorInfo& ctor) {
                    return std::ranges::find(ctor.fields, i) == ctor.fields.end();
                })) throw std::runtime_error(context + ": every constructor must initialize the reference field");
            if (syntax.serializable && !field.transient)
                throw std::runtime_error(context + ": serializing a borrowed reference requires @transient");
        }
        if (syntax.serializable && field.cppOnly && !field.transient)
            throw std::runtime_error(field.type.location.describe() + ": @cppOnly field in a serializable record requires @transient");
    }
    result.methods = resolveMemberFunctions(graph, syntax.methods, scope, owner);
    return result;
}

TypeData declare(const Declaration& decl) {
    if (std::holds_alternative<AliasDeclaration>(decl.data)) return AliasType{};
    if (std::holds_alternative<TagDeclaration>(decl.data)) return TagType{};
    if (std::holds_alternative<GraphDeclaration>(decl.data)) return GraphType{};
    if (const auto* syntax = std::get_if<Enum>(&decl.data)) {
        EnumType enumeration{.isFlags = syntax->isFlags};
        for (const auto& value : syntax->values) {
            auto& member = enumeration.values.emplace_back(Enumerator{
                .name = value.name, .initializer = value.initializer, .documentation = value.documentation});
            if (value.leftShift) member.initializer += " << " + value.leftShift->count;
        }
        return enumeration;
    }
    if (std::holds_alternative<Record>(decl.data)) return RecordType{};
    if (const auto* external = std::get_if<ExternalDeclaration>(&decl.data)) {
        return ExternalTypeNode{.isStruct = external->isStruct, .isString = external->isString,
                                .isPmr = external->isPmr, .constructorConstRef = external->constructorConstRef,
                                .container = external->container};
    }
    return InterfaceType{};
}

struct IntegerRange {
    int64_t min;
    uint64_t max;
};

template<class T>
constexpr IntegerRange integerRange() {
    return {std::numeric_limits<T>::min(), static_cast<uint64_t>(std::numeric_limits<T>::max())};
}

[[noreturn]] void enumOverflow(const EnumMember& member, const std::string& base) {
    throw std::runtime_error(member.initializerLocation.describe() + ": enum value out of range for " + base + ": " + member.name);
}

using EnumValue = decltype(Enumerator::value);

EnumValue integerLiteral(const EnumMember& member, IntegerRange range, const std::string& base) {
    std::string_view digits = member.initializer;
    const bool negative = digits.starts_with('-');
    if (negative && member.leftShift) {
        throw std::runtime_error(member.initializerLocation.describe() + ": expected nonnegative left shift operand");
    }
    if (negative) digits.remove_prefix(1);
    uint64_t magnitude{};
    const auto error = parseMagnitude(digits, magnitude);
    if (error == std::errc::invalid_argument) {
        throw std::runtime_error(member.initializerLocation.describe() + ": invalid integer literal " + member.initializer);
    }
    const uint64_t limit = negative ? uint64_t{0} - static_cast<uint64_t>(range.min) : range.max;
    if (error == std::errc::result_out_of_range || magnitude > limit) enumOverflow(member, base);
    if (const auto& shift = member.leftShift) {
        uint64_t count{};
        const auto countError = parseMagnitude(shift->count, count);
        if (countError == std::errc::invalid_argument) {
            throw std::runtime_error(shift->location.describe() + ": invalid integer shift count " + shift->count);
        }
        const auto width = static_cast<uint64_t>(std::bit_width(range.max)) + (range.min < 0);
        if (countError == std::errc::result_out_of_range || count >= width) {
            throw std::runtime_error(shift->location.describe() + ": shift count out of range for " + base + ": " + shift->count);
        }
        if (magnitude > (range.max >> count)) enumOverflow(member, base);
        magnitude <<= count;
    }
    if (range.min == 0) return magnitude;
    // Subtract from zero in the unsigned domain so INT64_MIN is representable.
    return static_cast<int64_t>(negative ? uint64_t{0} - magnitude : magnitude);
}

const std::map<std::string, IntegerRange>& integerRanges() {
    static const std::map<std::string, IntegerRange> ranges{
        {"int", integerRange<int>()}, {"unsigned", integerRange<unsigned>()}, {"short", integerRange<short>()},
        {"long", integerRange<long>()}, {"char", integerRange<char>()}, {"bool", integerRange<bool>()},
        {"int8_t", integerRange<int8_t>()}, {"uint8_t", integerRange<uint8_t>()},
        {"int16_t", integerRange<int16_t>()}, {"uint16_t", integerRange<uint16_t>()},
        {"int32_t", integerRange<int32_t>()}, {"uint32_t", integerRange<uint32_t>()},
        {"int64_t", integerRange<int64_t>()}, {"uint64_t", integerRange<uint64_t>()}};
    return ranges;
}

std::string parameterDefault(const TypeGraph& graph, const Parameter& parameter, TypeUse use, const std::string& scope, ModuleId owner) {
    const auto& value = parameter.defaultValue;
    if (value.empty()) return {};
    const auto& type = graph.node(canonicalType(graph, use));
    const auto& name = type.qualifiedName;
    if (value == "{}" && name != "void" && !std::holds_alternative<ReferenceType>(type.data) &&
        !std::holds_alternative<InterfaceType>(type.data)) return value;
    if (value.front() == '"') {
        const auto* external = std::get_if<ExternalTypeNode>(&type.data);
        if ((external && external->isString) || name == "std::string" || name == "std::string_view" ||
            qualifiedTypeName(graph, use.type) == "const char*") return value;
    }
    if (const auto* enumeration = std::get_if<EnumType>(&type.data); enumeration && value.find("::") != value.npos) {
        std::string expression;
        size_t start = 0;
        for (;;) {
            const auto end = value.find(" | ", start);
            const auto literal = value.substr(start, end - start);
            if (literal.rfind("::") == literal.npos || (end != value.npos && !enumeration->isFlags))
                throw std::runtime_error(use.location.describe() + ": invalid enum default " + value);
            const auto id = enumDefaultType(graph, use, literal, scope, owner);
            expression += "::" + graph.node(id).qualifiedName + literal.substr(literal.rfind("::"));
            if (end == value.npos) return expression;
            expression += " | ";
            start = end + 3;
        }
    }
    if (name == "bool" && (value == "true" || value == "false")) return value;
    if (std::holds_alternative<PointerType>(type.data) && value == "nullptr") return value;
    if ((name == "float" || name == "double") && validFloatingDefault(name, value)) {
        const auto decimal = value.find('.') == std::string::npos ? value + ".0" : value;
        return name == "float" ? decimal + "F" : decimal;
    }
    const auto& ranges = integerRanges();
    if (const auto it = ranges.find(name); it != ranges.end() && name != "bool") {
        std::string_view digits = value;
        const bool negative = digits.starts_with('-');
        if (negative) digits.remove_prefix(1);
        uint64_t magnitude{};
        const auto range = it->second;
        const auto limit = negative ? uint64_t{0} - static_cast<uint64_t>(range.min) : range.max;
        if ((!negative || range.min < 0) && parseMagnitude(digits, magnitude) == std::errc{} && magnitude <= limit) {
            if (negative && magnitude == (uint64_t{1} << 63)) return "(-9223372036854775807LL - 1)";
            if (negative && hasHexPrefix(digits)) return "-" + std::to_string(magnitude);
            return name == "uint64_t" ? value + "ULL" : value;
        }
    }
    throw std::runtime_error(use.location.describe() + ": " + parameter.name + ": invalid default " + value +
                             " for " + qualifiedTypeName(graph, use.type));
}

GraphType resolveGraph(TypeGraph& graph, const GraphDeclaration& syntax, const std::string& scope, ModuleId owner) {
    GraphType result;
    result.immovable = syntax.immovable;
    result.serializable = syntax.serializable;
    result.compareEqual = syntax.comparisons.contains("==");
    result.compareLess = syntax.comparisons.contains("<") || syntax.comparisons.contains(">");
    result.compareGreater = syntax.comparisons.contains(">");
    result.hashable = syntax.hashable;
    const auto descriptor = graph.findType(qualify(scope, syntax.vertexAlias));
    result.vertexType = canonicalType(graph, {descriptor, syntax.vertexType->location});
    result.indexedVertices = syntax.indexed;
    if (syntax.indexedStorage) result.indexedStorage = resolve(graph, *syntax.indexedStorage, scope, owner).type;
    result.vertexList = syntax.vertexList;
    result.mutableGraph = syntax.mutableGraph;
    result.removeVertex = syntax.removeVertex;
    if (syntax.removeVertex && (syntax.hierarchy || !syntax.alternatives.empty()))
        throw std::runtime_error(syntax.vertexType->location.describe() + ": remove_vertex requires a flat component graph");
    result.hierarchy = syntax.hierarchy;
    result.separateEdges = syntax.separateEdges;
    result.incidence = syntax.incidence;
    result.bidirectional = syntax.bidirectional;
    result.fields = resolveFields(graph, syntax.fields, scope, owner);
    result.methods = resolveMemberFunctions(graph, syntax.methods, scope, owner);
    const auto& vertex = graph.node(result.vertexType);
    if (syntax.indexed && !indexedVertexLimit(vertex.qualifiedName)) {
        throw std::runtime_error(syntax.vertexType->location.describe() + ": expected an integer descriptor for indexed vertices");
    }
    std::set<std::string> accessors, kinds;
    std::set<uint32_t> alternativeTags;
    const auto bindColumn = [&](const GraphComponent& component, bool alternative) -> ComponentInfo {
        const std::string role = alternative ? "alternative" : "component";
        const auto tag = canonicalType(graph, resolve(graph, component.tag, scope, owner));
        const auto& node = graph.node(tag);
        if (!std::holds_alternative<TagType>(node.data)) {
            throw std::runtime_error(component.tag.location.describe() + ": " + role + " requires an independently declared @tag: " + component.tag.name);
        }
        const auto accessor = component.kindName.empty() ? componentAccessorSuffix(node.name) : component.kindName;
        if (accessor.empty() || !(alternative ? kinds : accessors).insert(accessor).second) {
            const auto description = alternative ? "alternative kind" : "component accessor";
            throw std::runtime_error(component.tag.location.describe() + ": invalid or duplicate " + description + " for " + node.qualifiedName);
        }
        if (alternative && !alternativeTags.insert(tag.value).second)
            throw std::runtime_error(component.tag.location.describe() + ": duplicate polymorphic tag " + node.qualifiedName);
        const auto value = resolve(graph, component.value, scope, owner);
        return {tag, value.type, component.member, accessor};
    };
    for (const auto& component : syntax.components) result.components.push_back(bindColumn(component, false));
    for (const auto& alternative : syntax.alternatives) result.alternatives.push_back(bindColumn(alternative, true));
    const auto componentIndex = [&](const TypeRef& selected, const char* role) {
        const auto tag = canonicalType(graph, resolve(graph, selected, scope, owner));
        const auto it = std::ranges::find(result.components, tag, &ComponentInfo::tag);
        if (it == result.components.end())
            throw std::runtime_error(selected.location.describe() + ": " + role + " requires a component of this graph: " + selected.name);
        return static_cast<uint32_t>(it - result.components.begin());
    };
    if (syntax.indexedComponent) {
        const auto& selected = *syntax.indexedComponent;
        result.keyComponentIndex = componentIndex(selected, "component_index");
        const auto key = result.components[*result.keyComponentIndex].valueType;
        const auto& value = graph.node(canonicalType(graph, {key, selected.location})).qualifiedName;
        if (value == "bool" || !integerRanges().contains(value))
            throw std::runtime_error(selected.location.describe() + ": component_index requires an integer component: " + selected.name);
    }
    if (syntax.nameComponent) {
        const auto& selected = *syntax.nameComponent;
        result.nameComponentIndex = componentIndex(selected, "named_graph");
        const auto key = result.components[*result.nameComponentIndex].valueType;
        const auto& value = graph.node(canonicalType(graph, {key, selected.location})).qualifiedName;
        if (value != "ccstd::pmr::string" && value != "ccstd::string" && value != "std::pmr::string" && value != "std::string")
            throw std::runtime_error(selected.location.describe() + ": named_graph requires a string component: " + selected.name);
    }
    if (syntax.nameIndex) {
        if (!result.nameComponentIndex) throw std::runtime_error(syntax.nameIndex->describe() + ": name_index requires named_graph");
        result.nameIndex = true;
    }
    if (syntax.parentGraph) {
        if (!result.hierarchy) throw std::runtime_error(syntax.parentGraph->describe() + ": parent_graph requires hierarchy_graph");
        if (result.separateEdges) throw std::runtime_error(syntax.parentGraph->describe() + ": parent_graph requires single-parent shared edges");
        if (!result.nameComponentIndex) throw std::runtime_error(syntax.parentGraph->describe() + ": parent_graph requires named_graph");
        result.parentGraph = true;
    }
    if (syntax.addressableGraph) {
        if (!result.parentGraph) throw std::runtime_error(syntax.addressableGraph->describe() + ": addressable_graph requires parent_graph");
        result.addressableGraph = true;
    }
    if (result.indexedStorage) {
        const auto& location = syntax.indexedStorage->location;
        const auto& base = graph.node(*result.indexedStorage);
        const auto* instance = std::get_if<TemplateInstanceType>(&base.data);
        const auto* argument = instance && instance->arguments.size() == 1
            ? std::get_if<TypeId>(&instance->arguments.front()) : nullptr;
        if (!argument || canonicalType(graph, {*argument, location}) != result.vertexType)
            throw std::runtime_error(location.describe() + ": indexed storage requires an external template instantiated with the vertex descriptor type");
        if (!result.vertexList || !result.incidence || !result.bidirectional || result.mutableGraph || result.hierarchy ||
            !result.alternatives.empty() || result.components.empty() || result.nameComponentIndex || result.keyComponentIndex ||
            result.serializable || result.immovable || result.compareEqual || result.compareLess || result.hashable)
            throw std::runtime_error(location.describe() + ": custom indexed storage requires a flat bidirectional component graph with vertex_list_graph");
    }
    return result;
}

void resolveEnum(TypeGraph& graph, TypeId id, const Enum& syntax, const std::string& scope, ModuleId owner) {
    const auto& ranges = integerRanges();
    TypeId underlying;
    std::string base = "int";
    if (syntax.underlying) {
        const auto use = resolve(graph, *syntax.underlying, scope, owner);
        underlying = canonicalType(graph, use);
        base = graph.node(underlying).qualifiedName;
        if (!ranges.contains(base)) throw std::runtime_error(use.location.describe() + ": expected integral enum base");
    }
    auto& node = graph.getNode(id.value);
    auto& enumeration = std::get<EnumType>(node.data);
    enumeration.underlying = underlying;
    const auto range = ranges.at(base);
    std::map<std::string, EnumValue> previous;
    EnumValue number = range.min == 0 ? EnumValue{uint64_t{0}} : EnumValue{int64_t{0}};
    for (size_t i = 0; i < syntax.values.size(); ++i) {
        const auto& member = syntax.values[i];
        const auto& init = member.initializer;
        if (init.empty()) {
            if (i != 0) {
                std::visit([&](auto& last) {
                    if (last == static_cast<std::decay_t<decltype(last)>>(range.max)) enumOverflow(member, base);
                    ++last;
                }, number);
            }
        } else if (init.front() == '-' || (init.front() >= '0' && init.front() <= '9')) {
            number = integerLiteral(member, range, base);
        } else {
            const auto target = previous.find(init);
            if (target == previous.end()) {
                throw std::runtime_error(member.initializerLocation.describe() + ": " + node.name + "." + member.name +
                                         " must refer to an earlier member of the same enum: " + init);
            }
            number = target->second;
        }
        enumeration.values[i].value = number;
        previous.emplace(member.name, number);
    }
}

using TypeScriptRules = std::map<uint32_t, const TypeScriptBinding*>;

void bindTypeScriptExports(TypeGraph& graph, ModuleId owner, const Module& syntax) {
    const auto& exports = syntax.bindings.typescriptExports;
    if (exports.empty()) return;
    for (const auto& [name, at] : exports) {
        const auto decl = std::ranges::find(syntax.declarations, name, &Declaration::name);
        if (decl != syntax.declarations.end() && decl->cppOnly)
            throw std::runtime_error(at.describe() + ": cannot export @cppOnly type " + name);
        if (syntax.external || decl == syntax.declarations.end() ||
            !(std::holds_alternative<Record>(decl->data) || std::holds_alternative<Interface>(decl->data) ||
              std::holds_alternative<Enum>(decl->data) || std::holds_alternative<GraphDeclaration>(decl->data)))
            throw std::runtime_error(at.describe() + ": expected a local TypeScript export declaration for " + name);
    }
    for (const auto id : graph.modules.at(owner.value).declarations) {
        auto& node = graph.getNode(id.value);
        if (!exports.contains(node.name)) {
            node.typescriptName.reset();
            node.typescriptModule.clear();
        }
    }
}

TypeScriptRules bindTypes(TypeGraph& graph, const std::vector<ModuleInput>& inputs, const std::vector<std::string>& scopes) {
    TypeScriptRules rules;
    std::map<uint32_t, SourceLocation> bound;
    for (uint32_t i = 0; i < inputs.size(); ++i) {
        const auto& input = inputs[i];
        const ModuleId owner{i};
        bindTypeScriptExports(graph, owner, input.syntax);
        std::set<uint32_t> jsbRecords;
        for (const auto& [name, at] : input.syntax.bindings.jsbRecords) {
            const auto id = resolveName(graph, name, at, scopes[i], owner);
            const auto& node = graph.node(id);
            if (node.module != owner || !std::holds_alternative<RecordType>(node.data))
                throw std::runtime_error(at.describe() + ": expected a local JSB record " + name);
            if (!jsbRecords.insert(id.value).second) throw std::runtime_error(at.describe() + ": duplicate JSB record " + name);
        }
        for (const auto& rule : input.syntax.bindings.typescript) {
            const auto id = resolve(graph, rule.source, scopes[i], owner).type;
            auto& node = graph.getNode(id.value);
            const auto& at = rule.source.location;
            auto reject = [&](const std::string& reason) {
                throw std::runtime_error(at.describe() + ": " + reason + " for " + qualifiedTypeName(graph, id));
            };
            if (node.module != ModuleId{} && node.module != owner) reject("bind the type in its declaring module");
            if (node.cppOnly) reject("cannot bind @cppOnly type");
            const auto& exports = input.syntax.bindings.typescriptExports;
            if (rule.kind == BindingTarget::numberField && exports.contains(node.name))
                reject("number(field) has no named TypeScript export");
            if (rule.kind != BindingTarget::type && rule.kind != BindingTarget::numberField && !exports.empty() && !exports.contains(node.name))
                reject("binding requires a TypeScript export");
            const auto [first, inserted] = bound.emplace(id.value, at);
            if (!inserted) {
                throw std::runtime_error(at.describe() + ": duplicate TypeScript binding for " + qualifiedTypeName(graph, id) +
                                         " (first bound at " + first->second.describe() + ")");
            }
            const bool isEnum = std::holds_alternative<EnumType>(node.data);
            const bool isRecord = std::holds_alternative<RecordType>(node.data);
            if (const auto* instance = std::get_if<TemplateInstanceType>(&node.data)) {
                const auto& container = std::get<ExternalTypeNode>(graph.node(instance->declaration).data);
                bool dictionary = false;
                if (container.container == ContainerKind::Map && instance->arguments.size() == 2) {
                    const auto& key = graph.node(std::get<TypeId>(instance->arguments[0]));
                    const auto& value = graph.node(std::get<TypeId>(instance->arguments[1]));
                    const auto& ts = value.typescriptName;
                    const bool primitive = ts == "number" || ts == "string" || ts == "boolean";
                    dictionary = key.typescriptName == "string" && primitive && rule.target == "Record<string, " + *ts + ">";
                }
                if (rule.kind != BindingTarget::type || (!dictionary && !isTypedArrayName(rule.target)) || !rule.from.empty() || !rule.archiveFrom.empty())
                    reject(container.container == ContainerKind::Map ? "expected a matching string-keyed Record target without an import" :
                           "expected a built-in ES2015 TypedArray target without an import");
                node.typescriptContainer = ContainerKind::None;
                node.typescriptName = rule.target;
            } else if (rule.kind == BindingTarget::numberField) {
                if (!isRecord) reject("number(field) requires a record");
                node.typescriptNumber = true;
                node.typescriptName = "number";
                node.typescriptModule.clear();
            } else if (rule.kind == BindingTarget::type) {
                if (!std::holds_alternative<BuiltinType>(node.data) && !std::holds_alternative<ExternalTypeNode>(node.data) &&
                    !(isEnum && input.modelOnly())) reject("expected an external type");
                if (!rule.target.empty()) {
                    node.typescriptName = rule.target;
                    node.typescriptTypeOnly = rule.externalInterface;
                    node.typescriptModule = rule.externalTarget && rule.from.empty() && node.module == owner
                        ? input.syntax.bindings.typescriptFrom.generic_string() : rule.from.generic_string();
                }
                if (!rule.archiveFrom.empty()) {
                    if (!std::holds_alternative<ExternalTypeNode>(node.data)) reject("external archive codec requires an external type");
                    if (node.typescriptModule.empty()) reject("external archive codec requires a TypeScript import source");
                }
                node.typescriptArchiveModule = rule.archiveFrom.generic_string();
            } else if (rule.kind == BindingTarget::interfaceType && std::holds_alternative<InterfaceType>(node.data)) {
                if (rule.helpers || !rule.renamedFrom.empty()) reject("editor settings options on an interface method binding");
                const auto decl = std::ranges::find(input.syntax.declarations, node.name, &Declaration::name);
                const auto& members = std::get<Interface>(decl->data).methods;
                for (const auto& [name, method] : rule.methods) {
                    if (std::ranges::find(members, name, &Method::name) == members.end()) reject("unknown method " + name);
                    const auto visible = std::ranges::count_if(members, [&](const Method& member) { return member.name == name && !member.cppOnly; });
                    if (visible == 0)
                        reject("cannot assign a TS name to @cppOnly method " + node.name + "." + name);
                    if (visible > 1 && method.accessor != AccessorKind::None)
                        reject("ambiguous property accessor " + name);
                }
                rules.emplace(id.value, &rule);
            } else if (rule.kind == BindingTarget::classType && std::holds_alternative<GraphType>(node.data)) {
                if ((!rule.pool && rule.classMethods.empty()) || (rule.pool && (rule.pool->assignFields || !rule.pool->keepOnReset.empty())) ||
                    rule.constructor || rule.base || !rule.from.empty() || !rule.fieldDefaults.empty() || rule.mapKey != MapKeyKind::Native)
                    reject("expected a graph pool or methods binding");
            } else if (rule.kind == BindingTarget::interfaceType || rule.kind == BindingTarget::classType) {
                if (!isRecord || !rule.methods.empty() || !rule.callbacks.empty()) reject("expected a record binding");
                node.typescriptMapKey = rule.mapKey;
                rules.emplace(id.value, &rule);
            } else if (!isEnum) reject("expected an enum binding");
        }
    }
    // Flatten alias chains and inherit the binding of the final target.
    for (size_t i = 0; i < inputs.size(); ++i) {
        const auto& declarations = inputs[i].syntax.declarations;
        for (size_t j = 0; j < declarations.size(); ++j) {
            const auto* alias = std::get_if<AliasDeclaration>(&declarations[j].data);
            if (!alias) continue;
            const auto id = graph.modules[i].declarations[j];
            const auto targetId = canonicalType(graph, {id, alias->target.location});
            const auto& target = graph.node(targetId);
            const bool templateAlias = std::holds_alternative<TemplateInstanceType>(target.data);
            if (!isArithmeticType(target.qualifiedName) && !templateAlias)
                throw std::runtime_error(alias->target.location.describe() + ": expected a scalar or template instance alias target");
            auto& node = graph.getNode(id.value);
            if (templateAlias && !inputs[i].nativeHeader.empty()) node.cppHeader = inputs[i].nativeHeader;
            node.data = AliasType{targetId};
            node.typescriptName = target.typescriptName;
            node.typescriptModule = target.typescriptModule;
        }
    }
    // Binding sources can instantiate containers before their arguments receive explicit bindings.
    // Each argument precedes its enclosing container in creation order.
    for (auto v : graph.vertices()) {
        auto& node = graph.getNode(v);
        if (node.typescriptContainer != ContainerKind::None)
            node.typescriptName = typeScriptContainerName(graph, node);
    }
    return rules;
}

std::string accessorName(const MethodInfo& method) {
    const std::string_view prefix = method.accessor == AccessorKind::Getter ? "get" : "set";
    const auto& name = method.name;
    const auto upper = [](char c) { return c >= 'A' && c <= 'Z'; };
    if (!name.starts_with(prefix) || name.size() == prefix.size() || !upper(name[prefix.size()]))
        throw std::runtime_error(method.result.location.describe() + ": expected " + std::string(prefix) + "X accessor name, got " + name);
    auto property = name.substr(prefix.size());
    // URL maps to url, URLValue to urlValue, and PassID to passID.
    for (size_t i = 0; i < property.size() && upper(property[i]); ++i) {
        if (i && i + 1 < property.size() && !upper(property[i + 1])) break;
        property[i] += 'a' - 'A';
    }
    return property;
}

CallbackTypeInfo resolveCallbackType(const TypeGraph& graph, const TypeRef& type, const std::string& scope, ModuleId module) {
    if (type.pointer) throw std::runtime_error(type.location.describe() + ": expected a TypeScript callback type");
    const auto& name = type.name;
    if (name == "Readonly" || name == "Record") {
        if (type.arguments.size() != (name == "Readonly" ? 1 : 2))
            throw std::runtime_error(type.location.describe() + ": wrong number of type arguments for " + name);
        CallbackTypeInfo result{.name = name};
        for (const auto& arg : type.arguments) result.arguments.push_back(resolveCallbackType(graph, arg, scope, module));
        return result;
    }
    if (!type.arguments.empty()) throw std::runtime_error(type.location.describe() + ": unsupported callback type arguments for " + name);
    if (name == "any" || name == "string" || name == "number" || name == "boolean" || name == "void") return {.name = name};
    return {.declaration = {resolveName(graph, name, type.location, scope, module), type.location}};
}

InterfaceType resolveInterface(TypeGraph& graph, const Declaration& decl, const TypeScriptBinding& rule,
                               const std::string& scope, ModuleId module) {
    InterfaceType result;
    for (const auto& base : std::get<Interface>(decl.data).bases) {
        const auto id = resolve(graph, base, scope, module).type;
        const auto& node = graph.node(id);
        if ((!std::holds_alternative<InterfaceType>(node.data) && !std::holds_alternative<ExternalTypeNode>(node.data)) ||
            (node.module == module && id.value >= graph.findType(qualify(scope, decl.name)).value))
            throw std::runtime_error(base.location.describe() + ": expected an earlier interface or imported base class");
        result.bases.push_back(id);
    }
    std::map<std::string, const MethodInfo*> visibleNames, setters;
    result.methods = resolveMemberFunctions(graph, std::get<Interface>(decl.data).methods, scope, module);
    for (auto& member : result.methods) {
        if (member.cppOnly) continue;
        if (member.accessor != AccessorKind::None) member.declaredProperty = accessorName(member);
        auto name = member.declaredProperty.empty() ? member.name : member.declaredProperty;
        if (const auto it = rule.methods.find(member.name); it != rule.methods.end()) {
            const auto& binding = it->second;
            if (!binding.target.empty()) name = binding.target;
            if (binding.accessor != AccessorKind::None) {
                if (member.accessor != AccessorKind::None)
                    throw std::runtime_error(member.result.location.describe() + ": duplicate accessor binding for " + member.name);
                member.accessor = binding.accessor;
            }
            if (member.accessor == AccessorKind::Setter && (binding.nullable || !binding.optional.empty()))
                throw std::runtime_error(member.result.location.describe() + ": setter cannot have nullable or optional method bindings");
            if (binding.nullable) {
                if (member.resultPresence != ResultPresence::Required)
                    throw std::runtime_error(member.result.location.describe() + ": result optionality is already declared on " + member.name);
                member.resultPresence = ResultPresence::Nullable;
            }
        }
        member.typescriptName = name;
        if (member.optionalMethod && member.accessor != AccessorKind::None)
            throw std::runtime_error(member.result.location.describe() + ": @optionalMethod requires a regular method");
        if (member.resultPresence != ResultPresence::Required && !std::holds_alternative<PointerType>(graph.node(member.result.type).data))
            throw std::runtime_error(member.result.location.describe() + ": nullable or optional result requires a pointer");
        const bool isVoid = graph.node(member.result.type).qualifiedName == "void";
        if (member.accessor == AccessorKind::Getter && (!member.isConst || !member.parameters.empty() || isVoid))
            throw std::runtime_error(member.result.location.describe() + ": getter requires a const nullary value method");
        if (member.accessor == AccessorKind::Setter && (member.isConst || member.parameters.size() != 1 || !isVoid))
            throw std::runtime_error(member.result.location.describe() + ": setter " + member.name + " must return void and accept the property type");
        auto& names = member.accessor == AccessorKind::Setter ? setters : visibleNames;
        const auto [it, inserted] = names.emplace(name, &member);
        if (!inserted && (member.accessor != AccessorKind::None || it->second->accessor != AccessorKind::None || it->second->name != member.name))
            throw std::runtime_error(member.result.location.describe() + ": duplicate TS method " +
                                     decl.name + "." + name);
    }
    for (const auto& [name, setter] : setters) {
        const auto getter = visibleNames.find(name);
        if (getter == visibleNames.end() || getter->second->accessor != AccessorKind::Getter)
            throw std::runtime_error(setter->result.location.describe() + ": setter " + setter->name + " requires a getter for " + name);
    }
    std::set<std::string> callbackNames;
    for (const auto& callback : rule.callbacks) {
        if (visibleNames.contains(callback.name) || !callbackNames.insert(callback.name).second)
            throw std::runtime_error(callback.result.location.describe() + ": duplicate TypeScript member " + decl.name + "." + callback.name);
        CallbackInfo info{.name = callback.name, .optional = callback.optional,
                          .result = resolveCallbackType(graph, callback.result, scope, module), .optionalResult = callback.optionalResult, .documentation = callback.documentation};
        for (const auto& param : callback.parameters) {
            info.parameters.push_back({param.name, param.optional, resolveCallbackType(graph, param.type, scope, module)});
        }
        result.typescriptCallbacks.push_back(std::move(info));
    }
    return result;
}

std::vector<ModuleId> bindImports(std::vector<TypeModule>& modules, const std::vector<ModuleInput>& inputs,
                                  const std::map<std::string, ModuleId>& moduleNames) {
    for (size_t i = 0; i < inputs.size(); ++i) {
        auto& module = modules[i];
        for (const auto& import : inputs[i].syntax.imports) {
            const auto it = moduleNames.find(import.name);
            if (it == moduleNames.end()) {
                throw std::runtime_error(import.location.describe() + ": unknown imported module " + import.name);
            }
            if (std::ranges::find(module.imports, it->second) != module.imports.end()) {
                throw std::runtime_error(import.location.describe() + ": duplicate import " + import.name);
            }
            module.imports.push_back(it->second);
        }
    }

    // Postorder puts dependencies before consumers while keeping the workspace's module IDs.
    enum class State { unseen, visiting, done };
    struct Frame { ModuleId id; size_t nextImport{}; };
    std::vector<State> states(modules.size(), State::unseen);
    std::vector<Frame> stack;
    std::vector<ModuleId> order;
    for (const auto& [name, root] : moduleNames) {
        if (states[root.value] != State::unseen) continue;
        states[root.value] = State::visiting;
        stack.push_back({root});
        while (!stack.empty()) {
            auto& frame = stack.back();
            const auto& imports = modules[frame.id.value].imports;
            if (frame.nextImport == imports.size()) {
                states[frame.id.value] = State::done;
                order.push_back(frame.id);
                stack.pop_back();
                continue;
            }
            const auto index = frame.nextImport++;
            const auto next = imports[index];
            if (states[next.value] == State::visiting) {
                const auto first = std::ranges::find(stack, next, &Frame::id);
                std::string cycle;
                for (auto it = first; it != stack.end(); ++it) cycle += modules[it->id.value].name + " -> ";
                const auto& import = inputs[frame.id.value].syntax.imports[index];
                throw std::runtime_error(import.location.describe() + ": module import cycle: " + cycle + import.name);
            }
            if (states[next.value] == State::unseen) {
                states[next.value] = State::visiting;
                stack.push_back({next});
            }
        }
    }
    return order;
}

// Follow only by-value record fields. Indirection does not require constructing the pointee.
void checkRecordCycles(const TypeGraph& graph) {
    enum class State { unseen, visiting, done };
    std::vector<State> states(graph.num_vertices(), State::unseen);
    struct Frame { TypeId record; size_t nextField{}; };
    std::vector<Frame> stack;
    for (auto v : graph.vertices()) {
        if (states[v] != State::unseen || !std::holds_alternative<RecordType>(graph.getNode(v).data)) continue;
        states[v] = State::visiting;
        stack.push_back({TypeId{v}});
        while (!stack.empty()) {
            auto& frame = stack.back();
            const auto& fields = std::get<RecordType>(graph.node(frame.record).data).fields;
            if (frame.nextField == fields.size()) {
                states[frame.record.value] = State::done;
                stack.pop_back();
                continue;
            }
            const auto& field = fields[frame.nextField++];
            const auto target = field.type.type;
            if (!std::holds_alternative<RecordType>(graph.node(target).data)) continue;
            if (states[target.value] == State::visiting) {
                throw std::runtime_error(field.type.location.describe() + ": cyclic by-value field " +
                                         graph.node(frame.record).qualifiedName + "." + field.name);
            }
            if (states[target.value] == State::unseen) {
                states[target.value] = State::visiting;
                stack.push_back({target});
            }
        }
    }
}

} // namespace

bool isArithmeticType(const std::string& type) {
    return type == "float" || type == "double" || integerRanges().contains(type);
}

std::optional<uint64_t> indexedVertexLimit(const std::string& type) {
    const auto& ranges = integerRanges();
    const auto it = ranges.find(type);
    if (it == ranges.end() || type == "bool") return std::nullopt;
    return it->second.max;
}

TypeGraph buildTypeGraph(const std::vector<ModuleInput>& inputs, std::vector<ModuleId>* dependencyOrder) {
    TypeGraph graph;
    registerScalars(graph);
    std::map<std::string, ModuleId> moduleNames;
    std::vector<std::string> scopes;
    for (const auto& input : inputs) {
        const auto& module = input.syntax;
        const ModuleId owner{static_cast<uint32_t>(graph.modules.size())};
        if (!moduleNames.emplace(module.name, owner).second) throw std::runtime_error("duplicate module " + module.name);
        auto& entry = graph.modules.emplace_back(TypeModule{.name = module.name, .namespaces = module.namespaces,
                                                           .cppConstants = module.cppConstants});
        std::string scope;
        for (const auto& part : module.namespaces) scope = qualify(scope, part);
        const auto moduleNamespace = ensureNamespace(graph, scope);
        for (const auto& decl : module.declarations) {
            const auto& declarationScope = decl.externalScope.empty() ? scope : decl.externalScope;
            const auto parent = declarationScope == scope ? moduleNamespace : ensureNamespace(graph, declarationScope);
            auto qualified = qualify(declarationScope, decl.name);
            if (graph.findType(qualified) != TypeId{}) {
                throw std::runtime_error(decl.location.describe() + ": duplicate type " + qualified);
            }
            const auto* external = std::get_if<ExternalDeclaration>(&decl.data);
            TypeNode node{.name = decl.name, .qualifiedName = std::move(qualified), .module = owner, .data = declare(decl),
                          .cppHeader = decl.cppHeader, .cppForwardHeader = decl.cppForwardHeader, .documentation = decl.documentation};
            const bool tag = std::holds_alternative<TagType>(node.data);
            if (input.modelOnly()) {
                if (decl.externalScope.empty() && !tag && !external) node.typescriptName = decl.name;
            } else {
                const bool alias = std::holds_alternative<AliasType>(node.data);
                const auto& header = alias ? input.forwardHeader : input.nativeHeader;
                if (!header.empty()) {
                    if (!node.cppHeader.empty())
                        throw std::runtime_error(decl.location.describe() + ": C++ header already declared in .spec for " + node.qualifiedName);
                    node.cppHeader = header;
                }
                if (!tag && !alias && !input.typescriptModule.empty()) {
                    node.typescriptName = decl.name;
                    // Settings records export interfaces and factories, while ordinary records export constructors.
                    if (!input.typescriptSettings || !std::holds_alternative<RecordType>(node.data))
                        node.typescriptModule = input.typescriptModule;
                }
            }
            if (external && external->isString && !node.typescriptName) node.typescriptName = "string";
            const bool namedExternal = external && external->templateParameters.empty() && external->container == ContainerKind::None;
            if (!module.bindings.typescriptFrom.empty() && !node.typescriptName && (namedExternal || std::holds_alternative<Enum>(decl.data))) {
                node.typescriptName = decl.name;
                node.typescriptModule = module.bindings.typescriptFrom.generic_string();
            }
            if (decl.cppOnly) {
                node.cppOnly = true;
                node.typescriptName.reset();
                node.typescriptModule.clear();
            }
            node.variadicTemplate = external && external->variadicTemplate;
            auto id = graph.appendNode(std::move(node), parent);
            entry.declarations.push_back(id);
            if (const auto* iface = std::get_if<Interface>(&decl.data); iface && iface->archiveRole != ArchiveRole::none) {
                const bool inputRole = iface->archiveRole == ArchiveRole::input;
                auto& endpoint = inputRole ? entry.inputArchive : entry.outputArchive;
                if (endpoint != TypeId{})
                    throw std::runtime_error(decl.location.describe() + ": duplicate @archive(" + (inputRole ? "input" : "output")
                                             + ") in " + module.name + ", already declared by " + graph.node(endpoint).qualifiedName);
                endpoint = id;
            }
            if (external) {
                for (uint32_t p = 0; p < external->templateParameters.size(); ++p) {
                    auto param = graph.appendNode(TypeNode{.name = external->templateParameters[p], .module = owner,
                                                           .data = TemplateParameterType{id, p}}, id);
                    graph.getNode(id.value).templateParameters.push_back(param);
                }
            }
            if (const auto* g = std::get_if<GraphDeclaration>(&decl.data)) {
                graph.appendNode(TypeNode{.name = g->vertexAlias, .qualifiedName = qualify(graph.node(id).qualifiedName, g->vertexAlias),
                                          .module = owner, .data = AliasType{}, .cppHeader = graph.node(id).cppHeader}, id);
            }
        }
        if ((entry.inputArchive == TypeId{}) != (entry.outputArchive == TypeId{})) {
            const auto missing = entry.inputArchive == TypeId{} ? "input" : "output";
            throw std::runtime_error(module.name + ": missing @archive(" + missing + ") interface");
        }
        scopes.push_back(std::move(scope));
    }
    std::set<std::string> constantNames;
    for (size_t i = 0; i < graph.modules.size(); ++i) {
        for (const auto& constant : graph.modules[i].cppConstants) {
            const auto name = qualify(scopes[i], constant.name);
            if (!constantNames.insert(name).second || graph.findType(name) != TypeId{})
                throw std::runtime_error(constant.location.describe() + ": duplicate name " + name);
        }
    }
    auto order = bindImports(graph.modules, inputs, moduleNames);
    // Link all alias targets before resolving uses, including forward references in the same module.
    for (ModuleId owner : order) {
        const auto i = owner.value;
        for (const auto& decl : inputs[i].syntax.declarations) {
            if (const auto* alias = std::get_if<AliasDeclaration>(&decl.data)) {
                const auto target = resolve(graph, alias->target, scopes[i], owner);
                const auto id = graph.findType(qualify(scopes[i], decl.name));
                graph.getNode(id.value).data = AliasType{target.type};
            } else if (const auto* g = std::get_if<GraphDeclaration>(&decl.data)) {
                const auto target = resolve(graph, *g->vertexType, scopes[i], owner);
                const auto alias = graph.findType(qualify(qualify(scopes[i], decl.name), g->vertexAlias));
                graph.getNode(alias.value).data = AliasType{target.type};
            }
        }
    }
    const auto bindings = bindTypes(graph, inputs, scopes);
    const TypeScriptBinding noBinding;
    for (ModuleId owner : order) {
        const auto i = owner.value;
        const auto& input = inputs[i];
        // Member initializers may name an enumerator declared later in the same module.
        for (size_t j = 0; j < input.syntax.declarations.size(); ++j) {
            const auto& decl = input.syntax.declarations[j];
            if (const auto* enumeration = std::get_if<Enum>(&decl.data))
                resolveEnum(graph, graph.modules[i].declarations[j], *enumeration, decl.externalScope.empty() ? scopes[i] : decl.externalScope, owner);
        }
        for (size_t j = 0; j < input.syntax.declarations.size(); ++j) {
            const auto& decl = input.syntax.declarations[j];
            const auto id = graph.modules[i].declarations[j];
            // Resolving compound types can grow the graph. Reacquire the node by ID afterwards.
            const auto binding = bindings.find(id.value);
            const auto& rule = binding == bindings.end() ? noBinding : *binding->second;
            if (const auto* record = std::get_if<Record>(&decl.data)) {
                auto payload = resolveRecord(graph, id, *record, scopes[i], owner);
                const auto& node = graph.node(id);
                if (!input.modelOnly() && !node.typescriptModule.empty() && !node.typescriptNumber && rule.from.empty())
                    payload.typescriptConstructor = selectTypeScriptConstructor(node, payload, rule);
                graph.getNode(id.value).data = std::move(payload);
            } else if (const auto* g = std::get_if<GraphDeclaration>(&decl.data)) {
                auto payload = resolveGraph(graph, *g, qualify(scopes[i], decl.name), owner);
                graph.getNode(id.value).data = std::move(payload);
            } else if (std::holds_alternative<Interface>(decl.data)) {
                auto payload = resolveInterface(graph, decl, rule, scopes[i], owner);
                graph.getNode(id.value).data = std::move(payload);
            }
        }
    }
    // Numeric record bindings refer to resolved fields and may be used by other modules.
    for (const auto& input : inputs) {
        const auto owner = moduleNames.at(input.syntax.name);
        for (const auto& rule : input.syntax.bindings.typescript) {
            if (rule.kind != BindingTarget::numberField && rule.fieldDefaults.empty()) continue;
            const auto& at = rule.source.location;
            const auto id = resolveName(graph, rule.source.name, at, scopes[owner.value], owner);
            auto& fields = std::get<RecordType>(graph.getNode(id.value).data).fields;
            if (rule.kind == BindingTarget::numberField) {
                if (fields.size() != 1 || fields.front().name != rule.numberField ||
                    fields.front().transient || fields.front().isNullable || fields.front().cppOnly)
                    throw std::runtime_error(at.describe() + ": number(field) requires a single non-transient numeric field named " + rule.numberField);
                const auto& target = graph.node(canonicalType(graph, fields.front().type));
                if (target.typescriptName != "number" || !isArithmeticType(target.qualifiedName))
                    throw std::runtime_error(at.describe() + ": expected a numeric field for " + rule.source.name + "." + rule.numberField);
            }
            for (const auto& [name, initial] : rule.fieldDefaults) {
                const auto field = std::ranges::find(fields, name, &FieldInfo::name);
                if (field == fields.end()) throw std::runtime_error(at.describe() + ": unknown field " + rule.source.name + "." + name);
                field->typescriptInitializer = initial;
            }
        }
    }
    checkRecordCycles(graph);
    if (dependencyOrder) *dependencyOrder = std::move(order);
    return graph;
}

} // namespace cc::codegen
