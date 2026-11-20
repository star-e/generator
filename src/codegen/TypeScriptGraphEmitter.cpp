#include "codegen/GraphEmitter.h"
#include "model/TypeGraphBuilder.h"

#include <algorithm>

namespace cc::codegen {
namespace {

struct Column {
    std::string tagName;
    std::string type;

    std::string visitorMethod() const {
        auto method = tagName;
        if (method.front() >= 'A' && method.front() <= 'Z') method.front() += 'a' - 'A';
        return method;
    }
};

std::vector<Column> bindColumns(const TypeGraph& graph, const TypeNode& owner,
                                const std::vector<ComponentInfo>& columns, TypeImports& imports) {
    std::vector<Column> result;
    for (const auto& column : columns) {
        auto id = column.valueType;
        while (const auto* alias = std::get_if<AliasType>(&graph.node(id).data)) id = alias->target;
        const auto& value = graph.node(id);
        const auto context = owner.qualifiedName + "." + column.name;
        if (value.module != owner.module && value.typescriptModule.empty() && !value.typescriptNumber &&
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
    if (model.hierarchy) out << "            parent,\n            children: [],\n";
    out << "        });\n";
    if (model.parentGraph)
        out << "        const siblings = parent === this.N ? this.roots : this.vertices[parent].children;\n"
            << "        siblings.push(new cc$graph.OutE(v));\n";
    else if (model.hierarchy)
        out << "        if (parent !== this.N) this.vertices[parent].children.push(new cc$graph.OutE(v));\n";
    out << "        return v;\n    }\n";
}

} // namespace

void emitTypeScriptGraph(const TypeGraph& graph, TypeId id, std::ostream& out, TypeImports& imports,
                         const std::vector<BoundField>& fields) {
    const auto& node = graph.node(id);
    const auto& model = std::get<GraphType>(node.data);
    const auto components = bindColumns(graph, node, model.components, imports);
    const auto alternatives = bindColumns(graph, node, model.alternatives, imports);
    std::set<std::string> api{"constructor", "vertices", "N"};
    if (model.vertexList) api.insert({"v", "nv"});
    if (model.mutableGraph) api.insert({"addVertex", "clear"});
    if (model.nameComponentIndex) api.insert("vertexName");
    if (!alternatives.empty()) api.insert({"h", "w", "object", "value", "visitVertex", "j"});
    if (model.hierarchy) api.insert({"getParent", "reference", "parent", "child", "children", "numChildren", "edge", "source", "target", "oe", "od"});
    if (model.parentGraph) api.insert({"roots", "locateChild"});
    if (model.addressableGraph) api.insert({"locate", "locateRelative", "path"});
    for (const auto& component : components) api.insert("get" + component.tagName);
    for (const auto& field : fields)
        if (api.contains(field.name)) throw std::runtime_error(node.qualifiedName + ": graph field collides with TypeScript member " + field.name);
    if (model.hierarchy && std::ranges::find(components, "Parent", &Column::tagName) != components.end())
        throw std::runtime_error(node.qualifiedName + ": component accessor collides with TypeScript getParent");
    const std::string kinds = node.name + "Value";
    if (!alternatives.empty()) {
        for (const auto suffix : {"Value", "ValueType", "Object", "Visitor"}) {
            if (graph.findType(node.qualifiedName + suffix) != TypeId{})
                throw std::runtime_error(node.qualifiedName + ": TypeScript graph export collides with " + node.name + suffix);
        }
        out << "\nexport const enum " << kinds << " {\n";
        for (const auto& alt : alternatives) out << "    " << alt.tagName << ",\n";
        out << "}\n\nexport interface " << kinds << "Type {\n";
        for (const auto& alt : alternatives)
            out << "    [" << kinds << "." << alt.tagName << "]: " << alt.type << ";\n";
        out << "}\n\nexport interface " << node.name << "Visitor {\n";
        for (const auto& alt : alternatives)
            out << "    " << alt.visitorMethod() << "(value: " << alt.type << "): unknown;\n";
        out << "}\n\nexport type " << node.name << "Object = " << kinds << "Type[" << kinds << "];\n";
    }

    // Array indices stop at 2^32 - 2. Reserve the null descriptor for narrower index types too.
    const auto limit = std::min<uint64_t>(indexedVertexLimit(graph.node(model.vertexType).qualifiedName).value(), UINT32_MAX);
    if (model.serializable) out << "\nconst cc$adopt" << node.name << " = Symbol();\n";
    const auto storage = model.serializable ? "private " : "private readonly ";
    out << "\nexport class " << node.name << " {\n    readonly N = " << limit << ";\n"
        << "    " << storage << "vertices: {\n";
    if (!alternatives.empty())
        out << "        readonly id: " << kinds << ";\n        readonly object: " << node.name << "Object;\n";
    for (const auto& component : components)
        out << "        readonly component" << component.tagName << ": " << component.type << ";\n";
    if (model.hierarchy) out << "        readonly parent: number;\n        readonly children: cc$graph.OutE[];\n";
    out << "    }[] = [];\n";
    if (model.parentGraph) out << "    " << storage << "roots: cc$graph.OutE[] = [];\n";
    for (const auto& f : fields)
        out << "    " << (f.readonly ? "readonly " : "") << f.name << ": " << f.ts << " = " << f.tsDefault << ";\n";
    if (model.serializable) {
        out << "\n    [cc$adopt" << node.name << "] (decoded: " << node.name << "): void {\n"
            << "        this.vertices = decoded.vertices;\n";
        if (model.parentGraph) out << "        this.roots = decoded.roots;\n";
        for (const auto& f : model.fields) {
            if (f.transient) continue;
            const auto target = "this." + f.name;
            const auto source = "decoded." + f.name;
            const auto kind = graph.node(f.type.type).typescriptContainer;
            if (!f.isReadonly) out << "        " << target << " = " << source << ";\n";
            else if (kind == ContainerKind::Array)
                out << "        " << target << ".length = 0;\n        for (const item of " << source << ") " << target << ".push(item);\n";
            else if (kind == ContainerKind::Map)
                out << "        " << target << ".clear();\n        for (const [key, value] of " << source << ") " << target << ".set(key, value);\n";
            else if (kind == ContainerKind::Set)
                out << "        " << target << ".clear();\n        for (const item of " << source << ") " << target << ".add(item);\n";
            else out << "        Object.assign(" << target << ", " << source << ");\n";
        }
        out << "    }\n";
    }
    if (model.vertexList)
        out << "\n    v (): IterableIterator<number> { return this.vertices.keys(); }\n"
            << "    nv (): number { return this.vertices.length; }\n";
    if (model.mutableGraph) {
        out << "\n    clear (): void {\n        this.vertices.length = 0;\n";
        if (model.parentGraph) out << "        this.roots.length = 0;\n";
        for (const auto& f : fields) {
            if (!f.tsReset.empty()) out << "        " << f.tsReset << ";\n";
            else if (!f.readonly) out << "        this." << f.name << " = " << f.tsDefault << ";\n";
            else if (f.tsDefault.starts_with("new "))
                out << "        Object.assign(this." << f.name << ", " << f.tsDefault << ");\n";
        }
        out << "    }\n";
        addVertex(out, model, kinds, components);
    }
    for (const auto& component : components)
        out << "\n    get" << component.tagName << " (v: number): " << component.type << " {\n"
            << "        return this.vertices[v].component" << component.tagName << ";\n    }\n";
    if (model.nameComponentIndex)
        out << "\n    vertexName (v: number): string { return this.get" << components.at(*model.nameComponentIndex).tagName << "(v); }\n";
    if (!alternatives.empty()) {
        out << "\n    h (id: " << kinds << ", v: number): boolean { return this.w(v) === id; }\n"
            << "    w (v: number): " << kinds << " { return this.vertices[v].id; }\n"
            << "    object (v: number): " << node.name << "Object { return this.vertices[v].object; }\n"
            << "    value<T extends " << kinds << "> (id: T, v: number): " << kinds << "Type[T] {\n"
            << "        const vertex = this.vertices[v];\n"
            << "        if (vertex.id !== id) throw new Error('Vertex kind does not match');\n"
            << "        return vertex.object as " << kinds << "Type[T];\n    }\n"
            << "    visitVertex (visitor: " << node.name << "Visitor, v: number): unknown {\n"
            << "        const vertex = this.vertices[v];\n        switch (vertex.id) {\n";
        for (const auto& alt : alternatives)
            out << "        case " << kinds << "." << alt.tagName << ": return visitor." << alt.visitorMethod()
                << "(vertex.object as " << alt.type << ");\n";
        out << "        default: throw new Error('Unknown vertex kind');\n        }\n    }\n"
            << "    j<T extends " << node.name << "Object> (v: number): T { return this.object(v) as T; }\n";
    }
    if (model.hierarchy)
        out << "\n    getParent (v: number): number { return v === this.N ? this.N : this.vertices[v].parent; }\n"
            << "    reference (u: number, v: number): boolean { return u !== this.N && !!this.vertices[v] && this.getParent(v) === u; }\n"
            << "    parent (e: cc$graph.ED): number { return e.source as number; }\n"
            << "    children (v: number): cc$graph.OutEI { return new cc$graph.OutEI(this.vertices[v].children.values(), v); }\n"
            << "    child (e: cc$graph.ED): number { return e.target as number; }\n"
            << "    numChildren (v: number): number { return this.vertices[v].children.length; }\n"
            << "\n    edge (u: number, v: number): boolean { return this.reference(u, v); }\n"
            << "    source (e: cc$graph.ED): number { return this.parent(e); }\n"
            << "    target (e: cc$graph.ED): number { return this.child(e); }\n"
            << "    oe (v: number): cc$graph.OutEI { return this.children(v); }\n"
            << "    od (v: number): number { return this.numChildren(v); }\n";
    if (model.parentGraph) {
        out << "\n    locateChild (parent: number, name: string): number {\n"
            << "        if (parent !== this.N && !this.vertices[parent]) return this.N;\n"
            << "        const children = parent === this.N ? this.roots : this.vertices[parent].children;\n"
            << "        for (const edge of children) {\n"
            << "            const v = edge.target as number;\n"
            << "            if (this.vertexName(v) === name) return v;\n"
            << "        }\n        return this.N;\n    }\n";
    }
    if (model.addressableGraph)
        out << R"(
    locate (absPath: string): number { return this.locateRelative(absPath); }

    locateRelative (path: string, start: number = this.N): number {
        if (path.length === 0) return this.N;
        if (path[0] !== '/' && start !== this.N && !this.vertices[start]) return this.N;
        return cc$graph.findRelative(this, start, path) as number;
    }

    path (v: number): string {
        if (v !== this.N && !this.vertices[v]) throw new RangeError('Invalid vertex');
        return cc$graph.getPath(this, v);
    }
)";
    out << "}\n";
}

} // namespace cc::codegen
