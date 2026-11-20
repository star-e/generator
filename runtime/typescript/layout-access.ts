import { COCOS_RUNTIME, HTML5 } from 'internal:constants';
import type { DescriptorSetLayoutInfo } from '../../gfx';
import type { DescriptorSetData, LayoutType } from './layout-graph';
import type { UpdateFrequency } from './types';

// Keep this module usable while layout-graph initializes its derived classes.
const vulkan: LayoutType.VULKAN = 0;
export class Layout {
    static type: LayoutType = vulkan;
    static isWebGPU = false;
}

export abstract class PipelineLayoutAccess {
    abstract readonly descriptorSets: Map<UpdateFrequency, DescriptorSetData>;
    abstract readonly descriptorGroups: Map<UpdateFrequency, DescriptorSetData>;

    getSets (): Map<UpdateFrequency, DescriptorSetData> {
        return (COCOS_RUNTIME || HTML5) && Layout.isWebGPU ? this.descriptorGroups : this.descriptorSets;
    }

    getSet (frequency: UpdateFrequency): DescriptorSetData | undefined {
        return this.getSets().get(frequency);
    }
}

export function resetSetLayoutInfo (info: DescriptorSetLayoutInfo): void {
    info.bindings.length = 0;
}
