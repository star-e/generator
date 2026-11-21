module NativePipeline;
import EngineTypes;
import Customization;
import LayoutGraph;
import RenderCommon;
import Scene;
import Light;
import ReflectionProbe;
import PipelineExternals;
import Containers;
import PmrContainers;
import Strings;
import Maps;
import Gfx;
import RenderGraph;
import RenderInterface;
import PrivateInterface;
import PrivateTypes;
import FGDispatcher;
import Sets;

namespace cc::render {
    struct RenderGraphFilter {
        nullable validPasses: const ccstd::pmr::vector<bool>*;
        cpp {
            bool operator()(RenderGraph::Vertex pass) const { return (*validPasses)[pass]; }
        }
    }

    class NativeRenderNode implements RenderNode {
        nullable pipelineRuntime: const PipelineRuntime*;
        nullable renderGraph: RenderGraph*;
        nodeID: RenderGraph::Vertex = 0xFFFFFFFF;
        constructor(pipelineRuntime, renderGraph, nodeID);
    }

    class NativeSetter : NativeRenderNode implements Setter {
        nullable layoutGraph: const LayoutGraphData*;
        layoutID: LayoutGraphData::Vertex = 0xFFFFFFFF;
        constructor(layoutGraph, layoutID);
        void setVec4ArraySize(const ccstd::string& name, uint32_t sz);
        void setVec4ArrayElem(const ccstd::string& name, const cc::Vec4& vec, uint32_t id);
        void setMat4ArraySize(const ccstd::string& name, uint32_t sz);
        void setMat4ArrayElem(const ccstd::string& name, const cc::Mat4& mat, uint32_t id);
    }

    class NativeRenderSubpassBuilderImpl : NativeSetter implements RenderSubpassBuilder {}

    class NativeSceneBuilder final : SceneBuilder, NativeSetter {}

    class NativeRenderQueueBuilder final : RenderQueueBuilder, NativeSetter {}

    class NativeRenderSubpassBuilder final : RenderSubpassBuilder, NativeRenderSubpassBuilderImpl {}

    class NativeMultisampleRenderSubpassBuilder final : MultisampleRenderSubpassBuilder, NativeRenderSubpassBuilderImpl {}

    class NativeComputeSubpassBuilder final : ComputeSubpassBuilder, NativeSetter {}

    class NativeRenderPassBuilder final : RenderPassBuilder, NativeSetter {}

    class NativeMultisampleRenderPassBuilder final : MultisampleRenderPassBuilder, NativeSetter {
        subpassID: RenderGraph::Vertex = 0xFFFFFFFF;
        subpassLayoutID: LayoutGraphData::Vertex = 0xFFFFFFFF;
        constructor(subpassID, subpassLayoutID);
    }

    class NativeComputeQueueBuilder final : ComputeQueueBuilder, NativeSetter {}

    class NativeComputePassBuilder final : ComputePassBuilder, NativeSetter {}

    struct DrawInstance {
        nullable subModel: const scene::SubModel*;
        priority: uint32_t = 0;
        hash: uint32_t = 0;
        depth: float = 0;
        shaderID: uint32_t = 0;
        passIndex: uint32_t = 0;
    }

    struct ProbeHelperQueue {
        probeMap: ccstd::pmr::vector<scene::SubModel*>;
        static LayoutGraphData::Vertex getDefaultId(const LayoutGraphData& lg);
        void clear() noexcept { return probeMap.clear(); }
        void removeMacro() const;
        static int32_t getPassIndexFromLayout(const IntrusivePtr<scene::SubModel>& subModel, LayoutGraphData::Vertex phaseLayoutId);
        void applyMacro(const LayoutGraphData& lg, const scene::Model& model, LayoutGraphData::Vertex probeLayoutId);
    }

    struct RenderInstancingQueue {
        sortedBatches: ccstd::pmr::vector<pipeline::InstancedBuffer*>;
        passInstances: PmrUnorderedMap<const scene::Pass*, uint32_t>;
        instanceBuffers: ccstd::pmr::vector<IntrusivePtr<pipeline::InstancedBuffer>>;

        bool empty() const noexcept;
        void clear();
        void add(const scene::Pass& pass, scene::SubModel& submodel, uint32_t passID);
        void sort();
        void uploadBuffers(gfx::CommandBuffer* cmdBuffer) const;
        void recordCommandBuffer(gfx::RenderPass* renderPass, uint32_t subpassIndex,
            gfx::CommandBuffer* cmdBuffer, uint32_t lightByteOffset = 0xFFFFFFFF) const;
    }

