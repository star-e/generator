module RenderGraph;
import Gfx;
import RenderCommon;
import Strings;
import Containers;
import PmrContainers;
import Maps;
import RenderWindow;
import Scene;
import Light;
import EngineTypes;

namespace cc::render {
    @comparison("==") @hash struct ClearValue {
        x: double = 0;
        y: double = 0;
        z: double = 0;
        w: double = 0;
        constructor(x, y, z, w) defaults;
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
        constructor(slotName, accessType, attachmentType, loadOp, storeOp, clearFlags, clearColor, shaderStageFlags) defaults;
        @cppOnly constructor(slotName, slotName1, accessType, attachmentType, loadOp, storeOp, clearFlags, clearColor, shaderStageFlags);
    }

    @comparison("==") @hash struct ComputeView {
        name: ccstd::pmr::string;
        accessType: AccessType = AccessType::READ;
        plane: uint32_t = 0;
        clearFlags: gfx::ClearFlagBit = gfx::ClearFlagBit::NONE;
        clearValueType: ClearValueType = ClearValueType::NONE;
        @notElement readonly clearValue: ClearValue;
        shaderStageFlags: gfx::ShaderStageFlagBit = gfx::ShaderStageFlagBit::NONE;
        constructor(name, accessType, clearFlags, clearValueType, clearValue, shaderStageFlags) defaults;
        @cppOnly constructor(name, accessType, plane, clearFlags, clearValueType, clearValue, shaderStageFlags);
        bool isRead() const { return accessType != AccessType::WRITE; }
        bool isWrite() const { return accessType != AccessType::READ; }
    }

    @comparison("==") @hash struct Subpass {
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

    struct RasterSubpass {
        readonly rasterViews: PmrTransparentMap<ccstd::pmr::string, RasterView>;
        readonly computeViews: PmrTransparentMap<ccstd::pmr::string, ccstd::pmr::vector<ComputeView>>;
        readonly resolvePairs: ccstd::pmr::vector<ResolvePair>;
        readonly viewport: gfx::Viewport;
        subpassID: uint32_t = 0xFFFFFFFF;
        count: uint32_t = 1;
        quality: uint32_t = 0;
        showStatistics: bool = false;
        constructor(subpassID, count, quality) defaults;
    }

    struct ComputeSubpass {
        readonly rasterViews: PmrTransparentMap<ccstd::pmr::string, RasterView>;
        readonly computeViews: PmrTransparentMap<ccstd::pmr::string, ccstd::pmr::vector<ComputeView>>;
        subpassID: uint32_t = 0xFFFFFFFF;
        constructor(subpassID) defaults;
    }

    @comparison("==") @hash struct RasterPass {
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
        constructor(residency) defaults;
        bool hasSideEffects() const noexcept { return residency == ResourceResidency::PERSISTENT || residency == ResourceResidency::EXTERNAL || residency == ResourceResidency::BACKBUFFER; }
    }

    struct ManagedResource {
        unused: uint32_t = 0;
    }

    struct ManagedBuffer {
        nullable buffer: IntrusivePtr<gfx::Buffer>;
        fenceValue: uint64_t = 0;
        constructor(buffer) defaults;
    }

    struct ManagedTexture {
        nullable texture: IntrusivePtr<gfx::Texture>;
        fenceValue: uint64_t = 0;
        constructor(texture) defaults;
        bool checkResource(const ResourceDesc& desc) const;
    }

    struct PersistentBuffer {
        nullable buffer: IntrusivePtr<gfx::Buffer>;
        fenceValue: uint64_t = 0;
        constructor(buffer) defaults;
    }

    struct PersistentTexture {
        nullable texture: IntrusivePtr<gfx::Texture>;
        fenceValue: uint64_t = 0;
        constructor(texture) defaults;
        bool checkResource(const ResourceDesc& desc) const;
    }

    struct RenderSwapchain {
        nullable swapchain: gfx::Swapchain*;
        nullable renderWindow: scene::RenderWindow*;
        currentID: uint32_t = 0;
        numBackBuffers: uint32_t = 0;
        generation: uint32_t = 0xFFFFFFFF;
        isDepthStencil: bool = false;
        @cppOnly nullable texture: gfx::Texture*;
        constructor(swapchain, isDepthStencil) defaults;
        static gfx::Texture* getColorTexture(gfx::Swapchain* swapchain) noexcept;
        static gfx::Texture* getColorTexture(scene::RenderWindow* renderWindow) noexcept;
        static gfx::Texture* getDepthStencilTexture(gfx::Swapchain* swapchain) noexcept;
    }

