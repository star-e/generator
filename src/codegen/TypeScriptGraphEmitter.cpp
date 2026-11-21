#include "codegen/GraphEmitter.h"
#include "model/TypeGraphBuilder.h"

#include <algorithm>

namespace cc::codegen {
namespace {

struct Column {
    std::string tagName;
    std::string type;
    bool scalar;

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
        if (const auto* wrapper = std::get_if<TemplateInstanceType>(&graph.node(id).data);
            wrapper && graph.node(wrapper->declaration).qualifiedName == "cc::IntrusivePtr" &&
            wrapper->arguments.size() == 1 && std::holds_alternative<TypeId>(wrapper->arguments.front()))
            id = std::get<TypeId>(wrapper->arguments.front());
        const auto& value = graph.node(id);
        const auto context = owner.qualifiedName + "." + column.name;
        if (value.module != owner.module && value.typescriptModule.empty() && !value.typescriptNumber &&
            (std::holds_alternative<RecordType>(value.data) || std::holds_alternative<GraphType>(value.data)))
            throw std::runtime_error(context + ": missing TypeScript import mapping for " + value.qualifiedName);
        const auto& type = boundTypeScriptName(graph, id, owner.module, imports, context);
        result.push_back({column.publicName, type,
                          type == "number" || type == "boolean" || type == "string" || std::holds_alternative<EnumType>(value.data)});
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
    const bool rollback = model.nameIndex || model.separateEdges;
    const auto indent = rollback ? "            " : "        ";
    if (model.nameIndex) {
        const auto key = "component" + components.at(*model.nameComponentIndex).tagName;
        out << "        if (this.nameIndex.has(" << key << ")) throw new Error('Duplicate vertex name');\n"
            << "        this.nameIndex.set(" << key << ", v);\n";
    }
    if (rollback) out << "        try {\n";
    out << indent << "this.vertices.push({\n";
    if (polymorphic) out << indent << "    id, object,\n";
    for (const auto& component : components) out << indent << "    component" << component.tagName << ",\n";
    if (model.separateEdges) out << indent << "    parents: [], children: [],\n";
    else if (model.hierarchy) out << indent << "    parent,\n" << indent << "    children: [],\n";
    if (model.incidence) out << indent << "    outgoing: [],\n";
    if (model.bidirectional) out << indent << "    incoming: [],\n";
    out << indent << "});\n";
    if (model.parentGraph)
        out << indent << "const siblings = parent === this.N ? this.roots : this.vertices[parent].children;\n"
            << indent << "siblings.push(new OutE(v));\n";
    else if (model.separateEdges)
        out << indent << "if (parent !== this.N) this.addReference(parent, v);\n";
    else if (model.hierarchy)
        out << indent << "if (parent !== this.N) this.vertices[parent].children.push(new OutE(v));\n";
    if (rollback) {
        out << "        } catch (error) {\n            this.vertices.length = v;\n";
        if (model.nameIndex) out << "            this.nameIndex.delete(component" << components.at(*model.nameComponentIndex).tagName << ");\n";
        out << "            throw error;\n        }\n";
    }
    out << "        return v;\n    }\n";
}

void directedEdges(std::ostream& out, const GraphType& model) {
    out << R"(
    edge (u: number, v: number): boolean {
        return Number.isInteger(u) && u >= 0 && u < this.vertices.length && this.vertices[u].outgoing.some(e => e.target === v);
    }
    source (e: ED): number { return e.source as number; }
    target (e: ED): number { return e.target as number; }
    oe (v: number): OutEI { return new OutEI(this.vertices[v].outgoing.values(), v); }
    od (v: number): number { return this.vertices[v].outgoing.length; }
    adj (v: number): AdjI { return new AdjI(this, this.oe(v)); }
    ne (): number { return this.vertices.reduce((sum, row) => sum + row.outgoing.length, 0); }
)";
    if (model.bidirectional)
        out << "    ie (v: number): InEI { return new InEI(this.vertices[v].incoming.values(), v); }\n"
            << "    id (v: number): number { return this.vertices[v].incoming.length; }\n"
            << "    d (v: number): number { return this.od(v) + this.id(v); }\n";
}

void adjacencyMutation(std::ostream& out, bool references, bool bidirectional) {
    const auto operation = references ? "Reference" : "Edge";
    const auto outgoing = references ? "children" : "outgoing";
    const auto incoming = references ? "parents" : "incoming";
    out << "    add" << operation << R"( (u: number, v: number): ED | null {
        if (!Number.isInteger(u) || !Number.isInteger(v) || u < 0 || v < 0 ||
            u >= this.vertices.length || v >= this.vertices.length) return null;
        const edge = new ED(u, v);
        const out = this.vertices[u].)" << outgoing << R"(;
        out.push(new OutE(v));
)";
    if (bidirectional) out << R"(        try {
            this.vertices[v].)" << incoming << R"(.push(new OutE(u));
        } catch (error) {
            out.pop();
            throw error;
        }
)";
    out << R"(        return edge;
    }
    remove)" << operation << R"( (e: ED): void {
        const u = e.source as number, v = e.target as number;
        if (!Number.isInteger(u) || u < 0 || u >= this.vertices.length) return;
        const out = this.vertices[u].)" << outgoing << R"(;
        const index = out.findIndex(other => other.target === v);
        if (index < 0) return;
        out.splice(index, 1);
)";
    if (bidirectional) out << "        const incoming = this.vertices[v]." << incoming << ";\n"
                                << "        incoming.splice(incoming.findIndex(other => other.target === u), 1);\n";
    out << R"(    }
    remove)" << operation << R"(s (u: number, v: number): void {
        if (!Number.isInteger(u) || !Number.isInteger(v) || u < 0 || v < 0 ||
            u >= this.vertices.length || v >= this.vertices.length) return;
        const compact = (list: OutE[], neighbor: number): void => {
            let count = 0;
            for (const edge of list) if (edge.target !== neighbor) list[count++] = edge;
            list.length = count;
        };
        compact(this.vertices[u].)" << outgoing << R"(, v);
)";
    if (bidirectional) out << "        compact(this.vertices[v]." << incoming << ", u);\n";
    out << "    }\n";
}

} // namespace

