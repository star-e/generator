#include "cocos/renderer/pipeline/custom/BinaryArchive.h"
#include "cocos/renderer/pipeline/custom/LayoutGraphSerialization.h"
#include "cocos/renderer/pipeline/custom/LayoutGraphTypes.h"
#include <stdexcept>
#include <boost/container/pmr/monotonic_buffer_resource.hpp>
#include <fstream>
#include <string_view>
#include <type_traits>

namespace {
using namespace cc::render;
static_assert(std::is_same_v<UniformID, uint32_t>);
static_assert(!std::is_default_constructible_v<UniformBlockData>);
static_assert(std::is_aggregate_v<NameLocalID> && !std::is_convertible_v<uint32_t, NameLocalID>);
static_assert(std::is_convertible_v<NameLocalID, DescriptorData>);
using BlockAlloc = DescriptorBlockData::allocator_type;
static_assert(!std::is_default_constructible_v<DescriptorSetLayoutData>);
static_assert(!std::is_copy_constructible_v<DescriptorSetLayoutData> && !std::is_copy_assignable_v<DescriptorSetLayoutData>);
static_assert(std::is_nothrow_move_constructible_v<DescriptorSetLayoutData> && std::is_nothrow_move_assignable_v<DescriptorSetLayoutData>);
static_assert(!std::is_default_constructible_v<DescriptorBlockData>);
constexpr bool emptyDescriptorsNothrow = noexcept(cc::detail::makeEmptyPmr<decltype(DescriptorBlockData::descriptors)>(
    std::declval<const BlockAlloc&>(), 0U));
static_assert(std::is_nothrow_constructible_v<DescriptorBlockData, DescriptorTypeOrder, cc::gfx::ShaderStageFlagBit,
                                            uint32_t, const BlockAlloc&> == emptyDescriptorsNothrow);
static_assert(std::is_nothrow_constructible_v<DescriptorBlockData, DescriptorTypeOrder, cc::gfx::ShaderStageFlagBit,
                                            uint32_t, cc::gfx::MemoryAccessBit, cc::gfx::ViewDimension,
                                            cc::gfx::SampleType, cc::gfx::Format, const BlockAlloc&> == emptyDescriptorsNothrow);

bool checkDescriptorData() {
    using cc::gfx::Type;
    const NameLocalID id{23};
    ccstd::hash_t expectedHash = 0;
    ccstd::hash_combine(expectedHash, id.value);
    const DescriptorData empty;
    const DescriptorData one(id);
    const DescriptorData two(id, Type::FLOAT4);
    return id == NameLocalID{23} && id != NameLocalID{} && id < NameLocalID{} &&
           ccstd::hash<NameLocalID>{}(id) == expectedHash &&
           empty.descriptorID.value == UINT32_MAX && empty.type == Type::UNKNOWN && empty.count == 1 &&
           one.descriptorID == id && one.type == Type::UNKNOWN && one.count == 1 &&
           two.descriptorID == id && two.type == Type::FLOAT4 && two.count == 1;
}

void populate(LayoutGraph& g) {
    const auto pass = g.add_vertex("main");
    g.value(RenderStage_{}, pass) = RenderPassType::RENDER_PASS;
    g.add_vertex("other");
    const auto phase = g.add_vertex(RenderPhase_{}, "shadow");
    g.add_vertex(RenderPhase_{}, "shadow", 1);
    g.add_vertex("sub", pass);
    if (!g.add_edge(pass, phase).second) throw std::runtime_error("Cannot attach layout phase");
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
DescriptorBlockFlattened flattened(uint32_t size) {
    using cc::gfx::Type;
    DescriptorBlockFlattened value;
    for (uint32_t i = 2 - size; i < 2; ++i) {
        value.descriptorNames.emplace_back(i == 0 ? "z-depth" : "albedo");
        value.uniformBlockNames.emplace_back(i == 0 ? "u-view" : "u-draw");
        value.descriptors.emplace_back(i == 0 ? Type::SAMPLER2D : Type::FLOAT4).count = i + 2;
        auto& uniform = value.uniformBlocks.emplace_back();
        uniform.set = i + 1;
        uniform.binding = i + 7;
        uniform.name = i == 0 ? "View" : "Draw";
        if (i == 0) uniform.members.push_back({"tint", Type::FLOAT4, 2});
        uniform.count = i + 4;
    }
    value.capacity = size * 4;
    value.count = size * 2;
    return value;
}

UniformBlockData uniformBlock(uint32_t size, const UniformBlockData::allocator_type& alloc) {
    UniformBlockData block(alloc);
    block.bufferSize = size * 64;
    if (size == 2) block.uniforms.emplace_back();
    if (size) block.uniforms.emplace_back(0x80000009U, cc::gfx::Type::FLOAT4, 32).size = 16;
    return block;
}

DescriptorBlockData descriptorBlock(uint32_t size, const BlockAlloc& alloc) {
    using namespace cc::gfx;
    if (size == 0) return DescriptorBlockData(alloc);
    const auto type = DescriptorTypeOrder::STORAGE_IMAGE;
    const auto visibility = ShaderStageFlagBit::COMPUTE;
    auto block = size == 1 ? DescriptorBlockData(type, visibility, 31, alloc)
                           : DescriptorBlockData(type, visibility, 31, MemoryAccessBit::WRITE_ONLY, ViewDimension::TEX2D_ARRAY,
                                                 SampleType::UINT, Format::RGBA8, alloc);
    block.offset = size + 5;
    if (size == 2) block.descriptors.emplace_back(NameLocalID{UINT32_MAX}, Type::FLOAT4, 3);
    block.descriptors.emplace_back(NameLocalID{0}, Type::SAMPLER2D, 2);
    return block;
}

bool matches(const DescriptorBlockData& block, const DescriptorBlockData& expected) {
    if (block.type != expected.type || block.visibility != expected.visibility || block.offset != expected.offset ||
        block.capacity != expected.capacity || block.accessType != expected.accessType || block.viewDimension != expected.viewDimension ||
        block.sampleType != expected.sampleType || block.format != expected.format || block.descriptors.size() != expected.descriptors.size()) return false;
    for (size_t i = 0; i < block.descriptors.size(); ++i) {
        const auto& value = block.descriptors[i];
        const auto& want = expected.descriptors[i];
        if (value.descriptorID != want.descriptorID || value.type != want.type || value.count != want.count) return false;
    }
    return true;
}
DescriptorSetLayoutData setLayout(uint32_t size, const BlockAlloc& alloc) {
    DescriptorSetLayoutData layout(alloc);
    if (!size) return layout;
    layout.slot = size;
    layout.capacity = 8 * size;
    layout.uniformBlockCapacity = 3 * size;
    layout.samplerTextureCapacity = 5 * size;
    for (uint32_t i = 1; i <= size; ++i) layout.descriptorBlocks.push_back(descriptorBlock(i, alloc));
    for (uint32_t key : {17U, 42U}) {
        if (size == 1 && key == 17) continue;
        const NameLocalID id{key};
        auto& uniform = layout.uniformBlocks[id];
        uniform.set = size;
        uniform.binding = key;
        uniform.name = "Scene";
        uniform.members.push_back({"tint", cc::gfx::Type::FLOAT4, key});
        uniform.count = 2;
        layout.bindingMap.emplace(id, key + size);
    }
    return layout;
}

bool usesResource(const DescriptorSetLayoutData& layout, const BlockAlloc& alloc) {
    if (layout.get_allocator() != alloc || layout.descriptorBlocks.get_allocator().resource() != alloc.resource() ||
        layout.uniformBlocks.get_allocator().resource() != alloc.resource() || layout.bindingMap.get_allocator().resource() != alloc.resource()) return false;
    for (const auto& block : layout.descriptorBlocks)
        if (block.get_allocator() != alloc || block.descriptors.get_allocator().resource() != alloc.resource()) return false;
    return true;
}

bool checkSetLayoutMoves() {
    boost::container::pmr::monotonic_buffer_resource source, destination;
    auto input = setLayout(2, &source);
    DescriptorSetLayoutData value(7, 16, std::move(input.descriptorBlocks), std::move(input.uniformBlocks), std::move(input.bindingMap), &destination);
    if (!usesResource(value, &destination) || value.uniformBlockCapacity || value.samplerTextureCapacity) return false;
    value.uniformBlockCapacity = 6;
    value.samplerTextureCapacity = 10;
    DescriptorSetLayoutData moved(std::move(value), &source);
    return moved.slot == 7 && moved.capacity == 16 && moved.uniformBlockCapacity == 6 && moved.samplerTextureCapacity == 10 &&
           moved.descriptorBlocks.size() == 2 && matches(moved.descriptorBlocks.back(), descriptorBlock(2, &destination)) &&
           moved.uniformBlocks.at(NameLocalID{42}).binding == 42 && moved.bindingMap.at(NameLocalID{17}) == 19 && usesResource(moved, &source);
}

bool readSetLayouts(BinaryInputArchive& archive, const BlockAlloc& alloc) {
    DescriptorSetLayoutData layout(alloc);
    DescriptorSetLayoutData accumulated(alloc);
    for (uint32_t size : {0U, 1U, 2U, 0U}) {
        load(archive, layout);
        const auto expected = setLayout(size, alloc);
        if (layout.slot != expected.slot || layout.capacity != expected.capacity || layout.uniformBlockCapacity != expected.uniformBlockCapacity ||
            layout.samplerTextureCapacity != expected.samplerTextureCapacity || layout.descriptorBlocks.size() != size || !usesResource(layout, alloc)) return false;
        for (size_t i = 0; i < size; ++i)
            if (!matches(layout.descriptorBlocks[i], expected.descriptorBlocks[i])) return false;
        // The engine's native loader uses emplace, so an earlier value wins for a repeated key.
        accumulated.uniformBlocks.insert(expected.uniformBlocks.begin(), expected.uniformBlocks.end());
        accumulated.bindingMap.insert(expected.bindingMap.begin(), expected.bindingMap.end());
        if (layout.bindingMap != accumulated.bindingMap || layout.uniformBlocks.size() != accumulated.uniformBlocks.size()) return false;
        for (const auto& [key, want] : accumulated.uniformBlocks) {
            const auto& uniform = layout.uniformBlocks.at(key);
            if (uniform.set != want.set || uniform.binding != want.binding || uniform.name != want.name || uniform.count != want.count ||
                uniform.members.size() != 1 || uniform.members[0].name != "tint" ||
                uniform.members[0].type != cc::gfx::Type::FLOAT4 || uniform.members[0].count != key.value) return false;
        }
    }
    return true;
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

bool exchangeBlockArchives(bool writing, const char* path) {
    if (!checkDescriptorData() || !checkSetLayoutMoves()) return false;
    boost::container::pmr::monotonic_buffer_resource arena;
    constexpr uint32_t sizes[] = {0, 2, 1, 0};
    constexpr uint32_t ids[] = {0, 23, UINT32_MAX};
    if (writing) {
        std::ofstream file(path, std::ios::binary);
        BinaryOutputArchive archive(file, &arena);
        for (auto size : sizes) {
            save(archive, flattened(size));
            save(archive, uniformBlock(size, &arena));
            save(archive, descriptorBlock(size, &arena));
        }
        for (auto id : ids) {
            save(archive, NameLocalID{id});
            save(archive, DescriptorData{NameLocalID{id}, cc::gfx::Type::FLOAT4, 3});
        }
        for (uint32_t size : {0U, 1U, 2U, 0U}) save(archive, setLayout(size, &arena));
        file.close();
        return !file.fail();
    }
    std::ifstream file(path, std::ios::binary);
    file.exceptions(std::ios::badbit | std::ios::failbit);
    BinaryInputArchive archive(file, &arena);
    auto value = flattened(2);
    boost::container::pmr::monotonic_buffer_resource destination;
    UniformBlockData block(&destination);
    DescriptorBlockData descriptors(&destination);
    for (auto size : sizes) {
        load(archive, value);
        const auto expected = flattened(size);
        if (value.capacity != expected.capacity || value.count != expected.count ||
            value.descriptorNames != expected.descriptorNames || value.uniformBlockNames != expected.uniformBlockNames ||
            value.descriptors.size() != size || value.uniformBlocks.size() != size) return false;
        for (uint32_t i = 0; i < size; ++i) {
            const auto& uniform = value.uniformBlocks[i];
            const auto& want = expected.uniformBlocks[i];
            if (value.descriptors[i].type != expected.descriptors[i].type || value.descriptors[i].count != expected.descriptors[i].count ||
                uniform.set != want.set || uniform.binding != want.binding || uniform.name != want.name ||
                uniform.count != want.count || uniform.members.size() != want.members.size()) return false;
            if (!want.members.empty() && (uniform.members[0].name != "tint" || uniform.members[0].type != cc::gfx::Type::FLOAT4 ||
                                          uniform.members[0].count != 2)) return false;
        }
        load(archive, block);
        if (block.bufferSize != size * 64 || block.uniforms.size() != size ||
            block.get_allocator().resource() != &destination || block.uniforms.get_allocator().resource() != &destination) return false;
        if (size == 2) {
            const auto& first = block.uniforms.front();
            if (first.uniformID != UINT32_MAX || first.uniformType != cc::gfx::Type::UNKNOWN || first.offset || first.size) return false;
        }
        if (size) {
            const auto& last = block.uniforms.back();
            if (last.uniformID != 0x80000009U || last.uniformType != cc::gfx::Type::FLOAT4 || last.offset != 32 || last.size != 16) return false;
        }
        load(archive, descriptors);
        if (!matches(descriptors, descriptorBlock(size, &arena)) ||
            descriptors.get_allocator().resource() != &destination || descriptors.descriptors.get_allocator().resource() != &destination) return false;
    }
    DescriptorData descriptor;
    NameLocalID name;
    for (auto id : ids) {
        load(archive, name);
        load(archive, descriptor);
        if (name.value != id || descriptor.descriptorID != name || descriptor.type != cc::gfx::Type::FLOAT4 || descriptor.count != 3) return false;
    }
    return readSetLayouts(archive, &destination) && file.peek() == std::char_traits<char>::eof();
}
