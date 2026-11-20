module RenderGraph;
import Gfx;
import RenderCommon;
import GraphTags;
import Strings;
import PmrContainers;
import Maps;
import GfxResource;
import RenderWindow;

namespace cc::render {
    @comparison("==") @hash struct ClearValue {
        x: double = 0;
        y: double = 0;
        z: double = 0;
        w: double = 0;
        constructor(x, y, z, w);
    }

    @comparison("==") @hash struct RasterView {
        slotName: ccstd::pmr::string;
        slotName1: ccstd::pmr::string;
        accessType: AccessType = AccessType::WRITE;
        attachmentType: AttachmentType = AttachmentType::RENDER_TARGET;
        loadOp: gfx::LoadOp = gfx::LoadOp::LOAD;
        storeOp: gfx::StoreOp = gfx::StoreOp::STORE;
        clearFlags: gfx::ClearFlagBit = gfx::ClearFlagBit::ALL;
        readonly clearColor: gfx::Color;
        @notElement slotID: uint32_t = 0;
        shaderStageFlags: gfx::ShaderStageFlagBit = gfx::ShaderStageFlagBit::NONE;
        constructor(slotName, accessType, attachmentType, loadOp, storeOp, clearFlags, clearColor, shaderStageFlags);
        constructor(slotName, slotName1, accessType, attachmentType, loadOp, storeOp, clearFlags, clearColor, shaderStageFlags);
    }

    @comparison("==") @hash struct ComputeView {
        name: ccstd::pmr::string;
        accessType: AccessType = AccessType::READ;
        plane: uint32_t = 0;
        clearFlags: gfx::ClearFlagBit = gfx::ClearFlagBit::NONE;
        clearValueType: ClearValueType = ClearValueType::NONE;
        @notElement readonly clearValue: ClearValue;
        shaderStageFlags: gfx::ShaderStageFlagBit = gfx::ShaderStageFlagBit::NONE;
        constructor(name, accessType, clearFlags, clearValueType, clearValue, shaderStageFlags);
        constructor(name, accessType, plane, clearFlags, clearValueType, clearValue, shaderStageFlags);
        fn isRead() const -> bool => "accessType != AccessType::WRITE";
        fn isWrite() const -> bool => "accessType != AccessType::READ";
    }

    @requireAllocator @comparison("==") @hash struct Subpass {
        readonly rasterViews: PmrTransparentMap<ccstd::pmr::string, RasterView>;
        readonly computeViews: PmrTransparentMap<ccstd::pmr::string, ccstd::pmr::vector<ComputeView>>;
        readonly resolvePairs: ccstd::pmr::vector<ResolvePair>;
    }

    @tag struct Subpass_ {}
    @comparison("==") @hash graph SubpassGraph {
        vertex_descriptor Vertex = uint32_t;
        vertex_storage: indexed;
        vertex_list_graph;
        mutable_graph;
        incidence_graph { out_edges: vector; }
        bidirectional_graph { in_edges: vector; }
        component Name_: ccstd::pmr::string { member: names; }
        component Subpass_: Subpass { member: subpasses; }
        named_graph { component: Name_; }
    }

    @requireAllocator @comparison("==") @hash struct RasterPass {
        readonly rasterViews: PmrTransparentMap<ccstd::pmr::string, RasterView>;
        readonly computeViews: PmrTransparentMap<ccstd::pmr::string, ccstd::pmr::vector<ComputeView>>;
        @notElement readonly attachmentIndexMap: PmrTransparentMap<ccstd::pmr::string, uint32_t>;
        readonly textures: PmrTransparentMap<ccstd::pmr::string, gfx::ShaderStageFlagBit>;
        readonly subpassGraph: SubpassGraph;
        width: uint32_t = 0;
        height: uint32_t = 0;
        count: uint32_t = 1;
        quality: uint32_t = 0;
        @notElement readonly viewport: gfx::Viewport;
        @notElement versionName: ccstd::pmr::string;
        @notElement version: uint64_t = 0;
        @notElement hashValue: uint64_t = 0;
        @notElement showStatistics: bool = false;
    }

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

    struct ManagedBuffer {
        nullable buffer: IntrusivePtr<gfx::Buffer>;
        fenceValue: uint64_t = 0;
        constructor(buffer);
    }

    struct ManagedTexture {
        nullable texture: IntrusivePtr<gfx::Texture>;
        fenceValue: uint64_t = 0;
        constructor(texture);
        fn checkResource(desc: const ResourceDesc&) const -> bool;
    }

    struct PersistentBuffer {
        nullable buffer: IntrusivePtr<gfx::Buffer>;
        fenceValue: uint64_t = 0;
        constructor(buffer);
    }

    struct PersistentTexture {
        nullable texture: IntrusivePtr<gfx::Texture>;
        fenceValue: uint64_t = 0;
        constructor(texture);
        fn checkResource(desc: const ResourceDesc&) const -> bool;
    }

