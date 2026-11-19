import { DefaultVisitor, GraphColor, OutE, OutEI } from './graph';
import type { edge_descriptor, IncidenceGraph, VertexListGraph } from './graph';
import { depthFirstSearch, depthFirstVisit } from './graph-dfs';
import { HierarchyOnly } from './shift-probe';

function check (ok: boolean, message: string): void {
    if (!ok) throw new Error(message);
}

class Colors {
    constructor (readonly values: GraphColor[]) {}
    get (v: number): GraphColor { return this.values[v]; }
    put (v: number, color: GraphColor): void { this.values[v] = color; }
}

class Digraph implements IncidenceGraph, VertexListGraph {
    readonly N = null;
    constructor (readonly rows: OutE[][], readonly order: number[]) {}
    v (): IterableIterator<number> { return this.order.values(); }
    nv (): number { return this.order.length; }
    oe (v: number): OutEI { return new OutEI(this.rows[v].values(), v); }
    od (v: number): number { return this.rows[v].length; }
    source (e: edge_descriptor): number { return e.source as number; }
    target (e: edge_descriptor): number { return e.target as number; }
    edge (u: number, v: number): boolean { return this.rows[u].some(e => e.target === v); }
}

class Trace extends DefaultVisitor {
    readonly events: string[] = [];
    constructor (readonly color: Colors) { super(); }
    initializeVertex (v: number): void {
        check(this.color.get(v) === GraphColor.WHITE, 'Color at initializeVertex');
        this.events.push('init' + v);
    }
    startVertex (v: number): void { this.events.push('start' + v); }
    discoverVertex (v: number): void {
        check(this.color.get(v) === GraphColor.GRAY, 'Color at discoverVertex');
        this.events.push('open' + v);
    }
    examineEdge (e: edge_descriptor): void { this.edgeEvent('examine', e); }
    treeEdge (e: edge_descriptor): void { this.edgeEvent('tree', e); }
    backEdge (e: edge_descriptor): void { this.edgeEvent('back', e); }
    forwardOrCrossEdge (e: edge_descriptor): void { this.edgeEvent('forward/cross', e); }
    finishEdge (e: edge_descriptor): void { this.edgeEvent('finish', e); }
    finishVertex (v: number): void {
        check(this.color.get(v) === GraphColor.BLACK, 'Color at finishVertex');
        this.events.push('close' + v);
    }
    private edgeEvent (name: string, e: edge_descriptor): void {
        this.events.push(name + e.source + '>' + e.target);
    }
    expect (expected: string): void {
        const actual = this.events.join(' ');
        check(actual === expected, 'Expected ' + expected + '\nReceived ' + actual);
    }
}

const graph = new Digraph([[1, 2], [0, 2], [], [2], []].map(row => row.map(v => new OutE(v))), [4, 3, 2, 1, 0]);
const colors = new Colors(new Array(graph.nv()).fill(GraphColor.BLACK));
const search = new Trace(colors);
depthFirstSearch(graph, search, colors, 0);
search.expect('init4 init3 init2 init1 init0 start0 open0 examine0>1 tree0>1 open1 examine1>0 back1>0 finish1>0 ' +
    'examine1>2 tree1>2 open2 close2 finish1>2 close1 finish0>1 examine0>2 forward/cross0>2 finish0>2 close0 ' +
    'start4 open4 close4 start3 open3 examine3>2 forward/cross3>2 finish3>2 close3');

const visit = new Trace(colors);
colors.values.fill(GraphColor.WHITE);
colors.put(2, GraphColor.BLACK);
depthFirstVisit(graph, 0, visit, colors, { terminate: v => v === 1 });
visit.expect('start0 open0 examine0>1 tree0>1 open1 close1 finish0>1 examine0>2 forward/cross0>2 finish0>2 close0');
check(colors.get(3) === GraphColor.WHITE && colors.get(4) === GraphColor.WHITE, 'Visit reached a disconnected vertex');

const empty = new Trace(new Colors([]));
depthFirstSearch(new Digraph([], []), empty, empty.color, 0);
empty.expect('');

const chain = new HierarchyOnly();
const length = 25000;
let parent: number = chain.N;
for (let i = 0; i < length; ++i) parent = chain.addVertex(i, parent);
class ChainVisitor extends DefaultVisitor {
    opened = 0;
    closed = 0;
    discoverVertex (v: number): void { check(v === this.opened++, 'Long chain discovery order'); }
    finishVertex (v: number): void { check(v === length - ++this.closed, 'Long chain finish order'); }
}
const chainVisitor = new ChainVisitor();
depthFirstVisit(chain, 0, chainVisitor, new Colors(new Array(length).fill(GraphColor.WHITE)));
check(chainVisitor.opened === length && chainVisitor.closed === length, 'Incomplete long chain');
