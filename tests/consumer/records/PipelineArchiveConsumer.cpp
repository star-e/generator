#include "cocos/renderer/pipeline/custom/BinaryArchive.h"
#include "cocos/renderer/pipeline/custom/LayoutGraphSerialization.h"
#include "cocos/renderer/pipeline/custom/LayoutGraphTypes.h"
#include "gfx-empty/EmptyDescriptorSet.h"
#include "gfx-empty/EmptyDescriptorSetLayout.h"
#include "gfx-empty/EmptyPipelineLayout.h"
#include <boost/container/pmr/monotonic_buffer_resource.hpp>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
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

void fillGraph(LayoutGraphData& graph, Records& records) {
    const auto root = graph.add_vertex(RenderStage_{}, "main");
    const auto child = graph.add_vertex(RenderPhase_{}, "lighting", root);
    graph.getUpdate(root) = pass;
    graph.getUpdate(child) = phase;
    graph.getLayout(root).descriptorSets[pass] = std::move(records.set);
    graph.value(RenderStage_{}, root) = std::move(records.stage);
    graph.value(RenderPhase_{}, child) = std::move(records.renderPhase);
    graph.valueNames.emplace_back("camera");
    graph.valueNames.emplace_back("bloom");
    graph.attributeIndex.emplace("camera", id);
    graph.constantIndex.emplace("tint", NameLocalID{12});
    graph.shaderLayoutIndex.emplace("bloom", child);
    graph.effects.emplace("post", std::move(records.effect));
}

bool checkGraph(const LayoutGraphData& graph) {
    const auto alloc = graph.get_allocator();
    return graph.num_vertices() == 2 && graph.locate("/main/lighting") == 1 && graph.get_parent(1) == 0 &&
           graph.getUpdate(0) == pass && graph.getUpdate(1) == phase && checkSet(graph.getLayout(0).descriptorSets.at(pass), 7, alloc) &&
           graph.value(RenderStage_{}, 0).descriptorVisibility.at(id) == ShaderStageFlagBit::COMPUTE &&
           checkProgram(graph.value(RenderPhase_{}, 1).shaderPrograms.at(0), alloc) &&
           graph.valueNames.size() == 2 && graph.valueNames[0] == "camera" && graph.valueNames[1] == "bloom" &&
           graph.valueNames[0].get_allocator() == alloc && graph.attributeIndex.at("camera") == id &&
           graph.constantIndex.at("tint").value == 12 && graph.shaderLayoutIndex.at("bloom") == 1 &&
           graph.effects.size() == 1 && graph.effects.begin()->first.get_allocator() == alloc && checkEffect(graph.effects.at("post"), alloc);
}

bool checkTruncatedGraph(const LayoutGraphData& graph) {
    std::ostringstream output(std::ios::binary);
    BinaryOutputArchive writer(output, graph.get_allocator().resource());
    save(writer, graph);
    auto bytes = output.str();
    bytes.pop_back();
    std::istringstream input(bytes, std::ios::binary);
    input.exceptions(std::ios::badbit | std::ios::failbit);
    BinaryInputArchive reader(input, graph.get_allocator().resource());
    LayoutGraphData target{graph.get_allocator()};
    target.add_vertex(RenderStage_{}, "retained");
    target.valueNames.emplace_back("previous");
    target.constantMacros = "session";
    try { load(reader, target); }
    catch (const std::ios_base::failure&) {
        return target.num_vertices() == 1 && target.getName(0) == "retained" && target.valueNames.size() == 1 &&
               target.valueNames.front() == "previous" && target.effects.empty() && target.constantMacros == "session";
    }
    return false;
}

bool checkConvertedVertices(const LayoutGraphData& graph) {
    if (graph.num_vertices() != 4 || graph.locate("/scene") != 0 || graph.locate("/scene/light") != 1 ||
        graph.locate("/scene/light/opaque") != 2 || graph.locate("/scene/light/transparent") != 3 ||
        graph.get_parent(0) != graph.null_vertex() || graph.get_parent(1) != 0 ||
        graph.get_parent(2) != 1 || graph.get_parent(3) != 1 ||
        !graph.holds<RenderStage_>(0) || !graph.holds<RenderStage_>(1) ||
        !graph.holds<RenderPhase_>(2) || !graph.holds<RenderPhase_>(3)) return false;
    const auto& opaque = graph.value(RenderPhase_{}, 2);
    const auto& transparent = graph.value(RenderPhase_{}, 3);
    return graph.getUpdate(0) == pass && graph.getUpdate(1) == pass && graph.getUpdate(2) == phase && graph.getUpdate(3) == phase &&
           graph.getLayout(0).descriptorSets.empty() && graph.getLayout(0).descriptorGroups.empty() &&
           graph.getLayout(3).descriptorSets.empty() && graph.getLayout(3).descriptorGroups.empty() &&
           graph.shaderLayoutIndex.size() == 3 && graph.shaderLayoutIndex.at("depth") == 2 &&
           graph.shaderLayoutIndex.at("lit") == 2 && graph.shaderLayoutIndex.at("glass") == 3 &&
           opaque.shaderPrograms.size() == 2 && opaque.shaderIndex.size() == 2 &&
           opaque.shaderIndex.at("depth") == 0 && opaque.shaderIndex.at("lit") == 1 &&
           transparent.shaderPrograms.size() == 1 && transparent.shaderIndex.size() == 1 && transparent.shaderIndex.at("glass") == 0;
}