    struct RenderSwapchain {
        nullable swapchain: gfx::Swapchain*;
        nullable renderWindow: scene::RenderWindow*;
        currentID: uint32_t = 0;
        numBackBuffers: uint32_t = 0;
        generation: uint32_t = 0xFFFFFFFF;
        isDepthStencil: bool = false;
        @cppOnly nullable texture: gfx::Texture*;
        constructor(swapchain, isDepthStencil);
        static fn getColorTexture(swapchain: gfx::Swapchain*) noexcept -> gfx::Texture*;
        static fn getColorTexture(renderWindow: scene::RenderWindow*) noexcept -> gfx::Texture*;
        static fn getDepthStencilTexture(swapchain: gfx::Swapchain*) noexcept -> gfx::Texture*;
    }

    @requireAllocator struct PersistentRenderPassAndFramebuffer {
        nullable renderPass: IntrusivePtr<gfx::RenderPass>;
        nullable framebuffer: IntrusivePtr<gfx::Framebuffer>;
        readonly clearColors: ccstd::pmr::vector<gfx::Color>;
        clearDepth: float = 0;
        clearStencil: uint8_t = 0;
        constructor(renderPass, framebuffer);
    }

    struct FormatView {
        format: gfx::Format = gfx::Format::UNKNOWN;
    }

    struct SubresourceView {
        nullable textureView: IntrusivePtr<gfx::Texture>;
        format: gfx::Format = gfx::Format::UNKNOWN;
        indexOrFirstMipLevel: uint16_t = 0;
        numMipLevels: uint16_t = 0;
        firstArraySlice: uint16_t = 0;
        numArraySlices: uint16_t = 0;
        firstPlane: uint16_t = 0;
        numPlanes: uint16_t = 0;
    }

    @tag struct Desc_ {}
    @tag struct Traits_ {}
    @tag struct States_ {}
    @tag struct Sampler_ {}
    @tag struct Managed_ {}
    @tag struct ManagedBuffer_ {}
    @tag struct ManagedTexture_ {}
    @tag struct PersistentBuffer_ {}
    @tag struct PersistentTexture_ {}
    @tag struct Framebuffer_ {}
    @tag struct Swapchain_ {}
    @tag struct FormatView_ {}
    @tag struct SubresourceView_ {}
    @immovable graph ResourceGraph {
        vertex_descriptor Vertex = uint32_t;
        vertex_storage: indexed;
        vertex_list_graph;
        mutable_graph;
        hierarchy_graph { storage: shared_edges; }
        component Name_: ccstd::pmr::string { member: names; }
        component Desc_: ResourceDesc { member: descs; }
        component Traits_: ResourceTraits { member: traits; }
        component States_: ResourceStates { member: states; }
        component Sampler_: gfx::SamplerInfo { member: samplerInfo; }
        named_graph { component: Name_; }
        name_index;
        polymorphic_graph {
            Managed_: ManagedResource { member: resources; }
            ManagedBuffer_: ManagedBuffer { member: managedBuffers; }
            ManagedTexture_: ManagedTexture { member: managedTextures; }
            PersistentBuffer_: PersistentBuffer { member: buffers; }
            PersistentTexture_: PersistentTexture { member: textures; }
            Framebuffer_: IntrusivePtr<gfx::Framebuffer> { member: framebuffers; }
            Swapchain_: RenderSwapchain { member: swapchains; }
            FormatView_: FormatView { member: formatViews; }
            SubresourceView_: SubresourceView { member: subresourceViews; }
        }
        readonly renderPasses: ccstd::pmr::unordered_map<RasterPass, PersistentRenderPassAndFramebuffer>;
        nextFenceValue: uint64_t = 0;
        version: uint64_t = 0;
    }
}

binding typescript {
    poolModule "cocos/core/memop";
    poolDependency renderCommon: RenderCommon;
    ClearValue => class { pool cv; };
    RasterView => class {
        pool rv;
        constructor(slotName, accessType, attachmentType, loadOp, storeOp, clearFlags, clearColor, shaderStageFlags);
        reset(slotName, accessType, attachmentType, loadOp, storeOp, clearFlags, shaderStageFlags);
        field clearColor reset with reset();
    };
    ComputeView => class {
        pool cv1;
        constructor(name, accessType, clearFlags, clearValueType, clearValue, shaderStageFlags);
        reset(name, accessType, clearFlags, clearValueType, shaderStageFlags);
    };
    ResourceDesc => class { pool rd; reset defaults; };
    Subpass => class { pool s; reset defaults; };
    SubpassGraph => class { pool sg; };
    RasterPass => class { pool rp; reset defaults; mapKey string; field viewport reset with reset(); };
    ResourceTraits => class { pool rt; reset constructor; };
    ResourceStates => class { pool rs1; reset defaults; };
    ManagedResource => class { pool mr; reset defaults; };
    ManagedBuffer => class { pool mb; reset constructor; };
    ManagedTexture => class { pool mt; reset constructor; };
    PersistentBuffer => class { pool pb; reset constructor; };
    PersistentTexture => class { pool pt; reset constructor; };
    RenderSwapchain => class { pool rs; reset constructor; };
    PersistentRenderPassAndFramebuffer => class { pool prpaf; reset constructor; };
    FormatView => class { pool fv; reset defaults; };
    SubresourceView => class { pool sv; reset defaults; };
    ResourceGraph => class { pool rg; };
}
