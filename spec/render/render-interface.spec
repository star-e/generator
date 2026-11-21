module RenderInterface;
import EngineTypes;
import Containers;
import Gfx;
import Scene;
import Light;
import RenderWindow;
import RenderCommon;
import PipelineExternals;

namespace cc::render {
    interface PipelineRuntime {
        virtual bool activate(gfx::Swapchain* swapchain) = 0;
        virtual bool destroy() noexcept = 0;
        virtual void render(const ccstd::vector<scene::Camera*>& cameras) = 0;
        @getter virtual gfx::Device* getDevice() const = 0;
        @getter virtual const MacroRecord& getMacros() const = 0;
        @getter virtual pipeline::GlobalDSManager* getGlobalDSManager() const = 0;
        @getter virtual gfx::DescriptorSetLayout* getDescriptorSetLayout() const = 0;
        @getter virtual gfx::DescriptorSet* getDescriptorSet() const = 0;
        @getter virtual const ccstd::vector<gfx::CommandBuffer*>& getCommandBuffers() const = 0;
        @getter virtual pipeline::PipelineSceneData* getPipelineSceneData() const = 0;
        @getter virtual const ccstd::string& getConstantMacros() const = 0;
        @getter @nullable virtual scene::Model* getProfiler() const = 0;
        @setter virtual void setProfiler(scene::Model* profiler) = 0;
        @getter @nullable virtual pipeline::GeometryRenderer* getGeometryRenderer() const = 0;
        @getter virtual float getShadingScale() const = 0;
        @setter virtual void setShadingScale(float scale) = 0;
        virtual const ccstd::string& getMacroString(const ccstd::string& name) const = 0;
        virtual int32_t getMacroInt(const ccstd::string& name) const = 0;
        virtual bool getMacroBool(const ccstd::string& name) const = 0;
        virtual void setMacroString(const ccstd::string& name, const ccstd::string& value) = 0;
        virtual void setMacroInt(const ccstd::string& name, int32_t value) = 0;
        virtual void setMacroBool(const ccstd::string& name, bool value) = 0;
        virtual void onGlobalPipelineStateChanged() = 0;
        @cppOnly virtual void setValue(const ccstd::string& name, int32_t value) = 0;
        @cppOnly virtual void setValue(const ccstd::string& name, bool value) = 0;
        @cppOnly virtual bool isOcclusionQueryEnabled() const = 0;
        @cppOnly virtual void resetRenderQueue(bool reset) = 0;
        @cppOnly virtual bool isRenderQueueReset() const = 0;
    }

    enum class PipelineType : uint8_t { BASIC, STANDARD }

    flags SubpassCapabilities : uint32_t {
        NONE = 0,
        INPUT_DEPTH_STENCIL = 1 << 0,
        INPUT_COLOR = 1 << 1,
        INPUT_COLOR_MRT = 1 << 2,
        HETEROGENEOUS_SAMPLE_COUNT = 1 << 3,
    }

    struct PipelineCapabilities {
        subpass: SubpassCapabilities = SubpassCapabilities::NONE;
    }

    interface RenderNode {
        @getter virtual ccstd::string getName() const = 0;
        @setter virtual void setName(const ccstd::string& name) = 0;
        virtual void setCustomBehavior(const ccstd::string& name) = 0;
    }

