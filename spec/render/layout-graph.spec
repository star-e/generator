module LayoutGraph;
import ArchiveInterface;
import Gfx;
import RenderCommon;
import Strings;
import Containers;
import PmrContainers;
import Sets;
import Maps;
import GfxUniformBlock;
import GfxDescriptorSet;

namespace cc::render {
    enum class LayoutType : uint8_t {
        VULKAN,
        WEBGPU,
    }

    struct Layout {}

    enum class DescriptorTypeOrder : uint8_t {
        UNIFORM_BUFFER,
        DYNAMIC_UNIFORM_BUFFER,
        SAMPLER_TEXTURE,
        SAMPLER,
        TEXTURE,
        STORAGE_BUFFER,
        DYNAMIC_STORAGE_BUFFER,
        STORAGE_IMAGE,
        INPUT_ATTACHMENT,
    }

    @serialize struct Descriptor {
        type: gfx::Type = gfx::Type::UNKNOWN;
        count: uint32_t = 1;

        constructor(type);
    }

    @serialize struct DescriptorBlock {
        readonly descriptors: ccstd::map<ccstd::string, Descriptor>;
        readonly uniformBlocks: ccstd::map<ccstd::string, gfx::UniformBlock>;
        capacity: uint32_t = 0;
        count: uint32_t = 0;
    }

    @serialize struct DescriptorBlockFlattened {
        readonly descriptorNames: ccstd::vector<ccstd::string>;
        readonly uniformBlockNames: ccstd::vector<ccstd::string>;
        readonly descriptors: ccstd::vector<Descriptor>;
        readonly uniformBlocks: ccstd::vector<gfx::UniformBlock>;
        capacity: uint32_t = 0;
        count: uint32_t = 0;
    }

    @serialize @comparison("<") struct DescriptorBlockIndex {
        updateFrequency: UpdateFrequency = UpdateFrequency::PER_INSTANCE;
        parameterType: ParameterType = ParameterType::CONSTANTS;
        descriptorType: DescriptorTypeOrder = DescriptorTypeOrder::UNIFORM_BUFFER;
        visibility: gfx::ShaderStageFlagBit = gfx::ShaderStageFlagBit::NONE;

        constructor(updateFrequency, parameterType, descriptorType, visibility);
    }

    @serialize @comparison("<") struct DescriptorGroupBlockIndex {
        updateFrequency: UpdateFrequency = UpdateFrequency::PER_INSTANCE;
        parameterType: ParameterType = ParameterType::CONSTANTS;
        descriptorType: DescriptorTypeOrder = DescriptorTypeOrder::UNIFORM_BUFFER;
        visibility: gfx::ShaderStageFlagBit = gfx::ShaderStageFlagBit::NONE;
        accessType: gfx::MemoryAccessBit = gfx::MemoryAccessBit::READ_ONLY;
        viewDimension: gfx::ViewDimension = gfx::ViewDimension::UNKNOWN;
        sampleType: gfx::SampleType = gfx::SampleType::FLOAT;
        format: gfx::Format = gfx::Format::UNKNOWN;

        constructor(updateFrequency, parameterType, descriptorType, visibility, accessType, viewDimension, sampleType, format);
    }

    @serialize @requireAllocator struct DescriptorDB {
        readonly blocks: ccstd::pmr::map<DescriptorBlockIndex, DescriptorBlock>;
        readonly groupBlocks: ccstd::pmr::map<DescriptorGroupBlockIndex, DescriptorBlock>;
    }

    @serialize @requireAllocator struct RenderPhase {
        readonly shaders: PmrTransparentSet<ccstd::pmr::string>;
    }

    enum class RenderPassType : uint8_t {
        SINGLE_RENDER_PASS,
        RENDER_PASS,
        RENDER_SUBPASS,
    }

    @tag struct Name_ {}
    @tag struct Descriptors_ {}
    @tag struct Update_ {}
    @tag struct Layout_ {}
    @tag struct RenderStage_ {}
    @tag struct RenderPhase_ {}
    @serialize graph LayoutGraph {
        vertex_descriptor Vertex = uint32_t;
        vertex_storage: indexed;
        vertex_list_graph;
        mutable_graph;
        hierarchy_graph { storage: shared_edges; }
        component Name_: ccstd::pmr::string { member: names; }
        component Descriptors_: DescriptorDB { member: descriptors; }
        polymorphic_graph {
            RenderStage_: RenderPassType { member: stages; }
            RenderPhase_: RenderPhase { member: phases; }
        }
        named_graph { component: Name_; }
        parent_graph;
        addressable_graph;
    }

