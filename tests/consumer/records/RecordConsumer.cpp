#include "cocos/renderer/pipeline/custom/LayoutGraphTypes.h"
#include "cocos/renderer/pipeline/custom/RenderCommonNames.h"
#include "cocos/renderer/pipeline/custom/RenderCommonSerialization.h"
#include "cocos/renderer/pipeline/custom/BinaryArchive.h"
#include "ShiftProbe.h"
#include "ShiftSerialization.h"
#include "cocos/base/std/container/map.h"
#include <boost/container/pmr/monotonic_buffer_resource.hpp>

#include <array>
#include <cstddef>
#include <fstream>
#include <initializer_list>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_set>
#include <utility>

using cc::render::AccessType;
using cc::render::AttachmentType;
using cc::render::ClearValueType;
using cc::render::CopyPair;
using cc::render::LightInfo;
using cc::render::LightingMode;
using cc::render::MovePair;
using cc::render::ParameterType;
using cc::render::PipelineStatistics;
using cc::render::QueueHint;
using cc::render::ResolveFlags;
using cc::render::ResolvePair;
using cc::render::ResourceDimension;
using cc::render::ResourceFlags;
using cc::render::ResourceResidency;
using cc::render::SceneFlags;
using cc::render::TaskType;
using cc::render::UpdateFrequency;
using cc::render::UploadPair;

static_assert(std::is_aggregate_v<PipelineStatistics>);
constexpr PipelineStatistics stats{};
static_assert(stats.numRenderPasses == 0 && stats.numManagedTextures == 0 && stats.totalManagedTextures == 0 &&
              stats.numUploadBuffers == 0 && stats.numUploadBufferViews == 0 && stats.numFreeUploadBuffers == 0 &&
              stats.numFreeUploadBufferViews == 0 && stats.numDescriptorSets == 0 && stats.numFreeDescriptorSets == 0 &&
              stats.numInstancingBuffers == 0 && stats.numInstancingUniformBlocks == 0);

template<class Enum>
constexpr bool hasSequentialValues(std::initializer_list<Enum> members) {
    static_assert(std::is_same_v<std::underlying_type_t<Enum>, uint8_t>);
    unsigned expected = 0;
    for (auto member : members) {
        if (static_cast<unsigned>(member) != expected++) return false;
    }
    return true;
}

static_assert(hasSequentialValues({UpdateFrequency::PER_INSTANCE, UpdateFrequency::PER_BATCH, UpdateFrequency::PER_PHASE,
                                   UpdateFrequency::PER_PASS, UpdateFrequency::COUNT}));
using cc::render::DescriptorTypeOrder;
using cc::render::LayoutType;
using cc::render::RenderPassType;
static_assert(hasSequentialValues({LayoutType::VULKAN, LayoutType::WEBGPU}));
static_assert(hasSequentialValues({DescriptorTypeOrder::UNIFORM_BUFFER, DescriptorTypeOrder::DYNAMIC_UNIFORM_BUFFER,
                                   DescriptorTypeOrder::SAMPLER_TEXTURE, DescriptorTypeOrder::SAMPLER, DescriptorTypeOrder::TEXTURE,
                                   DescriptorTypeOrder::STORAGE_BUFFER, DescriptorTypeOrder::DYNAMIC_STORAGE_BUFFER,
                                   DescriptorTypeOrder::STORAGE_IMAGE, DescriptorTypeOrder::INPUT_ATTACHMENT}));
static_assert(hasSequentialValues({RenderPassType::SINGLE_RENDER_PASS, RenderPassType::RENDER_PASS, RenderPassType::RENDER_SUBPASS}));
static_assert(hasSequentialValues({ParameterType::CONSTANTS, ParameterType::CBV, ParameterType::UAV,
                                   ParameterType::SRV, ParameterType::TABLE, ParameterType::SSV}));