    interface Setter : RenderNode {
        virtual void setMat4(const ccstd::string& name, const Mat4& mat) = 0;
        virtual void setQuaternion(const ccstd::string& name, const Quaternion& quat) = 0;
        virtual void setColor(const ccstd::string& name, const gfx::Color& color) = 0;
        virtual void setVec4(const ccstd::string& name, const Vec4& vec) = 0;
        virtual void setVec2(const ccstd::string& name, const Vec2& vec) = 0;
        virtual void setFloat(const ccstd::string& name, float v) = 0;
        virtual void setArrayBuffer(const ccstd::string& name, const ArrayBuffer* arrayBuffer) = 0;
        virtual void setBuffer(const ccstd::string& name, gfx::Buffer* buffer) = 0;
        virtual void setTexture(const ccstd::string& name, gfx::Texture* texture) = 0;
        virtual void setSampler(const ccstd::string& name, gfx::Sampler* sampler) = 0;
        virtual void setBuiltinCameraConstants(const scene::Camera* camera) = 0;
        virtual void setBuiltinDirectionalLightConstants(const scene::DirectionalLight* light, const scene::Camera* camera) = 0;
        virtual void setBuiltinSphereLightConstants(const scene::SphereLight* light, const scene::Camera* camera) = 0;
        virtual void setBuiltinSpotLightConstants(const scene::SpotLight* light, const scene::Camera* camera) = 0;
        virtual void setBuiltinPointLightConstants(const scene::PointLight* light, const scene::Camera* camera) = 0;
        virtual void setBuiltinRangedDirectionalLightConstants(const scene::RangedDirectionalLight* light, const scene::Camera* camera) = 0;
        virtual void setBuiltinDirectionalLightFrustumConstants(const scene::Camera* camera, const scene::DirectionalLight* light, uint32_t csmLevel = 0) = 0;
        virtual void setBuiltinSpotLightFrustumConstants(const scene::SpotLight* light) = 0;
    }

    interface SceneBuilder : Setter {
        virtual void useLightFrustum(IntrusivePtr<scene::Light> light, uint32_t csmLevel = 0, const scene::Camera* optCamera = nullptr) = 0;
    }

    interface RenderQueueBuilder : Setter {
        virtual void addSceneOfCamera(scene::Camera* camera, LightInfo light, SceneFlags sceneFlags = SceneFlags::NONE) = 0;
        virtual SceneBuilder* addScene(const scene::Camera* camera, SceneFlags sceneFlags, scene::Light* light = nullptr, scene::RenderScene* scene = nullptr) = 0;
        virtual void addFullscreenQuad(Material* material, uint32_t passID, SceneFlags sceneFlags = SceneFlags::NONE) = 0;
        virtual void addCameraQuad(scene::Camera* camera, Material* material, uint32_t passID, SceneFlags sceneFlags = SceneFlags::NONE) = 0;
        virtual void addDraw3D(const scene::Camera* camera, const std::vector<scene::Model*>& models, SceneFlags sceneFlags = SceneFlags::NON_BUILTIN) = 0;
        virtual void addDraw2D(const scene::Camera* camera) = 0;
        virtual void addProfiler(const scene::Camera* camera) = 0;
        virtual void clearRenderTarget(const ccstd::string& name, const gfx::Color& color = {}) = 0;
        virtual void setViewport(const gfx::Viewport& viewport) = 0;
        virtual void addCustomCommand(std::string_view customBehavior) = 0;
    }

    interface BasicRenderPassBuilder : Setter {
        virtual void addRenderTarget(const ccstd::string& name, gfx::LoadOp loadOp = gfx::LoadOp::CLEAR, gfx::StoreOp storeOp = gfx::StoreOp::STORE, const gfx::Color& color = {}) = 0;
        virtual void addDepthStencil(const ccstd::string& name, gfx::LoadOp loadOp = gfx::LoadOp::CLEAR, gfx::StoreOp storeOp = gfx::StoreOp::STORE, float depth = 1, uint8_t stencil = 0, gfx::ClearFlagBit clearFlags = gfx::ClearFlagBit::DEPTH_STENCIL) = 0;
        virtual void addTexture(const ccstd::string& name, const ccstd::string& slotName, gfx::Sampler* sampler = nullptr, uint32_t plane = 0) = 0;
        virtual RenderQueueBuilder* addQueue(QueueHint hint = QueueHint::NONE, const ccstd::string& phaseName = "default", const ccstd::string& passName = "") = 0;
        virtual void setViewport(const gfx::Viewport& viewport) = 0;
        virtual void setVersion(const ccstd::string& name, uint64_t version) = 0;
        @getter virtual bool getShowStatistics() const = 0;
        @setter virtual void setShowStatistics(bool enable) = 0;
    }

