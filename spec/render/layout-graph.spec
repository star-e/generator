module LayoutGraph;
import Gfx;

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
    }
}
