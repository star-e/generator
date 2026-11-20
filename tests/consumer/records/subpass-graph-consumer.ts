import { RenderGraphObjectPool, Subpass } from './render-graph';
import { RenderCommonObjectPool } from './types';
import type { BidirectionalGraph } from './graph';

function assert(ok: boolean): void {
    if (!ok) throw new Error('SubpassGraph assertion failed');
}

const common = new RenderCommonObjectPool();
const pool = new RenderGraphObjectPool(common);
const graph = pool.createSubpassGraph();
const api: BidirectionalGraph = graph;
const subpass = pool.createSubpass();
const rasterViews = subpass.rasterViews, computeViews = subpass.computeViews, resolvePairs = subpass.resolvePairs;
rasterViews.set('color', pool.createRasterView());
computeViews.set('storage', [pool.createComputeView()]);
resolvePairs.push(common.createResolvePair());
const a = graph.addVertex('first', new Subpass());
const b = graph.addVertex('second', new Subpass());
const c = graph.addVertex('resolve', subpass);
graph.setName(b, 'first');
assert(graph.vertexName(a) === graph.vertexName(b) && graph.getSubpass(c) === subpass);
const edge = graph.addEdge(a, c);
if (edge === null) throw new Error('Expected a valid edge');
assert(graph.addEdge(b, c) !== null && graph.addEdge(a, c) !== null && graph.addEdge(c, c) !== null);
assert(graph.addEdge(graph.N, c) === null && graph.addEdge(a, 0.5) === null && graph.ne() === 4);
assert(Array.from(api.ie(c), e => graph.source(e)).join(',') === [a, b, a, c].join(',') &&
    Array.from(api.oe(a), e => graph.target(e)).join(',') === [c, c].join(',') &&
    Array.from(graph.adj(c)).join(',') === String(c) && api.d(c) === 5);
graph.removeEdge(edge);
assert(graph.od(a) === 1 && graph.id(c) === 3 && graph.edge(a, c));
graph.addEdge(a, c);
graph.removeEdges(a, c);
graph.removeEdge(edge);
graph.removeEdges(c, c);
assert(!graph.edge(a, c) && graph.edge(b, c) && graph.id(c) === 1 && graph.od(c) === 0 && graph.ne() === 1);
pool.reset();
assert(pool.createSubpassGraph() === graph && graph.ne() === 0 && graph.nv() === 0);
assert(pool.createSubpass() === subpass && subpass.rasterViews === rasterViews && rasterViews.size === 0 &&
    subpass.computeViews === computeViews && computeViews.size === 0 && subpass.resolvePairs === resolvePairs && resolvePairs.length === 0);
assert(graph.addVertex('again', subpass) === 0 && graph.id(0) === 0 && graph.od(0) === 0);