    interface BasicMultisampleRenderPassBuilder : BasicRenderPassBuilder {
        virtual void resolveRenderTarget(const ccstd::string& source, const ccstd::string& target) = 0;
        virtual void resolveDepthStencil(const ccstd::string& source, const ccstd::string& target, gfx::ResolveMode depthMode = gfx::ResolveMode::SAMPLE_ZERO, gfx::ResolveMode stencilMode = gfx::ResolveMode::SAMPLE_ZERO) = 0;
    }

    interface BasicPipeline : PipelineRuntime {
        @getter virtual PipelineType getType() const = 0;
        @getter virtual PipelineCapabilities getCapabilities() const = 0;
        virtual void beginSetup() = 0;
        virtual void endSetup() = 0;
        @getter virtual bool getEnableCpuLightCulling() const = 0;
        @setter virtual void setEnableCpuLightCulling(bool enable) = 0;
        virtual bool containsResource(const ccstd::string& name) const = 0;
        virtual uint32_t addRenderWindow(const ccstd::string& name, gfx::Format format, uint32_t width, uint32_t height, scene::RenderWindow* renderWindow, const ccstd::string& depthStencilName = "") = 0;
        virtual void updateRenderWindow(const ccstd::string& name, scene::RenderWindow* renderWindow, const ccstd::string& depthStencilName = "") = 0;
        virtual uint32_t addRenderTarget(const ccstd::string& name, gfx::Format format, uint32_t width, uint32_t height, ResourceResidency residency = ResourceResidency::MANAGED) = 0;
        virtual uint32_t addDepthStencil(const ccstd::string& name, gfx::Format format, uint32_t width, uint32_t height, ResourceResidency residency = ResourceResidency::MANAGED) = 0;
        virtual void updateRenderTarget(const ccstd::string& name, uint32_t width, uint32_t height, gfx::Format format = gfx::Format::UNKNOWN) = 0;
        virtual void updateDepthStencil(const ccstd::string& name, uint32_t width, uint32_t height, gfx::Format format = gfx::Format::UNKNOWN) = 0;
        virtual uint32_t addBuffer(const ccstd::string& name, uint32_t size, ResourceFlags flags, ResourceResidency residency) = 0;
        virtual void updateBuffer(const ccstd::string& name, uint32_t size) = 0;
        virtual uint32_t addExternalTexture(const ccstd::string& name, gfx::Texture* texture, ResourceFlags flags) = 0;
        virtual void updateExternalTexture(const ccstd::string& name, gfx::Texture* texture) = 0;
        virtual uint32_t addTexture(const ccstd::string& name, gfx::TextureType type, gfx::Format format, uint32_t width, uint32_t height, uint32_t depth, uint32_t arraySize, uint32_t mipLevels, gfx::SampleCount sampleCount, ResourceFlags flags, ResourceResidency residency) = 0;
        virtual void updateTexture(const ccstd::string& name, gfx::Format format, uint32_t width, uint32_t height, uint32_t depth, uint32_t arraySize, uint32_t mipLevels, gfx::SampleCount sampleCount) = 0;
        virtual uint32_t addResource(const ccstd::string& name, ResourceDimension dimension, gfx::Format format, uint32_t width, uint32_t height, uint32_t depth, uint32_t arraySize, uint32_t mipLevels, gfx::SampleCount sampleCount, ResourceFlags flags, ResourceResidency residency) = 0;
        virtual void updateResource(const ccstd::string& name, gfx::Format format, uint32_t width, uint32_t height, uint32_t depth, uint32_t arraySize, uint32_t mipLevels, gfx::SampleCount sampleCount) = 0;
        virtual void beginFrame() = 0;
        virtual void update(const scene::Camera* camera) = 0;
        virtual void endFrame() = 0;
        virtual BasicRenderPassBuilder* addRenderPass(uint32_t width, uint32_t height, const ccstd::string& passName = "default") = 0;
        virtual BasicMultisampleRenderPassBuilder* addMultisampleRenderPass(uint32_t width, uint32_t height, uint32_t count, uint32_t quality, const ccstd::string& passName = "default") = 0;
        virtual void addResolvePass(const ccstd::vector<ResolvePair>& resolvePairs) = 0;
        virtual void addCopyPass(const ccstd::vector<CopyPair>& copyPairs) = 0;
        virtual void addBuiltinReflectionProbePass(const scene::Camera* camera) = 0;
        @optionalResult virtual gfx::DescriptorSetLayout* getDescriptorSetLayout(const ccstd::string& shaderName, UpdateFrequency freq) = 0;
        virtual void setMat4(const ccstd::string& name, const Mat4& mat) = 0;
        virtual void setQuaternion(const ccstd::string& name, const Quaternion& quat) = 0;
        virtual void setColor(const ccstd::string& name, const gfx::Color& color) = 0;
        virtual void setVec4(const ccstd::string& name, const Vec4& vec) = 0;
        virtual void setVec2(const ccstd::string& name, const Vec2& vec) = 0;
        virtual void setFloat(const ccstd::string& name, float v) = 0;
        virtual void setArrayBuffer(const ccstd::string& name, const ArrayBuffer* arrayBuffer) = 0;
        virtual void setBuffer(const ccstd::string& name, gfx::Buffer* buffer) = 0;
        virtual void setTexture(const ccstd::string& name, gfx::Texture* texture) = 0;
        virtual void setSampler(const ccstd::string& name, gfx::Sampler* sampler) = 0;
        virtual void setBuiltinCameraConstants(const scene::Camera* camera) = 0;
        virtual void setBuiltinDirectionalLightConstants(const scene::DirectionalLight* light, const scene::Camera* camera) = 0;
        virtual void setBuiltinSphereLightConstants(const scene::SphereLight* light, const scene::Camera* camera) = 0;
        virtual void setBuiltinSpotLightConstants(const scene::SpotLight* light, const scene::Camera* camera) = 0;
        virtual void setBuiltinPointLightConstants(const scene::PointLight* light, const scene::Camera* camera) = 0;
        virtual void setBuiltinRangedDirectionalLightConstants(const scene::RangedDirectionalLight* light, const scene::Camera* camera) = 0;
        virtual void setBuiltinDirectionalLightFrustumConstants(const scene::Camera* camera, const scene::DirectionalLight* light, uint32_t csmLevel = 0) = 0;
        virtual void setBuiltinSpotLightFrustumConstants(const scene::SpotLight* light) = 0;
    }

