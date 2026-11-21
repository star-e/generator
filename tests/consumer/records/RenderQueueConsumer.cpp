#include "cocos/renderer/pipeline/custom/NativePipelineRecords.h"

#include <array>
#include <boost/container/pmr/monotonic_buffer_resource.hpp>
#include <type_traits>
#include <utility>

namespace {
using namespace cc::render;
using Arena = boost::container::pmr::monotonic_buffer_resource;

static_assert(!std::is_copy_constructible_v<NativeRenderQueue> && std::is_move_constructible_v<NativeRenderQueue>);
static_assert(!std::is_copy_assignable_v<DeviceRenderData> && std::is_move_assignable_v<DeviceRenderData>);

// Capture the value supplied by each call site without submitting GPU work.
uint32_t recordedOffset{};

template<class Queue>
bool checkOffset(const Queue& queue) {
    queue.recordCommandBuffer(nullptr, 0, nullptr, 19);
    if (recordedOffset != 19) return false;
    queue.recordCommandBuffer(nullptr, 0, nullptr);
    return recordedOffset == UINT32_MAX;
}

bool hasOrder(const RenderDrawQueue& queue, const std::array<uint32_t, 5>& passIndices) {
    if (queue.instances.size() != passIndices.size()) return false;
    for (size_t i = 0; i < passIndices.size(); ++i)
        if (queue.instances[i].passIndex != passIndices[i]) return false;
    return true;
}

bool checkDeviceRenderData(Arena& source, Arena& destination) {
    DeviceRenderData data{&source};
    const NameLocalID id{3};
    data.required = true;
    if (!data.hasNoData()) return false;
    data.hasConstants = true;
    if (data.hasNoData()) return false;
    data.hasConstants = false;
    data.buffers.emplace(id, nullptr);
    if (data.hasNoData()) return false;
    data.clear();
    data.textures.emplace(id, TextureWithAccessFlags{});
    if (data.hasNoData()) return false;
    data.clear();
    data.samplers.emplace(id, nullptr);
    if (data.hasNoData()) return false;
    data.hasConstants = true;
    data.required = true;
    data.buffers.emplace(id, nullptr);
    data.textures.emplace(id, TextureWithAccessFlags{});
    DeviceRenderData moved{std::move(data), &destination};
    if (!moved.hasConstants || !moved.required || !moved.buffers.count(id) || !moved.textures.count(id) ||
        !moved.samplers.count(id) || moved.buffers.get_allocator().resource() != &destination ||
        moved.textures.get_allocator().resource() != &destination || moved.samplers.get_allocator().resource() != &destination)
        return false;
    moved.clear();
    return moved.hasNoData() && !moved.required && !moved.hasConstants;
}
} // namespace

namespace cc::render {
void RenderDrawQueue::recordCommandBuffer(gfx::RenderPass*, uint32_t, gfx::CommandBuffer*, uint32_t offset) const {
    recordedOffset = offset;
}

void RenderInstancingQueue::recordCommandBuffer(gfx::RenderPass*, uint32_t, gfx::CommandBuffer*, uint32_t offset) const {
    recordedOffset = offset;
}
} // namespace cc::render

bool checkRenderQueues() {
    Arena source, destination;
    NativeRenderQueue queue{&source};
    if (queue.camera || queue.sceneFlags != SceneFlags::NONE || queue.lightByteOffset != UINT32_MAX ||
        !checkOffset(queue.opaqueQueue) || !checkOffset(queue.opaqueInstancingQueue)) return false;
    queue.opaqueQueue.instances = {
        {nullptr, 1, 0, 9.0F, 2, 0}, {nullptr, 0, 1, 4.0F, 1, 1}, {nullptr, 0, 0, 2.0F, 2, 2},
        {nullptr, 0, 0, 2.0F, 1, 3}, {nullptr, 0, 0, 9.0F, 1, 4},
    };
    queue.transparentQueue.instances = queue.opaqueQueue.instances;
    NativeRenderQueue moved{std::move(queue), &destination};
    if (moved.opaqueQueue.get_allocator().resource() != &destination || moved.transparentQueue.get_allocator().resource() != &destination ||
        moved.probeQueue.get_allocator().resource() != &destination || moved.opaqueInstancingQueue.get_allocator().resource() != &destination ||
        moved.transparentInstancingQueue.get_allocator().resource() != &destination) return false;
    moved.sort();
    return hasOrder(moved.opaqueQueue, {3, 2, 4, 0, 1}) && hasOrder(moved.transparentQueue, {4, 3, 2, 1, 0}) &&
           checkDeviceRenderData(source, destination);
}