bool checkConvertedNames(const LayoutGraphData& graph) {
    constexpr const char* names[]{"camera", "exposure", "shadowA", "shadowB", "history", "phaseColor"};
    if (graph.valueNames.size() != std::size(names) || graph.attributeIndex.size() != 5 || graph.constantIndex.size() != 1) return false;
    for (size_t i = 0; i < std::size(names); ++i) {
        const auto& index = i == 1 ? graph.constantIndex : graph.attributeIndex;
        if (graph.valueNames[i] != names[i] || index.at(names[i]).value != i) return false;
    }
    return true;
}

bool matchesBlock(const DescriptorBlockData& block, DescriptorTypeOrder type, uint32_t offset, uint32_t capacity) {
    return block.type == type && block.visibility == ShaderStageFlagBit::FRAGMENT && block.offset == offset && block.capacity == capacity;
}

bool matchesImageGroup(const DescriptorBlockData& block, uint32_t offset, uint32_t capacity) {
    return matchesBlock(block, DescriptorTypeOrder::STORAGE_IMAGE, offset, capacity) &&
           block.accessType == MemoryAccessBit::READ_WRITE && block.viewDimension == ViewDimension::TEX2D_ARRAY &&
           block.sampleType == SampleType::UINT && block.format == Format::RGBA8UI;
}

bool matchesDescriptor(const DescriptorData& descriptor, uint32_t nameID, Type type, uint32_t count) {
    return descriptor.descriptorID.value == nameID && descriptor.type == type && descriptor.count == count;
}

bool checkConvertedPass(const PipelineLayoutData& passLayout) {
    if (passLayout.descriptorSets.size() != 1 || passLayout.descriptorGroups.size() != 1) return false;
    const auto& set = passLayout.descriptorSets.at(pass).descriptorSetLayoutData;
    const auto& group = passLayout.descriptorGroups.at(pass).descriptorSetLayoutData;
    for (const auto* layout : {&set, &group}) {
        if (layout->descriptorBlocks.size() != 2 || layout->uniformBlocks.size() != 1 || layout->uniformBlockCapacity != 3) return false;
        const auto& block = layout->descriptorBlocks[0];
        const auto& camera = layout->uniformBlocks.at(NameLocalID{0});
        if (!matchesBlock(block, DescriptorTypeOrder::UNIFORM_BUFFER, 0, 3) || block.descriptors.size() != 1 ||
            !matchesDescriptor(block.descriptors[0], 0, Type::UNKNOWN, 1) || camera.name != "camera" ||
            camera.set != 2 || camera.binding != 5 || camera.count != 1 || camera.members.size() != 1 ||
            camera.members[0].name != "exposure" || camera.members[0].type != Type::FLOAT4 || camera.members[0].count != 2) return false;
    }
    const auto& samplers = set.descriptorBlocks[1];
    const auto& image = group.descriptorBlocks[1];
    return set.capacity == 7 && set.samplerTextureCapacity == 4 && group.capacity == 8 && group.samplerTextureCapacity == 0 &&
           matchesBlock(samplers, DescriptorTypeOrder::SAMPLER_TEXTURE, 3, 4) && samplers.descriptors.size() == 2 &&
           matchesDescriptor(samplers.descriptors[0], 2, Type::SAMPLER2D, 1) && matchesDescriptor(samplers.descriptors[1], 3, Type::SAMPLER2D, 2) &&
           matchesImageGroup(image, 3, 5) && image.descriptors.size() == 1 && matchesDescriptor(image.descriptors[0], 4, Type::IMAGE2D, 1);
}

