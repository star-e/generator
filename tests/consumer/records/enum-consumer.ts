import { AttachmentType, ClearValueType, LightingMode, ParameterType, ResolveFlags, ResourceFlags, TaskType, UpdateFrequency } from './types';
import { ShaderStageFlagBit } from '../../gfx';
import { ShiftProbe } from './shift-probe';
import { DescriptorTypeOrder, LayoutType, RenderPassType } from './layout-graph';

export const layoutEnums = {
    LayoutType: [LayoutType.VULKAN, LayoutType.WEBGPU],
    DescriptorTypeOrder: [DescriptorTypeOrder.UNIFORM_BUFFER, DescriptorTypeOrder.DYNAMIC_UNIFORM_BUFFER,
        DescriptorTypeOrder.SAMPLER_TEXTURE, DescriptorTypeOrder.SAMPLER, DescriptorTypeOrder.TEXTURE,
        DescriptorTypeOrder.STORAGE_BUFFER, DescriptorTypeOrder.DYNAMIC_STORAGE_BUFFER,
        DescriptorTypeOrder.STORAGE_IMAGE, DescriptorTypeOrder.INPUT_ATTACHMENT],
    RenderPassType: [RenderPassType.SINGLE_RENDER_PASS, RenderPassType.RENDER_PASS, RenderPassType.RENDER_SUBPASS],
};

export const descriptorBlockArgs = [UpdateFrequency.PER_PASS, ParameterType.UAV, DescriptorTypeOrder.DYNAMIC_STORAGE_BUFFER,
    ShaderStageFlagBit.VERTEX | ShaderStageFlagBit.FRAGMENT];

export const resolveFlags = [ResolveFlags.NONE, ResolveFlags.COLOR, ResolveFlags.DEPTH, ResolveFlags.STENCIL];
export const depthStencil = ResolveFlags.DEPTH | ResolveFlags.STENCIL;
export const resolvesColor = (depthStencil & ResolveFlags.COLOR) !== 0;
export const highBit = ShiftProbe.HIGH;

export const attachmentUsage = ResourceFlags.COLOR_ATTACHMENT | ResourceFlags.SAMPLED;
export function usesStorage(flags: ResourceFlags): boolean {
    return (flags & ResourceFlags.STORAGE) !== 0;
}

export const constEnumValues = {
    ParameterType: [ParameterType.CONSTANTS, ParameterType.CBV, ParameterType.UAV,
        ParameterType.SRV, ParameterType.TABLE, ParameterType.SSV],
    TaskType: [TaskType.SYNC, TaskType.ASYNC],
    LightingMode: [LightingMode.NONE, LightingMode.DEFAULT, LightingMode.CLUSTERED],
    AttachmentType: [AttachmentType.RENDER_TARGET, AttachmentType.DEPTH_STENCIL, AttachmentType.SHADING_RATE],
    ClearValueType: [ClearValueType.NONE, ClearValueType.FLOAT_TYPE, ClearValueType.INT_TYPE],
};
