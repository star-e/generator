#include "LayoutGraphDescriptorSet.h"
#include "cocos/renderer/pipeline/custom/details/GslUtils.h"
#include "gfx-base/GFXDevice.h"

namespace cc::render {

// The engine's LayoutGraphUtils.cpp owns the descriptor type mapping.
gfx::DescriptorType getGfxDescriptorType(DescriptorTypeOrder type);

void initializeDescriptorSetLayoutInfo(const DescriptorSetLayoutData& layout, gfx::DescriptorSetLayoutInfo& info) {
    for (const auto& block : layout.descriptorBlocks) {
        auto slot = block.offset;
        for (const auto& descriptor : block.descriptors) {
            auto& binding = info.bindings.emplace_back();
            binding.binding = slot;
            binding.descriptorType = getGfxDescriptorType(block.type);
            binding.count = descriptor.count;
            binding.stageFlags = block.visibility;
            slot += descriptor.count;
        }
    }
}

gfx::DescriptorSet* getOrCreatePerPassDescriptorSet(gfx::Device* device, LayoutGraphData& graph,
                                                    LayoutGraphData::vertex_descriptor vertex) {
    auto& sets = graph.getLayout(vertex).descriptorSets;
    auto iter = sets.find(UpdateFrequency::PER_PASS);
    if (iter == sets.end()) return nullptr;

    auto& data = iter->second;
    if (!data.descriptorSet) {
        if (!data.descriptorSetLayout) {
            data.descriptorSetLayout = device->createDescriptorSetLayout(data.descriptorSetLayoutInfo);
        }
        CC_ENSURES(data.descriptorSetLayout);
        data.descriptorSet = device->createDescriptorSet(gfx::DescriptorSetInfo{data.descriptorSetLayout.get()});
    }
    CC_ENSURES(data.descriptorSet);
    return data.descriptorSet.get();
}

} // namespace cc::render
