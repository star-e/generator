#include "cocos/renderer/pipeline/custom/BinaryArchive.h"
#include "cocos/renderer/pipeline/custom/LayoutGraphSerialization.h"
#include "cocos/renderer/pipeline/custom/LayoutGraphTypes.h"
#include "gfx-empty/EmptyDescriptorSet.h"
#include "gfx-empty/EmptyDescriptorSetLayout.h"
#include "gfx-empty/EmptyPipelineLayout.h"
#include <boost/container/pmr/monotonic_buffer_resource.hpp>
#include <fstream>
#include <type_traits>

namespace {
using namespace cc::render;
using namespace cc::gfx;
using Alloc = DescriptorSetData::allocator_type;
constexpr auto pass = UpdateFrequency::PER_PASS;
constexpr auto phase = UpdateFrequency::PER_PHASE;
constexpr NameLocalID id{42};

template<class T>
constexpr bool moveOnlyLayout = !std::is_default_constructible_v<T> && !std::is_copy_constructible_v<T> &&
    !std::is_copy_assignable_v<T> && std::is_nothrow_move_constructible_v<T> && std::is_nothrow_move_assignable_v<T> &&
    std::is_constructible_v<T, T&&, const Alloc&>;
static_assert(moveOnlyLayout<DescriptorSetData> && moveOnlyLayout<PipelineLayoutData> && moveOnlyLayout<ShaderBindingData> &&
              moveOnlyLayout<ShaderLayoutData> && moveOnlyLayout<TechniqueData> && moveOnlyLayout<EffectData> &&
              moveOnlyLayout<ShaderProgramData> && moveOnlyLayout<RenderStageData> && moveOnlyLayout<RenderPhaseData>);

void fillSet(DescriptorSetData& set, uint32_t slot) {
    set.descriptorSetLayoutData.slot = slot;
    set.descriptorSetLayoutData.bindingMap.emplace(id, slot + 1);
    auto& binding = set.descriptorSetLayoutInfo.bindings.emplace_back();
    binding.binding = slot;
    binding.count = 2;
    binding.stageFlags = ShaderStageFlagBit::COMPUTE;
    binding.descriptorType = DescriptorType::UNIFORM_BUFFER;
}

void fillProgram(ShaderProgramData& program) {
    fillSet(program.layout.descriptorSets[pass], 3);
    fillSet(program.layout.descriptorGroups[pass], 9);
}

struct Records {
    explicit Records(const Alloc& alloc) : set(alloc), program(alloc), effect(alloc), stage(alloc), renderPhase(alloc) {}
    DescriptorSetData set;
    ShaderProgramData program;
    EffectData effect;
    RenderStageData stage;
    RenderPhaseData renderPhase;
};

void populate(Records& records) {
    fillSet(records.set, 7);
    fillProgram(records.program);
    auto& shader = records.effect.techniques["lighting"].passes.emplace_back();
    shader.layoutData[pass].bindingMap.emplace(id, 12);
    shader.bindingData[phase].descriptorBindings.emplace(id, 23);
    records.stage.descriptorVisibility.emplace(id, ShaderStageFlagBit::COMPUTE);
    records.renderPhase.rootSignature = "main";
    records.renderPhase.shaderIndex.emplace("bloom", 0);
    fillProgram(records.renderPhase.shaderPrograms.emplace_back());
}

bool checkSet(const DescriptorSetData& value, uint32_t slot, const Alloc& alloc) {
    const auto& layout = value.descriptorSetLayoutData;
    if (layout.slot != slot || layout.bindingMap.at(id) != slot + 1 || value.descriptorSetLayoutInfo.bindings.size() != 1) return false;
    const auto& binding = value.descriptorSetLayoutInfo.bindings.front();
    return value.get_allocator() == alloc && layout.bindingMap.get_allocator().resource() == alloc.resource() &&
           binding.binding == slot && binding.count == 2 && binding.stageFlags == ShaderStageFlagBit::COMPUTE &&
           binding.descriptorType == DescriptorType::UNIFORM_BUFFER;
}

bool checkProgram(const ShaderProgramData& program, const Alloc& alloc) {
    return program.get_allocator() == alloc && checkSet(program.layout.descriptorSets.at(pass), 3, alloc) &&
           checkSet(program.layout.descriptorGroups.at(pass), 9, alloc);
}

bool checkEffect(const EffectData& effect, const Alloc& alloc) {
    if (effect.techniques.size() != 1) return false;
    const auto& [key, technique] = *effect.techniques.begin();
    if (key != "lighting" || key.get_allocator().resource() != alloc.resource() ||
        technique.get_allocator() != alloc || technique.passes.size() != 1) return false;
    const auto& shader = technique.passes.front();
    const auto& binding = shader.bindingData.at(phase);
    const auto& layout = shader.layoutData.at(pass);
    return shader.get_allocator() == alloc && binding.get_allocator() == alloc && layout.get_allocator() == alloc &&
           layout.bindingMap.at(id) == 12 && binding.descriptorBindings.at(id) == 23;
}
} // namespace

bool exchangePipelineArchives(bool writing, const char* path) {
    boost::container::pmr::monotonic_buffer_resource arena, destination;
    Records records(&destination);
    cc::IntrusivePtr<DescriptorSetLayout> layout(new EmptyDescriptorSetLayout);
    cc::IntrusivePtr<DescriptorSet> set(new EmptyDescriptorSet);
    cc::IntrusivePtr<PipelineLayout> pipeline(new EmptyPipelineLayout);
    records.set.descriptorSetLayout = layout;
    records.set.descriptorSet = set;
    records.program.pipelineLayout = pipeline;
    records.renderPhase.pipelineLayout = pipeline;
    if (writing) {
        populate(records);
        std::ofstream file(path, std::ios::binary);
        BinaryOutputArchive archive(file, &arena);
        save(archive, records.set);
        save(archive, records.program);
        save(archive, records.effect);
        save(archive, records.stage);
        save(archive, records.renderPhase);
        file.close();
        return !file.fail();
    }
    std::ifstream file(path, std::ios::binary);
    file.exceptions(std::ios::badbit | std::ios::failbit);
    BinaryInputArchive archive(file, &arena);
    load(archive, records.set);
    load(archive, records.program);
    load(archive, records.effect);
    load(archive, records.stage);
    load(archive, records.renderPhase);
    const auto& rp = records.renderPhase;
    if (file.peek() != std::char_traits<char>::eof() || records.set.descriptorSetLayout != layout || records.set.descriptorSet != set ||
        records.program.pipelineLayout != pipeline || rp.pipelineLayout != pipeline || !checkSet(records.set, 7, &destination) ||
        !checkProgram(records.program, &destination) || !checkEffect(records.effect, &destination) ||
        records.stage.descriptorVisibility.at(id) != ShaderStageFlagBit::COMPUTE || rp.rootSignature != "main" ||
        rp.shaderIndex.at("bloom") != 0 || rp.shaderPrograms.size() != 1 || !checkProgram(rp.shaderPrograms.front(), &destination)) return false;
    EffectData movedEffect(std::move(records.effect), &arena);
    RenderPhaseData movedPhase(std::move(records.renderPhase), &arena);
    return checkEffect(movedEffect, &arena) && movedPhase.get_allocator().resource() == &arena &&
           movedPhase.shaderIndex.get_allocator().resource() == &arena && movedPhase.pipelineLayout == pipeline &&
           checkProgram(movedPhase.shaderPrograms.front(), &arena);
}