static_assert(hasSequentialValues({ResourceResidency::MANAGED, ResourceResidency::MEMORYLESS, ResourceResidency::PERSISTENT,
                                   ResourceResidency::EXTERNAL, ResourceResidency::BACKBUFFER, ResourceResidency::CUSTOM}));
static_assert(hasSequentialValues({QueueHint::NONE, QueueHint::OPAQUE, QueueHint::MASK, QueueHint::BLEND}));
static_assert(QueueHint::RENDER_OPAQUE == QueueHint::OPAQUE);
static_assert(QueueHint::RENDER_CUTOUT == QueueHint::MASK);
static_assert(QueueHint::RENDER_TRANSPARENT == QueueHint::BLEND);
static_assert(hasSequentialValues({ResourceDimension::BUFFER, ResourceDimension::TEXTURE1D,
                                   ResourceDimension::TEXTURE2D, ResourceDimension::TEXTURE3D}));
static_assert(hasSequentialValues({TaskType::SYNC, TaskType::ASYNC}));
static_assert(hasSequentialValues({LightingMode::NONE, LightingMode::DEFAULT, LightingMode::CLUSTERED}));
static_assert(hasSequentialValues({AttachmentType::RENDER_TARGET, AttachmentType::DEPTH_STENCIL, AttachmentType::SHADING_RATE}));
static_assert(hasSequentialValues({AccessType::READ, AccessType::READ_WRITE, AccessType::WRITE}));
static_assert(hasSequentialValues({ClearValueType::NONE, ClearValueType::FLOAT_TYPE, ClearValueType::INT_TYPE}));
static_assert(std::is_same_v<std::underlying_type_t<ResourceFlags>, uint32_t>);
constexpr bool checkResourceFlags() {
    auto usage = ResourceFlags::COLOR_ATTACHMENT | ResourceFlags::SAMPLED;
    if (static_cast<uint32_t>(usage) != 0x18 || !any(usage & ResourceFlags::SAMPLED)) return false;
    if (any(usage & ResourceFlags::STORAGE) || !usage) return false;
    static_assert(!ResourceFlags::NONE && !any(ResourceFlags::NONE));
    if (&(usage |= ResourceFlags::STORAGE) != &usage || static_cast<uint32_t>(usage) != 0x1C) return false;
    if (&(usage &= ~ResourceFlags::SAMPLED) != &usage || static_cast<uint32_t>(usage) != 0x14) return false;
    return static_cast<uint32_t>(~ResourceFlags::NONE) == UINT32_MAX;
}
static_assert(checkResourceFlags());
static_assert(std::is_same_v<std::underlying_type_t<SceneFlags>, uint32_t>);
static_assert(SceneFlags::OPAQUE_OBJECT == SceneFlags::OPAQUE);
static_assert(SceneFlags::CUTOUT_OBJECT == SceneFlags::MASK);
static_assert(SceneFlags::TRANSPARENT_OBJECT == SceneFlags::BLEND);
static_assert(static_cast<uint32_t>(SceneFlags::ALL) == UINT32_MAX);
static_assert(static_cast<uint32_t>(SceneFlags::REFLECTION_PROBE | SceneFlags::OPAQUE_OBJECT) == 0x2001);
static_assert(std::is_same_v<std::underlying_type_t<ResolveFlags>, uint32_t>);
static_assert(!ResolveFlags::NONE && static_cast<uint32_t>(ResolveFlags::COLOR) == 1);
static_assert(static_cast<uint32_t>(ResolveFlags::DEPTH) == 2 && static_cast<uint32_t>(ResolveFlags::STENCIL) == 4);
constexpr auto depthStencil = ResolveFlags::DEPTH | ResolveFlags::STENCIL;
static_assert(static_cast<uint32_t>(depthStencil) == 6 && !any(depthStencil & ResolveFlags::COLOR));
static_assert(static_cast<uint32_t>(cc::ShiftProbe::HIGH) == 0x80000000U);
static_assert(std::is_same_v<decltype(LightInfo::light), cc::IntrusivePtr<cc::scene::Light>>);
static_assert(std::is_same_v<decltype(LightInfo::probe), cc::scene::ReflectionProbe*>);
static_assert(std::is_nothrow_move_constructible_v<ResolvePair> && !std::is_copy_constructible_v<ResolvePair>);
static_assert(!std::is_nothrow_constructible_v<ResolvePair, decltype(ResolvePair::source), decltype(ResolvePair::target),
                                               ResolveFlags, cc::gfx::ResolveMode, cc::gfx::ResolveMode, const ResolvePair::allocator_type&>);
