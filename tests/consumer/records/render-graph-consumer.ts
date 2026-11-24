import { Viewport } from '../../gfx';
import type { BidirectionalGraph, MutableReferenceGraph } from './graph';
import { ClearView, getRenderGraphValueName, RenderGraphObjectPool, RenderGraphValue as Kind, RenderGraphValueType } from './render-graph';
import { RenderCommonObjectPool } from './types';

function check(ok: boolean, message: string): void {
    if (!ok) throw new Error(message);
}

const pool = new RenderGraphObjectPool(new RenderCommonObjectPool());
const g = pool.createRenderGraph();
const deps: BidirectionalGraph = g;
const refs: MutableReferenceGraph = g;
const data = pool.createRenderData();
function add<T extends Kind>(kind: T, value: RenderGraphValueType[T], name = '', parent: number = g.N): number {
    const v = g.addVertex(kind, value, name, 'layout', data, true, parent);
    check(g.w(v) === kind && g.value(kind, v) === value, 'payload and kind');
    return v;
}
const pass = add(Kind.RasterPass, pool.createRasterPass(), 'pass');
const queue = add(Kind.Queue, pool.createRenderQueue(), 'queue', pass);
const scene = add(Kind.Scene, pool.createSceneData(), 'scene', queue);
const compute = add(Kind.Compute, pool.createComputePass(), 'pass');
check(g.ne() === 0 && refs.getParent(queue) === pass && g.getParent(scene) === queue && g.getParent(pass) === g.N, 'initial ownership');
check(g.vertexName(pass) === g.vertexName(compute) && g.index.size === 0, 'duplicate names');
const edge = g.addEdge(pass, compute), link = g.addReference(pass, compute);
if (!edge || !link) throw new Error('missing edge or reference');
g.addEdge(queue, compute);
g.addEdge(pass, compute);
check(g.addReference(queue, compute) === null && g.addReference(pass, compute) === null &&
    g.addReference(compute, pass) === null && g.addReference(pass, pass) === null, 'single-parent forest');
check(Array.from(deps.ie(compute), e => g.source(e)).join() === [pass, queue, pass].join() &&
    Array.from(g.parents(compute), e => g.parent(e)).join() === String(pass), 'independent incoming edges');
g.removeReferences(queue, compute);
check(g.getParent(compute) === pass, 'remove unrelated reference');
g.removeReference(link);
check(g.getParent(compute) === g.N && g.numParents(compute) === 0 && deps.id(compute) === 3, 'detach');
check(g.addReference(queue, compute) !== null && g.numParents(compute) === 1, 'reparent');
g.removeEdge(edge);
check(g.edge(pass, compute) && g.numParents(compute) === 1, 'remove one parallel edge');
g.removeEdges(pass, compute);
check(!g.edge(pass, compute) && g.reference(queue, compute), 'erase only dependencies');
g.removeReferences(queue, compute);
check(g.edge(queue, compute) && !g.reference(queue, compute) && g.addReference(pass, compute) !== null, 'erase only references');
check(Array.from(refs.children(pass), e => refs.child(e)).join() === [queue, compute].join() &&
    g.numChildren(queue) === 1 && g.ne() === 1, 'children and edge count');

add(Kind.RasterSubpass, pool.createRasterSubpass());
add(Kind.ComputeSubpass, pool.createComputeSubpass());
add(Kind.Resolve, pool.createResolvePass());
add(Kind.Copy, pool.createCopyPass());
add(Kind.Move, pool.createMovePass());
add(Kind.Raytrace, pool.createRaytracePass());
add(Kind.Blit, pool.createBlit());
add(Kind.Dispatch, pool.createDispatch());
const clear = add(Kind.Clear, [pool.createClearView('color')]);
const viewport = add(Kind.Viewport, new Viewport());
const views: ClearView[] = g.value(Kind.Clear, clear);
check(views[0].slotName === 'color' && g.value(Kind.Viewport, viewport) instanceof Viewport, 'clear and viewport');
const kinds = ['RasterPass', 'RasterSubpass', 'ComputeSubpass', 'Compute', 'Resolve', 'Copy', 'Move', 'Raytrace',
    'Queue', 'Scene', 'Blit', 'Dispatch', 'Clear', 'Viewport'];
check(kinds.every((name, id) => name === getRenderGraphValueName(id)) && g.nv() === kinds.length, 'kind order');
g.setValid(scene, false);
g.setLayout(pass, 'main-layout');
check(!g.getValid(scene) && g.getLayout(pass) === 'main-layout' && g.getData(scene) === data, 'components');
const index = g.index, sorted = g.sortedVertices, globalData = g.globalRenderData;
index.set('execution-index', compute);
sorted.push(pass, compute);
globalData.constants.set(2, [67]);
globalData.custom = 'global';
pool.reset();
check(pool.createRenderGraph() === g && g.nv() === 0 && g.ne() === 0, 'graph pool reset');
check(g.index === index && !index.size && g.sortedVertices === sorted && !sorted.length &&
    g.globalRenderData === globalData && !globalData.constants.size && globalData.custom === '', 'graph field reset');
check(add(Kind.Queue, pool.createRenderQueue()) === 0 && g.numChildren(0) === 0 && g.getParent(0) === g.N, 'reuse');