    struct RenderDrawQueue {
        instances: ccstd::pmr::vector<DrawInstance>;

        void add(const scene::Model& model, float depth, uint32_t subModelIdx, uint32_t passIdx);
        void sortOpaqueOrCutout();
        void sortTransparent();
        void recordCommandBuffer(gfx::RenderPass* renderPass, uint32_t subpassIndex,
            gfx::CommandBuffer* cmdBuffer, uint32_t lightByteOffset = 0xFFFFFFFF) const;
    }

    @moveOnly struct NativeRenderQueue {
        opaqueQueue: RenderDrawQueue;
        transparentQueue: RenderDrawQueue;
        probeQueue: ProbeHelperQueue;
        opaqueInstancingQueue: RenderInstancingQueue;
        transparentInstancingQueue: RenderInstancingQueue;
        nullable camera: const scene::Camera*;
        sceneFlags: SceneFlags = SceneFlags::NONE;
        lightByteOffset: uint32_t = 0xFFFFFFFF;

        void sort();
        void clear() noexcept;
        bool empty() const noexcept;
        void recordCommands(gfx::CommandBuffer* cmdBuffer, gfx::RenderPass* renderPass, uint32_t subpassIndex, SceneFlags sceneFlags) const;
    }

    @immovable struct ResourceGroup {
        instancingBuffers: PmrUnorderedSet<IntrusivePtr<pipeline::InstancedBuffer>>;
        ~ResourceGroup() noexcept;
    }

    @moveOnly struct BufferPool {
        nullable device: gfx::Device*;
        bufferSize: uint32_t = 0;
        dynamic: bool = false;
        currentBuffers: ccstd::pmr::vector<IntrusivePtr<gfx::Buffer>>;
        currentBufferViews: ccstd::pmr::vector<IntrusivePtr<gfx::Buffer>>;
        freeBuffers: ccstd::pmr::vector<IntrusivePtr<gfx::Buffer>>;
        freeBufferViews: ccstd::pmr::vector<IntrusivePtr<gfx::Buffer>>;
        constructor();
        constructor(device, bufferSize, dynamic);
        void init(gfx::Device* deviceIn, uint32_t sz, bool bDynamic);
        void syncResources();
        gfx::Buffer* allocateBuffer();
    }

    @moveOnly struct DescriptorSetPool {
        nullable device: gfx::Device*;
        nullable setLayout: IntrusivePtr<gfx::DescriptorSetLayout>;
        currentDescriptorSets: ccstd::pmr::vector<IntrusivePtr<gfx::DescriptorSet>>;
        freeDescriptorSets: ccstd::pmr::vector<IntrusivePtr<gfx::DescriptorSet>>;
        constructor();
        constructor(device, setLayout);
        void init(gfx::Device* deviceIn, IntrusivePtr<gfx::DescriptorSetLayout> layout);
        void syncDescriptorSets();
        const gfx::DescriptorSet& getCurrentDescriptorSet() const;
        gfx::DescriptorSet& getCurrentDescriptorSet();
        gfx::DescriptorSet* allocateDescriptorSet();
    }

    @moveOnly struct UniformBlockResource {
        cpuBuffer: ccstd::pmr::vector<char>;
        bufferPool: BufferPool;
        void init(gfx::Device* deviceIn, uint32_t sz, bool bDynamic);
        gfx::Buffer* createFromCpuBuffer();
    }

    @moveOnly struct ProgramResource {
        uniformBuffers: ccstd::pmr::unordered_map<NameLocalID, UniformBlockResource>;
        descriptorSetPool: DescriptorSetPool;
        void syncResources() noexcept;
    }

    @moveOnly struct LayoutGraphNodeResource {
        uniformBuffers: ccstd::pmr::unordered_map<NameLocalID, UniformBlockResource>;
        descriptorSetPool: DescriptorSetPool;
        programResources: PmrTransparentMap<ccstd::pmr::string, ProgramResource>;
        void syncResources() noexcept;
    }

    struct QuadResource {
        nullable quadVB: IntrusivePtr<gfx::Buffer>;
        nullable quadIB: IntrusivePtr<gfx::Buffer>;
        nullable quadIA: IntrusivePtr<gfx::InputAssembler>;
        constructor();
        constructor(quadVB, quadIB, quadIA);
    }