using CtorProbe = cc::AllocatorCtorProbe;
using ProbeString = decltype(CtorProbe::countIn);
using ProbeAlloc = const CtorProbe::allocator_type&;
static_assert(std::is_nothrow_constructible_v<CtorProbe, uint32_t, ProbeAlloc> ==
              noexcept(cc::detail::makeEmptyPmr<ProbeString>(std::declval<ProbeAlloc>(), "")));
static_assert(std::is_nothrow_constructible_v<CtorProbe, uint32_t, ProbeString, ProbeAlloc> ==
              std::is_nothrow_constructible_v<ProbeString, ProbeString&&, ProbeAlloc>);
static_assert(std::is_nothrow_move_assignable_v<ResolvePair> && std::is_copy_assignable_v<ResolvePair>);
static_assert(std::is_nothrow_move_constructible_v<UploadPair> && std::is_nothrow_move_assignable_v<UploadPair>);
static_assert(!std::is_copy_constructible_v<UploadPair> && !std::is_copy_assignable_v<UploadPair>);
static_assert(!std::is_constructible_v<UploadPair, const UploadPair&, const UploadPair::allocator_type&>);
static_assert(std::is_default_constructible_v<cc::MoveProbe> && std::is_move_constructible_v<cc::MoveProbe>);
static_assert(std::is_copy_constructible_v<cc::TextProbe>);
static_assert(!std::is_copy_constructible_v<cc::MoveProbe> && !std::is_copy_assignable_v<cc::MoveProbe>);

namespace pmr = boost::container::pmr;

template<class Pair>
bool uses(const Pair& pair, pmr::memory_resource* resource) {
    return pair.get_allocator().resource() == resource && pair.source.get_allocator().resource() == resource &&
           pair.target.get_allocator().resource() == resource;
}

bool checkResolvePair() {
    using cc::gfx::ResolveMode;
    pmr::monotonic_buffer_resource first, second;
    ResolvePair defaults;
    ResolvePair empty(&first);
    if (!uses(defaults, pmr::get_default_resource()) || !uses(empty, &first) || defaults != empty ||
        !empty.source.empty() || !empty.target.empty() || empty.resolveFlags != ResolveFlags::NONE ||
        empty.mode != ResolveMode::SAMPLE_ZERO || empty.mode1 != ResolveMode::SAMPLE_ZERO) return false;

    ccstd::pmr::string source(96, 's', &first), target(80, 't', &first);
    ResolvePair original(source, target, depthStencil, ResolveMode::MIN, ResolveMode::MAX, &second);
    if (!uses(original, &second) || original.source != source || original.target != target ||
        original.resolveFlags != depthStencil || original.mode != ResolveMode::MIN || original.mode1 != ResolveMode::MAX) return false;
    ResolvePair copied(original, &first);
    if (!uses(copied, &first) || copied != original) return false;
    if (ccstd::hash<ResolvePair>{}(copied) != ccstd::hash<ResolvePair>{}(original)) return false;
    std::unordered_set<ResolvePair, ccstd::hash<ResolvePair>> keys;
    keys.emplace(original, &second);
    if (keys.find(copied) == keys.end()) return false;
    ResolvePair moved(std::move(copied), &second);
    if (!uses(moved, &second) || moved != original) return false;
    empty = moved;
    if (!uses(empty, &first) || empty != original) return false;
    empty.mode1 = ResolveMode::SAMPLE_ZERO;
    return empty != original;
}

