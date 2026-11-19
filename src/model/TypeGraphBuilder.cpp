#include "model/TypeGraphBuilder.h"
#include "model/IntegerLiteral.h"

#include <algorithm>
#include <bit>
#include <limits>
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

void registerExternalTypes(TypeGraph& graph, const TypeTable& types) {
    for (const auto& [qualified, type] : types) {
        const auto pos = qualified.rfind("::");
        const auto parent = pos == std::string::npos ? TypeId{} : ensureNamespace(graph, qualified.substr(0, pos));
        const auto name = pos == std::string::npos ? qualified : qualified.substr(pos + 2);
        graph.appendNode(TypeNode{.name = name, .qualifiedName = qualified,
                                  .data = type.builtin ? TypeData{BuiltinType{}} : TypeData{ExternalTypeNode{}},
                                  .cppHeader = type.header, .typescriptName = type.typescript,
                                  .typescriptModule = type.typescriptModule}, parent);
    }
}

TypeId resolveName(const TypeGraph& graph, const std::string& name, const SourceLocation& location,
                   std::string scope, ModuleId owner) {
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
    if (!use.arguments.empty()) {
        std::vector<TemplateArgument> arguments;
        for (const auto& argument : use.arguments) arguments.emplace_back(resolve(graph, argument, scope, owner).type);
        id = graph.makeTemplateInstance(id, std::move(arguments));
    }
    return {use.pointer ? graph.makePointer(id) : id, use.location};
}

MethodInfo resolveMethod(TypeGraph& graph, const Method& method, const std::string& scope, ModuleId owner) {
    MethodInfo member{.result = resolve(graph, method.result, scope, owner), .name = method.name,
                      .isConst = method.isConst, .isNoexcept = method.isNoexcept};
    for (const auto& parameter : method.parameters) {
        member.parameters.push_back({resolve(graph, parameter.type, scope, owner), parameter.name});
    }
    return member;
}

RecordType resolveRecord(TypeGraph& graph, const Record& syntax, const std::string& scope, ModuleId owner) {
    RecordType result{.moveOnly = syntax.moveOnly, .serializable = syntax.serializable};
    for (const auto& field : syntax.fields) {
        auto use = resolve(graph, field.type, scope, owner);
        std::string_view magnitude = field.initializer;
        if (magnitude.starts_with('-')) magnitude.remove_prefix(1);
        if (hasHexPrefix(magnitude)) {
            const auto& type = graph.node(use.type).qualifiedName;
            if (!validUnsignedFieldDefault(type, field.initializer)) {
                throw std::runtime_error(use.location.describe() + ": " + field.name + ": invalid " + type +
                                         " default " + field.initializer);
            }
        }
        const auto pos = field.initializer.rfind("::");
        if (pos != std::string::npos) {
            const auto type = resolveName(graph, field.initializer.substr(0, pos), use.location, scope, owner);
            const auto& data = graph.node(type).data;
            const auto* enumeration = std::get_if<EnumType>(&data);
            if (type != use.type || (!enumeration && !std::holds_alternative<ExternalTypeNode>(data)) ||
                (enumeration && std::ranges::find(enumeration->values, field.initializer.substr(pos + 2),
                                                  &Enumerator::name) == enumeration->values.end())) {
                throw std::runtime_error(use.location.describe() + ": invalid enum default " + field.initializer);
            }
        }
        result.fields.push_back({std::move(use), field.name, field.initializer, field.isReadonly, field.isNullable, field.transient});
    }
    std::set<std::vector<uint32_t>> signatures;
    for (const auto& ctor : syntax.constructors) {
        std::vector<uint32_t> members, signature;
        for (const auto& name : ctor.fields) {
            const auto field = std::ranges::find(result.fields, name, &FieldInfo::name);
            if (field == result.fields.end()) throw std::runtime_error(ctor.location.describe() + ": unknown constructor field " + name);
            const auto index = static_cast<uint32_t>(field - result.fields.begin());
            if (std::ranges::find(members, index) != members.end()) {
                throw std::runtime_error(ctor.location.describe() + ": repeated constructor field " + name);
            }
            members.push_back(index);
            signature.push_back(field->type.type.value);
        }
        if (!signatures.insert(std::move(signature)).second) {
            throw std::runtime_error(ctor.location.describe() + ": duplicate constructor signature");
        }
        result.constructors.push_back(std::move(members));
    }
    for (const auto& method : syntax.methods) result.methods.push_back(resolveMethod(graph, method, scope, owner));
    return result;
}

