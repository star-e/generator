#include "cocos/renderer/pipeline/custom/BinaryArchive.h"
#include "cocos/renderer/pipeline/custom/LayoutGraphSerialization.h"
#include <boost/container/pmr/monotonic_buffer_resource.hpp>
#include <fstream>
#include <string_view>

namespace {
using namespace cc::render;

void populate(LayoutGraph& g) {
    const auto pass = g.add_vertex("main");
    g.value(RenderStage_{}, pass) = RenderPassType::RENDER_PASS;
    g.add_vertex("other");
    const auto phase = g.add_vertex(RenderPhase_{}, "shadow", pass);
    g.add_vertex(RenderPhase_{}, "shadow", 1);
    g.add_vertex("sub", pass);
    auto& shaders = g.value(RenderPhase_{}, phase).shaders;
    shaders.emplace("alpha");
    shaders.emplace("zeta");
    auto& db = g.getDescriptors(phase);
    DescriptorBlockIndex index;
    index.updateFrequency = UpdateFrequency::PER_PASS;
    auto& block = db.blocks[index];
    block.count = 3;
    block.capacity = 5;
    auto& descriptor = block.descriptors["atlas"];
    descriptor.type = cc::gfx::Type::SAMPLER2D;
    descriptor.count = 3;
    auto& uniform = block.uniformBlocks["scene"];
    uniform.set = 1;
    uniform.binding = 7;
    uniform.name = "Scene";
    uniform.members.push_back({"tint", cc::gfx::Type::FLOAT4, 2});
    uniform.count = 4;
    auto group = DescriptorGroupBlockIndex{};
    group.updateFrequency = UpdateFrequency::PER_PHASE;
    group.format = cc::gfx::Format::RGBA8;
    db.groupBlocks[group].capacity = 9;
}

bool matches(const LayoutGraph& g) {
    if (g.num_vertices() != 5 || g.locate("/main/shadow") != 2 || g.locate("/other/shadow") != 3 ||
        g.locate("/main/sub") != 4 || g.get_parent(1) != g.null_vertex() || g.out_degree(0) != 2 ||
        g.value(RenderStage_{}, 0) != RenderPassType::RENDER_PASS || !g.holds<RenderPhase_>(3)) return false;
    const auto& phase = g.value(RenderPhase_{}, 2);
    const auto& db = g.getDescriptors(2);
    if (phase.shaders.size() != 2 || !phase.shaders.count("alpha") || !phase.shaders.count("zeta") ||
        db.blocks.size() != 1 || db.groupBlocks.size() != 1 || db.blocks.begin()->first.updateFrequency != UpdateFrequency::PER_PASS ||
        db.groupBlocks.begin()->first.format != cc::gfx::Format::RGBA8 || db.groupBlocks.begin()->second.capacity != 9) return false;
    const auto& block = db.blocks.begin()->second;
    const auto& descriptor = block.descriptors.at("atlas");
    const auto& uniform = block.uniformBlocks.at("scene");
    return block.count == 3 && block.capacity == 5 && descriptor.count == 3 && descriptor.type == cc::gfx::Type::SAMPLER2D &&
           uniform.set == 1 && uniform.binding == 7 && uniform.name == "Scene" && uniform.count == 4 && uniform.members.size() == 1 &&
           uniform.members[0].name == "tint" && uniform.members[0].type == cc::gfx::Type::FLOAT4 && uniform.members[0].count == 2 &&
           db.get_allocator().resource() == g.get_allocator().resource() && phase.get_allocator().resource() == g.get_allocator().resource();
}
} // namespace

bool exchangeLayoutArchive(std::string_view mode, const char* path) {
    boost::container::pmr::monotonic_buffer_resource arena;
    LayoutGraph g{&arena};
    if (mode == "layout-write") {
        std::ofstream file(path, std::ios::binary);
        BinaryOutputArchive archive(file, &arena);
        save(archive, g);
        populate(g);
        save(archive, g);
        file.close();
        return !file.fail();
    }
    std::ifstream file(path, std::ios::binary);
    file.exceptions(std::ios::badbit | std::ios::failbit);
    BinaryInputArchive archive(file, &arena);
    g.add_vertex("previous");
    if (mode == "layout-invalid") {
        try { load(archive, g); }
        catch (const std::runtime_error&) { return g.num_vertices() == 1 && g.locate("/previous") == 0; }
        return false;
    }
    load(archive, g);
    if (g.num_vertices()) return false;
    load(archive, g);
    return matches(g) && file.peek() == std::char_traits<char>::eof();
}