auto subresources(const CopyPair& p) {
    return std::array{p.mipLevels, p.numSlices, p.sourceMostDetailedMip, p.sourceFirstSlice,
                      p.sourcePlaneSlice, p.targetMostDetailedMip, p.targetFirstSlice, p.targetPlaneSlice};
}

auto subresources(const MovePair& p) {
    return std::array{p.mipLevels, p.numSlices, p.targetMostDetailedMip, p.targetFirstSlice, p.targetPlaneSlice};
}

template<class Pair, class... Indices>
bool checkSubresources(Indices... indices) {
    pmr::monotonic_buffer_resource first, second;
    Pair empty(&first);
    const std::array<uint32_t, sizeof...(indices)> defaults{UINT32_MAX, UINT32_MAX};
    if (!uses(empty, &first) || !empty.source.empty() || !empty.target.empty() || subresources(empty) != defaults) return false;

    const ccstd::pmr::string source(96, 's', &first), target(80, 't', &first);
    const std::array<uint32_t, sizeof...(indices)> expected{indices...};
    auto matches = [&](const Pair& p, pmr::memory_resource* resource) {
        return uses(p, resource) && p.source == source && p.target == target && subresources(p) == expected;
    };
    Pair original(source, target, indices..., &second);
    if (!matches(original, &second)) return false;
    Pair copied(original, &first);
    if (!matches(copied, &first)) return false;
    Pair moved(std::move(copied), &second);
    return matches(moved, &second);
}

bool checkUploadPair() {
    pmr::monotonic_buffer_resource first, second;
    UploadPair empty(&first);
    if (empty.get_allocator().resource() != &first || !empty.source.empty() || !empty.target.empty() ||
        empty.mipLevels != UINT32_MAX || empty.numSlices != UINT32_MAX || empty.targetMostDetailedMip != 0 ||
        empty.targetFirstSlice != 0 || empty.targetPlaneSlice != 0) return false;

    const ccstd::vector<uint8_t> expected{0, 127, 255};
    auto bytes = expected;
    const auto* storage = bytes.data();
    const ccstd::pmr::string target(96, 't', &first);
    auto matches = [&](const UploadPair& p, pmr::memory_resource* resource) {
        return p.source.data() == storage && p.source == expected && p.target == target &&
               p.get_allocator().resource() == resource && p.target.get_allocator().resource() == resource &&
               p.mipLevels == 1 && p.numSlices == 2 && p.targetMostDetailedMip == 3 &&
               p.targetFirstSlice == 4 && p.targetPlaneSlice == 5;
    };
    UploadPair original(std::move(bytes), target, 1, 2, 3, 4, 5, &first);
    if (!matches(original, &first)) return false;
    UploadPair moved(std::move(original), &second);
    return matches(moved, &second);
}

