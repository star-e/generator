#pragma once

#include "cocos/renderer/pipeline/custom/LayoutGraphTypes.h"

namespace cc::render {

void initializeDescriptorSetLayoutInfo(const DescriptorSetLayoutData& layout, gfx::DescriptorSetLayoutInfo& info);
gfx::DescriptorSet* getOrCreatePerPassDescriptorSet(gfx::Device* device, LayoutGraphData& graph,
                                                    LayoutGraphData::vertex_descriptor vertex);

} // namespace cc::render
