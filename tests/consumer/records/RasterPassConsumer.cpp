#include "cocos/renderer/pipeline/custom/RenderGraphTypes.h"
#include <boost/container/pmr/monotonic_buffer_resource.hpp>
#include <type_traits>
#include <utility>

namespace {
using namespace cc::render;

void populate(RasterPass& pass) {
    pass.rasterViews["color"].slotName = "output";
    pass.computeViews["storage"].emplace_back().plane = 1;
    pass.textures["input"] = cc::gfx::ShaderStageFlagBit::FRAGMENT;
    const auto v = pass.subpassGraph.add_vertex("resolve");
    pass.subpassGraph.getSubpass(v).resolvePairs.emplace_back().target = "resolved";
    pass.width = 1280;
    pass.height = 720;
    pass.count = 4;
    pass.quality = 1;
}

ccstd::hash_t passHash(const RasterPass& pass) {
    ccstd::hash_t seed{};
    ccstd::hash_combine(seed, pass.rasterViews);
    ccstd::hash_combine(seed, pass.computeViews);
    ccstd::hash_combine(seed, pass.textures);
    ccstd::hash_combine(seed, pass.subpassGraph);
    ccstd::hash_combine(seed, pass.width);
    ccstd::hash_combine(seed, pass.height);
    ccstd::hash_combine(seed, pass.count);
    ccstd::hash_combine(seed, pass.quality);
    return seed;
}
} // namespace

bool checkRasterPassCache() {
    using namespace cc::render;
    using Cache = ccstd::pmr::unordered_map<RasterPass, PersistentRenderPassAndFramebuffer>;
    static_assert(std::is_same_v<decltype(ResourceGraph::renderPasses), Cache>);
    static_assert(!std::is_default_constructible_v<RasterPass>);
    static_assert(std::is_nothrow_move_constructible_v<RasterPass>);
    static_assert(std::is_nothrow_move_constructible_v<SubpassGraph>);
    static_assert(!noexcept(SubpassGraph(std::declval<SubpassGraph&&>(), std::declval<const SubpassGraph::allocator_type&>())));
    static_assert(std::is_nothrow_move_assignable_v<RasterPass> && std::is_nothrow_move_assignable_v<SubpassGraph>);
    boost::container::pmr::monotonic_buffer_resource arena, scratch;
    ResourceGraph resources{&arena};
    auto& cache = resources.renderPasses;
    RasterPass pass{&scratch}, lookup{&arena};
    if (pass.count != 1 || pass.quality != 0 || pass.viewport.maxDepth != 1) return false;
    populate(pass);
    populate(lookup);
    if (pass != lookup || ccstd::hash<RasterPass>{}(pass) != passHash(pass)) return false;
    PersistentRenderPassAndFramebuffer framebuffer{&scratch};
    framebuffer.clearColors.emplace_back();
    framebuffer.clearStencil = 5;
    const auto [it, added] = cache.emplace(pass, std::move(framebuffer));
    if (!added || cache.find(lookup) != it || it->second.clearStencil != 5 ||
        cache.get_allocator().resource() != &arena || it->first.get_allocator().resource() != &arena ||
        it->first.subpassGraph.get_allocator().resource() != &arena ||
        it->first.subpassGraph.getSubpass(0).resolvePairs.front().get_allocator().resource() != &arena ||
        it->second.clearColors.get_allocator().resource() != &arena || it->second.clearColors.size() != 1) return false;
    lookup.attachmentIndexMap["color"] = 3;
    lookup.viewport.width = 256;
    lookup.versionName = "debug";
    lookup.version = 4;
    lookup.hashValue = 99;
    lookup.showStatistics = true;
    lookup.subpassGraph.add_edge(0, 0);
    if (cache.find(lookup) != it || lookup != pass || ccstd::hash<RasterPass>{}(lookup) != passHash(pass)) return false;
    if (cache.emplace(lookup, PersistentRenderPassAndFramebuffer{&scratch}).second || cache.size() != 1) return false;
    for (auto member : {&RasterPass::width, &RasterPass::height, &RasterPass::count, &RasterPass::quality}) {
        ++(lookup.*member);
        if (cache.find(lookup) != cache.end() || lookup == pass) return false;
        --(lookup.*member);
    }
    RasterPass moved{std::move(lookup), &scratch};
    return moved == pass && moved.subpassGraph.edge(0, 0).second &&
           moved.subpassGraph.getSubpass(0).resolvePairs.front().get_allocator().resource() == &scratch &&
           cache.find(moved) == it && it->first.subpassGraph.out_degree(0) == 0;
}