int exchangeRecords(std::string_view command, const char* path) {
    using cc::gfx::ResolveMode;
    pmr::monotonic_buffer_resource memory;
    if (command == "write") {
        std::ofstream file(path, std::ios::binary);
        cc::render::BinaryOutputArchive archive(file, &memory);
        save(archive, LightInfo{nullptr, 77, true, nullptr});
        save(archive, ResolvePair{"color-msaa", "color", depthStencil, ResolveMode::MIN, ResolveMode::MAX, &memory});
        save(archive, CopyPair{"copy-src", "copy-dst", 1, 2, 3, 4, 5, 6, 7, 8, &memory});
        save(archive, MovePair{"move-src", "move-dst", 11, 12, 13, 14, 15, &memory});
        save(archive, PipelineStatistics{21, 22, 23, 24, 25, 26, 27, 28, 29, 30, UINT32_MAX});
        save(archive, cc::TextProbe{"plain text", "caption"});
        cc::ArrayArchiveProbe arrays;
        arrays.rows = {{true, false}, {}, {false, true, true}};
        save(archive, arrays);
        file.close();
        return file.good() ? 0 : 1;
    }
    if (command != "read") return 2;
    std::ifstream file(path, std::ios::binary);
    if (!file) return 1;
    cc::render::BinaryInputArchive archive(file, &memory);
    LightInfo light;
    alignas(std::max_align_t) std::byte token{};
    auto* probe = reinterpret_cast<cc::scene::ReflectionProbe*>(&token);
    light.probe = probe;
    ResolvePair resolve(&memory);
    CopyPair copy(&memory);
    MovePair move(&memory);
    PipelineStatistics statistics{};
    load(archive, light);
    load(archive, resolve);
    load(archive, copy);
    load(archive, move);
    load(archive, statistics);
    cc::TextProbe text;
    load(archive, text);
    cc::ArrayArchiveProbe arrays;
    load(archive, arrays);
    if (arrays.rows != decltype(arrays.rows){{true, false}, {}, {false, true, true}}) return 9;
    if (text.text != "plain text" || text.label != "caption") return 8;
    if (light.level != 77 || !light.culledByLight || light.probe != probe) return 3;
    if (resolve.source != "color-msaa" || resolve.target != "color" || resolve.resolveFlags != depthStencil ||
        resolve.mode != ResolveMode::MIN || resolve.mode1 != ResolveMode::MAX) return 4;
    if (copy.source != "copy-src" || copy.target != "copy-dst" ||
        subresources(copy) != std::array<uint32_t, 8>{1, 2, 3, 4, 5, 6, 7, 8}) return 5;
    if (move.source != "move-src" || move.target != "move-dst" ||
        subresources(move) != std::array<uint32_t, 5>{11, 12, 13, 14, 15}) return 6;
    const std::array counts{statistics.numRenderPasses, statistics.numManagedTextures, statistics.totalManagedTextures,
        statistics.numUploadBuffers, statistics.numUploadBufferViews, statistics.numFreeUploadBuffers,
        statistics.numFreeUploadBufferViews, statistics.numDescriptorSets, statistics.numFreeDescriptorSets,
        statistics.numInstancingBuffers, statistics.numInstancingUniformBlocks};
    if (counts != std::array<uint32_t, 11>{21, 22, 23, 24, 25, 26, 27, 28, 29, 30, UINT32_MAX}) return 7;
    return file.good() && file.peek() == std::char_traits<char>::eof() ? 0 : 1;
}

bool checkDescriptorBlockIndex() {
    using cc::gfx::ShaderStageFlagBit;
    using cc::render::DescriptorBlockIndex;
    const DescriptorBlockIndex base{};
    const DescriptorBlockIndex equivalent{UpdateFrequency::PER_INSTANCE, ParameterType::CONSTANTS,
                                          DescriptorTypeOrder::UNIFORM_BUFFER, ShaderStageFlagBit::NONE};
    const DescriptorBlockIndex visibility{base.updateFrequency, base.parameterType, base.descriptorType, ShaderStageFlagBit::FRAGMENT};
    const DescriptorBlockIndex descriptor{base.updateFrequency, base.parameterType, DescriptorTypeOrder::SAMPLER, base.visibility};
    const DescriptorBlockIndex parameter{base.updateFrequency, ParameterType::CBV, base.descriptorType, base.visibility};
    const DescriptorBlockIndex frequency{UpdateFrequency::PER_BATCH, base.parameterType, base.descriptorType, base.visibility};

    ccstd::pmr::map<DescriptorBlockIndex, int> blocks{
        {frequency, 4}, {visibility, 1}, {parameter, 3}, {base, 0}, {descriptor, 2}};
    if (blocks.size() != 5 || blocks.emplace(equivalent, 5).second || blocks.at(equivalent) != 0) return false;
    int expected = 0;
    for (const auto& block : blocks) {
        if (block.second != expected++) return false;
    }
    return true;
}

