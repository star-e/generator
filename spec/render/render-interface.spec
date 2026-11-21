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
        virtual gfx::Device* getDevice() const = 0;
        virtual const MacroRecord& getMacros() const = 0;
        virtual pipeline::GlobalDSManager* getGlobalDSManager() const = 0;
        virtual gfx::DescriptorSetLayout* getDescriptorSetLayout() const = 0;
        virtual gfx::DescriptorSet* getDescriptorSet() const = 0;
        virtual const ccstd::vector<gfx::CommandBuffer*>& getCommandBuffers() const = 0;
        virtual pipeline::PipelineSceneData* getPipelineSceneData() const = 0;
        virtual const ccstd::string& getConstantMacros() const = 0;
        virtual scene::Model* getProfiler() const = 0;
        virtual void setProfiler(scene::Model* profiler) = 0;
        virtual pipeline::GeometryRenderer* getGeometryRenderer() const = 0;
        virtual float getShadingScale() const = 0;
        virtual void setShadingScale(float scale) = 0;
        virtual const ccstd::string& getMacroString(const ccstd::string& name) const = 0;
        virtual int32_t getMacroInt(const ccstd::string& name) const = 0;
        virtual bool getMacroBool(const ccstd::string& name) const = 0;
        virtual void setMacroString(const ccstd::string& name, const ccstd::string& value) = 0;
        virtual void setMacroInt(const ccstd::string& name, int32_t value) = 0;
        virtual void setMacroBool(const ccstd::string& name, bool value) = 0;
        virtual void onGlobalPipelineStateChanged() = 0;
        virtual void setValue(const ccstd::string& name, int32_t value) = 0;
        virtual void setValue(const ccstd::string& name, bool value) = 0;
        virtual bool isOcclusionQueryEnabled() const = 0;
        virtual void resetRenderQueue(bool reset) = 0;
        virtual bool isRenderQueueReset() const = 0;
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
        virtual ccstd::string getName() const = 0;
        virtual void setName(const ccstd::string& name) = 0;
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
        virtual void setBuiltinDirectionalLightFrustumConstants(const scene::Camera* camera, const scene::DirectionalLight* light, uint32_t csmLevel) = 0;
        virtual void setBuiltinSpotLightFrustumConstants(const scene::SpotLight* light) = 0;
        void setBuiltinDirectionalLightFrustumConstants(const scene::Camera* camera, const scene::DirectionalLight* light) { return setBuiltinDirectionalLightFrustumConstants(camera, light, 0); }
    }

    interface SceneBuilder : Setter {
        virtual void useLightFrustum(IntrusivePtr<scene::Light> light, uint32_t csmLevel, const scene::Camera* optCamera) = 0;
        void useLightFrustum(IntrusivePtr<scene::Light> light) { return useLightFrustum(std::move(light), 0, nullptr); }
        void useLightFrustum(IntrusivePtr<scene::Light> light, uint32_t csmLevel) { return useLightFrustum(std::move(light), csmLevel, nullptr); }
    }

    interface RenderQueueBuilder : Setter {
        virtual void addSceneOfCamera(scene::Camera* camera, LightInfo light, SceneFlags sceneFlags) = 0;
        virtual SceneBuilder* addScene(const scene::Camera* camera, SceneFlags sceneFlags, scene::Light* light, scene::RenderScene* scene) = 0;
        virtual void addFullscreenQuad(Material* material, uint32_t passID, SceneFlags sceneFlags) = 0;
        virtual void addCameraQuad(scene::Camera* camera, Material* material, uint32_t passID, SceneFlags sceneFlags) = 0;
        virtual void addDraw3D(const scene::Camera* camera, const std::vector<scene::Model*>& models, SceneFlags sceneFlags) = 0;
        virtual void addDraw2D(const scene::Camera* camera) = 0;
        virtual void addProfiler(const scene::Camera* camera) = 0;
        virtual void clearRenderTarget(const ccstd::string& name, const gfx::Color& color) = 0;
        virtual void setViewport(const gfx::Viewport& viewport) = 0;
        virtual void addCustomCommand(std::string_view customBehavior) = 0;
        void addSceneOfCamera(scene::Camera* camera, LightInfo light) { return addSceneOfCamera(camera, std::move(light), SceneFlags::NONE); }
        SceneBuilder* addScene(const scene::Camera* camera, SceneFlags sceneFlags) { return addScene(camera, sceneFlags, nullptr, nullptr); }
        SceneBuilder* addScene(const scene::Camera* camera, SceneFlags sceneFlags, scene::Light* light) { return addScene(camera, sceneFlags, light, nullptr); }
        void addFullscreenQuad(Material* material, uint32_t passID) { return addFullscreenQuad(material, passID, SceneFlags::NONE); }
        void addCameraQuad(scene::Camera* camera, Material* material, uint32_t passID) { return addCameraQuad(camera, material, passID, SceneFlags::NONE); }
        void addDraw3D(const scene::Camera* camera, const std::vector<scene::Model*>& models) { return addDraw3D(camera, models, SceneFlags::NON_BUILTIN); }
        void clearRenderTarget(const ccstd::string& name) { return clearRenderTarget(name, {}); }
    }

    interface BasicRenderPassBuilder : Setter {
        virtual void addRenderTarget(const ccstd::string& name, gfx::LoadOp loadOp, gfx::StoreOp storeOp, const gfx::Color& color) = 0;
        virtual void addDepthStencil(const ccstd::string& name, gfx::LoadOp loadOp, gfx::StoreOp storeOp, float depth, uint8_t stencil, gfx::ClearFlagBit clearFlags) = 0;
        virtual void addTexture(const ccstd::string& name, const ccstd::string& slotName, gfx::Sampler* sampler, uint32_t plane) = 0;
        virtual RenderQueueBuilder* addQueue(QueueHint hint, const ccstd::string& phaseName, const ccstd::string& passName) = 0;
        virtual void setViewport(const gfx::Viewport& viewport) = 0;
        virtual void setVersion(const ccstd::string& name, uint64_t version) = 0;
        virtual bool getShowStatistics() const = 0;
        virtual void setShowStatistics(bool enable) = 0;
        void addRenderTarget(const ccstd::string& name) { return addRenderTarget(name, gfx::LoadOp::CLEAR, gfx::StoreOp::STORE, {}); }
        void addRenderTarget(const ccstd::string& name, gfx::LoadOp loadOp) { return addRenderTarget(name, loadOp, gfx::StoreOp::STORE, {}); }
        void addRenderTarget(const ccstd::string& name, gfx::LoadOp loadOp, gfx::StoreOp storeOp) { return addRenderTarget(name, loadOp, storeOp, {}); }
        void addDepthStencil(const ccstd::string& name) { return addDepthStencil(name, gfx::LoadOp::CLEAR, gfx::StoreOp::STORE, 1, 0, gfx::ClearFlagBit::DEPTH_STENCIL); }
        void addDepthStencil(const ccstd::string& name, gfx::LoadOp loadOp) { return addDepthStencil(name, loadOp, gfx::StoreOp::STORE, 1, 0, gfx::ClearFlagBit::DEPTH_STENCIL); }
        void addDepthStencil(const ccstd::string& name, gfx::LoadOp loadOp, gfx::StoreOp storeOp) { return addDepthStencil(name, loadOp, storeOp, 1, 0, gfx::ClearFlagBit::DEPTH_STENCIL); }
        void addDepthStencil(const ccstd::string& name, gfx::LoadOp loadOp, gfx::StoreOp storeOp, float depth) { return addDepthStencil(name, loadOp, storeOp, depth, 0, gfx::ClearFlagBit::DEPTH_STENCIL); }
        void addDepthStencil(const ccstd::string& name, gfx::LoadOp loadOp, gfx::StoreOp storeOp, float depth, uint8_t stencil) { return addDepthStencil(name, loadOp, storeOp, depth, stencil, gfx::ClearFlagBit::DEPTH_STENCIL); }
        void addTexture(const ccstd::string& name, const ccstd::string& slotName) { return addTexture(name, slotName, nullptr, 0); }
        void addTexture(const ccstd::string& name, const ccstd::string& slotName, gfx::Sampler* sampler) { return addTexture(name, slotName, sampler, 0); }
        RenderQueueBuilder* addQueue() { return addQueue(QueueHint::NONE, "default", ""); }
        RenderQueueBuilder* addQueue(QueueHint hint) { return addQueue(hint, "default", ""); }
        RenderQueueBuilder* addQueue(QueueHint hint, const ccstd::string& phaseName) { return addQueue(hint, phaseName, ""); }
    }

    interface BasicMultisampleRenderPassBuilder : BasicRenderPassBuilder {
        virtual void resolveRenderTarget(const ccstd::string& source, const ccstd::string& target) = 0;
        virtual void resolveDepthStencil(const ccstd::string& source, const ccstd::string& target, gfx::ResolveMode depthMode, gfx::ResolveMode stencilMode) = 0;
        void resolveDepthStencil(const ccstd::string& source, const ccstd::string& target) { return resolveDepthStencil(source, target, gfx::ResolveMode::SAMPLE_ZERO, gfx::ResolveMode::SAMPLE_ZERO); }
        void resolveDepthStencil(const ccstd::string& source, const ccstd::string& target, gfx::ResolveMode depthMode) { return resolveDepthStencil(source, target, depthMode, gfx::ResolveMode::SAMPLE_ZERO); }
    }

    interface BasicPipeline : PipelineRuntime {
        virtual PipelineType getType() const = 0;
        virtual PipelineCapabilities getCapabilities() const = 0;
        virtual void beginSetup() = 0;
        virtual void endSetup() = 0;
        virtual bool getEnableCpuLightCulling() const = 0;
        virtual void setEnableCpuLightCulling(bool enable) = 0;
        virtual bool containsResource(const ccstd::string& name) const = 0;
        virtual uint32_t addRenderWindow(const ccstd::string& name, gfx::Format format, uint32_t width, uint32_t height, scene::RenderWindow* renderWindow, const ccstd::string& depthStencilName) = 0;
        virtual void updateRenderWindow(const ccstd::string& name, scene::RenderWindow* renderWindow, const ccstd::string& depthStencilName) = 0;
        virtual uint32_t addRenderTarget(const ccstd::string& name, gfx::Format format, uint32_t width, uint32_t height, ResourceResidency residency) = 0;
        virtual uint32_t addDepthStencil(const ccstd::string& name, gfx::Format format, uint32_t width, uint32_t height, ResourceResidency residency) = 0;
        virtual void updateRenderTarget(const ccstd::string& name, uint32_t width, uint32_t height, gfx::Format format) = 0;
        virtual void updateDepthStencil(const ccstd::string& name, uint32_t width, uint32_t height, gfx::Format format) = 0;
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
        virtual BasicRenderPassBuilder* addRenderPass(uint32_t width, uint32_t height, const ccstd::string& passName) = 0;
        virtual BasicMultisampleRenderPassBuilder* addMultisampleRenderPass(uint32_t width, uint32_t height, uint32_t count, uint32_t quality, const ccstd::string& passName) = 0;
        virtual void addResolvePass(const ccstd::vector<ResolvePair>& resolvePairs) = 0;
        virtual void addCopyPass(const ccstd::vector<CopyPair>& copyPairs) = 0;
        virtual void addBuiltinReflectionProbePass(const scene::Camera* camera) = 0;
        virtual gfx::DescriptorSetLayout* getDescriptorSetLayout(const ccstd::string& shaderName, UpdateFrequency freq) = 0;
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
        virtual void setBuiltinDirectionalLightFrustumConstants(const scene::Camera* camera, const scene::DirectionalLight* light, uint32_t csmLevel) = 0;
        virtual void setBuiltinSpotLightFrustumConstants(const scene::SpotLight* light) = 0;
        uint32_t addRenderWindow(const ccstd::string& name, gfx::Format format, uint32_t width, uint32_t height, scene::RenderWindow* renderWindow) { return addRenderWindow(name, format, width, height, renderWindow, ""); }
        void updateRenderWindow(const ccstd::string& name, scene::RenderWindow* renderWindow) { return updateRenderWindow(name, renderWindow, ""); }
        uint32_t addRenderTarget(const ccstd::string& name, gfx::Format format, uint32_t width, uint32_t height) { return addRenderTarget(name, format, width, height, ResourceResidency::MANAGED); }
        uint32_t addDepthStencil(const ccstd::string& name, gfx::Format format, uint32_t width, uint32_t height) { return addDepthStencil(name, format, width, height, ResourceResidency::MANAGED); }
        void updateRenderTarget(const ccstd::string& name, uint32_t width, uint32_t height) { return updateRenderTarget(name, width, height, gfx::Format::UNKNOWN); }
        void updateDepthStencil(const ccstd::string& name, uint32_t width, uint32_t height) { return updateDepthStencil(name, width, height, gfx::Format::UNKNOWN); }
        BasicRenderPassBuilder* addRenderPass(uint32_t width, uint32_t height) { return addRenderPass(width, height, "default"); }
        BasicMultisampleRenderPassBuilder* addMultisampleRenderPass(uint32_t width, uint32_t height, uint32_t count, uint32_t quality) { return addMultisampleRenderPass(width, height, count, quality, "default"); }
        void setBuiltinDirectionalLightFrustumConstants(const scene::Camera* camera, const scene::DirectionalLight* light) { return setBuiltinDirectionalLightFrustumConstants(camera, light, 0); }
    }

    interface RenderSubpassBuilder : Setter {
        virtual void addRenderTarget(const ccstd::string& name, AccessType accessType, const ccstd::string& slotName, gfx::LoadOp loadOp, gfx::StoreOp storeOp, const gfx::Color& color) = 0;
        virtual void addDepthStencil(const ccstd::string& name, AccessType accessType, const ccstd::string& depthSlotName, const ccstd::string& stencilSlotName, gfx::LoadOp loadOp, gfx::StoreOp storeOp, float depth, uint8_t stencil, gfx::ClearFlagBit clearFlags) = 0;
        virtual void addTexture(const ccstd::string& name, const ccstd::string& slotName, gfx::Sampler* sampler, uint32_t plane) = 0;
        virtual void addStorageBuffer(const ccstd::string& name, AccessType accessType, const ccstd::string& slotName) = 0;
        virtual void addStorageImage(const ccstd::string& name, AccessType accessType, const ccstd::string& slotName) = 0;
        virtual void setViewport(const gfx::Viewport& viewport) = 0;
        virtual RenderQueueBuilder* addQueue(QueueHint hint, const ccstd::string& phaseName, const ccstd::string& passName) = 0;
        virtual bool getShowStatistics() const = 0;
        virtual void setShowStatistics(bool enable) = 0;
        virtual void setCustomShaderStages(const ccstd::string& name, gfx::ShaderStageFlagBit stageFlags) = 0;
        void addRenderTarget(const ccstd::string& name, AccessType accessType) { return addRenderTarget(name, accessType, "", gfx::LoadOp::CLEAR, gfx::StoreOp::STORE, {}); }
        void addRenderTarget(const ccstd::string& name, AccessType accessType, const ccstd::string& slotName) { return addRenderTarget(name, accessType, slotName, gfx::LoadOp::CLEAR, gfx::StoreOp::STORE, {}); }
        void addRenderTarget(const ccstd::string& name, AccessType accessType, const ccstd::string& slotName, gfx::LoadOp loadOp) { return addRenderTarget(name, accessType, slotName, loadOp, gfx::StoreOp::STORE, {}); }
        void addRenderTarget(const ccstd::string& name, AccessType accessType, const ccstd::string& slotName, gfx::LoadOp loadOp, gfx::StoreOp storeOp) { return addRenderTarget(name, accessType, slotName, loadOp, storeOp, {}); }
        void addDepthStencil(const ccstd::string& name, AccessType accessType) { return addDepthStencil(name, accessType, "", "", gfx::LoadOp::CLEAR, gfx::StoreOp::STORE, 1, 0, gfx::ClearFlagBit::DEPTH_STENCIL); }
        void addDepthStencil(const ccstd::string& name, AccessType accessType, const ccstd::string& depthSlotName) { return addDepthStencil(name, accessType, depthSlotName, "", gfx::LoadOp::CLEAR, gfx::StoreOp::STORE, 1, 0, gfx::ClearFlagBit::DEPTH_STENCIL); }
        void addDepthStencil(const ccstd::string& name, AccessType accessType, const ccstd::string& depthSlotName, const ccstd::string& stencilSlotName) { return addDepthStencil(name, accessType, depthSlotName, stencilSlotName, gfx::LoadOp::CLEAR, gfx::StoreOp::STORE, 1, 0, gfx::ClearFlagBit::DEPTH_STENCIL); }
        void addDepthStencil(const ccstd::string& name, AccessType accessType, const ccstd::string& depthSlotName, const ccstd::string& stencilSlotName, gfx::LoadOp loadOp) { return addDepthStencil(name, accessType, depthSlotName, stencilSlotName, loadOp, gfx::StoreOp::STORE, 1, 0, gfx::ClearFlagBit::DEPTH_STENCIL); }
        void addDepthStencil(const ccstd::string& name, AccessType accessType, const ccstd::string& depthSlotName, const ccstd::string& stencilSlotName, gfx::LoadOp loadOp, gfx::StoreOp storeOp) { return addDepthStencil(name, accessType, depthSlotName, stencilSlotName, loadOp, storeOp, 1, 0, gfx::ClearFlagBit::DEPTH_STENCIL); }
        void addDepthStencil(const ccstd::string& name, AccessType accessType, const ccstd::string& depthSlotName, const ccstd::string& stencilSlotName, gfx::LoadOp loadOp, gfx::StoreOp storeOp, float depth) { return addDepthStencil(name, accessType, depthSlotName, stencilSlotName, loadOp, storeOp, depth, 0, gfx::ClearFlagBit::DEPTH_STENCIL); }
        void addDepthStencil(const ccstd::string& name, AccessType accessType, const ccstd::string& depthSlotName, const ccstd::string& stencilSlotName, gfx::LoadOp loadOp, gfx::StoreOp storeOp, float depth, uint8_t stencil) { return addDepthStencil(name, accessType, depthSlotName, stencilSlotName, loadOp, storeOp, depth, stencil, gfx::ClearFlagBit::DEPTH_STENCIL); }
        void addTexture(const ccstd::string& name, const ccstd::string& slotName) { return addTexture(name, slotName, nullptr, 0); }
        void addTexture(const ccstd::string& name, const ccstd::string& slotName, gfx::Sampler* sampler) { return addTexture(name, slotName, sampler, 0); }
        RenderQueueBuilder* addQueue() { return addQueue(QueueHint::NONE, "default", ""); }
        RenderQueueBuilder* addQueue(QueueHint hint) { return addQueue(hint, "default", ""); }
        RenderQueueBuilder* addQueue(QueueHint hint, const ccstd::string& phaseName) { return addQueue(hint, phaseName, ""); }
    }

    interface MultisampleRenderSubpassBuilder : RenderSubpassBuilder {
        virtual void resolveRenderTarget(const ccstd::string& source, const ccstd::string& target) = 0;
        virtual void resolveDepthStencil(const ccstd::string& source, const ccstd::string& target, gfx::ResolveMode depthMode, gfx::ResolveMode stencilMode) = 0;
        void resolveDepthStencil(const ccstd::string& source, const ccstd::string& target) { return resolveDepthStencil(source, target, gfx::ResolveMode::SAMPLE_ZERO, gfx::ResolveMode::SAMPLE_ZERO); }
        void resolveDepthStencil(const ccstd::string& source, const ccstd::string& target, gfx::ResolveMode depthMode) { return resolveDepthStencil(source, target, depthMode, gfx::ResolveMode::SAMPLE_ZERO); }
    }

    interface ComputeQueueBuilder : Setter {
        virtual void addDispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ, Material* material, uint32_t passID) = 0;
        void addDispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ) { return addDispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ, nullptr, 0); }
        void addDispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ, Material* material) { return addDispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ, material, 0); }
    }

    interface ComputeSubpassBuilder : Setter {
        virtual void addRenderTarget(const ccstd::string& name, const ccstd::string& slotName) = 0;
        virtual void addTexture(const ccstd::string& name, const ccstd::string& slotName, gfx::Sampler* sampler, uint32_t plane) = 0;
        virtual void addStorageBuffer(const ccstd::string& name, AccessType accessType, const ccstd::string& slotName) = 0;
        virtual void addStorageImage(const ccstd::string& name, AccessType accessType, const ccstd::string& slotName) = 0;
        virtual ComputeQueueBuilder* addQueue(const ccstd::string& phaseName, const ccstd::string& passName) = 0;
        virtual void setCustomShaderStages(const ccstd::string& name, gfx::ShaderStageFlagBit stageFlags) = 0;
        void addTexture(const ccstd::string& name, const ccstd::string& slotName) { return addTexture(name, slotName, nullptr, 0); }
        void addTexture(const ccstd::string& name, const ccstd::string& slotName, gfx::Sampler* sampler) { return addTexture(name, slotName, sampler, 0); }
        ComputeQueueBuilder* addQueue() { return addQueue("default", ""); }
        ComputeQueueBuilder* addQueue(const ccstd::string& phaseName) { return addQueue(phaseName, ""); }
    }

    interface RenderPassBuilder : BasicRenderPassBuilder {
        virtual void addStorageBuffer(const ccstd::string& name, AccessType accessType, const ccstd::string& slotName) = 0;
        virtual void addStorageImage(const ccstd::string& name, AccessType accessType, const ccstd::string& slotName) = 0;
        virtual void addMaterialTexture(const ccstd::string& resourceName, gfx::ShaderStageFlagBit flags) = 0;
        virtual RenderSubpassBuilder* addRenderSubpass(const ccstd::string& subpassName) = 0;
        virtual MultisampleRenderSubpassBuilder* addMultisampleRenderSubpass(uint32_t count, uint32_t quality, const ccstd::string& subpassName) = 0;
        virtual ComputeSubpassBuilder* addComputeSubpass(const ccstd::string& subpassName) = 0;
        virtual void setCustomShaderStages(const ccstd::string& name, gfx::ShaderStageFlagBit stageFlags) = 0;
        void addMaterialTexture(const ccstd::string& resourceName) { return addMaterialTexture(resourceName, gfx::ShaderStageFlagBit::VERTEX | gfx::ShaderStageFlagBit::FRAGMENT); }
        ComputeSubpassBuilder* addComputeSubpass() { return addComputeSubpass(""); }
    }

    interface MultisampleRenderPassBuilder : BasicMultisampleRenderPassBuilder {
        virtual void addStorageBuffer(const ccstd::string& name, AccessType accessType, const ccstd::string& slotName) = 0;
        virtual void addStorageImage(const ccstd::string& name, AccessType accessType, const ccstd::string& slotName) = 0;
    }

    interface ComputePassBuilder : Setter {
        virtual void addTexture(const ccstd::string& name, const ccstd::string& slotName, gfx::Sampler* sampler, uint32_t plane) = 0;
        virtual void addStorageBuffer(const ccstd::string& name, AccessType accessType, const ccstd::string& slotName) = 0;
        virtual void addStorageImage(const ccstd::string& name, AccessType accessType, const ccstd::string& slotName) = 0;
        virtual void addMaterialTexture(const ccstd::string& resourceName, gfx::ShaderStageFlagBit flags) = 0;
        virtual ComputeQueueBuilder* addQueue(const ccstd::string& phaseName, const ccstd::string& passName) = 0;
        virtual void setCustomShaderStages(const ccstd::string& name, gfx::ShaderStageFlagBit stageFlags) = 0;
        void addTexture(const ccstd::string& name, const ccstd::string& slotName) { return addTexture(name, slotName, nullptr, 0); }
        void addTexture(const ccstd::string& name, const ccstd::string& slotName, gfx::Sampler* sampler) { return addTexture(name, slotName, sampler, 0); }
        void addMaterialTexture(const ccstd::string& resourceName) { return addMaterialTexture(resourceName, gfx::ShaderStageFlagBit::COMPUTE); }
        ComputeQueueBuilder* addQueue() { return addQueue("default", ""); }
        ComputeQueueBuilder* addQueue(const ccstd::string& phaseName) { return addQueue(phaseName, ""); }
    }

    interface Pipeline : BasicPipeline {
        virtual uint32_t addStorageBuffer(const ccstd::string& name, gfx::Format format, uint32_t size, ResourceResidency residency) = 0;
        virtual uint32_t addStorageTexture(const ccstd::string& name, gfx::Format format, uint32_t width, uint32_t height, ResourceResidency residency) = 0;
        virtual uint32_t addShadingRateTexture(const ccstd::string& name, uint32_t width, uint32_t height, ResourceResidency residency) = 0;
        virtual void updateStorageBuffer(const ccstd::string& name, uint32_t size, gfx::Format format) = 0;
        virtual void updateStorageTexture(const ccstd::string& name, uint32_t width, uint32_t height, gfx::Format format) = 0;
        virtual void updateShadingRateTexture(const ccstd::string& name, uint32_t width, uint32_t height) = 0;
        virtual RenderPassBuilder* addRenderPass(uint32_t width, uint32_t height, const ccstd::string& passName) = 0;
        virtual MultisampleRenderPassBuilder* addMultisampleRenderPass(uint32_t width, uint32_t height, uint32_t count, uint32_t quality, const ccstd::string& passName) = 0;
        virtual ComputePassBuilder* addComputePass(const ccstd::string& passName) = 0;
        virtual void addUploadPass(ccstd::vector<UploadPair>& uploadPairs) = 0;
        virtual void addMovePass(const ccstd::vector<MovePair>& movePairs) = 0;
        virtual uint32_t addCustomBuffer(const ccstd::string& name, const gfx::BufferInfo& info, const std::string& type) = 0;
        virtual uint32_t addCustomTexture(const ccstd::string& name, const gfx::TextureInfo& info, const std::string& type) = 0;
        uint32_t addStorageBuffer(const ccstd::string& name, gfx::Format format, uint32_t size) { return addStorageBuffer(name, format, size, ResourceResidency::MANAGED); }
        uint32_t addStorageTexture(const ccstd::string& name, gfx::Format format, uint32_t width, uint32_t height) { return addStorageTexture(name, format, width, height, ResourceResidency::MANAGED); }
        uint32_t addShadingRateTexture(const ccstd::string& name, uint32_t width, uint32_t height) { return addShadingRateTexture(name, width, height, ResourceResidency::MANAGED); }
        void updateStorageBuffer(const ccstd::string& name, uint32_t size) { return updateStorageBuffer(name, size, gfx::Format::UNKNOWN); }
        void updateStorageTexture(const ccstd::string& name, uint32_t width, uint32_t height) { return updateStorageTexture(name, width, height, gfx::Format::UNKNOWN); }
    }

    interface PipelinePassBuilder {
        virtual uint32_t getConfigOrder() const = 0;
        virtual uint32_t getRenderOrder() const = 0;
    }

    interface PipelineBuilder {
        virtual void windowResize(BasicPipeline* pipeline, scene::RenderWindow* window, scene::Camera* camera, uint32_t width, uint32_t height) = 0;
        virtual void setup(const ccstd::vector<scene::Camera*>& cameras, BasicPipeline* pipeline) = 0;
        virtual void onGlobalPipelineStateChanged() = 0;
    }

    interface RenderingModule {
        virtual uint32_t getPassID(const ccstd::string& name) const = 0;
        virtual uint32_t getSubpassID(uint32_t passID, const ccstd::string& name) const = 0;
        virtual uint32_t getPhaseID(uint32_t subpassOrPassID, const ccstd::string& name) const = 0;
    }

    struct Factory {
        static RenderingModule* init(gfx::Device* deviceIn, const ccstd::vector<uint8_t>& bufferIn);
        static void destroy(RenderingModule* renderingModule) noexcept;
        static Pipeline* createPipeline();
    }
}

binding typescript {
    export { PipelineType, SubpassCapabilities, PipelineCapabilities, RenderNode, Setter, SceneBuilder,
             RenderQueueBuilder, ComputeQueueBuilder, RenderingModule };
    PipelineCapabilities => class { constructor(); };
    RenderNode => interface {
        getter getName => name setter setName;
    };
    Setter => interface {
        method setBuiltinDirectionalLightFrustumConstants optional(csmLevel);
    };
    SceneBuilder => interface {
        method useLightFrustum optional(csmLevel, optCamera);
    };
    RenderQueueBuilder => interface {
        method addSceneOfCamera optional(sceneFlags);
        method addScene optional(light, scene);
        method addFullscreenQuad optional(sceneFlags);
        method addCameraQuad optional(sceneFlags);
        method addDraw3D optional(sceneFlags);
        method clearRenderTarget optional(color);
    };
    ComputeQueueBuilder => interface {
        method addDispatch optional(material, passID);
    };
}