TypeData declare(const Declaration& decl) {
    if (std::holds_alternative<TagDeclaration>(decl.data)) return TagType{};
    if (std::holds_alternative<GraphDeclaration>(decl.data)) return GraphType{};
    if (const auto* syntax = std::get_if<Enum>(&decl.data)) {
        EnumType enumeration{.isFlags = syntax->isFlags};
        for (const auto& value : syntax->values) {
            auto& member = enumeration.values.emplace_back(Enumerator{.name = value.name, .initializer = value.initializer,
                                                                      .deprecationMessage = value.deprecationMessage});
            if (value.leftShift) member.initializer += " << " + value.leftShift->count;
        }
        return enumeration;
    }
    if (std::holds_alternative<Record>(decl.data)) return RecordType{};
    if (const auto* external = std::get_if<ExternalDeclaration>(&decl.data)) return ExternalTypeNode{external->isStruct};
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

TypeId canonicalType(const TypeGraph& graph, TypeUse use) {
    std::set<uint32_t> visited;
    while (const auto* alias = std::get_if<AliasType>(&graph.node(use.type).data)) {
        if (!visited.insert(use.type.value).second) throw std::runtime_error(use.location.describe() + ": cyclic type alias");
        use.type = alias->target;
    }
    return use.type;
}

GraphType resolveGraph(TypeGraph& graph, const GraphDeclaration& syntax, const std::string& scope, ModuleId owner) {
    GraphType result;
    const auto descriptor = graph.findType(qualify(scope, syntax.vertexAlias));
    result.vertexType = canonicalType(graph, {descriptor, syntax.vertexType->location});
    result.indexedVertices = syntax.indexed;
    result.vertexList = syntax.vertexList;
    result.mutableGraph = syntax.mutableGraph;
    result.hierarchy = syntax.hierarchy;
    const auto& vertex = graph.node(result.vertexType);
    if (syntax.indexed && (vertex.qualifiedName == "bool" || !integerRanges().contains(vertex.qualifiedName))) {
        throw std::runtime_error(syntax.vertexType->location.describe() + ": expected an integer descriptor for indexed vertices");
    }
    std::set<std::string> accessors;
    for (const auto& component : syntax.components) {
        const auto tag = canonicalType(graph, resolve(graph, component.tag, scope, owner));
        const auto& node = graph.node(tag);
        if (!std::holds_alternative<TagType>(node.data)) {
            throw std::runtime_error(component.tag.location.describe() + ": component requires an independently declared @tag: " + component.tag.name);
        }
        const auto accessor = componentAccessorSuffix(node.name);
        if (accessor.empty() || !accessors.insert(accessor).second) {
            throw std::runtime_error(component.tag.location.describe() + ": invalid or duplicate component accessor for " + node.qualifiedName);
        }
        const auto value = resolve(graph, component.value, scope, owner);
        result.components.push_back({tag, value.type, component.member});
    }
    if (syntax.nameComponent) {
        const auto& selected = *syntax.nameComponent;
        const auto tag = canonicalType(graph, resolve(graph, selected, scope, owner));
        const auto it = std::ranges::find(result.components, tag, &ComponentInfo::tag);
        if (it == result.components.end())
            throw std::runtime_error(selected.location.describe() + ": named_graph requires a component of this graph: " + selected.name);
        const auto& value = graph.node(canonicalType(graph, {it->valueType, selected.location})).qualifiedName;
        if (value != "ccstd::pmr::string" && value != "ccstd::string" && value != "std::pmr::string" && value != "std::string")
            throw std::runtime_error(selected.location.describe() + ": named_graph requires a string component: " + selected.name);
        result.nameComponentIndex = static_cast<uint32_t>(it - result.components.begin());
    }
    if (syntax.parentGraph) {
        if (!result.hierarchy) throw std::runtime_error(syntax.parentGraph->describe() + ": parent_graph requires hierarchy_graph");
        if (!result.nameComponentIndex) throw std::runtime_error(syntax.parentGraph->describe() + ": parent_graph requires named_graph");
        result.parentGraph = true;
    }
    if (syntax.addressableGraph) {
        if (!result.parentGraph) throw std::runtime_error(syntax.addressableGraph->describe() + ": addressable_graph requires parent_graph");
        result.addressableGraph = true;
    }
    return result;
}

void resolveEnum(TypeGraph& graph, TypeId id, const Enum& syntax, const std::string& scope, ModuleId owner) {
    const auto& ranges = integerRanges();
    TypeId underlying;
    std::string base = "int";
    if (syntax.underlying) {
        const auto use = resolve(graph, *syntax.underlying, scope, owner);
        underlying = use.type;
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

std::optional<std::string> projectMethod(const Projection& rules, const std::string& decl, const std::string& name) {
    if (auto group = rules.find(decl); group != rules.end()) {
        if (auto rule = group->second.find(name); rule != group->second.end()) return rule->second;
    }
    return name;
}

InterfaceType resolveInterface(TypeGraph& graph, const Declaration& decl, const Projection& projection,
                               const std::string& scope, ModuleId module) {
    InterfaceType result;
    std::set<std::string> visibleNames;
    for (const auto& method : std::get<Interface>(decl.data).methods) {
        auto member = resolveMethod(graph, method, scope, module);
        member.typescriptName = projectMethod(projection, decl.name, method.name);
        if (member.typescriptName && !visibleNames.insert(*member.typescriptName).second) {
            throw std::runtime_error(method.result.location.describe() + ": duplicate TS method " +
                                     decl.name + "." + *member.typescriptName);
        }
        result.methods.push_back(std::move(member));
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

TypeGraph buildTypeGraph(const std::vector<ModuleInput>& inputs, const TypeTable& externalTypes) {
    TypeGraph graph;
    registerExternalTypes(graph, externalTypes);
    std::map<std::string, ModuleId> moduleNames;
    std::vector<std::string> scopes;
    for (const auto& input : inputs) {
        const auto& module = input.syntax;
        const ModuleId owner{static_cast<uint32_t>(graph.modules.size())};
        if (!moduleNames.emplace(module.name, owner).second) throw std::runtime_error("duplicate module " + module.name);
        auto& entry = graph.modules.emplace_back(TypeModule{.name = module.name, .namespaces = module.namespaces});
        std::string scope;
        for (const auto& part : module.namespaces) scope = qualify(scope, part);
        const auto parent = ensureNamespace(graph, scope);
        for (const auto& decl : module.declarations) {
            auto qualified = qualify(scope, decl.name);
            if (graph.findType(qualified) != TypeId{}) {
                throw std::runtime_error(decl.location.describe() + ": duplicate type " + qualified);
            }
            const bool external = std::holds_alternative<ExternalDeclaration>(decl.data);
            TypeNode node{.name = decl.name, .qualifiedName = std::move(qualified), .module = owner, .data = declare(decl)};
            if (const auto it = input.externalTypes.find(decl.name); it != input.externalTypes.end()) {
                node.cppHeader = it->second.header;
                node.typescriptName = it->second.typescript;
                node.typescriptModule = it->second.typescriptModule;
            } else if (!external && !std::holds_alternative<TagType>(node.data)) {
                node.typescriptName = decl.name;
            }
            auto id = graph.appendNode(std::move(node), parent);
            entry.declarations.push_back(id);
            if (const auto* g = std::get_if<GraphDeclaration>(&decl.data)) {
                graph.appendNode(TypeNode{.name = g->vertexAlias, .qualifiedName = qualify(graph.node(id).qualifiedName, g->vertexAlias),
                                          .module = owner, .data = AliasType{}}, id);
            }
        }
        for (const auto& [name, mapping] : input.externalTypes) {
            const auto it = std::ranges::find(module.declarations, name, &Declaration::name);
            if (it == module.declarations.end() || (!std::holds_alternative<ExternalDeclaration>(it->data) &&
                                                    !std::holds_alternative<Enum>(it->data))) {
                throw std::runtime_error(module.name + ": mapping for unknown external type " + name);
            }
        }
        scopes.push_back(std::move(scope));
    }
    for (ModuleId owner : bindImports(graph.modules, inputs, moduleNames)) {
        const auto i = owner.value;
        const auto& input = inputs[i];
        // Bind descriptor aliases before their uses, including references from a preceding graph.
        for (const auto& decl : input.syntax.declarations) {
            if (const auto* g = std::get_if<GraphDeclaration>(&decl.data)) {
                const auto target = resolve(graph, *g->vertexType, scopes[i], owner);
                const auto alias = graph.findType(qualify(qualify(scopes[i], decl.name), g->vertexAlias));
                graph.getNode(alias.value).data = AliasType{target.type};
            }
        }
        for (size_t j = 0; j < input.syntax.declarations.size(); ++j) {
            const auto& decl = input.syntax.declarations[j];
            const auto id = graph.modules[i].declarations[j];
            // Resolving compound types can grow the graph. Reacquire the node by ID afterwards.
            if (const auto* record = std::get_if<Record>(&decl.data)) {
                auto payload = resolveRecord(graph, *record, scopes[i], owner);
                graph.getNode(id.value).data = std::move(payload);
            } else if (const auto* g = std::get_if<GraphDeclaration>(&decl.data)) {
                auto payload = resolveGraph(graph, *g, qualify(scopes[i], decl.name), owner);
                graph.getNode(id.value).data = std::move(payload);
            } else if (const auto* enumeration = std::get_if<Enum>(&decl.data)) {
                resolveEnum(graph, id, *enumeration, scopes[i], owner);
            } else if (std::holds_alternative<Interface>(decl.data)) {
                auto payload = resolveInterface(graph, decl, input.projection, scopes[i], owner);
                graph.getNode(id.value).data = std::move(payload);
            }
        }
    }
    checkRecordCycles(graph);
    return graph;
}

} // namespace cc::codegen