    using UniformID = uint32_t;

    @serialize struct UniformData {
        uniformID: UniformID = 0xFFFFFFFF;
        uniformType: gfx::Type = gfx::Type::UNKNOWN;
        offset: uint32_t = 0;
        size: uint32_t = 0;

        constructor(uniformID, uniformType, offset);
    }

    @serialize @requireAllocator struct UniformBlockData {
        bufferSize: uint32_t = 0;
        readonly uniforms: ccstd::pmr::vector<UniformData>;
    }

    @serialize @comparison("==", "<") @hash struct NameLocalID {
        value: uint32_t = 0xFFFFFFFF;
    }

    @serialize struct DescriptorData {
        descriptorID: NameLocalID;
        type: gfx::Type = gfx::Type::UNKNOWN;
        count: uint32_t = 1;

        constructor(descriptorID, type, count);
        constructor(descriptorID, type);
        constructor(descriptorID);
    }

    @serialize @requireAllocator struct DescriptorBlockData {
        type: DescriptorTypeOrder = DescriptorTypeOrder::UNIFORM_BUFFER;
        visibility: gfx::ShaderStageFlagBit = gfx::ShaderStageFlagBit::NONE;
        offset: uint32_t = 0;
        capacity: uint32_t = 0;
        accessType: gfx::MemoryAccessBit = gfx::MemoryAccessBit::READ_ONLY;
        viewDimension: gfx::ViewDimension = gfx::ViewDimension::UNKNOWN;
        sampleType: gfx::SampleType = gfx::SampleType::FLOAT;
        format: gfx::Format = gfx::Format::UNKNOWN;
        readonly descriptors: ccstd::pmr::vector<DescriptorData>;

        constructor(type, visibility, capacity, accessType, viewDimension, sampleType, format);
        constructor(type, visibility, capacity);
    }

    @serialize @moveOnly @requireAllocator struct DescriptorSetLayoutData {
        slot: uint32_t = 0xFFFFFFFF;
        capacity: uint32_t = 0;
        uniformBlockCapacity: uint32_t = 0;
        samplerTextureCapacity: uint32_t = 0;
        readonly descriptorBlocks: ccstd::pmr::vector<DescriptorBlockData>;
        readonly uniformBlocks: PmrUnorderedMap<NameLocalID, gfx::UniformBlock>;
        readonly bindingMap: PmrFlatMap<NameLocalID, uint32_t>;

        constructor(slot, capacity, descriptorBlocks, uniformBlocks, bindingMap);
    }

    @serialize @moveOnly @requireAllocator struct DescriptorSetData {
        readonly descriptorSetLayoutData: DescriptorSetLayoutData;
        readonly descriptorSetLayoutInfo: gfx::DescriptorSetLayoutInfo;
        @transient nullable descriptorSetLayout: IntrusivePtr<gfx::DescriptorSetLayout>;
        @transient nullable descriptorSet: IntrusivePtr<gfx::DescriptorSet>;
        constructor(descriptorSetLayoutData, descriptorSetLayout, descriptorSet);
    }

    @serialize @moveOnly @requireAllocator struct PipelineLayoutData {
        readonly descriptorSets: ccstd::pmr::map<UpdateFrequency, DescriptorSetData>;
        readonly descriptorGroups: ccstd::pmr::map<UpdateFrequency, DescriptorSetData>;
    }

    @serialize @moveOnly @requireAllocator struct ShaderBindingData {
        readonly descriptorBindings: PmrFlatMap<NameLocalID, uint32_t>;
    }

    @serialize @moveOnly @requireAllocator struct ShaderLayoutData {
        readonly layoutData: ccstd::pmr::map<UpdateFrequency, DescriptorSetLayoutData>;
        readonly bindingData: ccstd::pmr::map<UpdateFrequency, ShaderBindingData>;
    }

    @serialize @moveOnly @requireAllocator struct TechniqueData {
        readonly passes: ccstd::pmr::vector<ShaderLayoutData>;
    }

