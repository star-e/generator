#include "model/TypeGraphBuilder.h"

#include <algorithm>
#include <set>
#include <stdexcept>

namespace cc::codegen {
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
                                  .cppHeader = type.header, .typescriptName = type.typescript}, parent);
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
            if (declaration.module != ModuleId{} && declaration.module != owner) {
                throw std::runtime_error(location.describe() + ": type requires a module import: " + name);
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

RecordType resolveRecord(TypeGraph& graph, const Record& syntax, const std::string& scope, ModuleId owner) {
    RecordType result;
    for (const auto& field : syntax.fields) {
        auto use = resolve(graph, field.type, scope, owner);
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
        result.fields.push_back({std::move(use), field.name, field.initializer, field.isReadonly, field.isNullable});
    }
    return result;
}

TypeData declare(const Declaration& decl) {
    if (const auto* syntax = std::get_if<Enum>(&decl.data)) {
        EnumType enumeration;
        for (const auto& value : syntax->values) enumeration.values.push_back({value, {}});
        return enumeration;
    }
    if (std::holds_alternative<Record>(decl.data)) return RecordType{};
    return InterfaceType{};
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
        MethodInfo member{.result = resolve(graph, method.result, scope, module), .name = method.name,
                          .isConst = method.isConst, .isNoexcept = method.isNoexcept,
                          .typescriptName = projectMethod(projection, decl.name, method.name)};
        if (member.typescriptName && !visibleNames.insert(*member.typescriptName).second) {
            throw std::runtime_error(method.result.location.describe() + ": duplicate TS method " +
                                     decl.name + "." + *member.typescriptName);
        }
        for (const auto& parameter : method.parameters) {
            member.parameters.push_back({resolve(graph, parameter.type, scope, module), parameter.name});
        }
        result.methods.push_back(std::move(member));
    }
    return result;
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
    std::set<std::string> moduleNames;
    std::vector<std::string> scopes;
    for (const auto& input : inputs) {
        const auto& module = input.syntax;
        if (!moduleNames.insert(module.name).second) throw std::runtime_error("duplicate module " + module.name);
        const ModuleId owner{static_cast<uint32_t>(graph.modules.size())};
        auto& entry = graph.modules.emplace_back(TypeModule{module.name, module.namespaces, {}});
        std::string scope;
        for (const auto& part : module.namespaces) scope = qualify(scope, part);
        const auto parent = ensureNamespace(graph, scope);
        for (const auto& decl : module.declarations) {
            auto qualified = qualify(scope, decl.name);
            if (graph.findType(qualified) != TypeId{}) {
                throw std::runtime_error(decl.location.describe() + ": duplicate type " + qualified);
            }
            auto id = graph.appendNode(TypeNode{.name = decl.name, .qualifiedName = std::move(qualified),
                                                .module = owner, .data = declare(decl),
                                                .typescriptName = decl.name}, parent);
            entry.declarations.push_back(id);
        }
        scopes.push_back(std::move(scope));
    }
    for (size_t i = 0; i < inputs.size(); ++i) {
        const auto& input = inputs[i];
        const ModuleId owner{static_cast<uint32_t>(i)};
        for (size_t j = 0; j < input.syntax.declarations.size(); ++j) {
            const auto& decl = input.syntax.declarations[j];
            const auto id = graph.modules[i].declarations[j];
            // Resolving compound types can grow the graph. Reacquire the node by ID afterwards.
            if (const auto* record = std::get_if<Record>(&decl.data)) {
                auto payload = resolveRecord(graph, *record, scopes[i], owner);
                graph.getNode(id.value).data = std::move(payload);
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
