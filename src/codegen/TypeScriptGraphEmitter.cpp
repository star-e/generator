#include "codegen/Documentation.h"
#include "codegen/GraphEmitter.h"
#include "model/TypeGraphBuilder.h"

#include <algorithm>

namespace cc::codegen {
namespace {

struct Column {
    std::string tagName;
    std::string type;
    std::string storageName;
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
        result.push_back({column.publicName, type, "_" + column.name,
                          type == "number" || type == "boolean" || type == "string" || std::holds_alternative<EnumType>(value.data)});
    }
    return result;
}

struct VertexColumn {
    std::string name;
    std::string arrayType;
    std::string initialValue;
};

std::vector<VertexColumn> vertexMetadataColumns(const GraphType& model, const std::string& kinds) {
    std::vector<VertexColumn> columns;
    if (model.incidence) columns.push_back({"_outEdges", "OutE[][]", "[]"});
    if (model.bidirectional) columns.push_back({"_inEdges", "OutE[][]", "[]"});
    if (model.hierarchy) {
        columns.push_back({"_children", "OutE[][]", "[]"});
        if (model.separateEdges) columns.push_back({"_parents", "OutE[][]", "[]"});
        else columns.push_back({"_parents", "number[]", "parent"});
    }
    if (!model.alternatives.empty()) {
        columns.push_back({"_kinds", kinds + "[]", "id"});
        columns.push_back({"_indices", "number[]", "objectIndex"});
    }
    return columns;
}

void addVertex(std::ostream& out, const GraphType& model, const std::string& kinds,
               const std::vector<Column>& components, const std::vector<VertexColumn>& columns,
               const std::string& vertexCount) {
    const bool polymorphic = !model.alternatives.empty();
    out << "\n    addVertex";
    if (polymorphic) out << "<T extends " << kinds << ">";
    out << " (\n";
    if (polymorphic) out << "        id: T,\n        object: " << kinds << "Type[T],\n";
    for (const auto& component : components)
        out << "        component" << component.tagName << ": " << component.type << ",\n";
    if (model.hierarchy) out << "        parent: number = this.N,\n";
    out << "    ): number {\n"
        << "        const v = " << vertexCount << ";\n"
        << "        if (v >= this.N) throw new RangeError('Vertex limit exceeded');\n";
    if (columns.empty()) {
        out << "        ++this._vertexCount;\n        return v;\n    }\n";
        return;
    }
    if (model.nameIndex) {
        const auto key = "component" + components.at(*model.nameComponentIndex).tagName;
        out << "        if (this.nameIndex.has(" << key << ")) throw new Error('Duplicate vertex name');\n";
    }
    if (polymorphic)
        out << "        const objects = this._objects[id];\n        const objectIndex = objects.length;\n";
    if (model.parentGraph)
        out << "        const siblings = parent === this.N ? this.roots : this._children[parent];\n";
    else if (model.hierarchy && !model.separateEdges)
        out << "        const siblings = this._children[parent];\n";
    out << "        try {\n";
    if (polymorphic) out << "            objects.push(object);\n";
    for (const auto& column : columns)
        out << "            this." << column.name << ".push(" << column.initialValue << ");\n";
    if (model.nameIndex)
        out << "            this.nameIndex.set(component" << components.at(*model.nameComponentIndex).tagName << ", v);\n";
    if (model.parentGraph)
        out << "            siblings.push(new OutE(v));\n";
    else if (model.separateEdges)
        out << "            if (parent !== this.N) this.addReference(parent, v);\n";
    else if (model.hierarchy)
        out << "            if (parent !== this.N) siblings.push(new OutE(v));\n";
    out << "        } catch (error) {\n";
    if (model.nameIndex) out << "            this.nameIndex.delete(component" << components.at(*model.nameComponentIndex).tagName << ");\n";
    if (polymorphic) out << "            objects.length = objectIndex;\n";
    for (const auto& column : columns) out << "            this." << column.name << ".length = v;\n";
    out << "            throw error;\n        }\n        return v;\n    }\n";
}

void edgeTraversal(std::ostream& out, bool sharedEdges, bool bidirectional) {
    const auto outgoing = sharedEdges ? "_children" : "_outEdges";
    out << "\n    // IncidenceGraph\n    edge (u: number, v: number): boolean {\n";
    if (sharedEdges)
        out << "        return this._parents[v] === u;\n";
    else
        out << "        return this._outEdges[u].some(e => e.target === v);\n";
    out << R"(    }
    source (e: ED): number {
        return e.source as number;
    }
    target (e: ED): number {
        return e.target as number;
    }
    oe (v: number): OutEI {
        return new OutEI(this.)" << outgoing << R"([v].values(), v);
    }
    od (v: number): number {
        return this.)" << outgoing << R"([v].length;
    }
)";