    interface RenderSubpassBuilder : Setter {
        virtual void addRenderTarget(const ccstd::string& name, AccessType accessType, const ccstd::string& slotName = "", gfx::LoadOp loadOp = gfx::LoadOp::CLEAR, gfx::StoreOp storeOp = gfx::StoreOp::STORE, const gfx::Color& color = {}) = 0;
        virtual void addDepthStencil(const ccstd::string& name, AccessType accessType, const ccstd::string& depthSlotName = "", const ccstd::string& stencilSlotName = "", gfx::LoadOp loadOp = gfx::LoadOp::CLEAR, gfx::StoreOp storeOp = gfx::StoreOp::STORE, float depth = 1, uint8_t stencil = 0, gfx::ClearFlagBit clearFlags = gfx::ClearFlagBit::DEPTH_STENCIL) = 0;
        virtual void addTexture(const ccstd::string& name, const ccstd::string& slotName, gfx::Sampler* sampler = nullptr, uint32_t plane = 0) = 0;
        virtual void addStorageBuffer(const ccstd::string& name, AccessType accessType, const ccstd::string& slotName) = 0;
        virtual void addStorageImage(const ccstd::string& name, AccessType accessType, const ccstd::string& slotName) = 0;
        virtual void setViewport(const gfx::Viewport& viewport) = 0;
        virtual RenderQueueBuilder* addQueue(QueueHint hint = QueueHint::NONE, const ccstd::string& phaseName = "default", const ccstd::string& passName = "") = 0;
        @getter virtual bool getShowStatistics() const = 0;
        @setter virtual void setShowStatistics(bool enable) = 0;
        virtual void setCustomShaderStages(const ccstd::string& name, gfx::ShaderStageFlagBit stageFlags) = 0;
    }