    struct PersistentRenderPassAndFramebuffer {
        nullable renderPass: IntrusivePtr<gfx::RenderPass>;
        nullable framebuffer: IntrusivePtr<gfx::Framebuffer>;
        readonly clearColors: ccstd::pmr::vector<gfx::Color>;
        clearDepth: float = 0;
        clearStencil: uint8_t = 0;
        constructor(renderPass, framebuffer) defaults;
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

        void validateSwapchains();
        void mount(gfx::Device* device, Vertex vertID);
        void unmount(uint64_t completedFenceValue);
        bool isTexture(Vertex resID) const noexcept;
        bool isTextureView(Vertex resID) const noexcept;
        gfx::Texture* getTexture(Vertex resID);
        gfx::Buffer* getBuffer(Vertex resID);
        void invalidatePersistentRenderPassAndFramebuffer(gfx::Texture* pTexture);
    }

    struct ComputePass {
        readonly computeViews: PmrTransparentMap<ccstd::pmr::string, ccstd::pmr::vector<ComputeView>>;
        readonly textures: PmrTransparentMap<ccstd::pmr::string, gfx::ShaderStageFlagBit>;
    }

    struct ResolvePass {
        readonly resolvePairs: ccstd::pmr::vector<ResolvePair>;
    }

    @moveOnly struct CopyPass {
        readonly copyPairs: ccstd::pmr::vector<CopyPair>;
        readonly uploadPairs: ccstd::pmr::vector<UploadPair>;
    }

    struct MovePass {
        readonly movePairs: ccstd::pmr::vector<MovePair>;
    }

    struct RaytracePass {
        readonly computeViews: PmrTransparentMap<ccstd::pmr::string, ccstd::pmr::vector<ComputeView>>;
    }

    struct ClearView {
        slotName: ccstd::pmr::string;
        clearFlags: gfx::ClearFlagBit = gfx::ClearFlagBit::ALL;
        readonly clearColor: gfx::Color;
        constructor(slotName, clearFlags, clearColor) defaults;
    }

    struct RenderQueue {
        hint: QueueHint = QueueHint::RENDER_OPAQUE;
        phaseID: uint32_t = 0xFFFFFFFF;
        passLayoutID: uint32_t = 0xFFFFFFFF;
        viewport: gfx::Viewport;
        constructor(hint, phaseID, passLayoutID) defaults;
        @cppOnly constructor(phaseID, passLayoutID);
    }

    flags CullingFlags : uint32_t { NONE = 0, CAMERA_FRUSTUM = 0x1, LIGHT_FRUSTUM = 0x2, LIGHT_BOUNDS = 0x4 }

    struct SceneData {
        nullable scene: const scene::RenderScene*;
        nullable camera: const scene::Camera*;
        readonly light: LightInfo;
        flags: SceneFlags = SceneFlags::NONE;
        cullingFlags: CullingFlags = CullingFlags::CAMERA_FRUSTUM;
        nullable shadingLight: IntrusivePtr<scene::Light>;
        constructor(scene, camera, flags, light, cullingFlags, shadingLight) defaults;
    }

    struct Dispatch {
        nullable material: IntrusivePtr<Material>;
        passID: uint32_t = 0;
        threadGroupCountX: uint32_t = 0;
        threadGroupCountY: uint32_t = 0;
        threadGroupCountZ: uint32_t = 0;
        constructor(material, passID, threadGroupCountX, threadGroupCountY, threadGroupCountZ) defaults;
    }

    enum class BlitType : uint8_t { FULLSCREEN_QUAD, DRAW_2D, DRAW_PROFILE, DRAW_3D }

    struct Blit {
        nullable material: IntrusivePtr<Material>;
        passID: uint32_t = 0;
        sceneFlags: SceneFlags = SceneFlags::NONE;
        nullable camera: const scene::Camera*;
        blitType: BlitType = BlitType::FULLSCREEN_QUAD;
        readonly models: ccstd::pmr::vector<IntrusivePtr<scene::Model>>;
        constructor(material, passID, sceneFlags, camera, blitType) defaults;
        @cppOnly constructor(camera, blitType, models);
    }