    if (bidirectional || sharedEdges) {
        out << "\n    // BidirectionalGraph\n";
        if (sharedEdges)
            out << R"(    ie (v: number): OneInEI {
        return new OneInEI(this._parents[v], v, this.N);
    }
    id (v: number): number {
        return this._parents[v] === this.N ? 0 : 1;
    }
    d (v: number): number {
        return this._children[v].length + (this._parents[v] === this.N ? 0 : 1);
    }
)";
        else
            out << R"(    ie (v: number): InEI {
        return new InEI(this._inEdges[v].values(), v);
    }
    id (v: number): number {
        return this._inEdges[v].length;
    }
    d (v: number): number {
        return this._outEdges[v].length + this._inEdges[v].length;
    }
)";
    }
    out << "\n    // AdjacencyGraph\n"
        << R"(    adj (v: number): AdjI {
        return new AdjI(this, new OutEI(this.)" << outgoing << R"([v].values(), v));
    }
)";
}

void referenceQueries(std::ostream& out, bool separateEdges) {
    out << "\n    // ReferenceGraph\n";
    if (separateEdges)
        out << R"(    reference (u: number, v: number): boolean {
        return this._children[u].some(e => e.target === v);
    }
)";
    else
        out << R"(    reference (u: number, v: number): boolean {
        return this._parents[v] === u;
    }
)";
    out << R"(    parent (e: ED): number {
        return e.source as number;
    }
    child (e: ED): number {
        return e.target as number;
    }
)";
    if (separateEdges)
        out << R"(    parents (v: number): InEI {
        return new InEI(this._parents[v].values(), v);
    }
)";
    out << R"(    children (v: number): OutEI {
        return new OutEI(this._children[v].values(), v);
    }
)";
    if (separateEdges)
        out << R"(    numParents (v: number): number {
        return this._parents[v].length;
    }
)";
    out << R"(    numChildren (v: number): number {
        return this._children[v].length;
    }
)";
    if (separateEdges)
        out << R"(    getParent (v: number): number {
        if (v === this.N) return this.N;
        const parents = this._parents[v];
        return parents.length ? parents[0].target as number : this.N;
    }
)";
    else
        out << R"(    getParent (v: number): number {
        return this._parents[v] ?? this.N;
    }
)";
}

void adjacencyMutation(std::ostream& out, bool references, bool bidirectional) {
    const auto operation = references ? "Reference" : "Edge";
    const auto outgoing = references ? "_children" : "_outEdges";
    const auto incoming = references ? "_parents" : "_inEdges";
    out << "    add" << operation << R"( (u: number, v: number): ED | null {
        const edge = new ED(u, v);
        const out = this.)" << outgoing << R"([u];
        out.push(new OutE(v));
)";
    if (bidirectional) out << R"(        try {
            this.)" << incoming << R"([v].push(new OutE(u));
        } catch (error) {
            out.pop();
            throw error;
        }
)";
    out << R"(        return edge;
    }
    remove)" << operation << R"( (e: ED): void {
        const u = e.source as number, v = e.target as number;
        const out = this.)" << outgoing << R"([u];
        const index = out.findIndex(other => other.target === v);
        if (index < 0) return;
        out.splice(index, 1);
)";
    if (bidirectional) out << "        const incoming = this." << incoming << "[v];\n"
                                << "        incoming.splice(incoming.findIndex(other => other.target === u), 1);\n";
    out << R"(    }
    remove)" << operation << R"(s (u: number, v: number): void {
        const compact = (list: OutE[], neighbor: number): void => {
            let count = 0;
            for (const edge of list) if (edge.target !== neighbor) list[count++] = edge;
            list.length = count;
        };
        compact(this.)" << outgoing << R"([u], v);
)";
    if (bidirectional) out << "        compact(this." << incoming << "[v], u);\n";
    out << "    }\n";
}

} // namespace

