#include "cocos/renderer/pipeline/custom/LayoutGraphUtils.h"
#include "cocos/renderer/pipeline/custom/BinaryArchive.h"
#include "cocos/renderer/pipeline/custom/LayoutGraphSerialization.h"
#include "TestDevice.h"
#include "gfx-empty/EmptyDescriptorSet.h"
#include "gfx-empty/EmptyDescriptorSetLayout.h"
#include <boost/container/pmr/monotonic_buffer_resource.hpp>
#include <fstream>

namespace {
using namespace cc::gfx;
using namespace cc::render;

PipelineLayoutData& layoutAt(LayoutGraphData& graph, uint32_t vertex) {
#ifdef CC_TEST_LEGACY_LAYOUT
    return graph.layouts.at(vertex);
#else
    return graph.getLayout(vertex);
#endif
}

class DescriptorDevice final : public test::TestDevice {
public:
    unsigned layouts = 0;
    unsigned sets = 0;

private:
    DescriptorSetLayout* createDescriptorSetLayout() override {
        ++layouts;
        return new EmptyDescriptorSetLayout();
    }
    DescriptorSet* createDescriptorSet() override {
        ++sets;
        return new EmptyDescriptorSet();
    }
};

bool checkCache(LayoutGraphData& graph) {
    DescriptorDevice device;
    if (getOrCreatePerPassDescriptorSet(&device, graph, 0) || device.layouts || device.sets) return false;
    auto& data = layoutAt(graph, 1).descriptorSets.at(UpdateFrequency::PER_PASS);
    if (data.descriptorSetLayout || data.descriptorSet) return false;
    data.descriptorSetLayoutInfo.bindings.clear();
    initializeDescriptorSetLayoutInfo(data.descriptorSetLayoutData, data.descriptorSetLayoutInfo);
    auto* set = getOrCreatePerPassDescriptorSet(&device, graph, 1);
    const auto* layout = data.descriptorSetLayout.get();
    if (!set || !layout || device.layouts != 1 || device.sets != 1 ||
        set != data.descriptorSet.get() || set->getLayout() != layout) return false;
    const auto& bindings = layout->getBindings();
    if (bindings.size() != 3 || bindings[0].binding != 0 || bindings[1].binding != 3 ||
        bindings[2].binding != 4 || layout->getDescriptorCount() != 4) return false;
    // Keep the original set alive so a replacement cannot reuse its address.
    const auto first = data.descriptorSet;
    data.descriptorSet = nullptr;
    set = getOrCreatePerPassDescriptorSet(&device, graph, 1);
    if (!set || set == first.get() || set != data.descriptorSet.get() || set->getLayout() != layout ||
        data.descriptorSetLayout.get() != layout || device.layouts != 1 || device.sets != 2) return false;
    return getOrCreatePerPassDescriptorSet(&device, graph, 1) == set && device.layouts == 1 && device.sets == 2;
}

bool checkOffsets(boost::container::pmr::memory_resource* resource) {
    DescriptorSetLayoutData layout{resource};
    auto& block = layout.descriptorBlocks.emplace_back();
    block.offset = 7;
    block.type = DescriptorTypeOrder::DYNAMIC_STORAGE_BUFFER;
    block.visibility = ShaderStageFlagBit::COMPUTE;
    block.descriptors.emplace_back().count = 3;
    block.descriptors.emplace_back().count = 1;
    layout.descriptorBlocks.emplace_back().capacity = 6;
    auto& last = layout.descriptorBlocks.emplace_back();
    last.offset = 20;
    last.type = DescriptorTypeOrder::INPUT_ATTACHMENT;
    last.descriptors.emplace_back();
    DescriptorSetLayoutInfo info;
    info.bindings.emplace_back().binding = 42;
    initializeDescriptorSetLayoutInfo(layout, info);
    const auto& b = info.bindings;
    return b.size() == 4 && b[0].binding == 42 && b[1].binding == 7 && b[1].count == 3 &&
        b[1].descriptorType == DescriptorType::DYNAMIC_STORAGE_BUFFER && b[1].stageFlags == ShaderStageFlagBit::COMPUTE &&
        b[2].binding == 10 && b[2].count == 1 && b[3].binding == 20 && b[3].descriptorType == DescriptorType::INPUT_ATTACHMENT;
}
} // namespace

bool checkLayoutDescriptors(const char* inputPath, const char* outputPath) {
    boost::container::pmr::monotonic_buffer_resource arena;
    std::ifstream input(inputPath, std::ios::binary);
    input.exceptions(std::ios::badbit | std::ios::failbit);
    BinaryInputArchive archive(input, &arena);
    LayoutGraphData graph{&arena};
    load(archive, graph);
    std::ofstream output(outputPath, std::ios::binary);
    output.exceptions(std::ios::badbit | std::ios::failbit);
#ifdef CC_TEST_LEGACY_LAYOUT
    const auto size = graph.layouts.size();
#else
    const auto size = graph.num_vertices();
#endif
    if (size != 4) return false;
    for (uint32_t vertex = 0; vertex < size; ++vertex) {
        const auto& pipeline = layoutAt(graph, vertex);
        for (const auto* sets : {&pipeline.descriptorSets, &pipeline.descriptorGroups}) {
            for (const auto& [rate, data] : *sets) {
                DescriptorSetLayoutInfo info;
                initializeDescriptorSetLayoutInfo(data.descriptorSetLayoutData, info);
                output << vertex << ' ' << (sets == &pipeline.descriptorSets ? "set " : "group ") << unsigned(rate) << '\n';
                for (const auto& b : info.bindings) {
                    output << b.binding << ' ' << unsigned(b.descriptorType) << ' ' << b.count << ' ' << unsigned(b.stageFlags)
                           << ' ' << unsigned(b.access) << ' ' << unsigned(b.viewDimension) << ' ' << unsigned(b.sampleType)
                           << ' ' << unsigned(b.format) << ' ' << b.immutableSamplers.size() << '\n';
                }
            }
        }
    }
    output.close();
    return checkOffsets(&arena) && checkCache(graph);
}