bool checkDescriptorGroupBlockIndex() {
    using cc::gfx::Format;
    using cc::gfx::MemoryAccessBit;
    using cc::gfx::SampleType;
    using cc::gfx::ShaderStageFlagBit;
    using cc::gfx::ViewDimension;
    using cc::render::DescriptorGroupBlockIndex;
    // Change one field at a time, from the least significant to the most significant.
    std::array<DescriptorGroupBlockIndex, 9> keys{};
    keys[1].format = Format::ASTC_SRGBA_12X12;
    keys[2].sampleType = SampleType::UINT;
    keys[3].viewDimension = ViewDimension::TEX2D_ARRAY;
    keys[4].accessType = MemoryAccessBit::READ_WRITE;
    keys[5].visibility = ShaderStageFlagBit::FRAGMENT;
    keys[6].descriptorType = DescriptorTypeOrder::SAMPLER;
    keys[7].parameterType = ParameterType::UAV;
    keys[8].updateFrequency = UpdateFrequency::PER_PASS;

    ccstd::pmr::map<DescriptorGroupBlockIndex, size_t> blocks;
    for (size_t i = keys.size(); i > 0; --i) {
        if (!blocks.emplace(keys[i - 1], i - 1).second) return false;
    }
    const DescriptorGroupBlockIndex equivalent{UpdateFrequency::PER_INSTANCE, ParameterType::CONSTANTS,
        DescriptorTypeOrder::UNIFORM_BUFFER, ShaderStageFlagBit::NONE, MemoryAccessBit::READ_ONLY,
        ViewDimension::UNKNOWN, SampleType::FLOAT, Format::UNKNOWN};
    const auto found = blocks.find(equivalent);
    if (found == blocks.end() || found->second != 0) return false;
    size_t expected = 0;
    for (const auto& block : blocks) {
        if (block.second != expected++) return false;
    }
    return true;
}

bool checkDescriptorDB() {
    using cc::render::DescriptorBlockIndex;
    using cc::render::DescriptorDB;
    using cc::render::DescriptorGroupBlockIndex;
    static_assert(!std::is_default_constructible_v<DescriptorDB> && !std::is_copy_constructible_v<DescriptorDB>);
    using Alloc = const DescriptorDB::allocator_type&;
    static_assert(std::is_nothrow_constructible_v<DescriptorDB, Alloc> ==
                  (std::is_nothrow_constructible_v<decltype(DescriptorDB::blocks), Alloc> &&
                   std::is_nothrow_constructible_v<decltype(DescriptorDB::groupBlocks), Alloc>));
    static_assert(std::is_nothrow_move_constructible_v<DescriptorDB> && std::is_copy_assignable_v<DescriptorDB>);
    pmr::monotonic_buffer_resource first, second;
    auto usesResource = [](const DescriptorDB& db, pmr::memory_resource& resource) {
        return db.get_allocator().resource() == &resource && db.blocks.get_allocator().resource() == &resource &&
               db.groupBlocks.get_allocator().resource() == &resource;
    };
    cc::NestedProbe owner{&first};
    if (owner.pair.get_allocator().resource() != &first) return false;
    DescriptorDB original{&first};
    if (!usesResource(original, first) || !original.blocks.empty() || !original.groupBlocks.empty()) return false;
    original.blocks[DescriptorBlockIndex{}].count = 3;
    original.groupBlocks[DescriptorGroupBlockIndex{}].capacity = 5;
    DescriptorDB copy{original, &second};
    if (!usesResource(copy, second)) return false;
    original.blocks.clear();
    original.groupBlocks.clear();
    DescriptorDB moved{std::move(copy), &first};
    return usesResource(moved, first) && moved.blocks.at(DescriptorBlockIndex{}).count == 3 &&
           moved.groupBlocks.at(DescriptorGroupBlockIndex{}).capacity == 5;
}

