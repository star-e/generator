import { FormatView, ManagedResource, RenderGraphObjectPool, ResourceDesc, ResourceGraph, ResourceGraphValue,
    ResourceStates, ResourceTraits, getResourceGraphValueName } from './render-graph';
import type { ResourceGraphValueType, ResourceGraphVisitor } from './render-graph';
import type { Buffer, Texture, Framebuffer, RenderPass, Swapchain } from '../../gfx';
import type { RenderWindow } from '../../render-scene/core/render-window';
import { RenderCommonObjectPool, ResourceResidency } from './types';
import { AccessFlagBit, Color, Format, SamplerInfo, SampleCount, TextureType } from '../../gfx';

function assert(ok: boolean): void {
    if (!ok) throw new Error('ResourceGraph assertion failed');
}

const resourcePool = new RenderGraphObjectPool(new RenderCommonObjectPool());
const resources: ResourceGraph = resourcePool.createResourceGraph();
resources.nextFenceValue = 11;
resources.version = 3;
resources.clear();
assert(resources.nextFenceValue === 0 && resources.version === 0);
const desc: ResourceDesc = resourcePool.createResourceDesc();
const traits: ResourceTraits = resourcePool.createResourceTraits(ResourceResidency.EXTERNAL);
const states: ResourceStates = resourcePool.createResourceStates();
const managed: ManagedResource = resourcePool.createManagedResource();
const sampler = new SamplerInfo();
function throws(action: () => void): boolean {
    try { action(); } catch (error) { return error instanceof Error; }
    return false;
}
assert(!resources.contains('color') && resources.find('color') === resources.N && throws(() => resources.vertex('color')));
const res = resources.addVertex(ResourceGraphValue.Managed, managed, 'color', desc, traits, states, sampler);
assert(res === 0 && resources.contains('color') && resources.find('color') === res && resources.vertex('color') === res);
assert(throws(() => resources.addVertex(ResourceGraphValue.Managed, managed, 'color', desc, traits, states, sampler)));
assert([...resources.v()].length === 1 && resources.getName(res) === 'color' && resources.getDesc(res) === desc &&
    resources.getTraits(res) === traits && resources.getStates(res) === states && resources.getSampler(res) === sampler &&
    resources.value(ResourceGraphValue.Managed, res) === managed);
assert(desc.sampleCount === SampleCount.X1 && desc.viewType === TextureType.TEX2D && states.states === AccessFlagBit.NONE);
const view: FormatView = resourcePool.createFormatView();
const buffer = {} as Buffer;
const texture = {} as Texture;
const framebuffer = {} as Framebuffer;
const pass = {} as RenderPass;
const cached = resourcePool.createPersistentRenderPassAndFramebuffer(pass, framebuffer);
assert(cached.renderPass === pass && cached.framebuffer === framebuffer && cached.clearDepth === 0 && cached.clearStencil === 0);
const clearColors = cached.clearColors;
assert(clearColors.length === 0);
clearColors.push(new Color(0.25, 0.5, 0.75, 1));
cached.clearDepth = 0.5;
cached.clearStencil = 7;
const swapchain = {} as Swapchain;
const mb = resourcePool.createManagedBuffer(buffer);
const mt = resourcePool.createManagedTexture(texture);
const pb = resourcePool.createPersistentBuffer(buffer);
const pt = resourcePool.createPersistentTexture(texture);
const sc = resourcePool.createRenderSwapchain(swapchain, true);
const sub = resourcePool.createSubresourceView();
assert(mb.buffer === buffer && pb.buffer === buffer && mt.texture === texture && pt.texture === texture);
assert(sc.swapchain === swapchain && sc.isDepthStencil && !('texture' in sc) && sub.textureView === null);

const visitor: ResourceGraphVisitor = {
    managed: v => v === managed && 0, managedBuffer: v => v === mb && 1, managedTexture: v => v === mt && 2,
    persistentBuffer: v => v === pb && 3, persistentTexture: v => v === pt && 4,
    framebuffer: v => v === framebuffer && 5, swapchain: v => v === sc && 6,
    formatView: v => v === view && 7, subresourceView: v => v === sub && 8,
};
function check<T extends ResourceGraphValue>(id: T, payload: ResourceGraphValueType[T], ordinal: number, name: string): void {
    const v = resources.addVertex(id, payload, name, desc, traits, states, sampler);
    assert(id === ordinal && getResourceGraphValueName(id) === name && resources.h(id, v) && resources.w(v) === id &&
        resources.object(v) === payload && resources.value(id, v) === payload && resources.visitVertex(visitor, v) === ordinal);
}
check(ResourceGraphValue.Managed, managed, 0, 'Managed');
check(ResourceGraphValue.ManagedBuffer, mb, 1, 'ManagedBuffer');
check(ResourceGraphValue.ManagedTexture, mt, 2, 'ManagedTexture');
check(ResourceGraphValue.PersistentBuffer, pb, 3, 'PersistentBuffer');
check(ResourceGraphValue.PersistentTexture, pt, 4, 'PersistentTexture');
check(ResourceGraphValue.Framebuffer, framebuffer, 5, 'Framebuffer');
check(ResourceGraphValue.Swapchain, sc, 6, 'Swapchain');
check(ResourceGraphValue.FormatView, view, 7, 'FormatView');
check(ResourceGraphValue.SubresourceView, sub, 8, 'SubresourceView');
assert(throws(() => resources.setName(res, 'SubresourceView')) && resources.getName(res) === 'color' &&
    resources.vertex('color') === res && resources.vertex('SubresourceView') !== res);
