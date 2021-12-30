import * as impl from './graph';

export const enum TransversalEnum {
    DepthFirst,
    BreadthFirst,
}

export const enum NodeType {
    Sphere,
    Box,
    Mesh,
    Light,
}

export class Node {
    content = '';
    flags = 0;
}

export class Sphere {
    x = 0;
    y = 0;
    z = 0;
    radius = 1;
}

export class Box {
    x = 0;
    y = 0;
    z = 0;
    sizeX = 1;
    sizeY = 1;
    sizeZ = 1;
}

export class Mesh {
    assetPath = '';
}

export class Light {
    x = 0;
    y = 0;
    z = 0;
    dirX = 1;
    dirY = 0;
    dirZ = 0;
}

//=================================================================
// SceneGraph
//=================================================================
// PolymorphicGraph Concept
export const enum SceneGraphValue {
    sphere,
    box,
    mesh,
    light,
}

interface SceneGraphValueType {
    [SceneGraphValue.sphere]: Sphere
    [SceneGraphValue.box]: Box
    [SceneGraphValue.mesh]: Mesh
    [SceneGraphValue.light]: Light
}

export interface SceneGraphVisitor {
    sphere(value: Sphere): unknown;
    box(value: Box): unknown;
    mesh(value: Mesh): unknown;
    light(value: Light): unknown;
}

type SceneGraphObject = Sphere | Box | Mesh | Light;

//-----------------------------------------------------------------
// Graph Concept
export class SceneGraphVertex {
    constructor (
        readonly id: SceneGraphValue,
        readonly object: SceneGraphObject,
        readonly name: string,
    ) {
        this._id = id;
        this._object = object;
        this._name = name;
    }
    readonly _outEdges: impl.OutE[] = [];
    readonly _inEdges: impl.OutE[] = [];
    readonly _children: impl.OutE[] = [];
    readonly _parents: impl.OutE[] = [];
    readonly _id: SceneGraphValue;
    readonly _object: SceneGraphObject;
    readonly _name: string;
}

//-----------------------------------------------------------------
// PropertyGraph Concept
export class SceneGraphNameMap implements impl.PropertyMap {
    constructor (readonly vertices: SceneGraphVertex[]) {
        this._vertices = vertices;
    }
    get (v: number): string {
        return this._vertices[v]._name;
    }
    readonly _vertices: SceneGraphVertex[];
}

export class SceneGraphnodeMap implements impl.PropertyMap {
    constructor (readonly nodes: Node[]) {
        this._nodes = nodes;
    }
    get (v: number): Node {
        return this._nodes[v];
    }
    readonly _nodes: Node[];
}

//-----------------------------------------------------------------
// ComponentGraph Concept
export const enum SceneGraphComponent {
    node,
}

interface SceneGraphComponentType {
    [SceneGraphComponent.node]: Node;
}

interface SceneGraphComponentPropertyMap {
    [SceneGraphComponent.node]: SceneGraphnodeMap;
}