    @serialize @moveOnly @requireAllocator struct EffectData {
        readonly techniques: ccstd::pmr::map<ccstd::pmr::string, TechniqueData>;
    }

    @serialize @moveOnly @requireAllocator struct ShaderProgramData {
        readonly layout: PipelineLayoutData;
        @transient nullable pipelineLayout: IntrusivePtr<gfx::PipelineLayout>;
    }

    @serialize @moveOnly @requireAllocator struct RenderStageData {
        readonly descriptorVisibility: PmrUnorderedMap<NameLocalID, gfx::ShaderStageFlagBit>;
    }

    @serialize @moveOnly @requireAllocator struct RenderPhaseData {
        rootSignature: ccstd::pmr::string;
        readonly shaderPrograms: ccstd::pmr::vector<ShaderProgramData>;
        readonly shaderIndex: PmrTransparentMap<ccstd::pmr::string, uint32_t>;
        @transient nullable pipelineLayout: IntrusivePtr<gfx::PipelineLayout>;
    }

    @serialize graph LayoutGraphData {
        vertex_descriptor Vertex = uint32_t;
        vertex_storage: indexed;
        vertex_list_graph;
        mutable_graph;
        hierarchy_graph { storage: shared_edges; }
        component Name_: ccstd::pmr::string { member: names; }
        component Update_: UpdateFrequency { member: updateFrequencies; }
        component Layout_: PipelineLayoutData { member: layouts; }
        polymorphic_graph {
            RenderStage_: RenderStageData { member: stages; }
            RenderPhase_: RenderPhaseData { member: phases; }
        }
        named_graph { component: Name_; }
        parent_graph;
        addressable_graph;

        readonly valueNames: ccstd::pmr::vector<ccstd::pmr::string>;
        readonly attributeIndex: PmrFlatMap<ccstd::pmr::string, NameLocalID>;
        readonly constantIndex: PmrFlatMap<ccstd::pmr::string, NameLocalID>;
        readonly shaderLayoutIndex: PmrFlatMap<ccstd::pmr::string, uint32_t>;
        readonly effects: PmrFlatMap<ccstd::pmr::string, EffectData>;
        @transient constantMacros: ccstd::string;
    }
}

binding typescript {
    poolModule "cocos/core/memop";
    poolDependency renderCommon: RenderCommon;
    LayoutType => const enum;
    Layout => class from "cocos/rendering/custom/layout-access" { pool l; };
    DescriptorTypeOrder => const enum { names; };
    RenderPassType => const enum;
    Descriptor => class { pool d; reset constructor; };
    DescriptorBlock => class { pool db; reset defaults; };
    DescriptorBlockFlattened => class { pool dbf; reset defaults; };
    DescriptorBlockIndex => class { pool dbi assign; mapKey json; };
    DescriptorGroupBlockIndex => class { pool dgbi assign; mapKey json; };
    DescriptorDB => class { pool dd; reset defaults; };
    RenderPhase => class { pool rp; reset defaults; };
    LayoutGraph => class { pool lg; };
    UniformData => class { pool ud; reset constructor; };
    UniformBlockData => class { pool ubd; reset defaults; };
    NameLocalID => number(value);
    DescriptorData => class { pool dd1; field descriptorID default 0; reset constructor; };
    DescriptorBlockData => class { pool dbd; reset constructor; };
    DescriptorSetLayoutData => class { pool dsld; reset(slot, capacity); };
    DescriptorSetData => class {
        pool dsd;
        reset(descriptorSetLayout, descriptorSet);
        field descriptorSetLayoutInfo reset with resetSetLayoutInfo from "cocos/rendering/custom/layout-access";
    };
    PipelineLayoutData => class {
        pool pld;
        reset defaults;
        extends PipelineLayoutAccess from "cocos/rendering/custom/layout-access";
    };
    ShaderBindingData => class { pool sbd; reset defaults; };
    ShaderLayoutData => class { pool sld; reset defaults; };
    TechniqueData => class { pool td; reset defaults; };
    EffectData => class { pool ed; reset defaults; };
    ShaderProgramData => class { pool spd; reset defaults; };
    RenderStageData => class { pool rsd; reset defaults; };
    RenderPhaseData => class { pool rpd; reset defaults; };
    LayoutGraphData => class { pool lgd; };
}
