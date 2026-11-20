import { FormatView, ManagedResource, RenderGraphObjectPool, ResourceDesc, ResourceGraph, ResourceGraphValue,
    ResourceStates, ResourceTraits } from './render-graph';
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
view.format = Format.RGBA8;
desc.width = 256;
resourcePool.reset();
assert(resourcePool.createResourceGraph() === resources && resources.nv() === 0 &&
    resourcePool.createResourceDesc().width === 0 && resourcePool.createFormatView().format === Format.UNKNOWN);