    @comparison("==") @hash struct FrustumCullingKey {
        nullable camera: const scene::Camera*;
        nullable probe: const scene::ReflectionProbe*;
        nullable light: const scene::Light*;
        lightLevel: uint32_t = 0xFFFFFFFF;
        castShadow: bool = false;
        probePass: bool = false;
    }

    @comparison("==") @hash struct FrustumCullingID {
        value: uint32_t = 0xFFFFFFFF;
        explicit operator uint32_t() const { return value; }
    }

    struct FrustumCulling {
        resultIndex: ccstd::pmr::unordered_map<FrustumCullingKey, FrustumCullingID>;
    }

    @comparison("==") @hash struct LightBoundsCullingID {
        value: uint32_t = 0xFFFFFFFF;
        explicit operator uint32_t() const { return value; }
    }

    @comparison("==") @hash struct LightBoundsCullingKey {
        frustumCullingID: FrustumCullingID;
        nullable camera: const scene::Camera*;
        nullable probe: const scene::ReflectionProbe*;
        nullable cullingLight: const scene::Light*;
    }

    struct LightBoundsCulling {
        resultIndex: ccstd::pmr::unordered_map<LightBoundsCullingKey, LightBoundsCullingID>;
    }

    struct NativeRenderQueueID {
        value: uint32_t = 0xFFFFFFFF;
        explicit operator uint32_t() const { return value; }
    }

    @comparison("==") @hash struct NativeRenderQueueKey {
        frustumCulledResultID: FrustumCullingID;
        lightBoundsCulledResultID: LightBoundsCullingID;
        queueLayoutID: uint32_t = 0xFFFFFFFF;
    }

    struct NativeRenderQueueQuery {
        frustumCulledResultID: FrustumCullingID;
        lightBoundsCulledResultID: LightBoundsCullingID;
        renderQueueTarget: NativeRenderQueueID;
    }

    struct LightBoundsCullingResult {
        instances: ccstd::vector<const scene::Model*>;
        lightByteOffset: uint32_t = 0xFFFFFFFF;
    }

    @moveOnly struct SceneCulling {
        frustumCullings: ccstd::pmr::unordered_map<const scene::RenderScene*, FrustumCulling>;
        frustumCullingResults: ccstd::pmr::vector<ccstd::vector<const scene::Model*>>;
        lightBoundsCullings: ccstd::pmr::unordered_map<const scene::RenderScene*, LightBoundsCulling>;
        lightBoundsCullingResults: ccstd::pmr::vector<LightBoundsCullingResult>;
        renderQueueIndex: ccstd::pmr::unordered_map<NativeRenderQueueKey, NativeRenderQueueID>;
        renderQueues: ccstd::pmr::vector<NativeRenderQueue>;
        renderQueueQueryIndex: PmrFlatMap<RenderGraph::Vertex, NativeRenderQueueQuery>;
        numFrustumCulling: uint32_t = 0;
        numLightBoundsCulling: uint32_t = 0;
        numRenderQueues: uint32_t = 0;
        gpuCullingPassID: RenderGraph::Vertex = 0xFFFFFFFF;
        enableLightCulling: bool = true;

        void clear() noexcept;
        void buildRenderQueues(const RenderGraph& rg, const LayoutGraphData& lg, const NativePipeline& ppl);
        cpp {
        private:
            FrustumCullingID getOrCreateFrustumCulling(const SceneData& sceneData);
            LightBoundsCullingID getOrCreateLightBoundsCulling(const SceneData& sceneData, FrustumCullingID frustumCullingID);
            NativeRenderQueueID getOrCreateRenderQueue(const NativeRenderQueueKey& key, SceneFlags flags, const scene::Camera* camera);
            void collectCullingQueries(const RenderGraph& rg);
            void batchFrustumCulling(const NativePipeline& ppl);
            void batchLightBoundsCulling();
            void fillRenderQueues();
        public:
        }
    }

