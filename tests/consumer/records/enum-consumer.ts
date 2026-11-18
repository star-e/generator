import { AttachmentType, ClearValueType, LightingMode, ParameterType, TaskType } from './types';

export const constEnumValues = {
    ParameterType: [ParameterType.CONSTANTS, ParameterType.CBV, ParameterType.UAV,
        ParameterType.SRV, ParameterType.TABLE, ParameterType.SSV],
    TaskType: [TaskType.SYNC, TaskType.ASYNC],
    LightingMode: [LightingMode.NONE, LightingMode.DEFAULT, LightingMode.CLUSTERED],
    AttachmentType: [AttachmentType.RENDER_TARGET, AttachmentType.DEPTH_STENCIL, AttachmentType.SHADING_RATE],
    ClearValueType: [ClearValueType.NONE, ClearValueType.FLOAT_TYPE, ClearValueType.INT_TYPE],
};