void emitTypeScriptGraph(const TypeGraph& graph, TypeId id, std::ostream& out, TypeImports& valueImports, TypeImports& typeImports,
                         const std::vector<BoundField>& fields, std::string_view classMethods, std::set<std::string> api) {
    const auto& node = graph.node(id);
    const auto& model = std::get<GraphType>(node.data);
    if (model.keyComponentIndex || model.removeVertex)
        throw std::runtime_error(node.qualifiedName + ": component_index and remove_vertex currently require C++ graph output");
    auto components = bindColumns(graph, node, model.components, typeImports);
    const auto alternatives = bindColumns(graph, node, model.alternatives, typeImports);
    const auto hasSetter = [&](size_t i) { return components[i].scalar && (i != model.nameComponentIndex || ((model.nameIndex || model.incidence) && model.mutableGraph)); };
    const std::string kinds = node.name + "Value";
    auto columns = vertexMetadataColumns(model, kinds);
    api.insert({"constructor", "N"});
    if (columns.empty() && components.empty()) api.insert("_vertexCount");
    if (!alternatives.empty()) api.insert("_objects");
    for (const auto& column : columns) api.insert(column.name);
    if (model.vertexList) api.insert({"v", "nv"});
    if (model.mutableGraph) api.insert({"addVertex", "clear"});
    const bool sharedEdges = model.hierarchy && !model.separateEdges;
    if (model.hierarchy || model.incidence) {
        constexpr auto runtime = "cocos/rendering/custom/graph";
        auto& values = valueImports[runtime];
        values.insert("OutEI");
        (model.mutableGraph ? values : typeImports[runtime]).insert("ED");
        (model.mutableGraph ? values : typeImports[runtime]).insert("OutE");
        if (model.incidence || sharedEdges) values.insert("AdjI");
        if (model.hierarchy || model.bidirectional) values.insert(sharedEdges ? "OneInEI" : "InEI");
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
    for (const auto& field : fields) {
        if (!api.insert(field.name).second) throw std::runtime_error(node.qualifiedName + ": graph field collides with TypeScript member " + field.name);
    }
    api.insert("__proto__");
    for (auto& component : components) {
        while (!api.insert(component.storageName).second) component.storageName += '_';
        columns.push_back({component.storageName, typeScriptArrayName(component.type), "component" + component.tagName});
    }
    const auto vertexList = columns.empty() ? std::string{} : "this." + columns.front().name;
    const auto vertexCount = columns.empty() ? "this._vertexCount" : vertexList + ".length";
    if (model.hierarchy && std::ranges::find(components, "Parent", &Column::tagName) != components.end())
        throw std::runtime_error(node.qualifiedName + ": component accessor collides with TypeScript getParent");
    if (!alternatives.empty()) {
        const auto scope = node.qualifiedName.substr(0, node.qualifiedName.size() - node.name.size());
        for (const auto& symbol : {kinds, kinds + "Type", node.name + "Object", node.name + "Visitor", "get" + kinds + "Name"}) {
            if (graph.findType(scope + symbol) != TypeId{})
                throw std::runtime_error(node.qualifiedName + ": TypeScript graph export collides with " + symbol);
        }
        out << "export const enum " << kinds << " {\n";
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
        out << "}\n\nexport type " << node.name << "Object = " << kinds << "Type[" << kinds << "];\n\n";
    }

    // Array indices stop at 2^32 - 2. Reserve the null descriptor for narrower index types too.
    const auto limit = std::min<uint64_t>(indexedVertexLimit(graph.node(model.vertexType).qualifiedName).value(), UINT32_MAX);
    if (model.serializable) out << "const adopt" << node.name << " = Symbol(); // eslint-disable-line symbol-description\n\n";
    const auto storage = model.serializable ? "private " : "private readonly ";
    writeDocumentation(out, node.documentation);
    out << "export class " << node.name << " {\n    // Graph\n    readonly N = " << limit << ";\n";
    if (model.incidence || sharedEdges) edgeTraversal(out, sharedEdges, model.bidirectional);
    if (model.vertexList) {
        out << "\n    // VertexListGraph\n";
        if (vertexList.empty())
            out << "    *v (): IterableIterator<number> {\n        for (let v = 0; v < this._vertexCount; ++v) yield v;\n    }\n";
        else out << "    v (): IterableIterator<number> {\n        return " << vertexList << ".keys();\n    }\n";
        out << "    nv (): number {\n        return " << vertexCount << ";\n    }\n";
    }
    if (model.incidence || sharedEdges) {
        out << "\n    // EdgeListGraph\n    ne (): number {\n";
        if (model.incidence) out << "        return this._outEdges.reduce((sum, edges) => sum + edges.length, 0);\n";
        else if (model.parentGraph) out << "        return this._children.length - this.roots.length;\n";
        else out << "        return this._children.reduce((n, edges) => n + edges.length, 0);\n";
        out << "    }\n";
    }
    if (model.mutableGraph) {
        out << "\n    // MutableGraph\n    clear (): void {\n";
        if (vertexList.empty()) out << "        this._vertexCount = 0;\n";
        for (const auto& column : columns) out << "        this." << column.name << ".length = 0;\n";
        for (const auto& alt : alternatives) out << "        this._objects[" << kinds << "." << alt.tagName << "].length = 0;\n";
        if (model.nameIndex) out << "        this.nameIndex.clear();\n";
        if (model.parentGraph) out << "        this.roots.length = 0;\n";
        for (const auto& f : fields) {
            if (!f.tsReset.empty()) out << "        " << f.tsReset << ";\n";
            else if (!f.readonly) out << "        this." << f.name << " = " << f.tsDefault << ";\n";
            else if (f.tsDefault.starts_with("new "))
                out << "        Object.assign(this." << f.name << ", " << f.tsDefault << ");\n";
        }
        out << "    }\n";
        addVertex(out, model, kinds, components, columns, vertexCount);
        if (sharedEdges) {
            out << "\n    addEdge (u: number, v: number): ED | null {\n"
                << "        if (this._parents[v] !== this.N" << (model.serializable ? " || u >= v" : "") << ") return null;\n"
                << "        const children = this._children[u];\n"
                << "        const index = children.findIndex(e => (e.target as number) > v);\n";
            if (!model.serializable)
                out << "        for (let p = u; p !== this.N; p = this._parents[p]) if (p === v) return null;\n";
            out << "        children.splice(index < 0 ? children.length : index, 0, new OutE(v));\n";
            if (model.parentGraph) out << "        this.roots.splice(this.roots.findIndex(e => e.target === v), 1);\n";
            out << "        this._parents[v] = u;\n        return new ED(u, v);\n    }\n";
        } else if (model.incidence) {
            adjacencyMutation(out, false, model.bidirectional);
        }
    }
    if (model.nameComponentIndex)
        out << "\n    // NamedGraph\n    vertexName (v: number): string {\n        return this.get" << components.at(*model.nameComponentIndex).tagName << "(v);\n    }\n";
    if (!components.empty()) out << "\n    // ComponentGraph\n";
    for (size_t i = 0; i < components.size(); ++i) {
        const auto& component = components[i];
        out << "    get" << component.tagName << " (v: number): " << component.type << " {\n"
            << "        return this." << component.storageName << "[v];\n    }\n";
        if (hasSetter(i)) {
            out << "    set" << component.tagName << " (v: number, value: " << component.type << "): void {\n";
            if (model.nameIndex && i == model.nameComponentIndex) {
                out << "        const previous = this." << component.storageName << "[v];\n"
                    << "        if (previous === value) return;\n"
                    << "        if (this.nameIndex.has(value)) throw new Error('Duplicate vertex name');\n"
                    << "        this.nameIndex.set(value, v);\n        this.nameIndex.delete(previous);\n"
                    << "        this." << component.storageName << "[v] = value;\n";
            } else {
                out << "        this." << component.storageName << "[v] = value;\n";
            }
            out << "    }\n";
        }
    }
    if (!alternatives.empty()) {
        out << "\n    // PolymorphicGraph\n    h (id: " << kinds << ", v: number): boolean {\n        return this.w(v) === id;\n    }\n"
            << "    w (v: number): " << kinds << " {\n        return this._kinds[v];\n    }\n"
            << "    object (v: number): " << node.name << "Object {\n        return this._objects[this.w(v)][this._indices[v]];\n    }\n"
            << "    value<T extends " << kinds << "> (id: T, v: number): " << kinds << "Type[T] {\n"
            << "        return this._objects[id][this._indices[v]];\n    }\n"
            << "    visitVertex (visitor: " << node.name << "Visitor, v: number): unknown {\n"
            << "        const index = this._indices[v];\n        switch (this.w(v)) {\n";
        for (const auto& alt : alternatives)
            out << "        case " << kinds << "." << alt.tagName << ": return visitor." << alt.visitorMethod()
                << "(this._objects[" << kinds << "." << alt.tagName << "][index]);\n";
        out << "        default: throw new Error('Unknown vertex kind');\n        }\n    }\n"
            << "    j<T extends " << node.name << "Object> (v: number): T {\n        return this.object(v) as T;\n    }\n";
    }
    if (model.hierarchy) {
        referenceQueries(out, model.separateEdges);
        if (model.mutableGraph) {
            out << "\n    // MutableReferenceGraph\n";
            if (model.separateEdges) adjacencyMutation(out, true, true);
            else out << R"(    addReference (u: number, v: number): ED | null {
        return this.addEdge(u, v);
    }
)";
        }
    }
    if (model.parentGraph) {
        out << "\n    // ParentGraph\n    locateChild (parent: number, name: string): number {\n"
            << "        const children = parent === this.N ? this.roots : this._children[parent];\n"
            << "        for (const edge of children) {\n"
            << "            const v = edge.target as number;\n"
            << "            if (this.vertexName(v) === name) return v;\n"
            << "        }\n        return this.N;\n    }\n";
    }
    if (model.addressableGraph)
        out << R"(
    // AddressableGraph
    locate (absPath: string): number {
        return this.locateRelative(absPath);
    }

    locateRelative (path: string, start: number = this.N): number {
        if (path.length === 0) return this.N;
        return findRelative(this, start, path) as number;
    }

    path (v: number): string {
        return getPath(this, v);
    }
)";
    if (model.nameIndex)
        out << "\n    // UuidGraph\n"
            << R"(    contains (key: string): boolean {
        return this.nameIndex.has(key);
    }
    vertex (key: string): number {
        return this.nameIndex.get(key)!;
    }
    find (key: string): number {
        return this.nameIndex.get(key) ?? this.N;
    }
)";

    out << '\n';
    if (vertexList.empty()) out << "    private _vertexCount = 0;\n";
    for (const auto& column : columns)
        out << "    " << storage << column.name << ": " << column.arrayType << " = [];\n";
    if (!alternatives.empty()) {
        out << "    " << storage << "_objects: { [K in " << kinds << "]: " << kinds << "Type[K][] } = {\n";
        for (const auto& alt : alternatives) out << "        [" << kinds << "." << alt.tagName << "]: [],\n";
        out << "    };\n";
    }
    if (model.nameIndex) out << "    " << storage << "nameIndex = new Map<string, number>();\n";
    if (model.parentGraph) out << "    " << storage << "roots: OutE[] = [];\n";
    std::string initializers;
    for (const auto& f : fields) {
        writeDocumentation(out, f.documentation, "    ");
        out << "    " << (f.safeUnsignedInteger ? "declare " : "") << (f.readonly ? "readonly " : "") << f.name << ": " << f.ts;
        if (f.safeUnsignedInteger) initializers += "        " + f.tsUint64Initializer(f.tsDefault) + ";\n";
        else out << " = " << f.tsDefault;
        out << ";\n";
    }
    if (!initializers.empty()) out << "\n    constructor () {\n" << initializers << "    }\n";
    if (model.serializable) {
        out << "\n    [adopt" << node.name << "] (decoded: " << node.name << "): void {\n";
        if (vertexList.empty()) out << "        this._vertexCount = decoded._vertexCount;\n";
        for (const auto& column : columns) out << "        this." << column.name << " = decoded." << column.name << ";\n";
        if (!alternatives.empty()) out << "        this._objects = decoded._objects;\n";
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
    out << classMethods << "}\n";
}

} // namespace cc::codegen