    @immovable struct LightResource {
        cpuBuffer: ccstd::pmr::vector<char>;
        nullable programLibrary: const NativeProgramLibrary*;
        nullable device: gfx::Device*;
        elementSize: uint32_t = 0;
        maxNumLights: uint32_t = 16;
        binding: uint32_t = 0xFFFFFFFF;
        resized: bool = false;
        nullable lightBuffer: IntrusivePtr<gfx::Buffer>;
        nullable firstLightBufferView: IntrusivePtr<gfx::Buffer>;
        lights: ccstd::pmr::vector<const scene::Light*>;
        lightIndex: PmrFlatMap<const scene::Light*, uint32_t>;

        void init(const NativeProgramLibrary& programLib, gfx::Device* deviceIn, uint32_t maxNumLights);
        void buildLights(SceneCulling& sceneCulling, bool bHDR, const scene::Shadows* shadowInfo);
        void tryUpdateRenderSceneLocalDescriptorSet(const SceneCulling& sceneCulling);
        void clear();
        uint32_t addLight(const scene::Light* light, bool bHDR, float exposure, const scene::Shadows* shadowInfo);
        void buildLightBuffer(gfx::CommandBuffer* cmdBuffer) const;
    }

    @comparison("==", "<") @hash struct DescriptorSetKey {
        nodeID: uint32_t = 0xFFFFFFFF;
        frequency: UpdateFrequency = UpdateFrequency::PER_INSTANCE;
        constructor(nodeID, frequency);
    }

    @moveOnly struct DescriptorSetContext {
        nullable descriptorSet: IntrusivePtr<gfx::DescriptorSet>;
        constructor();
        constructor(descriptorSet);
    }

    struct TextureWithAccessFlags {
        nullable texture: IntrusivePtr<gfx::Texture>;
        accessFlags: gfx::AccessFlagBit = gfx::AccessFlagBit::NONE;
    }

    @moveOnly struct DeviceRenderData {
        hasConstants: bool = false;
        required: bool = false;
        buffers: PmrFlatMap<NameLocalID, IntrusivePtr<gfx::Buffer>>;
        textures: PmrFlatMap<NameLocalID, TextureWithAccessFlags>;
        samplers: PmrFlatMap<NameLocalID, gfx::Sampler*>;

        void clear() noexcept {
            hasConstants = false;
            required = false;
            buffers.clear();
            textures.clear();
            samplers.clear();
        }
        bool hasNoData() const noexcept {
            return !hasConstants && buffers.empty() && textures.empty() && samplers.empty();
        }
    }

    @immovable struct NativeRenderContext {
        defaultResource: std::unique_ptr<gfx::DefaultResource>;
        nextFenceValue: uint64_t = 0;
        resourceGroups: ccstd::pmr::map<uint64_t, ResourceGroup>;
        layoutGraphResources: ccstd::pmr::vector<LayoutGraphNodeResource>;
        fullscreenQuad: QuadResource;
        sceneCulling: SceneCulling;
        lightResources: LightResource;
        resourceGraphIndex: ccstd::pmr::unordered_map<RenderGraph::Vertex, PmrFlatMap<NameLocalID, ResourceGraph::Vertex>>;
        graphNodeRenderData: ccstd::pmr::unordered_map<DescriptorSetKey, DeviceRenderData>;
        graphNodeDescriptorSets: ccstd::pmr::unordered_map<DescriptorSetKey, gfx::DescriptorSet*>;
        constructor(defaultResource);
        void clearPreviousResources(uint64_t finishedFenceValue) noexcept;
    }

    @immovable class NativeProgramLibrary final : ProgramLibrary {
        layoutGraph: LayoutGraphData;
        phases: PmrFlatMap<uint32_t, ProgramGroup>;
        unsycPool: boost::container::pmr::unsynchronized_pool_resource;
        mergeHighFrequency: bool = false;
        fixedLocal: bool = true;
        localLayoutData: DescriptorSetLayoutData;
        nullable localDescriptorSetLayout: IntrusivePtr<gfx::DescriptorSetLayout>;
        nullable emptyDescriptorSetLayout: IntrusivePtr<gfx::DescriptorSetLayout>;
        nullable emptyPipelineLayout: IntrusivePtr<gfx::PipelineLayout>;
        nullable pipeline: PipelineRuntime*;
        nullable device: gfx::Device*;
        void init(gfx::Device* deviceIn);
        void setPipeline(PipelineRuntime* pipelineIn);
        void destroy();
    }