//-----------------------------------------------------------------
// SceneGraph Implementation
export class SceneGraph implements impl.BidirectionalGraph
, impl.AdjacencyGraph
, impl.VertexListGraph
, impl.MutableGraph
, impl.PropertyGraph
, impl.NamedGraph
, impl.ComponentGraph
, impl.PolymorphicGraph
, impl.ReferenceGraph
, impl.MutableReferenceGraph
, impl.AddressableGraph {
    //-----------------------------------------------------------------
    // Graph
    // type vertex_descriptor = number;
    nullVertex (): number { return 0xFFFFFFFF; }
    // type edge_descriptor = impl.ED;
    readonly directed_category: impl.directional = impl.directional.bidirectional;
    readonly edge_parallel_category: impl.parallel = impl.parallel.allow;
    readonly traversal_category: impl.traversal = impl.traversal.incidence
        | impl.traversal.bidirectional
        | impl.traversal.adjacency
        | impl.traversal.vertex_list;
    //-----------------------------------------------------------------
    // IncidenceGraph
    // type out_edge_iterator = impl.OutEI;
    // type degree_size_type = number;
    edge (u: number, v: number): boolean {
        for (const oe of this._vertices[u]._outEdges) {
            if (v === oe.target as number) {
                return true;
            }
        }
        return false;
    }
    source (e: impl.ED): number {
        return e.source as number;
    }
    target (e: impl.ED): number {
        return e.target as number;
    }
    outEdges (v: number): impl.OutEI {
        return new impl.OutEI(this._vertices[v]._outEdges.values(), v);
    }
    outDegree (v: number): number {
        return this._vertices[v]._outEdges.length;
    }
    //-----------------------------------------------------------------
    // BidirectionalGraph
    // type in_edge_iterator = impl.InEI;
    inEdges (v: number): impl.InEI {
        return new impl.InEI(this._vertices[v]._inEdges.values(), v);
    }
    inDegree (v: number): number {
        return this._vertices[v]._inEdges.length;
    }
    degree (v: number): number {
        return this.outDegree(v) + this.inDegree(v);
    }
    //-----------------------------------------------------------------
    // AdjacencyGraph
    // type adjacency_iterator = impl.AdjI;
    adjacentVertices (v: number): impl.AdjI {
        return new impl.AdjI(this, this.outEdges(v));
    }
    //-----------------------------------------------------------------
    // VertexListGraph
    vertices (): IterableIterator<number> {
        return this._vertices.keys();
    }
    numVertices (): number {
        return this._vertices.length;
    }
    //-----------------------------------------------------------------
    // MutableGraph
    addVertex<T extends SceneGraphValue> (
        id: SceneGraphValue,
        object: SceneGraphValueType[T],
        name: string,
        node: Node,
        u = 0xFFFFFFFF,
    ): number {
        const vert = new SceneGraphVertex(id, object, name);
        const v = this._vertices.length;
        this._vertices.push(vert);
        this._nodes.push(node);

        // ReferenceGraph
        if (u !== 0xFFFFFFFF) {
            this._vertices[u]._children.push(new impl.OutE(v));
            vert._parents.push(new impl.OutE(u));
        }

        return v;
    }
    clearVertex (v: number): void {
        // ReferenceGraph(Separated)
        const vert = this._vertices[v];
        // clear out edges
        for (const oe of vert._outEdges) {
            const target = this._vertices[oe.target as number];
            for (let i = 0; i !== target._inEdges.length;) { // remove all edges
                if (target._inEdges[i].target === v) {
                    target._inEdges.splice(i, 1);
                } else {
                    ++i;
                }
            }
        }
        vert._outEdges.length = 0;

        // clear in edges
        for (const ie of vert._inEdges) {
            const source = this._vertices[ie.target as number];
            for (let i = 0; i !== source._outEdges.length;) { // remove all edges
                if (source._outEdges[i].target === v) {
                    source._outEdges.splice(i, 1);
                } else {
                    ++i;
                }
            }
        }
        vert._inEdges.length = 0;

        // clear child edges
        for (const oe of vert._children) {
            const target = this._vertices[oe.target as number];
            for (let i = 0; i !== target._parents.length;) { // remove all edges
                if (target._parents[i].target === v) {
                    target._parents.splice(i, 1);
                } else {
                    ++i;
                }
            }
        }
        vert._children.length = 0;

        // clear parent edges
        for (const ie of vert._parents) {
            const source = this._vertices[ie.target as number];
            for (let i = 0; i !== source._children.length;) { // remove all edges
                if (source._children[i].target === v) {
                    source._children.splice(i, 1);
                } else {
                    ++i;
                }
            }
        }
        vert._parents.length = 0;
    }
    removeVertex (u: number): void {
        this._vertices.splice(u, 1);
        this._nodes.splice(u, 1);

        const sz = this._vertices.length;
        if (u === sz) {
            return;
        }

        for (let v = 0; v !== sz; ++v) {
            const vert = this._vertices[v];
            impl.reindexEdgeList(vert._outEdges, u);
            impl.reindexEdgeList(vert._inEdges, u);
            // ReferenceGraph (Separated)
            impl.reindexEdgeList(vert._children, u);
            impl.reindexEdgeList(vert._parents, u);
        }
    }
    addEdge (u: number, v: number): impl.ED | null {
        // update in/out edge list
        this._vertices[u]._outEdges.push(new impl.OutE(v));
        this._vertices[v]._inEdges.push(new impl.OutE(u));
        return new impl.ED(u, v);
    }
    removeEdges (u: number, v: number): void {
        const source = this._vertices[u];
        // remove out edges of u
        for (let i = 0; i !== source._outEdges.length;) { // remove all edges
            if (source._outEdges[i].target === v) {
                source._outEdges.splice(i, 1);
            } else {
                ++i;
            }
        }
        // remove in edges of v
        const target = this._vertices[v];
        for (let i = 0; i !== target._inEdges.length;) { // remove all edges
            if (target._inEdges[i].target === u) {
                target._inEdges.splice(i, 1);
            } else {
                ++i;
            }
        }
    }
    removeEdge (e: impl.ED): void {
        const u = e.source as number;
        const v = e.target as number;
        const source = this._vertices[u];
        for (let i = 0; i !== source._outEdges.length;) {
            if (source._outEdges[i].target === v) {
                source._outEdges.splice(i, 1);
                break; // remove one edge
            } else {
                ++i;
            }
        }
        const target = this._vertices[v];
        for (let i = 0; i !== target._inEdges.length;) {
            if (target._inEdges[i].target === u) {
                target._inEdges.splice(i, 1);
                break; // remove one edge
            } else {
                ++i;
            }
        }
    }
    //-----------------------------------------------------------------
    // NamedGraph
    vertexName (v: number): string {
        return this._vertices[v]._name;
    }
    vertexNameMap (): SceneGraphNameMap {
        return new SceneGraphNameMap(this._vertices);
    }
    //-----------------------------------------------------------------
    // PropertyGraph
    get (tag: string): SceneGraphNameMap | SceneGraphnodeMap {
        switch (tag) {
        // NamedGraph
        case 'name':
            return new SceneGraphNameMap(this._vertices);
        // Components
        case 'node':
            return new SceneGraphnodeMap(this._nodes);
        default:
            throw Error('property map not found');
        }
    }
    //-----------------------------------------------------------------
    // ComponentGraph
    component<T extends SceneGraphComponent> (id: T, v: number): SceneGraphComponentType[T] {
        switch (id) {
        case SceneGraphComponent.node:
            return this._nodes[v] as SceneGraphComponentType[T];
        default:
            throw Error('component not found');
        }
    }
    componentMap<T extends SceneGraphComponent> (id: T): SceneGraphComponentPropertyMap[T] {
        switch (id) {
        case SceneGraphComponent.node:
            return new SceneGraphnodeMap(this._nodes) as SceneGraphComponentPropertyMap[T];
        default:
            throw Error('component map not found');
        }
    }
    getnode (v: number): Node {
        return this._nodes[v];
    }
    //-----------------------------------------------------------------
    // PolymorphicGraph
    holds (id: SceneGraphValue, v: number): boolean {
        return this._vertices[v]._id === id;
    }
    id (v: number): SceneGraphValue {
        return this._vertices[v]._id;
    }
    object (v: number): SceneGraphObject {
        return this._vertices[v]._object;
    }
    value<T extends SceneGraphValue> (id: T, v: number): SceneGraphValueType[T] {
        if (this._vertices[v]._id === id) {
            return this._vertices[v]._object as SceneGraphValueType[T];
        } else {
            throw Error('value id not match');
        }
    }
    tryValue<T extends SceneGraphValue> (id: T, v: number): SceneGraphValueType[T] | null {
        if (this._vertices[v]._id === id) {
            return this._vertices[v]._object as SceneGraphValueType[T];
        } else {
            return null;
        }
    }
    visitVertex (visitor: SceneGraphVisitor, v: number): unknown {
        const vert = this._vertices[v];
        switch (vert._id) {
        case SceneGraphValue.sphere:
            return visitor.sphere(vert._object as Sphere);
        case SceneGraphValue.box:
            return visitor.box(vert._object as Box);
        case SceneGraphValue.mesh:
            return visitor.mesh(vert._object as Mesh);
        case SceneGraphValue.light:
            return visitor.light(vert._object as Light);
        default:
            throw Error('polymorphic type not found');
        }
    }
    getSphere (v: number): Sphere {
        if (this._vertices[v]._id === SceneGraphValue.sphere) {
            return this._vertices[v]._object as Sphere;
        } else {
            throw Error('value id not match');
        }
    }
    getBox (v: number): Box {
        if (this._vertices[v]._id === SceneGraphValue.box) {
            return this._vertices[v]._object as Box;
        } else {
            throw Error('value id not match');
        }
    }
    getMesh (v: number): Mesh {
        if (this._vertices[v]._id === SceneGraphValue.mesh) {
            return this._vertices[v]._object as Mesh;
        } else {
            throw Error('value id not match');
        }
    }
    getLight (v: number): Light {
        if (this._vertices[v]._id === SceneGraphValue.light) {
            return this._vertices[v]._object as Light;
        } else {
            throw Error('value id not match');
        }
    }
    tryGetSphere (v: number): Sphere | null {
        if (this._vertices[v]._id === SceneGraphValue.sphere) {
            return this._vertices[v]._object as Sphere;
        } else {
            return null;
        }
    }
    tryGetBox (v: number): Box | null {
        if (this._vertices[v]._id === SceneGraphValue.box) {
            return this._vertices[v]._object as Box;
        } else {
            return null;
        }
    }
    tryGetMesh (v: number): Mesh | null {
        if (this._vertices[v]._id === SceneGraphValue.mesh) {
            return this._vertices[v]._object as Mesh;
        } else {
            return null;
        }
    }
    tryGetLight (v: number): Light | null {
        if (this._vertices[v]._id === SceneGraphValue.light) {
            return this._vertices[v]._object as Light;
        } else {
            return null;
        }
    }
    //-----------------------------------------------------------------
    // ReferenceGraph
    // type reference_descriptor = impl.ED;
    // type child_iterator = impl.OutEI;
    // type parent_iterator = impl.InEI;
    reference (u: number, v: number): boolean {
        for (const oe of this._vertices[u]._children) {
            if (v === oe.target as number) {
                return true;
            }
        }
        return false;
    }
    parent (e: impl.ED): number {
        return e.source as number;
    }
    child (e: impl.ED): number {
        return e.target as number;
    }
    parents (v: number): impl.InEI {
        return new impl.InEI(this._vertices[v]._parents.values(), v);
    }
    children (v: number): impl.OutEI {
        return new impl.OutEI(this._vertices[v]._children.values(), v);
    }
    numParents (v: number): number {
        return this._vertices[v]._parents.length;
    }
    numChildren (v: number): number {
        return this._vertices[v]._children.length;
    }
    getParent (v: number): number {
        if (v === 0xFFFFFFFF) {
            return 0xFFFFFFFF;
        }
        const list = this._vertices[v]._parents;
        if (list.length === 0) {
            return 0xFFFFFFFF;
        } else {
            return list[0].target as number;
        }
    }
    isAncestor (ancestor: number, descendent: number): boolean {
        const pseudo = 0xFFFFFFFF;
        if (ancestor === descendent) {
            // when ancestor === descendent, is_ancestor is defined as false
            return false;
        }
        if (ancestor === pseudo) {
            // special case: pseudo root is always ancestor
            return true;
        }
        if (descendent === pseudo) {
            // special case: pseudo root is never descendent
            return false;
        }
        for (let parent = this.getParent(descendent); parent !== pseudo;) {
            if (ancestor === parent) {
                return true;
            }
            parent = this.getParent(parent);
        }
        return false;
    }
    //-----------------------------------------------------------------
    // MutableReferenceGraph
    addReference (u: number, v: number): impl.ED | null {
        // update in/out edge list
        this._vertices[u]._children.push(new impl.OutE(v));
        this._vertices[v]._parents.push(new impl.OutE(u));
        return new impl.ED(u, v);
    }
    removeReference (e: impl.ED): void {
        const u = e.source as number;
        const v = e.target as number;
        const source = this._vertices[u];
        for (let i = 0; i !== source._children.length;) {
            if (source._children[i].target === v) {
                source._children.splice(i, 1);
                break; // remove one edge
            } else {
                ++i;
            }
        }
        const target = this._vertices[v];
        for (let i = 0; i !== target._parents.length;) {
            if (target._parents[i].target === u) {
                target._parents.splice(i, 1);
                break; // remove one edge
            } else {
                ++i;
            }
        }
    }
    removeReferences (u: number, v: number): void {
        const source = this._vertices[u];
        // remove out edges of u
        for (let i = 0; i !== source._children.length;) { // remove all edges
            if (source._children[i].target === v) {
                source._children.splice(i, 1);
            } else {
                ++i;
            }
        }
        // remove in edges of v
        const target = this._vertices[v];
        for (let i = 0; i !== target._parents.length;) { // remove all edges
            if (target._parents[i].target === u) {
                target._parents.splice(i, 1);
            } else {
                ++i;
            }
        }
    }
    //-----------------------------------------------------------------
    // ParentGraph
    locateChild (u: number, name: string): number {
        if (u === 0xFFFFFFFF) {
            for (const v of this._vertices.keys()) {
                const vert = this._vertices[v];
                if (vert._parents.length === 0 && vert._name === name) {
                    return v;
                }
            }
            return 0xFFFFFFFF;
        }
        for (const oe of this._vertices[u]._children) {
            const child = oe.target as number;
            if (name === this._vertices[child]._name) {
                return child;
            }
        }
        return 0xFFFFFFFF;
    }
    //-----------------------------------------------------------------
    // AddressableGraph
    contains (absPath: string): boolean {
        return impl.findRelative(this, 0xFFFFFFFF, absPath) as number !== 0xFFFFFFFF;
    }
    locate (absPath: string): number {
        return impl.findRelative(this, 0xFFFFFFFF, absPath) as number;
    }
    locateRelative (path: string, start = 0xFFFFFFFF): number {
        return impl.findRelative(this, start, path) as number;
    }
    path (v: number): string {
        return impl.getPath(this, v);
    }

    readonly components: string[] = ['node'];
    readonly _vertices: SceneGraphVertex[] = [];
    readonly _nodes: Node[] = [];
}
/*
import { TransversalEnum, NodeType, Node, Sphere,
Box, Mesh, Light,
SceneGraph } from './tutorial';
*/
