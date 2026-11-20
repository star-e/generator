import { FormatView, ManagedResource, RenderGraphObjectPool, ResourceDesc, ResourceGraph, ResourceGraphValue,
    ResourceStates, ResourceTraits, getResourceGraphValueName } from './render-graph';
import type { ResourceGraphValueType, ResourceGraphVisitor } from './render-graph';
import type { Buffer, Texture, Framebuffer, Swapchain } from '../../gfx';
import type { RenderWindow } from '../../render-scene/core/render-window';
import { RenderCommonObjectPool, ResourceResidency } from './types';
import { AccessFlagBit, Format, SamplerInfo, SampleCount, TextureType } from '../../gfx';

function assert(ok: boolean): void {
    if (!ok) throw new Error('ResourceGraph assertion failed');
}

const resourcePool = new RenderGraphObjectPool(new RenderCommonObjectPool());
const resources: ResourceGraph = resourcePool.createResourceGraph();
const desc: ResourceDesc = resourcePool.createResourceDesc();
const traits: ResourceTraits = resourcePool.createResourceTraits(ResourceResidency.EXTERNAL);
const states: ResourceStates = resourcePool.createResourceStates();
const managed: ManagedResource = resourcePool.createManagedResource();
const sampler = new SamplerInfo();
const res = resources.addVertex(ResourceGraphValue.Managed, managed, 'color', desc, traits, states, sampler);
assert([...resources.v()].length === 1 && resources.getName(res) === 'color' && resources.getDesc(res) === desc &&
    resources.getTraits(res) === traits && resources.getStates(res) === states && resources.getSampler(res) === sampler &&
    resources.value(ResourceGraphValue.Managed, res) === managed);
assert(desc.sampleCount === SampleCount.X1 && desc.viewType === TextureType.TEX2D && states.states === AccessFlagBit.NONE);
const view: FormatView = resourcePool.createFormatView();
const buffer = {} as Buffer;
const texture = {} as Texture;
const framebuffer = {} as Framebuffer;
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
let rejected = false;
try { resources.value(ResourceGraphValue.Framebuffer, res); } catch (error) { rejected = error instanceof Error; }
assert(rejected);
if (false) {
    // @ts-expect-error The framebuffer payload requires a GFX object.
    resources.addVertex(ResourceGraphValue.Framebuffer, null, '', desc, traits, states, sampler);
    // @ts-expect-error This cached native pointer has no TypeScript binding.
    sc.texture = texture;
}
for (const value of [mb, mt, pb, pt]) {
    value.fenceValue = Number.MAX_SAFE_INTEGER;
    assert(Object.keys(value).indexOf('fenceValue') !== -1);
    for (const invalid of [-1, 0.5, NaN, Infinity, Number.MAX_SAFE_INTEGER + 1]) {
        let rangeError = false;
        try { value.fenceValue = invalid; } catch (error) { rangeError = error instanceof RangeError; }
        assert(rangeError && value.fenceValue === Number.MAX_SAFE_INTEGER);
    }
}
sc.renderWindow = {} as RenderWindow;
sc.currentID = sc.numBackBuffers = sc.generation = 2;
sub.textureView = texture;
sub.format = Format.RGBA8;
sub.indexOrFirstMipLevel = sub.numMipLevels = sub.firstArraySlice = sub.numArraySlices = sub.firstPlane = sub.numPlanes = 1;
view.format = Format.RGBA8;
desc.width = 256;
resourcePool.reset();
assert(resourcePool.createResourceGraph() === resources && resources.nv() === 0 &&
    resourcePool.createResourceDesc().width === 0 && resourcePool.createFormatView().format === Format.UNKNOWN);
assert(resourcePool.createManagedBuffer() === mb && resourcePool.createPersistentBuffer() === pb &&
    resourcePool.createManagedTexture() === mt && resourcePool.createPersistentTexture() === pt);
assert(mb.buffer === null && mt.texture === null && pb.buffer === null && pt.texture === null &&
    [mb, mt, pb, pt].every(v => v.fenceValue === 0));
assert(resourcePool.createRenderSwapchain() === sc && sc.swapchain === null && sc.renderWindow === null &&
    !sc.isDepthStencil && sc.currentID === 0 && sc.numBackBuffers === 0 && sc.generation === 0xFFFFFFFF);
const reused = resourcePool.createSubresourceView();
assert(reused === sub && reused.textureView === null && reused.format === Format.UNKNOWN &&
    [reused.indexOrFirstMipLevel, reused.numMipLevels, reused.firstArraySlice, reused.numArraySlices, reused.firstPlane, reused.numPlanes].every(v => v === 0));
