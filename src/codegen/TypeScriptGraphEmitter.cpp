#include "codegen/GraphEmitter.h"
#include "model/TypeGraphBuilder.h"

#include <algorithm>

namespace cc::codegen {
namespace {

struct Column {
    std::string tagName;
    std::string type;
};

std::vector<Column> bindColumns(const TypeGraph& graph, const TypeNode& owner,
                                const std::vector<ComponentInfo>& columns, TypeImports& imports) {
    std::vector<Column> result;
    for (const auto& column : columns) {
        auto id = column.valueType;
        while (const auto* alias = std::get_if<AliasType>(&graph.node(id).data)) id = alias->target;
        const auto& value = graph.node(id);
        const auto context = owner.qualifiedName + "." + column.name;
        if (value.module != owner.module && value.typescriptModule.empty() &&
            (std::holds_alternative<RecordType>(value.data) || std::holds_alternative<GraphType>(value.data)))
            throw std::runtime_error(context + ": missing TypeScript import mapping for " + value.qualifiedName);
        result.push_back({componentAccessorSuffix(graph.node(column.tag).name),
                          boundTypeScriptName(graph, id, owner.module, imports, context)});
    }
    return result;
}

void addVertex(std::ostream& out, const GraphType& model, const std::string& kinds,
               const std::vector<Column>& components) {
    const bool polymorphic = !model.alternatives.empty();
    out << "\n    addVertex";
    if (polymorphic) out << "<T extends " << kinds << ">";
    out << " (\n";
    if (polymorphic) out << "        id: T,\n        object: " << kinds << "Type[T],\n";
    for (const auto& component : components)
        out << "        component" << component.tagName << ": " << component.type << ",\n";
    if (model.hierarchy) out << "        parent: number = this.N,\n";
    out << "    ): number {\n"
        << "        const v = this.vertices.length;\n"
        << "        if (v >= this.N) throw new RangeError('Vertex limit exceeded');\n";
    if (model.hierarchy)
        out << "        if (parent !== this.N && (!Number.isInteger(parent) || parent < 0 || parent >= v)) {\n"
            << "            throw new RangeError('Invalid parent vertex');\n        }\n";
    out << "        this.vertices.push({\n";
    if (polymorphic) out << "            id, object,\n";
    for (const auto& component : components) out << "            component" << component.tagName << ",\n";
    if (model.hierarchy) out << "            parent,\n";
    if (model.parentGraph) out << "            children: [],\n";
    out << "        });\n";
    if (model.parentGraph)
        out << "        const siblings = parent === this.N ? this.roots : this.vertices[parent].children;\n"
            << "        siblings.push(v);\n";
    out << "        return v;\n    }\n";
}

} // namespace

void emitTypeScriptGraph(const TypeGraph& graph, TypeId id, std::ostream& out, TypeImports& imports) {
    const auto& node = graph.node(id);
    const auto& model = std::get<GraphType>(node.data);
    const auto components = bindColumns(graph, node, model.components, imports);
    const auto alternatives = bindColumns(graph, node, model.alternatives, imports);
    if (model.hierarchy && std::ranges::find(components, "Parent", &Column::tagName) != components.end())
        throw std::runtime_error(node.qualifiedName + ": component accessor collides with TypeScript getParent");
    const std::string kinds = node.name + "Value";
    if (!alternatives.empty()) {
        for (const auto suffix : {"Value", "ValueType"}) {
            if (graph.findType(node.qualifiedName + suffix) != TypeId{})
                throw std::runtime_error(node.qualifiedName + ": TypeScript graph export collides with " + node.name + suffix);
        }
        out << "\nexport const enum " << kinds << " {\n";
        for (const auto& alt : alternatives) out << "    " << alt.tagName << ",\n";
        out << "}\n\nexport interface " << kinds << "Type {\n";
        for (const auto& alt : alternatives)
            out << "    [" << kinds << "." << alt.tagName << "]: " << alt.type << ";\n";
        out << "}\n";
    }

    // Array indices stop at 2^32 - 2. Reserve the null descriptor for narrower index types too.
    const auto limit = std::min<uint64_t>(indexedVertexLimit(graph.node(model.vertexType).qualifiedName).value(), UINT32_MAX);
    out << "\nexport class " << node.name << " {\n    readonly N = " << limit << ";\n"
        << "    private readonly vertices: {\n";
    if (!alternatives.empty())
        out << "        readonly id: " << kinds << ";\n        readonly object: " << kinds << "Type[" << kinds << "];\n";
    for (const auto& component : components)
        out << "        readonly component" << component.tagName << ": " << component.type << ";\n";
    if (model.hierarchy) out << "        readonly parent: number;\n";
    if (model.parentGraph) out << "        readonly children: number[];\n";
    out << "    }[] = [];\n";
    if (model.parentGraph) out << "    private readonly roots: number[] = [];\n";
    if (model.vertexList)
        out << "\n    v (): IterableIterator<number> { return this.vertices.keys(); }\n"
            << "    nv (): number { return this.vertices.length; }\n";
    if (model.mutableGraph) addVertex(out, model, kinds, components);
    for (const auto& component : components)
        out << "\n    get" << component.tagName << " (v: number): " << component.type << " {\n"
            << "        return this.vertices[v].component" << component.tagName << ";\n    }\n";
    if (!alternatives.empty())
        out << "\n    w (v: number): " << kinds << " { return this.vertices[v].id; }\n"
            << "    value<T extends " << kinds << "> (id: T, v: number): " << kinds << "Type[T] {\n"
            << "        const vertex = this.vertices[v];\n"
            << "        if (vertex.id !== id) throw new Error('Vertex kind does not match');\n"
            << "        return vertex.object as " << kinds << "Type[T];\n    }\n";
    if (model.hierarchy)
        out << "\n    getParent (v: number): number { return v === this.N ? this.N : this.vertices[v].parent; }\n";
    if (model.parentGraph) {
        const auto& name = components.at(model.nameComponentIndex.value());
        out << "\n    locateChild (parent: number, name: string): number {\n"
            << "        if (parent !== this.N && !this.vertices[parent]) return this.N;\n"
            << "        const children = parent === this.N ? this.roots : this.vertices[parent].children;\n"
            << "        for (const v of children) {\n"
            << "            if (this.get" << name.tagName << "(v) === name) return v;\n"
            << "        }\n        return this.N;\n    }\n";
    }
    out << "}\n";
}

} // namespace cc::codegen
