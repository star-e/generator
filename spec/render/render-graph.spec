module RenderGraph;
import Gfx;
import RenderCommon;
import GraphTags;
import Strings;
import GfxResource;

namespace cc::render {
    struct ResourceDesc {
        dimension: ResourceDimension = ResourceDimension::BUFFER;
        alignment: uint32_t = 0;
        width: uint32_t = 0;
        height: uint32_t = 0;
        depthOrArraySize: uint16_t = 0;
        mipLevels: uint16_t = 0;
        format: gfx::Format = gfx::Format::UNKNOWN;
        sampleCount: gfx::SampleCount = gfx::SampleCount::X1;
        textureFlags: gfx::TextureFlagBit = gfx::TextureFlagBit::NONE;
        flags: ResourceFlags = ResourceFlags::NONE;
        viewType: gfx::TextureType = gfx::TextureType::TEX2D;
    }

    struct ResourceStates {
        states: gfx::AccessFlagBit = gfx::AccessFlagBit::NONE;
    }

    struct ResourceTraits {
        residency: ResourceResidency = ResourceResidency::MANAGED;
        constructor(residency);
        fn hasSideEffects() const noexcept -> bool => "residency == ResourceResidency::PERSISTENT || residency == ResourceResidency::EXTERNAL || residency == ResourceResidency::BACKBUFFER";
    }

    struct ManagedResource {
        unused: uint32_t = 0;
    }

    struct FormatView {
        format: gfx::Format = gfx::Format::UNKNOWN;
    }

    @tag struct Desc_ {}
    @tag struct Traits_ {}
    @tag struct States_ {}
    @tag struct Sampler_ {}
    @tag struct Managed_ {}
    @immovable graph ResourceGraph {
        vertex_descriptor Vertex = uint32_t;
        vertex_storage: indexed;
        vertex_list_graph;
        mutable_graph;
        component Name_: ccstd::pmr::string { member: names; }
        component Desc_: ResourceDesc { member: descs; }
        component Traits_: ResourceTraits { member: traits; }
        component States_: ResourceStates { member: states; }
        component Sampler_: gfx::SamplerInfo { member: samplerInfo; }
        named_graph { component: Name_; }
        polymorphic_graph {
            Managed_: ManagedResource { member: resources; }
        }
    }
}

binding typescript {
    poolModule "cocos/core/memop";
    poolDependency renderCommon: RenderCommon;
    ResourceDesc => class { pool rd; reset defaults; };
    ResourceTraits => class { pool rt; reset constructor; };
    ResourceStates => class { pool rs1; reset defaults; };
    ManagedResource => class { pool mr; reset defaults; };
    FormatView => class { pool fv; reset defaults; };
    ResourceGraph => class { pool rg; };
}
