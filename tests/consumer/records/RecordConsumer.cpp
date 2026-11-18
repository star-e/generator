#include "cocos/renderer/pipeline/custom/RenderCommonNames.h"
#include "cocos/renderer/pipeline/custom/RenderCommonSerialization.h"
#include "cocos/renderer/pipeline/custom/BinaryArchive.h"
#include "ShiftProbe.h"
#include <boost/container/pmr/monotonic_buffer_resource.hpp>

#include <array>
#include <cstddef>
#include <fstream>
#include <initializer_list>
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
static_assert(std::is_nothrow_move_assignable_v<ResolvePair> && std::is_copy_assignable_v<ResolvePair>);
static_assert(std::is_nothrow_move_constructible_v<UploadPair> && std::is_nothrow_move_assignable_v<UploadPair>);
static_assert(!std::is_copy_constructible_v<UploadPair> && !std::is_copy_assignable_v<UploadPair>);
static_assert(!std::is_constructible_v<UploadPair, const UploadPair&, const UploadPair::allocator_type&>);
static_assert(std::is_default_constructible_v<cc::MoveProbe> && std::is_move_constructible_v<cc::MoveProbe>);
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

int main(int argc, char** argv) {
    if (argc == 3) return exchangeRecords(argv[1], argv[2]);
    if (argc != 1) return 2;
    using cc::render::getName;
    using namespace std::string_view_literals;
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
