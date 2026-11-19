module LayoutGraph;
import Gfx;
import RenderCommon;
import Strings;
import Containers;
import GfxUniformBlock;

namespace cc::render {
    enum class LayoutType : uint8_t {
        VULKAN,
        WEBGPU,
    }

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

    struct Descriptor {
        type: gfx::Type = gfx::Type::UNKNOWN;
        count: uint32_t = 1;

        constructor(type);
    }

    struct DescriptorBlockFlattened {
        readonly descriptorNames: ccstd::vector<ccstd::string>;
        readonly uniformBlockNames: ccstd::vector<ccstd::string>;
        readonly descriptors: ccstd::vector<Descriptor>;
        readonly uniformBlocks: ccstd::vector<gfx::UniformBlock>;
        capacity: uint32_t = 0;
        count: uint32_t = 0;
    }

    @comparison("<") struct DescriptorBlockIndex {
        updateFrequency: UpdateFrequency = UpdateFrequency::PER_INSTANCE;
        parameterType: ParameterType = ParameterType::CONSTANTS;
        descriptorType: DescriptorTypeOrder = DescriptorTypeOrder::UNIFORM_BUFFER;
        visibility: gfx::ShaderStageFlagBit = gfx::ShaderStageFlagBit::NONE;

        constructor(updateFrequency, parameterType, descriptorType, visibility);
    }

    @comparison("<") struct DescriptorGroupBlockIndex {
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

    enum class RenderPassType : uint8_t {
        SINGLE_RENDER_PASS,
        RENDER_PASS,
        RENDER_SUBPASS,
    }

    @tag struct Name_ {}
    graph LayoutGraph {
        vertex_descriptor Vertex = uint32_t;
        vertex_storage: indexed;
        vertex_list_graph;
        mutable_graph;
        hierarchy_graph { storage: shared_edges; }
        component Name_: ccstd::pmr::string { member: names; }
        named_graph { component: Name_; }
        parent_graph;
        addressable_graph;
    }
}

binding typescript {
    LayoutType => const enum;
    DescriptorTypeOrder => const enum { names; };
    RenderPassType => const enum;
    Descriptor => class { reset constructor; };
    DescriptorBlockFlattened => class { reset defaults; };
}