bool checkRenderPhase() {
    using cc::render::RenderPhase;
    using Shaders = decltype(RenderPhase::shaders);
    using Alloc = const RenderPhase::allocator_type&;
    static_assert(std::is_same_v<Shaders, cc::PmrTransparentSet<ccstd::pmr::string>>);
    static_assert(!std::is_default_constructible_v<RenderPhase>);
    static_assert(std::is_nothrow_constructible_v<RenderPhase, Alloc> == std::is_nothrow_constructible_v<Shaders, Alloc>);
    constexpr std::string_view vertex = "forward/opaque/lighting/vertex";
    constexpr std::string_view fragment = "forward/opaque/lighting/fragment";
    pmr::monotonic_buffer_resource first, second;
    auto matches = [&](const RenderPhase& phase, pmr::memory_resource& resource) {
        if (phase.get_allocator().resource() != &resource || phase.shaders.get_allocator().resource() != &resource ||
            phase.shaders.size() != 2 || phase.shaders.find(vertex) == phase.shaders.end() ||
            phase.shaders.find(fragment) == phase.shaders.end()) return false;
        for (const auto& shader : phase.shaders) {
            if (shader.get_allocator().resource() != &resource) return false;
        }
        return true;
    };
    RenderPhase original{&first};
    if (!original.shaders.empty() || !original.shaders.emplace(vertex).second ||
        !original.shaders.emplace(fragment).second || original.shaders.emplace(vertex).second ||
        !matches(original, first)) return false;
    RenderPhase copy{original, &second};
    original.shaders.clear();
    if (!matches(copy, second)) return false;
    RenderPhase moved{std::move(copy), &first};
    return matches(moved, first);
}

bool checkLayoutGraph(const char* pathSamples);
bool checkLayoutNames();
int checkStageAllocation(unsigned count, int failAfter);
bool writeDfsCases(const char* path);
bool exchangeLayoutArchive(std::string_view mode, const char* path);
bool exchangeBlockArchives(bool writing, const char* path);
bool exchangePipelineArchives(bool writing, const char* path);
bool readConvertedLayout(const char* path);
bool printLayoutArchive(const char* inputPath, const char* outputPath);
bool checkLayoutDescriptors(const char* inputPath, const char* outputPath);
bool checkResourceGraph();
bool checkSubpassGraph();
bool checkRasterPassCache();
bool checkPassRecords();