    interface MultisampleRenderSubpassBuilder : RenderSubpassBuilder {
        virtual void resolveRenderTarget(const ccstd::string& source, const ccstd::string& target) = 0;
        virtual void resolveDepthStencil(const ccstd::string& source, const ccstd::string& target, gfx::ResolveMode depthMode = gfx::ResolveMode::SAMPLE_ZERO, gfx::ResolveMode stencilMode = gfx::ResolveMode::SAMPLE_ZERO) = 0;
    }

    interface ComputeQueueBuilder : Setter {
        virtual void addDispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ, Material* material = nullptr, uint32_t passID = 0) = 0;
    }

    interface ComputeSubpassBuilder : Setter {
        virtual void addRenderTarget(const ccstd::string& name, const ccstd::string& slotName) = 0;
        virtual void addTexture(const ccstd::string& name, const ccstd::string& slotName, gfx::Sampler* sampler = nullptr, uint32_t plane = 0) = 0;
        virtual void addStorageBuffer(const ccstd::string& name, AccessType accessType, const ccstd::string& slotName) = 0;
        virtual void addStorageImage(const ccstd::string& name, AccessType accessType, const ccstd::string& slotName) = 0;
        virtual ComputeQueueBuilder* addQueue(const ccstd::string& phaseName = "default", const ccstd::string& passName = "") = 0;
        virtual void setCustomShaderStages(const ccstd::string& name, gfx::ShaderStageFlagBit stageFlags) = 0;
    }

    interface RenderPassBuilder : BasicRenderPassBuilder {
        virtual void addStorageBuffer(const ccstd::string& name, AccessType accessType, const ccstd::string& slotName) = 0;
        virtual void addStorageImage(const ccstd::string& name, AccessType accessType, const ccstd::string& slotName) = 0;
        virtual void addMaterialTexture(const ccstd::string& resourceName, gfx::ShaderStageFlagBit flags = gfx::ShaderStageFlagBit::VERTEX | gfx::ShaderStageFlagBit::FRAGMENT) = 0;
        virtual RenderSubpassBuilder* addRenderSubpass(const ccstd::string& subpassName) = 0;
        virtual MultisampleRenderSubpassBuilder* addMultisampleRenderSubpass(uint32_t count, uint32_t quality, const ccstd::string& subpassName) = 0;
        virtual ComputeSubpassBuilder* addComputeSubpass(const ccstd::string& subpassName = "") = 0;
        virtual void setCustomShaderStages(const ccstd::string& name, gfx::ShaderStageFlagBit stageFlags) = 0;
    }

    interface MultisampleRenderPassBuilder : BasicMultisampleRenderPassBuilder {
        virtual void addStorageBuffer(const ccstd::string& name, AccessType accessType, const ccstd::string& slotName) = 0;
        virtual void addStorageImage(const ccstd::string& name, AccessType accessType, const ccstd::string& slotName) = 0;
    }

    interface ComputePassBuilder : Setter {
        virtual void addTexture(const ccstd::string& name, const ccstd::string& slotName, gfx::Sampler* sampler = nullptr, uint32_t plane = 0) = 0;
        virtual void addStorageBuffer(const ccstd::string& name, AccessType accessType, const ccstd::string& slotName) = 0;
        virtual void addStorageImage(const ccstd::string& name, AccessType accessType, const ccstd::string& slotName) = 0;
        virtual void addMaterialTexture(const ccstd::string& resourceName, gfx::ShaderStageFlagBit flags = gfx::ShaderStageFlagBit::COMPUTE) = 0;
        virtual ComputeQueueBuilder* addQueue(const ccstd::string& phaseName = "default", const ccstd::string& passName = "") = 0;
        virtual void setCustomShaderStages(const ccstd::string& name, gfx::ShaderStageFlagBit stageFlags) = 0;
    }

    interface Pipeline : BasicPipeline {
        virtual uint32_t addStorageBuffer(const ccstd::string& name, gfx::Format format, uint32_t size, ResourceResidency residency = ResourceResidency::MANAGED) = 0;
        virtual uint32_t addStorageTexture(const ccstd::string& name, gfx::Format format, uint32_t width, uint32_t height, ResourceResidency residency = ResourceResidency::MANAGED) = 0;
        virtual uint32_t addShadingRateTexture(const ccstd::string& name, uint32_t width, uint32_t height, ResourceResidency residency = ResourceResidency::MANAGED) = 0;
        virtual void updateStorageBuffer(const ccstd::string& name, uint32_t size, gfx::Format format = gfx::Format::UNKNOWN) = 0;
        virtual void updateStorageTexture(const ccstd::string& name, uint32_t width, uint32_t height, gfx::Format format = gfx::Format::UNKNOWN) = 0;
        virtual void updateShadingRateTexture(const ccstd::string& name, uint32_t width, uint32_t height) = 0;
        virtual RenderPassBuilder* addRenderPass(uint32_t width, uint32_t height, const ccstd::string& passName) = 0;
        virtual MultisampleRenderPassBuilder* addMultisampleRenderPass(uint32_t width, uint32_t height, uint32_t count, uint32_t quality, const ccstd::string& passName) = 0;
        virtual ComputePassBuilder* addComputePass(const ccstd::string& passName) = 0;
        virtual void addUploadPass(ccstd::vector<UploadPair>& uploadPairs) = 0;
        virtual void addMovePass(const ccstd::vector<MovePair>& movePairs) = 0;
        virtual uint32_t addCustomBuffer(const ccstd::string& name, const gfx::BufferInfo& info, const std::string& type) = 0;
        virtual uint32_t addCustomTexture(const ccstd::string& name, const gfx::TextureInfo& info, const std::string& type) = 0;
    }

    interface PipelinePassBuilder {
        virtual uint32_t getConfigOrder() const = 0;
        virtual uint32_t getRenderOrder() const = 0;
    }

    interface PipelineBuilder {
        @optionalMethod virtual void windowResize(BasicPipeline* pipeline, scene::RenderWindow* window, scene::Camera* camera, uint32_t width, uint32_t height) = 0;
        virtual void setup(const ccstd::vector<scene::Camera*>& cameras, BasicPipeline* pipeline) = 0;
        @optionalMethod virtual void onGlobalPipelineStateChanged() = 0;
    }

    interface RenderingModule {
        virtual uint32_t getPassID(const ccstd::string& name) const = 0;
        virtual uint32_t getSubpassID(uint32_t passID, const ccstd::string& name) const = 0;
        virtual uint32_t getPhaseID(uint32_t subpassOrPassID, const ccstd::string& name) const = 0;
    }

    @cppOnly struct Factory {
        static RenderingModule* init(gfx::Device* deviceIn, const ccstd::vector<uint8_t>& bufferIn);
        static void destroy(RenderingModule* renderingModule) noexcept;
        static Pipeline* createPipeline();
    }
}

binding typescript {
    PipelineCapabilities => class { constructor(); };
    PipelinePassBuilder => interface {
        callback configCamera?(camera: Readonly<scene::Camera>, pplConfigs: Readonly<Record<string, any>>,
                               cameraConfigs: Record<string, any>): void;
        callback windowResize?(ppl: BasicPipeline, pplConfigs: Readonly<Record<string, any>>,
                               cameraConfigs: Readonly<Record<string, any>>, window: scene::RenderWindow,
                               camera: scene::Camera, width: number, height: number): void;
        callback setup?(ppl: BasicPipeline, pplConfigs: Readonly<Record<string, any>>,
                        cameraConfigs: Readonly<Record<string, any>>, camera: scene::Camera, context: Record<string, any>,
                        prevRenderPass?: BasicRenderPassBuilder): BasicRenderPassBuilder | undefined;
    };
}
