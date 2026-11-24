import { DefaultVisitor, depthFirstSearch, depthFirstVisit, GraphColor, OutE, OutEI, ReferenceGraphView } from './graph';
import type { edge_descriptor, IncidenceGraph, VertexListGraph } from './graph';
import type { InputArchive } from './archive';
import { DescriptorDB, LayoutGraph, LayoutGraphValue, RenderPassType } from './layout-graph';
import { HierarchyOnly, MultiParentProbe } from './shift-probe';
import { ComputePass, RasterPass, RenderData, RenderGraph, RenderGraphValue, RenderQueue, SceneData } from './render-graph';

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
    compareNative (samples: InputArchive): void {
        const count = samples.n();
        check(count === this.events.length, 'Native and TS DFS event counts differ');
        for (let i = 0; i < count; ++i) {
            const expected = samples.s();
            check(this.events[i] === expected, 'DFS event ' + i + ': native ' + expected + ', TS ' + this.events[i]);
        }
    }
}

export function checkNativeDfs (samples: InputArchive): void {
    const count = samples.n();
    const rows = Array.from({ length: count }, () => Array.from({ length: samples.n() }, () => new OutE(samples.n())));
    const order = Array.from({ length: count }, () => samples.n());
    const graph = new Digraph(rows, order);
    const colors = new Colors(new Array(count).fill(GraphColor.BLACK));
    const search = new Trace(colors);
    depthFirstSearch(graph, search, colors, 0);
    search.expect('init4 init3 init2 init1 init0 start0 open0 examine0>1 tree0>1 open1 examine1>0 back1>0 finish1>0 ' +
        'examine1>2 tree1>2 open2 close2 finish1>2 close1 finish0>1 examine0>2 forward/cross0>2 finish0>2 close0 ' +
        'start4 open4 close4 start3 open3 examine3>2 forward/cross3>2 finish3>2 close3');
    search.compareNative(samples);

    const visit = new Trace(colors);
    colors.values.fill(GraphColor.WHITE);
    colors.put(2, GraphColor.BLACK);
    depthFirstVisit(graph, 0, visit, colors, { terminate: v => v === 1 });
    visit.expect('start0 open0 examine0>1 tree0>1 open1 close1 finish0>1 examine0>2 forward/cross0>2 finish0>2 close0');
    check(colors.get(3) === GraphColor.WHITE && colors.get(4) === GraphColor.WHITE, 'Visit reached a disconnected vertex');
    visit.compareNative(samples);

    const view = new Digraph([[new OutE(0)], []], [0, 1]);
    view.target = () => 1;
    const viewColors = new Colors([GraphColor.WHITE, GraphColor.WHITE]);
    depthFirstVisit(view, 0, new DefaultVisitor(), viewColors);
    check(viewColors.get(1) === GraphColor.BLACK, 'DFS follows the target exposed by the graph view');

    const empty = new Trace(new Colors([]));
    depthFirstSearch(new Digraph([], []), empty, empty.color, 0);
    empty.expect('');
    empty.compareNative(samples);

    const hierarchy = new HierarchyOnly();
    const top = hierarchy.addVertex(0);
    const leaf = hierarchy.addVertex(1, top);
    const hierarchyColors = new Colors([GraphColor.WHITE, GraphColor.WHITE]);
    depthFirstVisit(hierarchy, top, new DefaultVisitor(), hierarchyColors);
    check(hierarchyColors.get(leaf) === GraphColor.BLACK, 'Single-root visit without vertex enumeration');

    const multi = new MultiParentProbe();
    const a = multi.addVertex(0), b = multi.addVertex(0), child = multi.addVertex(0, a);
    const second = multi.addReference(b, child);
    if (!second) throw new Error('expected second parent');
    check(multi.addReference(a, child) !== null &&
        Array.from(multi.parents(child), e => multi.parent(e)).join() === [a, b, a].join(), 'multiple parent links');
    multi.removeReference(second);
    check(multi.getParent(child) === a && multi.numParents(child) === 2, 'remove one parent link');
    multi.removeReferences(a, child);
    check(multi.numParents(child) === 0, 'remove parallel parent links');

    const forest = new LayoutGraph();
    const size = samples.n();
    for (let v = 0; v < size; ++v) addStage(forest, samples.n());
    const forestVisitor = new Trace(new Colors(new Array(size).fill(GraphColor.BLACK)));
    depthFirstSearch(forest, forestVisitor, forestVisitor.color);
    forestVisitor.compareNative(samples);

    const chain = new LayoutGraph();
    const length = samples.n();
    check(length === 25000, 'Long chain size');
    let parent: number = chain.N;
    for (let i = 0; i < length; ++i) parent = addStage(chain, parent);
    const chainVisitor = new Trace(new Colors(new Array(length).fill(GraphColor.WHITE)));
    depthFirstVisit(chain, 0, chainVisitor, chainVisitor.color);
    const opened = chainVisitor.events.filter(event => event.startsWith('open'));
    const closed = chainVisitor.events.filter(event => event.startsWith('close'));
    check(opened.length === length && opened.every((event, i) => event === 'open' + i), 'Long chain discovery order');
    check(closed.length === length && closed.every((event, i) => event === 'close' + (length - 1 - i)), 'Long chain finish order');
    chainVisitor.compareNative(samples);
    checkRenderGraphView(samples);
}

function checkRenderGraphView (samples: InputArchive): void {
    const graph = new RenderGraph();
    const pass = graph.addVertex(RenderGraphValue.RasterPass, new RasterPass(), 'pass', '', new RenderData(), true);
    const queue = graph.addVertex(RenderGraphValue.Queue, new RenderQueue(), 'queue', '', new RenderData(), true, pass);
    const scene = graph.addVertex(RenderGraphValue.Scene, new SceneData(), 'scene', '', new RenderData(), true, queue);
    const compute = graph.addVertex(RenderGraphValue.Compute, new ComputePass(), 'compute', '', new RenderData(), true);
    graph.addEdge(pass, compute);
    const refs = new ReferenceGraphView(graph);
    const visit = (expected: string): void => {
        const color = new Colors(new Array(refs.nv()).fill(GraphColor.WHITE));
        const visitor = new Trace(color);
        depthFirstVisit(refs, pass, visitor, color);
        visitor.expect(expected);
        visitor.compareNative(samples);
        check(color.get(compute) === GraphColor.WHITE, 'Ownership DFS followed an execution dependency');
    };
    const branch = 'start0 open0 examine0>1 tree0>1 open1 examine1>2 tree1>2 open2 close2 finish1>2 close1 finish0>1 ';
    visit(branch + 'close0');
    const alternate = graph.addVertex(RenderGraphValue.Queue, new RenderQueue(), 'alternate', '', new RenderData(), true, pass);
    graph.removeReferences(queue, scene);
    graph.addReference(alternate, scene);
    visit('start0 open0 examine0>1 tree0>1 open1 close1 finish0>1 ' +
        'examine0>4 tree0>4 open4 examine4>2 tree4>2 open2 close2 finish4>2 close4 finish0>4 close0');
}

function addStage (g: LayoutGraph, parent: number): number {
    return g.addVertex(LayoutGraphValue.RenderStage, RenderPassType.SINGLE_RENDER_PASS, 'v', new DescriptorDB(), parent);
}