    @moveOnly struct RenderData {
        readonly constants: PmrFlatMap<uint32_t, ccstd::pmr::vector<char>>;
        readonly buffers: PmrFlatMap<uint32_t, IntrusivePtr<gfx::Buffer>>;
        readonly textures: PmrFlatMap<uint32_t, IntrusivePtr<gfx::Texture>>;
        readonly samplers: PmrFlatMap<uint32_t, gfx::Sampler*>;
        custom: ccstd::pmr::string;
    }

    @tag struct Data_ {}
    @tag struct Valid_ {}
    @tag struct Queue_ {}
    @tag struct Scene_ {}
    @tag struct Blit_ {}
    @tag struct Dispatch_ {}
    @tag struct Clear_ {}
    @tag struct Viewport_ {}
    graph RenderGraph {
        vertex_descriptor Vertex = uint32_t;
        vertex_storage: indexed;
        vertex_list_graph;
        mutable_graph;
        incidence_graph { out_edges: vector; }
        bidirectional_graph { in_edges: vector; }
        hierarchy_graph { storage: separate_edges; }
        component Name_: ccstd::pmr::string { member: names; }
        component Layout_: ccstd::pmr::string { member: layoutNodes; }
        component Data_: RenderData { member: data; }
        component Valid_: bool { member: valid; }
        named_graph { component: Name_; }
        polymorphic_graph {
            RasterPassTag as RasterPass: RasterPass { member: rasterPasses; }
            RasterSubpassTag as RasterSubpass: RasterSubpass { member: rasterSubpasses; }
            ComputeSubpassTag as ComputeSubpass: ComputeSubpass { member: computeSubpasses; }
            ComputeTag as Compute: ComputePass { member: computePasses; }
            ResolveTag as Resolve: ResolvePass { member: resolvePasses; }
            CopyTag as Copy: CopyPass { member: copyPasses; }
            MoveTag as Move: MovePass { member: movePasses; }
            RaytraceTag as Raytrace: RaytracePass { member: raytracePasses; }
            Queue_: RenderQueue { member: renderQueues; }
            Scene_: SceneData { member: scenes; }
            Blit_: Blit { member: blits; }
            Dispatch_: Dispatch { member: dispatches; }
            Clear_: ccstd::pmr::vector<ClearView> { member: clearViews; }
            Viewport_: gfx::Viewport { member: viewports; }
        }
        readonly index: PmrUnorderedStringMap<ccstd::pmr::string, uint32_t>;
        readonly sortedVertices: ccstd::pmr::vector<uint32_t>;
        readonly globalRenderData: RenderData;
        Vertex getPassID(Vertex nodeID) const;
        ccstd::string print(boost::container::pmr::memory_resource* scratch) const;
    }
}

binding typescript {
    poolModule "cocos/core/memop";
    poolDependency renderCommon: RenderCommon;
    ClearValue => class { pool cv; };
    RasterView => class { pool rv; };
    ComputeView => class {
        pool cv1;
        reset(name, accessType, clearFlags, clearValueType, shaderStageFlags);
    };
    ResourceDesc => class { pool rd; reset defaults; };
    Subpass => class { pool s; reset defaults; };
    SubpassGraph => class { pool sg; };
    RasterSubpass => class { pool rs2; reset constructor; };
    ComputeSubpass => class { pool cs; reset constructor; };
    RasterPass => class { pool rp; reset defaults; mapKey string; };
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
    ComputePass => class { pool cp; reset defaults; };
    ResolvePass => class { pool rp1; reset defaults; };
    CopyPass => class { pool cp1; reset defaults; };
    MovePass => class { pool mp; reset defaults; };
    RaytracePass => class { pool rp2; reset defaults; };
    ClearView => class { pool cv2; };
    RenderQueue => class { pool rq; field viewport default null; };
    CullingFlags => const enum;
    SceneData => class { pool sd; reset(scene, camera, flags, cullingFlags, shadingLight); };
    Dispatch => class { pool d; };
    BlitType => const enum;
    Blit => class { pool b; };
    RenderData => class { pool rd1; reset defaults; };
    RenderGraph => class { pool rg1; };
}