    struct PipelineCustomization {
        currentContext: std::shared_ptr<CustomPipelineContext>;
        contexts: PmrTransparentMap<ccstd::pmr::string, std::shared_ptr<CustomPipelineContext>>;
        renderPasses: PmrTransparentMap<ccstd::pmr::string, std::shared_ptr<CustomRenderPass>>;
        renderSubpasses: PmrTransparentMap<ccstd::pmr::string, std::shared_ptr<CustomRenderSubpass>>;
        computeSubpasses: PmrTransparentMap<ccstd::pmr::string, std::shared_ptr<CustomComputeSubpass>>;
        computePasses: PmrTransparentMap<ccstd::pmr::string, std::shared_ptr<CustomComputePass>>;
        renderQueues: PmrTransparentMap<ccstd::pmr::string, std::shared_ptr<CustomRenderQueue>>;
        renderCommands: PmrTransparentMap<ccstd::pmr::string, std::shared_ptr<CustomRenderCommand>>;
    }

    struct BuiltinShadowTransform {
        shadowView: Mat4;
        shadowProj: Mat4;
        shadowViewProj: Mat4;
        validFrustum: geometry::Frustum;
        splitFrustum: geometry::Frustum;
        lightViewFrustum: geometry::Frustum;
        castLightViewBoundingBox: geometry::AABB;
        shadowCameraFar: float = 0;
        splitCameraNear: float = 0;
        splitCameraFar: float = 0;
        csmAtlas: Vec4;
    }

    @comparison("<") struct BuiltinCascadedShadowMapKey {
        nullable camera: const scene::Camera*;
        nullable light: const scene::DirectionalLight*;
    }

    struct BuiltinCascadedShadowMap {
        layers: Array4<BuiltinShadowTransform>;
        specialLayer: BuiltinShadowTransform;
        shadowDistance: float = 0;
    }

    @immovable class NativePipeline final : Pipeline {
        cpp {
        private:
            ccstd::vector<gfx::CommandBuffer*> _commandBuffers;
        public:
        }
        unsyncPool: boost::container::pmr::unsynchronized_pool_resource;
        nullable device: gfx::Device*;
        nullable swapchain: gfx::Swapchain*;
        macros: MacroRecord;
        constantMacros: ccstd::string;
        globalDSManager: std::unique_ptr<pipeline::GlobalDSManager>;
        name: ccstd::pmr::string;
        nullable programLibrary: NativeProgramLibrary*;
        nullable profiler: scene::Model*;
        lightingMode: LightingMode = LightingMode::DEFAULT;
        nullable pipelineSceneData: IntrusivePtr<pipeline::PipelineSceneData>;
        nativeContext: NativeRenderContext;
        resourceGraph: ResourceGraph;
        renderGraph: RenderGraph;
        mutable builtinCSMs: PmrFlatMap<BuiltinCascadedShadowMapKey, BuiltinCascadedShadowMap>;
        statistics: PipelineStatistics;
        custom: PipelineCustomization;
        defaultFramebufferHasDepthStencil: bool = false;

        constructor() noexcept = custom;
        void executeRenderGraph(const RenderGraph& rg);
        void addCustomContext(std::string_view name, std::shared_ptr<CustomPipelineContext> ptr);
        void addCustomRenderPass(std::string_view name, std::shared_ptr<CustomRenderPass> ptr);
        void addCustomRenderSubpass(std::string_view name, std::shared_ptr<CustomRenderSubpass> ptr);
        void addCustomComputeSubpass(std::string_view name, std::shared_ptr<CustomComputeSubpass> ptr);
        void addCustomComputePass(std::string_view name, std::shared_ptr<CustomComputePass> ptr);
        void addCustomRenderQueue(std::string_view name, std::shared_ptr<CustomRenderQueue> ptr);
        void addCustomRenderCommand(std::string_view name, std::shared_ptr<CustomRenderCommand> ptr);
        void setCustomContext(std::string_view name);
        void prepareDescriptorSets(gfx::CommandBuffer& cmdBuff, const FrameGraphDispatcher& rdg, RenderGraph::Vertex passID);
    }

    class NativeProgramProxy final : ProgramProxy {
        nullable shader: IntrusivePtr<gfx::Shader>;
        nullable pipelineState: IntrusivePtr<gfx::PipelineState>;
        constructor();
        constructor(shader);
        constructor(shader, pipelineState);
    }

    class NativeRenderingModule final : RenderingModule {
        programLibrary: std::shared_ptr<NativeProgramLibrary>;
        constructor();
        constructor(programLibrary);
    }
}
