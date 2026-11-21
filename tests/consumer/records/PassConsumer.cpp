#include "cocos/renderer/pipeline/custom/RenderGraphTypes.h"
#include <boost/container/pmr/monotonic_buffer_resource.hpp>
#include <type_traits>
#include <utility>

namespace {
using namespace cc::render;
using Arena = boost::container::pmr::monotonic_buffer_resource;

bool checkSubpasses(Arena& scratch, Arena& storage) {
    static_assert(!std::is_default_constructible_v<RasterSubpass>);
    auto* defaultResource = boost::container::pmr::get_default_resource();
    RasterSubpass partial{std::allocator_arg, defaultResource, 7};
    RasterSubpass allFields{std::allocator_arg, defaultResource, 7, 4, 2};
    if (partial.subpassID != 7 || partial.count != 1 || partial.quality != 0 ||
        allFields.count != 4 || allFields.quality != 2 ||
        partial.computeViews.get_allocator().resource() != defaultResource ||
        allFields.resolvePairs.get_allocator().resource() != defaultResource) return false;

    RasterSubpass defaults{&scratch};
    ComputeSubpass computeDefaults{&scratch};
    if (defaults.subpassID != 0xFFFFFFFFU || defaults.count != 1 || defaults.quality != 0 ||
        defaults.showStatistics || defaults.viewport.maxDepth != 1 || defaults.get_allocator().resource() != &scratch ||
        computeDefaults.subpassID != 0xFFFFFFFFU) return false;

    RasterSubpass raster{std::allocator_arg, &scratch, 7, 4, 2};
    raster.rasterViews["color"].slotName = "output";
    raster.computeViews["image"].emplace_back().plane = 2;
    raster.resolvePairs.emplace_back().target = "resolved";
    raster.viewport.width = 320;
    raster.showStatistics = true;
    RasterSubpass clone{raster, &storage};
    if (clone.subpassID != 7 || clone.count != 4 || clone.quality != 2 || clone.viewport.width != 320 ||
        !clone.showStatistics || clone.rasterViews.at("color").slotName != "output" ||
        clone.computeViews.at("image").front().plane != 2 || clone.resolvePairs.at(0).target != "resolved" ||
        clone.rasterViews.at("color").get_allocator().resource() != &storage ||
        clone.computeViews.at("image").front().get_allocator().resource() != &storage ||
        clone.resolvePairs.front().get_allocator().resource() != &storage) return false;

    ComputeSubpass compute{std::allocator_arg, &scratch, 9};
    compute.rasterViews["target"].slotID = 1;
    compute.computeViews["source"].emplace_back().name = "input";
    ComputeSubpass moved{std::move(compute), &storage};
    return moved.subpassID == 9 && moved.rasterViews.at("target").slotID == 1 &&
           moved.computeViews.at("source").front().name == "input" &&
           moved.computeViews.at("source").front().get_allocator().resource() == &storage;
}

bool checkCopyPass(Arena& storage) {
    static_assert(!std::is_constructible_v<CopyPass, const CopyPass&, CopyPass::allocator_type>);
    static_assert(!std::is_copy_constructible_v<CopyPass> && !std::is_copy_assignable_v<CopyPass>);
    static_assert(std::is_move_constructible_v<CopyPass> && std::is_move_assignable_v<CopyPass>);

    // The pass must keep its upload data after the source arena has been destroyed.
    CopyPass moved = [&storage] {
        Arena scratch;
        CopyPass pass{&scratch};
        pass.copyPairs.emplace_back().source = "source-color-texture";
        auto& upload = pass.uploadPairs.emplace_back();
        upload.source = {0, 127, 128, 255};
        upload.target = "uploaded-texture-layer";
        upload.targetFirstSlice = 3;
        return CopyPass{std::move(pass), &storage};
    }();

    const auto& copy = moved.copyPairs.at(0);
    const auto& upload = moved.uploadPairs.at(0);
    return copy.source == "source-color-texture" && upload.target == "uploaded-texture-layer" &&
           upload.targetFirstSlice == 3 && upload.source == ccstd::vector<uint8_t>{0, 127, 128, 255} &&
           moved.copyPairs.get_allocator().resource() == &storage && moved.uploadPairs.get_allocator().resource() == &storage &&
           copy.get_allocator().resource() == &storage && upload.target.get_allocator().resource() == &storage;
}
} // namespace

bool checkPassRecords() {
    using namespace cc::render;
    Arena scratch, storage;
    if (!checkSubpasses(scratch, storage) || !checkCopyPass(storage)) return false;

    ComputePass compute{&scratch};
    compute.computeViews["color"].emplace_back().plane = 1;
    compute.textures["color"] = cc::gfx::ShaderStageFlagBit::COMPUTE;
    ComputePass computeCopy{compute, &storage};
    RaytracePass raytrace{&scratch};
    raytrace.computeViews["hit"].emplace_back().plane = 3;
    RaytracePass raytraceCopy{raytrace, &storage};
    ResolvePass resolve{&scratch};
    resolve.resolvePairs.emplace_back().target = "resolved";
    ResolvePass resolveCopy{resolve, &storage};
    MovePass move{&scratch};
    move.movePairs.emplace_back().source = "input";
    MovePass moveCopy{move, &storage};
    return computeCopy.textures.at("color") == cc::gfx::ShaderStageFlagBit::COMPUTE &&
           computeCopy.computeViews.at("color").front().plane == 1 && raytraceCopy.computeViews.at("hit").front().plane == 3 &&
           resolveCopy.resolvePairs.at(0).target == "resolved" && moveCopy.movePairs.at(0).source == "input" &&
           computeCopy.computeViews.at("color").front().get_allocator().resource() == &storage &&
           raytraceCopy.computeViews.at("hit").front().get_allocator().resource() == &storage &&
           resolveCopy.resolvePairs.front().get_allocator().resource() == &storage && moveCopy.movePairs.front().get_allocator().resource() == &storage;
}