void emitTypeScriptGraph(const TypeGraph& graph, TypeId id, std::ostream& out, TypeImports& valueImports, TypeImports& typeImports,
                         const std::vector<BoundField>& fields, std::string_view classMethods) {
    const auto& node = graph.node(id);
    const auto& model = std::get<GraphType>(node.data);
    if (model.keyComponentIndex || model.removeVertex)
        throw std::runtime_error(node.qualifiedName + ": component_index and remove_vertex currently require C++ graph output");
    const auto components = bindColumns(graph, node, model.components, typeImports);
    const auto alternatives = bindColumns(graph, node, model.alternatives, typeImports);
    const auto hasSetter = [&](size_t i) { return components[i].scalar && (i != model.nameComponentIndex || ((model.nameIndex || model.incidence) && model.mutableGraph)); };
    std::set<std::string> api{"constructor", "vertices", "N"};
    if (model.vertexList) api.insert({"v", "nv"});
    if (model.mutableGraph) api.insert({"addVertex", "clear"});
    const bool sharedEdges = model.hierarchy && !model.separateEdges;
    if (model.hierarchy || model.incidence) {
        constexpr auto runtime = "cocos/rendering/custom/graph";
        auto& values = valueImports[runtime];
        values.insert("OutEI");
        (model.mutableGraph ? values : typeImports[runtime]).insert("ED");
        (model.mutableGraph || sharedEdges ? values : typeImports[runtime]).insert("OutE");
        if (model.incidence || sharedEdges) values.insert("AdjI");
        if (model.hierarchy || model.bidirectional) values.insert("InEI");
        if (model.addressableGraph) values.insert({"findRelative", "getPath"});
    }
    if (model.mutableGraph && model.hierarchy) api.insert("addReference");
    if (model.mutableGraph && sharedEdges) api.insert("addEdge");
    if (model.separateEdges) {
        api.insert({"parents", "numParents"});
        if (model.mutableGraph) api.insert({"removeReference", "removeReferences"});
    }
    if (model.incidence || sharedEdges) api.insert({"edge", "source", "target", "oe", "od", "adj", "ne"});
    if (model.bidirectional || sharedEdges) api.insert({"ie", "id", "d"});
    if (model.incidence && model.mutableGraph) api.insert({"addEdge", "removeEdge", "removeEdges"});
    if (model.nameComponentIndex) api.insert("vertexName");
    if (model.nameIndex) api.insert({"nameIndex", "contains", "vertex", "find"});
    if (!alternatives.empty()) api.insert({"h", "w", "object", "value", "visitVertex", "j"});
    if (model.hierarchy) api.insert({"getParent", "reference", "parent", "child", "children", "numChildren"});
    if (model.parentGraph) api.insert({"roots", "locateChild"});
    if (model.addressableGraph) api.insert({"locate", "locateRelative", "path"});
    for (size_t i = 0; i < components.size(); ++i) {
        api.insert("get" + components[i].tagName);
        if (hasSetter(i)) api.insert("set" + components[i].tagName);
    }
    for (const auto& field : fields)
        if (api.contains(field.name)) throw std::runtime_error(node.qualifiedName + ": graph field collides with TypeScript member " + field.name);
    if (model.hierarchy && std::ranges::find(components, "Parent", &Column::tagName) != components.end())
        throw std::runtime_error(node.qualifiedName + ": component accessor collides with TypeScript getParent");
    const std::string kinds = node.name + "Value";
    if (!alternatives.empty()) {
        const auto scope = node.qualifiedName.substr(0, node.qualifiedName.size() - node.name.size());
        for (const auto& symbol : {kinds, kinds + "Type", node.name + "Object", node.name + "Visitor", "get" + kinds + "Name"}) {
            if (graph.findType(scope + symbol) != TypeId{})
                throw std::runtime_error(node.qualifiedName + ": TypeScript graph export collides with " + symbol);
        }
        out << "\nexport const enum " << kinds << " {\n";
        for (const auto& alt : alternatives) out << "    " << alt.tagName << ",\n";
        out << "}\n\nexport function get" << kinds << "Name (kind: " << kinds << "): string {\n    switch (kind) {\n";
        for (const auto& alt : alternatives)
            out << "    case " << kinds << "." << alt.tagName << ": return '" << alt.tagName << "';\n";
        out << "    default: return '';\n    }\n}\n\nexport interface " << kinds << "Type {\n";
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
    for (size_t i = 0; i < components.size(); ++i)
        out << "        " << (hasSetter(i) ? "" : "readonly ") << "component" << components[i].tagName << ": " << components[i].type << ";\n";
    if (model.separateEdges) out << "        readonly parents: OutE[];\n        readonly children: OutE[];\n";
    else if (model.hierarchy) out << "        parent: number;\n        readonly children: OutE[];\n";
    if (model.incidence) out << "        readonly outgoing: OutE[];\n";
    if (model.bidirectional) out << "        readonly incoming: OutE[];\n";
    out << "    }[] = [];\n";
    if (model.nameIndex) out << "    " << storage << "nameIndex = new Map<string, number>();\n";
    if (model.parentGraph) out << "    " << storage << "roots: OutE[] = [];\n";
    std::string initializers;
    for (const auto& f : fields) {
        out << "    " << (f.safeUnsignedInteger ? "declare " : "") << (f.readonly ? "readonly " : "") << f.name << ": " << f.ts;
        if (f.safeUnsignedInteger) initializers += "        " + f.tsUint64Initializer(f.tsDefault) + ";\n";
        else out << " = " << f.tsDefault;
        out << ";\n";
    }
    if (!initializers.empty()) out << "\n    constructor () {\n" << initializers << "    }\n";
    if (model.serializable) {
        out << "\n    [cc$adopt" << node.name << "] (decoded: " << node.name << "): void {\n"
            << "        this.vertices = decoded.vertices;\n";
        if (model.nameIndex) out << "        this.nameIndex = decoded.nameIndex;\n";
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
        if (model.nameIndex) out << "        this.nameIndex.clear();\n";
        if (model.parentGraph) out << "        this.roots.length = 0;\n";
        for (const auto& f : fields) {
            if (!f.tsReset.empty()) out << "        " << f.tsReset << ";\n";
            else if (!f.readonly) out << "        this." << f.name << " = " << f.tsDefault << ";\n";
            else if (f.tsDefault.starts_with("new "))
                out << "        Object.assign(this." << f.name << ", " << f.tsDefault << ");\n";
        }
        out << "    }\n";
        addVertex(out, model, kinds, components);
        if (sharedEdges) {
            out << "\n    addEdge (u: number, v: number): ED | null {\n"
                << "        if (!Number.isInteger(u) || !Number.isInteger(v) || !this.vertices[u] || !this.vertices[v] ||\n"
                << "            this.vertices[v].parent !== this.N" << (model.serializable ? " || u >= v" : "") << ") return null;\n";
            if (!model.serializable)
                out << "        for (let p = u; p !== this.N; p = this.vertices[p].parent) if (p === v) return null;\n";
            out << "        const children = this.vertices[u].children;\n"
                << "        const index = children.findIndex(e => (e.target as number) > v);\n"
                << "        children.splice(index < 0 ? children.length : index, 0, new OutE(v));\n";
            if (model.parentGraph) out << "        this.roots.splice(this.roots.findIndex(e => e.target === v), 1);\n";
            out << "        this.vertices[v].parent = u;\n        return new ED(u, v);\n    }\n"
                << "    addReference (u: number, v: number): ED | null { return this.addEdge(u, v); }\n";
        }
    }
    for (size_t i = 0; i < components.size(); ++i) {
        const auto& component = components[i];
        out << "\n    get" << component.tagName << " (v: number): " << component.type << " {\n"
            << "        return this.vertices[v].component" << component.tagName << ";\n    }\n";
        if (hasSetter(i)) {
            out << "    set" << component.tagName << " (v: number, value: " << component.type << "): void {\n";
            if (model.nameIndex && i == model.nameComponentIndex) {
                out << "        if (!Number.isInteger(v) || v < 0 || v >= this.vertices.length) throw new RangeError('Invalid vertex');\n"
                    << "        const row = this.vertices[v];\n"
                    << "        if (row.component" << component.tagName << " === value) return;\n"
                    << "        if (this.nameIndex.has(value)) throw new Error('Duplicate vertex name');\n"
                    << "        this.nameIndex.set(value, v);\n        this.nameIndex.delete(row.component" << component.tagName << ");\n"
                    << "        row.component" << component.tagName << " = value;\n";
            } else {
                out << "        this.vertices[v].component" << component.tagName << " = value;\n";
            }
            out << "    }\n";
        }
    }
    if (model.incidence) {
        directedEdges(out, model);
        if (model.mutableGraph) adjacencyMutation(out, false, model.bidirectional);
    }
    if (model.nameIndex)
        out << "\n    contains (key: string): boolean { return this.nameIndex.has(key); }\n"
            << "    find (key: string): number {\n        const v = this.nameIndex.get(key);\n"
            << "        return v === undefined ? this.N : v;\n    }\n"
            << "    vertex (key: string): number {\n        const v = this.find(key);\n"
            << "        if (v === this.N) throw new RangeError('Unknown vertex name');\n        return v;\n    }\n";
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
    if (model.hierarchy) {
        out << "\n    parent (e: ED): number { return e.source as number; }\n"
            << "    children (v: number): OutEI { return new OutEI(this.vertices[v].children.values(), v); }\n"
            << "    child (e: ED): number { return e.target as number; }\n"
            << "    numChildren (v: number): number { return this.vertices[v].children.length; }\n";
    }
    if (model.separateEdges) {
        out << "    parents (v: number): InEI { return new InEI(this.vertices[v].parents.values(), v); }\n"
            << "    numParents (v: number): number { return this.vertices[v].parents.length; }\n"
            << "    getParent (v: number): number {\n        const row = this.vertices[v];\n"
            << "        return row && row.parents.length ? row.parents[0].target as number : this.N;\n    }\n"
            << "    reference (u: number, v: number): boolean { return Number.isInteger(u) && !!this.vertices[u] && this.vertices[u].children.some(e => e.target === v); }\n";
        if (model.mutableGraph) adjacencyMutation(out, true, true);
    } else if (model.hierarchy)
        out << "\n    getParent (v: number): number { return v === this.N ? this.N : this.vertices[v].parent; }\n"
            << "    reference (u: number, v: number): boolean { return u !== this.N && !!this.vertices[v] && this.getParent(v) === u; }\n"
            << "\n    edge (u: number, v: number): boolean { return this.reference(u, v); }\n"
            << "    source (e: ED): number { return this.parent(e); }\n"
            << "    target (e: ED): number { return this.child(e); }\n"
            << "    oe (v: number): OutEI { return this.children(v); }\n"
            << "    od (v: number): number { return this.numChildren(v); }\n"
            << "    ie (v: number): InEI {\n"
            << "        const parent = this.getParent(v);\n"
            << "        return new InEI((parent === this.N ? [] : [new OutE(parent)]).values(), v);\n    }\n"
            << "    id (v: number): number { return this.getParent(v) === this.N ? 0 : 1; }\n"
            << "    d (v: number): number { return this.id(v) + this.od(v); }\n"
            << "    adj (v: number): AdjI { return new AdjI(this, this.oe(v)); }\n"
            << (model.parentGraph ? "    ne (): number { return this.vertices.length - this.roots.length; }\n" :
                                    "    ne (): number { return this.vertices.reduce((n, vertex) => n + vertex.children.length, 0); }\n");
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
        return findRelative(this, start, path) as number;
    }

    path (v: number): string {
        if (v !== this.N && !this.vertices[v]) throw new RangeError('Invalid vertex');
        return getPath(this, v);
    }
)";
    out << classMethods << "}\n";
}

} // namespace cc::codegen
