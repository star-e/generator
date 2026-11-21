import { AttachmentType, ClearValueType, LightingMode, ParameterType, ResolveFlags, ResourceFlags, TaskType, UpdateFrequency } from './types';
import { Format, MemoryAccessBit, SampleType, ShaderStageFlagBit, ViewDimension } from '../../gfx';
import { DefaultCtor, EmptyCtor, ImplicitCtor, RequiredCtor, ShiftProbe, OverloadedCtor, WrappedCtor } from './shift-probe';
import { DescriptorTypeOrder, LayoutType, RenderPassType } from './layout-graph';

export function checkConstructors (): boolean {
    const required = new RequiredCtor(12, true);
    const defaults = new DefaultCtor(undefined, undefined);
    return new ImplicitCtor().count === 7 && new EmptyCtor().count === 7 && required.count === 12
        && defaults.count === 7 && !defaults.enabled && defaults.kind === 0
        && new WrappedCtor().value.count === 7 && new OverloadedCtor(7, required).value === required;
}

function checkConstructorArity (): void {
    // @ts-expect-error Both fields are mandatory even though they have initializers.
    new RequiredCtor(1);
    // @ts-expect-error The implicit constructor takes no arguments.
    new ImplicitCtor(1);
    // @ts-expect-error The declared empty constructor takes no arguments.
    new EmptyCtor(1);
}

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
export const descriptorGroupBlockArgs = [...descriptorBlockArgs, MemoryAccessBit.READ_WRITE, ViewDimension.TEX2D_ARRAY,
    SampleType.UINT, Format.ASTC_SRGBA_12X12];
export const descriptorBlockDataArgs = [DescriptorTypeOrder.STORAGE_IMAGE, ShaderStageFlagBit.COMPUTE, 31,
    MemoryAccessBit.WRITE_ONLY, ViewDimension.TEX2D_ARRAY, SampleType.UINT, Format.RGBA8] as const;

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