bool checkConvertedPhase(const PipelineLayoutData& phaseLayout) {
    if (phaseLayout.descriptorSets.size() != 2 || phaseLayout.descriptorGroups.size() != 1) return false;
    const auto& instance = phaseLayout.descriptorSets.at(UpdateFrequency::PER_INSTANCE).descriptorSetLayoutData;
    const auto& batch = phaseLayout.descriptorGroups.at(UpdateFrequency::PER_BATCH).descriptorSetLayoutData;
    const auto& color = phaseLayout.descriptorSets.at(phase).descriptorSetLayoutData;
    if (instance.descriptorBlocks.size() != 1 || batch.descriptorBlocks.size() != 1 || color.descriptorBlocks.size() != 1) return false;
    const auto& texture = color.descriptorBlocks[0];
    return instance.capacity == 6 && instance.uniformBlockCapacity == 6 && instance.uniformBlocks.empty() &&
           matchesBlock(instance.descriptorBlocks[0], DescriptorTypeOrder::DYNAMIC_UNIFORM_BUFFER, 0, 6) &&
           instance.descriptorBlocks[0].descriptors.empty() && batch.capacity == 8 && batch.uniformBlocks.empty() &&
           matchesImageGroup(batch.descriptorBlocks[0], 0, 8) && batch.descriptorBlocks[0].descriptors.empty() &&
           color.capacity == 1 && matchesBlock(texture, DescriptorTypeOrder::TEXTURE, 0, 1) &&
           texture.descriptors.size() == 1 && matchesDescriptor(texture.descriptors[0], 5, Type::TEXTURE2D, 1);
}
} // namespace

bool readConvertedLayout(const char* path) {
    boost::container::pmr::monotonic_buffer_resource arena;
    std::ifstream file(path, std::ios::binary);
    file.exceptions(std::ios::badbit | std::ios::failbit);
    BinaryInputArchive archive(file, &arena);
    LayoutGraphData graph{&arena};
    load(archive, graph);
    if (file.peek() != std::char_traits<char>::eof()) std::cerr << "Trailing bytes in converted layout\n";
    else if (!checkConvertedVertices(graph)) std::cerr << "Converted vertex or shader index mismatch\n";
    else if (!checkConvertedNames(graph)) std::cerr << "Converted descriptor or constant name mismatch\n";
    else if (!checkConvertedPass(graph.getLayout(1))) std::cerr << "Converted subpass descriptor layout mismatch\n";
    else if (!checkConvertedPhase(graph.getLayout(2))) std::cerr << "Converted phase descriptor layout mismatch\n";
    else return true;
    return false;
}

bool exchangePipelineArchives(bool writing, const char* path) {
    boost::container::pmr::monotonic_buffer_resource arena, destination;
    DescriptorSetData omitted{std::allocator_arg, &destination};
    if (omitted.descriptorSetLayoutData.get_allocator().resource() != &destination ||
        omitted.descriptorSetLayoutData.slot != UINT32_MAX) return false;
    Records records(&destination);
    cc::IntrusivePtr<DescriptorSetLayout> layout(new EmptyDescriptorSetLayout);
    cc::IntrusivePtr<DescriptorSet> set(new EmptyDescriptorSet);
    cc::IntrusivePtr<PipelineLayout> pipeline(new EmptyPipelineLayout);
    records.set.descriptorSetLayout = layout;
    records.set.descriptorSet = set;
    records.program.pipelineLayout = pipeline;
    records.renderPhase.pipelineLayout = pipeline;
    LayoutGraphData graph{&destination};
    graph.constantMacros = writing ? "writer macros" : "reader macros";
    if (writing) {
        populate(records);
        std::ofstream file(path, std::ios::binary);
        BinaryOutputArchive archive(file, &arena);
        save(archive, records.set);
        save(archive, records.program);
        save(archive, records.effect);
        save(archive, records.stage);
        save(archive, records.renderPhase);
        save(archive, graph);
        fillGraph(graph, records);
        save(archive, graph);
        file.close();
        return !file.fail() && checkTruncatedGraph(graph);
    }
    std::ifstream file(path, std::ios::binary);
    file.exceptions(std::ios::badbit | std::ios::failbit);
    BinaryInputArchive archive(file, &arena);
    load(archive, records.set);
    load(archive, records.program);
    load(archive, records.effect);
    load(archive, records.stage);
    load(archive, records.renderPhase);
    graph.add_vertex(RenderStage_{}, "old");
    graph.valueNames.emplace_back("stale");
    graph.attributeIndex.emplace("stale", NameLocalID{7});
    load(archive, graph);
    if (graph.num_vertices() || !graph.valueNames.empty() || !graph.attributeIndex.empty() || graph.constantMacros != "reader macros") return false;
    load(archive, graph);
    if (!checkGraph(graph) || graph.constantMacros != "reader macros") return false;
    LayoutGraphData movedGraph{std::move(graph), &arena};
    if (!checkGraph(movedGraph) || movedGraph.constantMacros != "reader macros" || graph.num_vertices()) return false;
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
