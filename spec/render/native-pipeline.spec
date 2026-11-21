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

namespace cc::render {
    struct DrawInstance {
        nullable subModel: const scene::SubModel*;
        priority: uint32_t = 0;
        hash: uint32_t = 0;
        depth: float = 0;
        shaderID: uint32_t = 0;
        passIndex: uint32_t = 0;
    }

    @requireAllocator struct ProbeHelperQueue {
        probeMap: ccstd::pmr::vector<scene::SubModel*>;
        static LayoutGraphData::Vertex getDefaultId(const LayoutGraphData& lg);
        void clear() noexcept { return probeMap.clear(); }
        void removeMacro() const;
        static int32_t getPassIndexFromLayout(const IntrusivePtr<scene::SubModel>& subModel, LayoutGraphData::Vertex phaseLayoutId);
        void applyMacro(const LayoutGraphData& lg, const scene::Model& model, LayoutGraphData::Vertex probeLayoutId);
    }

    @requireAllocator struct RenderInstancingQueue {
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

    @requireAllocator struct RenderDrawQueue {
        instances: ccstd::pmr::vector<DrawInstance>;

        void add(const scene::Model& model, float depth, uint32_t subModelIdx, uint32_t passIdx);
        void sortOpaqueOrCutout();
        void sortTransparent();
        void recordCommandBuffer(gfx::RenderPass* renderPass, uint32_t subpassIndex,
            gfx::CommandBuffer* cmdBuffer, uint32_t lightByteOffset = 0xFFFFFFFF) const;
    }

    @requireAllocator @moveOnly struct NativeRenderQueue {
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

    @requireAllocator @moveOnly struct BufferPool {
        nullable device: gfx::Device*;
        bufferSize: uint32_t = 0;
        dynamic: bool = false;
        currentBuffers: ccstd::pmr::vector<IntrusivePtr<gfx::Buffer>>;
        currentBufferViews: ccstd::pmr::vector<IntrusivePtr<gfx::Buffer>>;
        freeBuffers: ccstd::pmr::vector<IntrusivePtr<gfx::Buffer>>;
        freeBufferViews: ccstd::pmr::vector<IntrusivePtr<gfx::Buffer>>;
        constructor(device, bufferSize, dynamic);
        void init(gfx::Device* deviceIn, uint32_t sz, bool bDynamic);
        void syncResources();
        gfx::Buffer* allocateBuffer();
    }

    @requireAllocator @moveOnly struct DescriptorSetPool {
        nullable device: gfx::Device*;
        nullable setLayout: IntrusivePtr<gfx::DescriptorSetLayout>;
        currentDescriptorSets: ccstd::pmr::vector<IntrusivePtr<gfx::DescriptorSet>>;
        freeDescriptorSets: ccstd::pmr::vector<IntrusivePtr<gfx::DescriptorSet>>;
        constructor(device, setLayout);
        void init(gfx::Device* deviceIn, IntrusivePtr<gfx::DescriptorSetLayout> layout);
        void syncDescriptorSets();
        const gfx::DescriptorSet& getCurrentDescriptorSet() const;
        gfx::DescriptorSet& getCurrentDescriptorSet();
        gfx::DescriptorSet* allocateDescriptorSet();
    }

    @requireAllocator @moveOnly struct UniformBlockResource {
        cpuBuffer: ccstd::pmr::vector<char>;
        bufferPool: BufferPool;
        void init(gfx::Device* deviceIn, uint32_t sz, bool bDynamic);
        gfx::Buffer* createFromCpuBuffer();
    }

    @requireAllocator @moveOnly struct ProgramResource {
        uniformBuffers: ccstd::pmr::unordered_map<NameLocalID, UniformBlockResource>;
        descriptorSetPool: DescriptorSetPool;
        void syncResources() noexcept;
    }

    @requireAllocator @moveOnly struct LayoutGraphNodeResource {
        uniformBuffers: ccstd::pmr::unordered_map<NameLocalID, UniformBlockResource>;
        descriptorSetPool: DescriptorSetPool;
        programResources: PmrTransparentMap<ccstd::pmr::string, ProgramResource>;
        void syncResources() noexcept;
    }

    struct QuadResource {
        nullable quadVB: IntrusivePtr<gfx::Buffer>;
        nullable quadIB: IntrusivePtr<gfx::Buffer>;
        nullable quadIA: IntrusivePtr<gfx::InputAssembler>;
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

    @requireAllocator struct FrustumCulling {
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

    @requireAllocator struct LightBoundsCulling {
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

    @moveOnly struct DescriptorSetContext {
        nullable descriptorSet: IntrusivePtr<gfx::DescriptorSet>;
        constructor(descriptorSet);
    }

    struct TextureWithAccessFlags {
        nullable texture: IntrusivePtr<gfx::Texture>;
        accessFlags: gfx::AccessFlagBit = gfx::AccessFlagBit::NONE;
    }

    @requireAllocator @moveOnly struct DeviceRenderData {
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

    @requireAllocator struct PipelineCustomization {
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
}
