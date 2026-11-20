extern module GfxDescriptorSet;
namespace cc::gfx {
    struct DescriptorSetLayoutInfo;
    class DescriptorSetLayout;
    class DescriptorSet;
    class PipelineLayout;
}

binding typescript {
    DescriptorSetLayoutInfo => DescriptorSetLayoutInfo from "cocos/gfx" archive from "cocos/rendering/custom/serialization";
    DescriptorSetLayout => DescriptorSetLayout from "cocos/gfx";
    DescriptorSet => DescriptorSet from "cocos/gfx";
    PipelineLayout => PipelineLayout from "cocos/gfx";
}
