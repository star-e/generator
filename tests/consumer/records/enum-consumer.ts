import { AttachmentType, ClearValueType, LightingMode, ParameterType, ResolveFlags, ResourceFlags, TaskType } from './types';
import { ShiftProbe } from './shift-probe';

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
