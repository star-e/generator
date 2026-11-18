#include "model/TypeGraphBuilder.h"

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

TypeUse resolve(TypeGraph& graph, const TypeRef& use, std::string scope, ModuleId owner) {
    for (;;) {
        const auto id = graph.findType(qualify(scope, use.name));
        if (id != TypeId{}) {
            const auto& declaration = graph.node(id);
            if (std::holds_alternative<NamespaceType>(declaration.data)) {
                throw std::runtime_error(use.location.describe() + ": namespace used as type " + use.name);
            }
            if (declaration.module != ModuleId{} && declaration.module != owner) {
                throw std::runtime_error(use.location.describe() + ": type requires a module import: " + use.name);
            }
            return {use.pointer ? graph.makePointer(id) : id, use.location};
        }
        if (scope.empty()) throw std::runtime_error(use.location.describe() + ": unknown type " + use.name);
        const auto parent = scope.rfind("::");
        scope = parent == std::string::npos ? std::string{} : scope.substr(0, parent);
    }
}

std::optional<std::string> projectMethod(const Projection& rules, const std::string& decl, const std::string& name) {
    if (auto group = rules.find(decl); group != rules.end()) {
        if (auto rule = group->second.find(name); rule != group->second.end()) return rule->second;
    }
    return name;
}

InterfaceType resolveInterface(TypeGraph& graph, const Interface& syntax, const Projection& projection,
                               const std::string& scope, ModuleId module) {
    InterfaceType result;
    std::set<std::string> visibleNames;
    for (const auto& method : syntax.methods) {
        MethodInfo member{.result = resolve(graph, method.result, scope, module), .name = method.name,
                          .isConst = method.isConst, .isNoexcept = method.isNoexcept,
                          .typescriptName = projectMethod(projection, syntax.name, method.name)};
        if (member.typescriptName && !visibleNames.insert(*member.typescriptName).second) {
            throw std::runtime_error(method.result.location.describe() + ": duplicate TS method " +
                                     syntax.name + "." + *member.typescriptName);
        }
        for (const auto& parameter : method.parameters) {
            member.parameters.push_back({resolve(graph, parameter.type, scope, module), parameter.name});
        }
        result.methods.push_back(std::move(member));
    }
    return result;
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
        for (const auto& decl : module.interfaces) {
            auto qualified = qualify(scope, decl.name);
            if (graph.findType(qualified) != TypeId{}) {
                throw std::runtime_error(decl.location.describe() + ": duplicate type " + qualified);
            }
            auto id = graph.appendNode(TypeNode{.name = decl.name, .qualifiedName = std::move(qualified),
                                                .module = owner, .data = InterfaceType{},
                                                .typescriptName = decl.name}, parent);
            entry.declarations.push_back(id);
        }
        scopes.push_back(std::move(scope));
    }
    for (size_t i = 0; i < inputs.size(); ++i) {
        const auto& input = inputs[i];
        const ModuleId owner{static_cast<uint32_t>(i)};
        for (size_t j = 0; j < input.syntax.interfaces.size(); ++j) {
            const auto id = graph.modules[i].declarations[j];
            // Resolving compound types can grow the graph. Reacquire the node by ID afterwards.
            auto payload = resolveInterface(graph, input.syntax.interfaces[j], input.projection, scopes[i], owner);
            graph.getNode(id.value).data = std::move(payload);
        }
    }
    return graph;
}

} // namespace cc::codegen