int main(int argc, char** argv) {
    if (argc == 4 && std::string_view(argv[1]) == "layout-descriptors")
        return checkLayoutDescriptors(argv[2], argv[3]) ? 0 : 1;
    if (argc == 4 && std::string_view(argv[1]) == "layout-print")
        return printLayoutArchive(argv[2], argv[3]) ? 0 : 1;
    if (argc == 4 && std::string_view(argv[1]) == "stage-allocation")
        return checkStageAllocation(std::stoul(argv[2]), std::stoi(argv[3]));
    if (argc == 3) {
        const std::string_view mode(argv[1]);
        if (mode == "layout-write" || mode == "layout-read" || mode == "layout-invalid")
            return exchangeLayoutArchive(mode, argv[2]) ? 0 : 1;
        if (mode == "blocks-write" || mode == "blocks-read")
            return exchangeBlockArchives(mode == "blocks-write", argv[2]) ? 0 : 1;
        if (mode == "pipeline-write" || mode == "pipeline-read")
            return exchangePipelineArchives(mode == "pipeline-write", argv[2]) ? 0 : 1;
        if (mode == "converted-layout-read") return readConvertedLayout(argv[2]) ? 0 : 1;
    }
    if (argc == 3 && std::string_view(argv[1]) == "dfs-cases") return writeDfsCases(argv[2]) ? 0 : 1;
    const bool writePathCases = argc == 3 && std::string_view(argv[1]) == "path-cases";
    if (argc == 3 && !writePathCases) return exchangeRecords(argv[1], argv[2]);
    if (argc != 1 && !writePathCases) return 2;
    if (!checkLayoutGraph(writePathCases ? argv[2] : nullptr)) return 9;
    if (!checkLayoutNames()) return 17;
    if (!checkResourceGraph()) return 18;
    if (!checkSubpassGraph()) return 19;
    if (!checkRasterPassCache()) return 20;
    if (!checkPassRecords()) return 21;
    if (!checkDescriptorBlockIndex()) return 10;
    if (!checkDescriptorGroupBlockIndex()) return 12;
    if (!checkDescriptorDB()) return 15;
    if (!checkRenderPhase()) return 16;
    cc::render::DescriptorBlock block;
    if (!block.descriptors.empty() || !block.uniformBlocks.empty() || block.capacity != 0 || block.count != 0) return 14;
    block.descriptors.try_emplace("buffer", cc::gfx::Type::FLOAT4);
    block.uniformBlocks["camera"].name = "CameraBlock";
    if (block.descriptors.at("buffer").type != cc::gfx::Type::FLOAT4 ||
        block.uniformBlocks.at("camera").name != "CameraBlock") return 14;
    cc::render::DescriptorBlockFlattened flat;
    flat.descriptorNames.emplace_back("buffer");
    flat.uniformBlockNames.emplace_back("camera");
    flat.descriptors.emplace_back(cc::gfx::Type::FLOAT4);
    flat.uniformBlocks.emplace_back().name = flat.uniformBlockNames.front();
    const auto copy = flat;
    flat.uniformBlocks.clear();
    if (copy.uniformBlocks.front().name != "camera" || copy.descriptorNames.front() != "buffer" ||
        copy.descriptors.front().type != cc::gfx::Type::FLOAT4 || copy.capacity != 0 || copy.count != 0) return 13;
    const cc::ComparisonProbe first{1, 9, 3}, second{1, 0, 4}, same{1, 2, 3};
    if (!(first == same && first != second && first < second && second > first) || first > same || first < same) return 11;
    using cc::render::getName;
    using namespace std::string_view_literals;
    using cc::render::Descriptor;
    static_assert(std::is_convertible_v<cc::gfx::Type, Descriptor>);
    const Descriptor defaultDescriptor;
    const Descriptor sampler = cc::gfx::Type::SAMPLER;
    if (defaultDescriptor.type != cc::gfx::Type::UNKNOWN || defaultDescriptor.count != 1 ||
        sampler.type != cc::gfx::Type::SAMPLER || sampler.count != 1) return 8;
    if (getName(QueueHint::RENDER_OPAQUE) != "OPAQUE"sv || getName(static_cast<QueueHint>(255)) != ""sv ||
        getName(cc::render::RasterPassTag{}) != "RasterPass"sv || getName(stats) != "PipelineStatistics"sv) return 7;
    if (!checkUploadPair()) return 6;
    if (!checkResolvePair()) return 3;
    if (!checkSubresources<CopyPair>(1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U)) return 4;
    if (!checkSubresources<MovePair>(1U, 2U, 3U, 4U, 5U)) return 5;
    LightInfo empty;
    if (empty.light.get() || empty.probe || empty.level != 0 || empty.culledByLight) return 1;

    LightInfo shortForm(nullptr, 7);
    if (shortForm.light.get() || shortForm.probe || shortForm.level != 7 || shortForm.culledByLight) return 2;

    // The generated type stores an opaque probe pointer and never dereferences it.
    alignas(std::max_align_t) std::byte token{};
    auto* probe = reinterpret_cast<cc::scene::ReflectionProbe*>(&token);
    LightInfo full(nullptr, 42, true, probe);
    return full.light.get() || full.probe != probe || full.level != 42 || !full.culledByLight;
}
