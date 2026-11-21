#include "generated/TypeGraphTypes.h"
#include "model/TypeName.h"

#include <cstddef>
#include <stdexcept>
#include <type_traits>

namespace cc::codegen {

TypeId TypeGraph::findType(const std::string& qualifiedName) const {
    const auto it = nameIndex.find(qualifiedName);
    return it == nameIndex.end() ? TypeId{} : it->second;
}

const TypeNode& TypeGraph::node(TypeId id) const {
    return nodes.at(id.value);
}

TypeId TypeGraph::appendNode(TypeNode newNode, TypeId parent) {
    if (nodes.size() >= null_vertex()) throw std::length_error("too many type nodes");
    if (newNode.module != ModuleId{} && newNode.module.value >= modules.size()) {
        throw std::out_of_range("invalid declaration module");
    }
    const TypeId id{static_cast<Vertex>(nodes.size())};
    VertexType adjacency;
    if (parent != TypeId{}) {
        if (parent.value >= id.value) throw std::out_of_range("invalid scope vertex");
        adjacency.mInEdges.push_back(parent.value);
    }
    auto name = nameIndex.end();
    if (!newNode.qualifiedName.empty()) {
        const auto inserted = nameIndex.emplace(newNode.qualifiedName, id);
        if (!inserted.second) throw std::invalid_argument("duplicate type " + newNode.qualifiedName);
        name = inserted.first;
    }
    try {
        nodes.push_back(std::move(newNode));
        mVertices.push_back(std::move(adjacency));
        if (parent != TypeId{}) mVertices[parent.value].mOutEdges.push_back(id.value);
    } catch (...) {
        if (mVertices.size() > id.value) mVertices.pop_back();
        if (nodes.size() > id.value) nodes.pop_back();
        if (name != nameIndex.end()) nameIndex.erase(name);
        throw;
    }
    return id;
}

namespace {

// Layouts previously checked by the seed generator.
template<class T, std::size_t Size, std::size_t Alignment>
constexpr bool hasFixedLayout = std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T> &&
                                sizeof(T) == Size && alignof(T) == Alignment;

static_assert(hasFixedLayout<AccessorKind, 4, 4> && hasFixedLayout<ResultPresence, 4, 4>);
static_assert(hasFixedLayout<NamespaceType, 1, 1> && hasFixedLayout<TagType, 1, 1> && hasFixedLayout<BuiltinType, 1, 1>);
static_assert(hasFixedLayout<ExternalTypeNode, 8, 4>);
static_assert(offsetof(ExternalTypeNode, isStruct) == 0 && offsetof(ExternalTypeNode, isString) == 1);
static_assert(offsetof(ExternalTypeNode, isPmr) == 2 && offsetof(ExternalTypeNode, constructorConstRef) == 3);
static_assert(offsetof(ExternalTypeNode, container) == 4);
static_assert(hasFixedLayout<AliasType, 4, 4> && offsetof(AliasType, target) == 0);
static_assert(hasFixedLayout<PointerType, 4, 4> && offsetof(PointerType, pointee) == 0);
static_assert(hasFixedLayout<ReferenceType, 8, 4> && offsetof(ReferenceType, referred) == 0 &&
              offsetof(ReferenceType, rvalue) == 4);
static_assert(hasFixedLayout<QualifiedType, 8, 4> && offsetof(QualifiedType, base) == 0);
static_assert(offsetof(QualifiedType, isConst) == 4 && offsetof(QualifiedType, isVolatile) == 5);
static_assert(hasFixedLayout<TemplateParameterType, 8, 4> && offsetof(TemplateParameterType, owner) == 0 &&
              offsetof(TemplateParameterType, index) == 4);

template<class Payload>
TypeId intern(TypeGraph& graph, Payload value) {
    for (auto v : graph.vertices()) {
        const auto* existing = std::get_if<Payload>(&graph.getNode(v).data);
        if (existing && *existing == value) return TypeId{v};
    }
    return graph.appendNode(TypeNode{.data = std::move(value)}, {});
}

} // namespace

TypeId TypeGraph::makePointer(TypeId pointee) {
    if (std::holds_alternative<ReferenceType>(node(pointee).data)) {
        throw std::invalid_argument("cannot form a pointer to a reference");
    }
    return intern(*this, PointerType{pointee});
}

TypeId TypeGraph::makeReference(TypeId referred, bool rvalue) {
    if (const auto* ref = std::get_if<ReferenceType>(&node(referred).data)) {
        rvalue = rvalue && ref->rvalue;
        referred = ref->referred;
    }
    return intern(*this, ReferenceType{referred, rvalue});
}

TypeId TypeGraph::makeQualified(TypeId base, bool isConst, bool isVolatile) {
    const auto& data = node(base).data;
    if ((!isConst && !isVolatile) || std::holds_alternative<ReferenceType>(data)) return base;
    if (const auto* qualified = std::get_if<QualifiedType>(&data)) {
        isConst |= qualified->isConst;
        isVolatile |= qualified->isVolatile;
        base = qualified->base;
    }
    return intern(*this, QualifiedType{base, isConst, isVolatile});
}

TypeId TypeGraph::makeFunction(TypeId result, std::vector<TypeId> parameters, bool isNoexcept) {
    node(result);
    for (auto type : parameters) node(type);
    return intern(*this, FunctionType{result, std::move(parameters), isNoexcept});
}

TypeId TypeGraph::makeTemplateInstance(TypeId declaration, std::vector<TemplateArgument> arguments) {
    const auto& decl = node(declaration);
    const auto* external = std::get_if<ExternalTypeNode>(&decl.data);
    const auto container = external ? external->container : ContainerKind::None;
    if ((!decl.templateParameters.empty() && arguments.size() != decl.templateParameters.size()) ||
        ((container == ContainerKind::Array || container == ContainerKind::Set) && arguments.size() != 1) ||
        (container == ContainerKind::Map && arguments.size() != 2))
        throw std::invalid_argument("wrong number of template arguments for " + decl.qualifiedName);
    for (const auto& arg : arguments) {
        if (const auto* type = std::get_if<TypeId>(&arg)) node(*type);
        else if (!decl.templateParameters.empty() || container != ContainerKind::None)
            throw std::invalid_argument("expected a type argument for " + decl.qualifiedName);
    }
    const auto id = intern(*this, TemplateInstanceType{declaration, std::move(arguments)});
    auto& instance = getNode(id.value);
    if (container != ContainerKind::None && !instance.typescriptName) {
        instance.typescriptContainer = container;
        instance.typescriptName = typeScriptContainerName(*this, instance);
    }
    return id;
}

} // namespace cc::codegen
