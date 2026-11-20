module NativePipeline;
import Customization;
import LayoutGraph;
import Scene;
import Light;
import ReflectionProbe;
import PipelineExternals;
import Containers;
import PmrContainers;
import Strings;
import Maps;
import GfxResource;
import Gfx;
import GfxDescriptorSet;

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
        static fn getDefaultId(lg: const LayoutGraphData&) -> LayoutGraphData::Vertex;
        fn clear() noexcept -> void => "probeMap.clear()";
        fn removeMacro() const -> void;
        static fn getPassIndexFromLayout(subModel: const IntrusivePtr<scene::SubModel>&, phaseLayoutId: LayoutGraphData::Vertex) -> int32_t;
        fn applyMacro(lg: const LayoutGraphData&, model: const scene::Model&, probeLayoutId: LayoutGraphData::Vertex) -> void;
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
        fn init(deviceIn: gfx::Device*, sz: uint32_t, bDynamic: bool) -> void;
        fn syncResources() -> void;
        fn allocateBuffer() -> gfx::Buffer*;
    }

    @requireAllocator @moveOnly struct DescriptorSetPool {
        nullable device: gfx::Device*;
        nullable setLayout: IntrusivePtr<gfx::DescriptorSetLayout>;
        currentDescriptorSets: ccstd::pmr::vector<IntrusivePtr<gfx::DescriptorSet>>;
        freeDescriptorSets: ccstd::pmr::vector<IntrusivePtr<gfx::DescriptorSet>>;
        constructor(device, setLayout);
        fn init(deviceIn: gfx::Device*, layout: IntrusivePtr<gfx::DescriptorSetLayout>) -> void;
        fn syncDescriptorSets() -> void;
        fn getCurrentDescriptorSet() const -> const gfx::DescriptorSet&;
        fn getCurrentDescriptorSet() -> gfx::DescriptorSet&;
        fn allocateDescriptorSet() -> gfx::DescriptorSet*;
    }

    @requireAllocator @moveOnly struct UniformBlockResource {
        cpuBuffer: ccstd::pmr::vector<char>;
        bufferPool: BufferPool;
        fn init(deviceIn: gfx::Device*, sz: uint32_t, bDynamic: bool) -> void;
        fn createFromCpuBuffer() -> gfx::Buffer*;
    }

    @requireAllocator @moveOnly struct ProgramResource {
        uniformBuffers: ccstd::pmr::unordered_map<NameLocalID, UniformBlockResource>;
        descriptorSetPool: DescriptorSetPool;
        fn syncResources() noexcept -> void;
    }

    @requireAllocator @moveOnly struct LayoutGraphNodeResource {
        uniformBuffers: ccstd::pmr::unordered_map<NameLocalID, UniformBlockResource>;
        descriptorSetPool: DescriptorSetPool;
        programResources: PmrTransparentMap<ccstd::pmr::string, ProgramResource>;
        fn syncResources() noexcept -> void;
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