assert(throws(() => resources.setName(resources.N, 'missing')) && !resources.contains('missing'));
resources.setName(res, 'color');
resources.setName(res, 'a\0b');
assert(!resources.contains('a') && !resources.contains('color') && resources.vertex('a\0b') === res && resources.vertexName(res) === 'a\0b');
resources.setName(res, '');
assert(resources.vertex('') === res && !resources.contains('a\0b'));
let rejected = false;
try { resources.value(ResourceGraphValue.Framebuffer, res); } catch (error) { rejected = error instanceof Error; }
assert(rejected);
if (false) {
    // @ts-expect-error The framebuffer payload requires a GFX object.
    resources.addVertex(ResourceGraphValue.Framebuffer, null, '', desc, traits, states, sampler);
    // @ts-expect-error This cached native pointer has no TypeScript binding.
    sc.texture = texture;
}
function checkUint64<K extends string>(value: Record<K, number>, field: K): void {
    assert(value[field] === 0 && Object.keys(value).indexOf(field) !== -1);
    value[field] = Number.MAX_SAFE_INTEGER;
    for (const invalid of [-1, 0.5, NaN, Infinity, Number.MAX_SAFE_INTEGER + 1]) {
        let rangeError = false;
        try { value[field] = invalid; } catch (error) { rangeError = error instanceof RangeError; }
        assert(rangeError && value[field] === Number.MAX_SAFE_INTEGER);
    }
}
for (const value of [mb, mt, pb, pt]) checkUint64(value, 'fenceValue');
checkUint64(resources, 'nextFenceValue');
checkUint64(resources, 'version');
sc.renderWindow = {} as RenderWindow;
sc.currentID = sc.numBackBuffers = sc.generation = 2;
sub.textureView = texture;
sub.format = Format.RGBA8;
sub.indexOrFirstMipLevel = sub.numMipLevels = sub.firstArraySlice = sub.numArraySlices = sub.firstPlane = sub.numPlanes = 1;
view.format = Format.RGBA8;
desc.width = 256;

function addSubview(name: string, parent: number = resources.N): number {
    return resources.addVertex(ResourceGraphValue.SubresourceView, resourcePool.createSubresourceView(),
        name, desc, traits, states, sampler, parent);
}
const depth = addSubview('depth');
const base = resources.addVertex(ResourceGraphValue.ManagedTexture, resourcePool.createManagedTexture(),
    'texture', desc, traits, states, sampler);
const stencil = addSubview('stencil', base);
const mip = resources.addVertex(ResourceGraphValue.FormatView, resourcePool.createFormatView(),
    'depth-mip', desc, traits, states, sampler, depth);
const vertexCount = resources.nv();
const link = resources.addReference(base, depth);
assert(link !== null && resources.source(link) === base && resources.target(link) === depth);
assert(resources.addEdge(base, depth) === null && resources.addReference(stencil, depth) === null &&
    resources.addEdge(base, base) === null && resources.addReference(mip, base) === null &&
    resources.addEdge(resources.N, base) === null);
assert(throws(() => addSubview('orphan', vertexCount)) && !resources.contains('orphan') && resources.nv() === vertexCount);
assert(resources.ne() === 3 && resources.getParent(base) === resources.N && resources.getParent(depth) === base &&
    resources.getParent(stencil) === base && resources.getParent(mip) === depth);
const children = [...resources.children(base)].map(e => resources.child(e));
assert(children.length === 2 && children.indexOf(depth) >= 0 && children.indexOf(stencil) >= 0 &&
    resources.od(base) === 2 && resources.id(depth) === 1 && resources.edge(base, depth) && resources.reference(depth, mip));
assert(resources.vertex('texture') === base && resources.vertex('depth') === depth && resources.vertex('depth-mip') === mip);
resourcePool.reset();
assert(resourcePool.createResourceGraph() === resources && resources.nv() === 0 && resources.ne() === 0 && !resources.contains('depth') &&
    resourcePool.createResourceDesc().width === 0 && resourcePool.createFormatView().format === Format.UNKNOWN);
assert(resources.nextFenceValue === 0 && resources.version === 0);
const recycled = resourcePool.createPersistentRenderPassAndFramebuffer();
assert(recycled === cached && recycled.renderPass === null && recycled.framebuffer === null &&
    recycled.clearColors === clearColors && clearColors.length === 0 && recycled.clearDepth === 0 && recycled.clearStencil === 0);
assert(!resources.contains('SubresourceView') && resources.find('') === resources.N &&
    resources.addVertex(ResourceGraphValue.Managed, managed, '', desc, traits, states, sampler) === 0 && resources.vertex('') === 0);
assert(resources.getParent(0) === resources.N && resources.numChildren(0) === 0);
assert(resourcePool.createManagedBuffer() === mb && resourcePool.createPersistentBuffer() === pb &&
    resourcePool.createManagedTexture() === mt && resourcePool.createPersistentTexture() === pt);
assert(mb.buffer === null && mt.texture === null && pb.buffer === null && pt.texture === null &&
    [mb, mt, pb, pt].every(v => v.fenceValue === 0));
assert(resourcePool.createRenderSwapchain() === sc && sc.swapchain === null && sc.renderWindow === null &&
    !sc.isDepthStencil && sc.currentID === 0 && sc.numBackBuffers === 0 && sc.generation === 0xFFFFFFFF);
const reused = resourcePool.createSubresourceView();
assert(reused === sub && reused.textureView === null && reused.format === Format.UNKNOWN &&
    [reused.indexOrFirstMipLevel, reused.numMipLevels, reused.firstArraySlice, reused.numArraySlices, reused.firstPlane, reused.numPlanes].every(v => v === 0));
