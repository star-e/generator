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
    if (const auto* syntax = std::get_if<Enum>(&decl.data)) {
        EnumType enumeration;
        for (const auto& value : syntax->values) enumeration.values.push_back({value, {}});
        return enumeration;
    }
    if (std::holds_alternative<Record>(decl.data)) return RecordType{};
    if (const auto* external = std::get_if<ExternalDeclaration>(&decl.data)) return ExternalTypeNode{external->isStruct};
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
            if (external) {
                if (const auto it = input.externalTypes.find(decl.name); it != input.externalTypes.end()) {
                    node.cppHeader = it->second.header;
                    node.typescriptName = it->second.typescript;
                    node.typescriptModule = it->second.typescriptModule;
                }
            } else {
                node.typescriptName = decl.name;
            }
            auto id = graph.appendNode(std::move(node), parent);
            entry.declarations.push_back(id);
        }
        for (const auto& [name, mapping] : input.externalTypes) {
            const auto it = std::ranges::find(module.declarations, name, &Declaration::name);
            if (it == module.declarations.end() || !std::holds_alternative<ExternalDeclaration>(it->data)) {
                throw std::runtime_error(module.name + ": mapping for unknown external type " + name);
            }
        }
        scopes.push_back(std::move(scope));
    }
    for (ModuleId owner : bindImports(graph.modules, inputs, moduleNames)) {
        const auto i = owner.value;
        const auto& input = inputs[i];
        for (size_t j = 0; j < input.syntax.declarations.size(); ++j) {
            const auto& decl = input.syntax.declarations[j];
            const auto id = graph.modules[i].declarations[j];
            // Resolving compound types can grow the graph. Reacquire the node by ID afterwards.
            if (const auto* record = std::get_if<Record>(&decl.data)) {
                auto payload = resolveRecord(graph, *record, scopes[i], owner);
                graph.getNode(id.value).data = std::move(payload);
            } else if (const auto* enumeration = std::get_if<Enum>(&decl.data); enumeration && enumeration->underlying) {
                const auto underlying = resolve(graph, *enumeration->underlying, scopes[i], owner);
                const auto& base = graph.node(underlying.type);
                static const std::set<std::string> integral{"int", "unsigned", "short", "long", "char", "bool",
                    "int8_t", "uint8_t", "int16_t", "uint16_t", "int32_t", "uint32_t", "int64_t", "uint64_t"};
                if (!integral.contains(base.qualifiedName)) {
                    throw std::runtime_error(underlying.location.describe() + ": expected integral enum base");
                }
                std::get<EnumType>(graph.getNode(id.value).data).underlying = underlying.type;
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
