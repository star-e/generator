module RenderInterface;
import EngineTypes;
import Containers;
import Gfx;
import GfxResource;
import Scene;
import Light;
import RenderWindow;
import GfxDescriptorSet;
import RenderCommon;
import PipelineCapabilities;
import PipelineExternals;

namespace cc::render {
    interface PipelineRuntime {
        bool activate(gfx::Swapchain* swapchain);
        bool destroy() noexcept;
        void render(const ccstd::vector<scene::Camera*>& cameras);
        gfx::Device* getDevice() const;
        const MacroRecord& getMacros() const;
        pipeline::GlobalDSManager* getGlobalDSManager() const;
        gfx::DescriptorSetLayout* getDescriptorSetLayout() const;
        gfx::DescriptorSet* getDescriptorSet() const;
        const ccstd::vector<gfx::CommandBuffer*>& getCommandBuffers() const;
        pipeline::PipelineSceneData* getPipelineSceneData() const;
        const ccstd::string& getConstantMacros() const;
        scene::Model* getProfiler() const;
        void setProfiler(scene::Model* profiler);
        pipeline::GeometryRenderer* getGeometryRenderer() const;
        float getShadingScale() const;
        void setShadingScale(float scale);
        const ccstd::string& getMacroString(const ccstd::string& name) const;
        int32_t getMacroInt(const ccstd::string& name) const;
        bool getMacroBool(const ccstd::string& name) const;
        void setMacroString(const ccstd::string& name, const ccstd::string& value);
        void setMacroInt(const ccstd::string& name, int32_t value);
        void setMacroBool(const ccstd::string& name, bool value);
        void onGlobalPipelineStateChanged();
        void setValue(const ccstd::string& name, int32_t value);
        void setValue(const ccstd::string& name, bool value);
        bool isOcclusionQueryEnabled() const;
        void resetRenderQueue(bool reset);
        bool isRenderQueueReset() const;
    }

    interface RenderNode {
        ccstd::string getName() const;
        void setName(const ccstd::string& name);
        void setCustomBehavior(const ccstd::string& name);
    }

    interface Setter : RenderNode {
        void setMat4(const ccstd::string& name, const Mat4& mat);
        void setQuaternion(const ccstd::string& name, const Quaternion& quat);
        void setColor(const ccstd::string& name, const gfx::Color& color);
        void setVec4(const ccstd::string& name, const Vec4& vec);
        void setVec2(const ccstd::string& name, const Vec2& vec);
        void setFloat(const ccstd::string& name, float v);
        void setArrayBuffer(const ccstd::string& name, const ArrayBuffer* arrayBuffer);
        void setBuffer(const ccstd::string& name, gfx::Buffer* buffer);
        void setTexture(const ccstd::string& name, gfx::Texture* texture);
        void setSampler(const ccstd::string& name, gfx::Sampler* sampler);
        void setBuiltinCameraConstants(const scene::Camera* camera);
        void setBuiltinDirectionalLightConstants(const scene::DirectionalLight* light, const scene::Camera* camera);
        void setBuiltinSphereLightConstants(const scene::SphereLight* light, const scene::Camera* camera);
        void setBuiltinSpotLightConstants(const scene::SpotLight* light, const scene::Camera* camera);
        void setBuiltinPointLightConstants(const scene::PointLight* light, const scene::Camera* camera);
        void setBuiltinRangedDirectionalLightConstants(const scene::RangedDirectionalLight* light, const scene::Camera* camera);
        void setBuiltinDirectionalLightFrustumConstants(const scene::Camera* camera, const scene::DirectionalLight* light, uint32_t csmLevel);
        void setBuiltinSpotLightFrustumConstants(const scene::SpotLight* light);
        fn setBuiltinDirectionalLightFrustumConstants(camera: const scene::Camera*, light: const scene::DirectionalLight*) -> void => "setBuiltinDirectionalLightFrustumConstants(camera, light, 0)";
    }

    interface SceneBuilder : Setter {
        void useLightFrustum(IntrusivePtr<scene::Light> light, uint32_t csmLevel, const scene::Camera* optCamera);
        fn useLightFrustum(light: IntrusivePtr<scene::Light>) -> void => "useLightFrustum(std::move(light), 0, nullptr)";
        fn useLightFrustum(light: IntrusivePtr<scene::Light>, csmLevel: uint32_t) -> void => "useLightFrustum(std::move(light), csmLevel, nullptr)";
    }

    interface RenderQueueBuilder : Setter {
        void addSceneOfCamera(scene::Camera* camera, LightInfo light, SceneFlags sceneFlags);
        SceneBuilder* addScene(const scene::Camera* camera, SceneFlags sceneFlags, scene::Light* light, scene::RenderScene* scene);
        void addFullscreenQuad(Material* material, uint32_t passID, SceneFlags sceneFlags);
        void addCameraQuad(scene::Camera* camera, Material* material, uint32_t passID, SceneFlags sceneFlags);
        void addDraw3D(const scene::Camera* camera, const std::vector<scene::Model*>& models, SceneFlags sceneFlags);
        void addDraw2D(const scene::Camera* camera);
        void addProfiler(const scene::Camera* camera);
        void clearRenderTarget(const ccstd::string& name, const gfx::Color& color);
        void setViewport(const gfx::Viewport& viewport);
        void addCustomCommand(std::string_view customBehavior);
        fn addSceneOfCamera(camera: scene::Camera*, light: LightInfo) -> void => "addSceneOfCamera(camera, std::move(light), SceneFlags::NONE)";
        fn addScene(camera: const scene::Camera*, sceneFlags: SceneFlags) -> SceneBuilder* => "addScene(camera, sceneFlags, nullptr, nullptr)";
        fn addScene(camera: const scene::Camera*, sceneFlags: SceneFlags, light: scene::Light*) -> SceneBuilder* => "addScene(camera, sceneFlags, light, nullptr)";
        fn addFullscreenQuad(material: Material*, passID: uint32_t) -> void => "addFullscreenQuad(material, passID, SceneFlags::NONE)";
        fn addCameraQuad(camera: scene::Camera*, material: Material*, passID: uint32_t) -> void => "addCameraQuad(camera, material, passID, SceneFlags::NONE)";
        fn addDraw3D(camera: const scene::Camera*, models: const std::vector<scene::Model*>&) -> void => "addDraw3D(camera, models, SceneFlags::NON_BUILTIN)";
        fn clearRenderTarget(name: const ccstd::string&) -> void => "clearRenderTarget(name, {})";
    }

    interface BasicRenderPassBuilder : Setter {
        void addRenderTarget(const ccstd::string& name, gfx::LoadOp loadOp, gfx::StoreOp storeOp, const gfx::Color& color);
        void addDepthStencil(const ccstd::string& name, gfx::LoadOp loadOp, gfx::StoreOp storeOp, float depth, uint8_t stencil, gfx::ClearFlagBit clearFlags);
        void addTexture(const ccstd::string& name, const ccstd::string& slotName, gfx::Sampler* sampler, uint32_t plane);
        RenderQueueBuilder* addQueue(QueueHint hint, const ccstd::string& phaseName, const ccstd::string& passName);
        void setViewport(const gfx::Viewport& viewport);
        void setVersion(const ccstd::string& name, uint64_t version);
        bool getShowStatistics() const;
        void setShowStatistics(bool enable);
        fn addRenderTarget(name: const ccstd::string&) -> void => "addRenderTarget(name, gfx::LoadOp::CLEAR, gfx::StoreOp::STORE, {})";
        fn addRenderTarget(name: const ccstd::string&, loadOp: gfx::LoadOp) -> void => "addRenderTarget(name, loadOp, gfx::StoreOp::STORE, {})";
        fn addRenderTarget(name: const ccstd::string&, loadOp: gfx::LoadOp, storeOp: gfx::StoreOp) -> void => "addRenderTarget(name, loadOp, storeOp, {})";
        fn addDepthStencil(name: const ccstd::string&) -> void => "addDepthStencil(name, gfx::LoadOp::CLEAR, gfx::StoreOp::STORE, 1, 0, gfx::ClearFlagBit::DEPTH_STENCIL)";
        fn addDepthStencil(name: const ccstd::string&, loadOp: gfx::LoadOp) -> void => "addDepthStencil(name, loadOp, gfx::StoreOp::STORE, 1, 0, gfx::ClearFlagBit::DEPTH_STENCIL)";
        fn addDepthStencil(name: const ccstd::string&, loadOp: gfx::LoadOp, storeOp: gfx::StoreOp) -> void => "addDepthStencil(name, loadOp, storeOp, 1, 0, gfx::ClearFlagBit::DEPTH_STENCIL)";
        fn addDepthStencil(name: const ccstd::string&, loadOp: gfx::LoadOp, storeOp: gfx::StoreOp, depth: float) -> void => "addDepthStencil(name, loadOp, storeOp, depth, 0, gfx::ClearFlagBit::DEPTH_STENCIL)";
        fn addDepthStencil(name: const ccstd::string&, loadOp: gfx::LoadOp, storeOp: gfx::StoreOp, depth: float, stencil: uint8_t) -> void => "addDepthStencil(name, loadOp, storeOp, depth, stencil, gfx::ClearFlagBit::DEPTH_STENCIL)";
        fn addTexture(name: const ccstd::string&, slotName: const ccstd::string&) -> void => "addTexture(name, slotName, nullptr, 0)";
        fn addTexture(name: const ccstd::string&, slotName: const ccstd::string&, sampler: gfx::Sampler*) -> void => "addTexture(name, slotName, sampler, 0)";
        fn addQueue() -> RenderQueueBuilder* => "addQueue(QueueHint::NONE, \"default\", \"\")";
        fn addQueue(hint: QueueHint) -> RenderQueueBuilder* => "addQueue(hint, \"default\", \"\")";
        fn addQueue(hint: QueueHint, phaseName: const ccstd::string&) -> RenderQueueBuilder* => "addQueue(hint, phaseName, \"\")";
    }

    interface BasicMultisampleRenderPassBuilder : BasicRenderPassBuilder {
        void resolveRenderTarget(const ccstd::string& source, const ccstd::string& target);
        void resolveDepthStencil(const ccstd::string& source, const ccstd::string& target, gfx::ResolveMode depthMode, gfx::ResolveMode stencilMode);
        fn resolveDepthStencil(source: const ccstd::string&, target: const ccstd::string&) -> void => "resolveDepthStencil(source, target, gfx::ResolveMode::SAMPLE_ZERO, gfx::ResolveMode::SAMPLE_ZERO)";
        fn resolveDepthStencil(source: const ccstd::string&, target: const ccstd::string&, depthMode: gfx::ResolveMode) -> void => "resolveDepthStencil(source, target, depthMode, gfx::ResolveMode::SAMPLE_ZERO)";
    }

    interface BasicPipeline : PipelineRuntime {
        PipelineType getType() const;
        PipelineCapabilities getCapabilities() const;
        void beginSetup();
        void endSetup();
        bool getEnableCpuLightCulling() const;
        void setEnableCpuLightCulling(bool enable);
        bool containsResource(const ccstd::string& name) const;
        uint32_t addRenderWindow(const ccstd::string& name, gfx::Format format, uint32_t width, uint32_t height, scene::RenderWindow* renderWindow, const ccstd::string& depthStencilName);
        void updateRenderWindow(const ccstd::string& name, scene::RenderWindow* renderWindow, const ccstd::string& depthStencilName);
        uint32_t addRenderTarget(const ccstd::string& name, gfx::Format format, uint32_t width, uint32_t height, ResourceResidency residency);
        uint32_t addDepthStencil(const ccstd::string& name, gfx::Format format, uint32_t width, uint32_t height, ResourceResidency residency);
        void updateRenderTarget(const ccstd::string& name, uint32_t width, uint32_t height, gfx::Format format);
        void updateDepthStencil(const ccstd::string& name, uint32_t width, uint32_t height, gfx::Format format);
        uint32_t addBuffer(const ccstd::string& name, uint32_t size, ResourceFlags flags, ResourceResidency residency);
        void updateBuffer(const ccstd::string& name, uint32_t size);
        uint32_t addExternalTexture(const ccstd::string& name, gfx::Texture* texture, ResourceFlags flags);
        void updateExternalTexture(const ccstd::string& name, gfx::Texture* texture);
        uint32_t addTexture(const ccstd::string& name, gfx::TextureType type, gfx::Format format, uint32_t width, uint32_t height, uint32_t depth, uint32_t arraySize, uint32_t mipLevels, gfx::SampleCount sampleCount, ResourceFlags flags, ResourceResidency residency);
        void updateTexture(const ccstd::string& name, gfx::Format format, uint32_t width, uint32_t height, uint32_t depth, uint32_t arraySize, uint32_t mipLevels, gfx::SampleCount sampleCount);
        uint32_t addResource(const ccstd::string& name, ResourceDimension dimension, gfx::Format format, uint32_t width, uint32_t height, uint32_t depth, uint32_t arraySize, uint32_t mipLevels, gfx::SampleCount sampleCount, ResourceFlags flags, ResourceResidency residency);
        void updateResource(const ccstd::string& name, gfx::Format format, uint32_t width, uint32_t height, uint32_t depth, uint32_t arraySize, uint32_t mipLevels, gfx::SampleCount sampleCount);
        void beginFrame();
        void update(const scene::Camera* camera);
        void endFrame();
        BasicRenderPassBuilder* addRenderPass(uint32_t width, uint32_t height, const ccstd::string& passName);
        BasicMultisampleRenderPassBuilder* addMultisampleRenderPass(uint32_t width, uint32_t height, uint32_t count, uint32_t quality, const ccstd::string& passName);
        void addResolvePass(const ccstd::vector<ResolvePair>& resolvePairs);
        void addCopyPass(const ccstd::vector<CopyPair>& copyPairs);
        void addBuiltinReflectionProbePass(const scene::Camera* camera);
        gfx::DescriptorSetLayout* getDescriptorSetLayout(const ccstd::string& shaderName, UpdateFrequency freq);
        void setMat4(const ccstd::string& name, const Mat4& mat);
        void setQuaternion(const ccstd::string& name, const Quaternion& quat);
        void setColor(const ccstd::string& name, const gfx::Color& color);
        void setVec4(const ccstd::string& name, const Vec4& vec);
        void setVec2(const ccstd::string& name, const Vec2& vec);
        void setFloat(const ccstd::string& name, float v);
        void setArrayBuffer(const ccstd::string& name, const ArrayBuffer* arrayBuffer);
        void setBuffer(const ccstd::string& name, gfx::Buffer* buffer);
        void setTexture(const ccstd::string& name, gfx::Texture* texture);
        void setSampler(const ccstd::string& name, gfx::Sampler* sampler);
        void setBuiltinCameraConstants(const scene::Camera* camera);
        void setBuiltinDirectionalLightConstants(const scene::DirectionalLight* light, const scene::Camera* camera);
        void setBuiltinSphereLightConstants(const scene::SphereLight* light, const scene::Camera* camera);
        void setBuiltinSpotLightConstants(const scene::SpotLight* light, const scene::Camera* camera);
        void setBuiltinPointLightConstants(const scene::PointLight* light, const scene::Camera* camera);
        void setBuiltinRangedDirectionalLightConstants(const scene::RangedDirectionalLight* light, const scene::Camera* camera);
        void setBuiltinDirectionalLightFrustumConstants(const scene::Camera* camera, const scene::DirectionalLight* light, uint32_t csmLevel);
        void setBuiltinSpotLightFrustumConstants(const scene::SpotLight* light);
        fn addRenderWindow(name: const ccstd::string&, format: gfx::Format, width: uint32_t, height: uint32_t, renderWindow: scene::RenderWindow*) -> uint32_t => "addRenderWindow(name, format, width, height, renderWindow, \"\")";
        fn updateRenderWindow(name: const ccstd::string&, renderWindow: scene::RenderWindow*) -> void => "updateRenderWindow(name, renderWindow, \"\")";
        fn addRenderTarget(name: const ccstd::string&, format: gfx::Format, width: uint32_t, height: uint32_t) -> uint32_t => "addRenderTarget(name, format, width, height, ResourceResidency::MANAGED)";
        fn addDepthStencil(name: const ccstd::string&, format: gfx::Format, width: uint32_t, height: uint32_t) -> uint32_t => "addDepthStencil(name, format, width, height, ResourceResidency::MANAGED)";
        fn updateRenderTarget(name: const ccstd::string&, width: uint32_t, height: uint32_t) -> void => "updateRenderTarget(name, width, height, gfx::Format::UNKNOWN)";
        fn updateDepthStencil(name: const ccstd::string&, width: uint32_t, height: uint32_t) -> void => "updateDepthStencil(name, width, height, gfx::Format::UNKNOWN)";
        fn addRenderPass(width: uint32_t, height: uint32_t) -> BasicRenderPassBuilder* => "addRenderPass(width, height, \"default\")";
        fn addMultisampleRenderPass(width: uint32_t, height: uint32_t, count: uint32_t, quality: uint32_t) -> BasicMultisampleRenderPassBuilder* => "addMultisampleRenderPass(width, height, count, quality, \"default\")";
        fn setBuiltinDirectionalLightFrustumConstants(camera: const scene::Camera*, light: const scene::DirectionalLight*) -> void => "setBuiltinDirectionalLightFrustumConstants(camera, light, 0)";
    }

    interface RenderSubpassBuilder : Setter {
        void addRenderTarget(const ccstd::string& name, AccessType accessType, const ccstd::string& slotName, gfx::LoadOp loadOp, gfx::StoreOp storeOp, const gfx::Color& color);
        void addDepthStencil(const ccstd::string& name, AccessType accessType, const ccstd::string& depthSlotName, const ccstd::string& stencilSlotName, gfx::LoadOp loadOp, gfx::StoreOp storeOp, float depth, uint8_t stencil, gfx::ClearFlagBit clearFlags);
        void addTexture(const ccstd::string& name, const ccstd::string& slotName, gfx::Sampler* sampler, uint32_t plane);
        void addStorageBuffer(const ccstd::string& name, AccessType accessType, const ccstd::string& slotName);
        void addStorageImage(const ccstd::string& name, AccessType accessType, const ccstd::string& slotName);
        void setViewport(const gfx::Viewport& viewport);
        RenderQueueBuilder* addQueue(QueueHint hint, const ccstd::string& phaseName, const ccstd::string& passName);
        bool getShowStatistics() const;
        void setShowStatistics(bool enable);
        void setCustomShaderStages(const ccstd::string& name, gfx::ShaderStageFlagBit stageFlags);
        fn addRenderTarget(name: const ccstd::string&, accessType: AccessType) -> void => "addRenderTarget(name, accessType, \"\", gfx::LoadOp::CLEAR, gfx::StoreOp::STORE, {})";
        fn addRenderTarget(name: const ccstd::string&, accessType: AccessType, slotName: const ccstd::string&) -> void => "addRenderTarget(name, accessType, slotName, gfx::LoadOp::CLEAR, gfx::StoreOp::STORE, {})";
        fn addRenderTarget(name: const ccstd::string&, accessType: AccessType, slotName: const ccstd::string&, loadOp: gfx::LoadOp) -> void => "addRenderTarget(name, accessType, slotName, loadOp, gfx::StoreOp::STORE, {})";
        fn addRenderTarget(name: const ccstd::string&, accessType: AccessType, slotName: const ccstd::string&, loadOp: gfx::LoadOp, storeOp: gfx::StoreOp) -> void => "addRenderTarget(name, accessType, slotName, loadOp, storeOp, {})";
        fn addDepthStencil(name: const ccstd::string&, accessType: AccessType) -> void => "addDepthStencil(name, accessType, \"\", \"\", gfx::LoadOp::CLEAR, gfx::StoreOp::STORE, 1, 0, gfx::ClearFlagBit::DEPTH_STENCIL)";
        fn addDepthStencil(name: const ccstd::string&, accessType: AccessType, depthSlotName: const ccstd::string&) -> void => "addDepthStencil(name, accessType, depthSlotName, \"\", gfx::LoadOp::CLEAR, gfx::StoreOp::STORE, 1, 0, gfx::ClearFlagBit::DEPTH_STENCIL)";
        fn addDepthStencil(name: const ccstd::string&, accessType: AccessType, depthSlotName: const ccstd::string&, stencilSlotName: const ccstd::string&) -> void => "addDepthStencil(name, accessType, depthSlotName, stencilSlotName, gfx::LoadOp::CLEAR, gfx::StoreOp::STORE, 1, 0, gfx::ClearFlagBit::DEPTH_STENCIL)";
        fn addDepthStencil(name: const ccstd::string&, accessType: AccessType, depthSlotName: const ccstd::string&, stencilSlotName: const ccstd::string&, loadOp: gfx::LoadOp) -> void => "addDepthStencil(name, accessType, depthSlotName, stencilSlotName, loadOp, gfx::StoreOp::STORE, 1, 0, gfx::ClearFlagBit::DEPTH_STENCIL)";
        fn addDepthStencil(name: const ccstd::string&, accessType: AccessType, depthSlotName: const ccstd::string&, stencilSlotName: const ccstd::string&, loadOp: gfx::LoadOp, storeOp: gfx::StoreOp) -> void => "addDepthStencil(name, accessType, depthSlotName, stencilSlotName, loadOp, storeOp, 1, 0, gfx::ClearFlagBit::DEPTH_STENCIL)";
        fn addDepthStencil(name: const ccstd::string&, accessType: AccessType, depthSlotName: const ccstd::string&, stencilSlotName: const ccstd::string&, loadOp: gfx::LoadOp, storeOp: gfx::StoreOp, depth: float) -> void => "addDepthStencil(name, accessType, depthSlotName, stencilSlotName, loadOp, storeOp, depth, 0, gfx::ClearFlagBit::DEPTH_STENCIL)";
        fn addDepthStencil(name: const ccstd::string&, accessType: AccessType, depthSlotName: const ccstd::string&, stencilSlotName: const ccstd::string&, loadOp: gfx::LoadOp, storeOp: gfx::StoreOp, depth: float, stencil: uint8_t) -> void => "addDepthStencil(name, accessType, depthSlotName, stencilSlotName, loadOp, storeOp, depth, stencil, gfx::ClearFlagBit::DEPTH_STENCIL)";
        fn addTexture(name: const ccstd::string&, slotName: const ccstd::string&) -> void => "addTexture(name, slotName, nullptr, 0)";
        fn addTexture(name: const ccstd::string&, slotName: const ccstd::string&, sampler: gfx::Sampler*) -> void => "addTexture(name, slotName, sampler, 0)";
        fn addQueue() -> RenderQueueBuilder* => "addQueue(QueueHint::NONE, \"default\", \"\")";
        fn addQueue(hint: QueueHint) -> RenderQueueBuilder* => "addQueue(hint, \"default\", \"\")";
        fn addQueue(hint: QueueHint, phaseName: const ccstd::string&) -> RenderQueueBuilder* => "addQueue(hint, phaseName, \"\")";
    }

    interface MultisampleRenderSubpassBuilder : RenderSubpassBuilder {
        void resolveRenderTarget(const ccstd::string& source, const ccstd::string& target);
        void resolveDepthStencil(const ccstd::string& source, const ccstd::string& target, gfx::ResolveMode depthMode, gfx::ResolveMode stencilMode);
        fn resolveDepthStencil(source: const ccstd::string&, target: const ccstd::string&) -> void => "resolveDepthStencil(source, target, gfx::ResolveMode::SAMPLE_ZERO, gfx::ResolveMode::SAMPLE_ZERO)";
        fn resolveDepthStencil(source: const ccstd::string&, target: const ccstd::string&, depthMode: gfx::ResolveMode) -> void => "resolveDepthStencil(source, target, depthMode, gfx::ResolveMode::SAMPLE_ZERO)";
    }

    interface ComputeQueueBuilder : Setter {
        void addDispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ, Material* material, uint32_t passID);
        fn addDispatch(threadGroupCountX: uint32_t, threadGroupCountY: uint32_t, threadGroupCountZ: uint32_t) -> void => "addDispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ, nullptr, 0)";
        fn addDispatch(threadGroupCountX: uint32_t, threadGroupCountY: uint32_t, threadGroupCountZ: uint32_t, material: Material*) -> void => "addDispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ, material, 0)";
    }

    interface ComputeSubpassBuilder : Setter {
        void addRenderTarget(const ccstd::string& name, const ccstd::string& slotName);
        void addTexture(const ccstd::string& name, const ccstd::string& slotName, gfx::Sampler* sampler, uint32_t plane);
        void addStorageBuffer(const ccstd::string& name, AccessType accessType, const ccstd::string& slotName);
        void addStorageImage(const ccstd::string& name, AccessType accessType, const ccstd::string& slotName);
        ComputeQueueBuilder* addQueue(const ccstd::string& phaseName, const ccstd::string& passName);
        void setCustomShaderStages(const ccstd::string& name, gfx::ShaderStageFlagBit stageFlags);
        fn addTexture(name: const ccstd::string&, slotName: const ccstd::string&) -> void => "addTexture(name, slotName, nullptr, 0)";
        fn addTexture(name: const ccstd::string&, slotName: const ccstd::string&, sampler: gfx::Sampler*) -> void => "addTexture(name, slotName, sampler, 0)";
        fn addQueue() -> ComputeQueueBuilder* => "addQueue(\"default\", \"\")";
        fn addQueue(phaseName: const ccstd::string&) -> ComputeQueueBuilder* => "addQueue(phaseName, \"\")";
    }

    interface RenderPassBuilder : BasicRenderPassBuilder {
        void addStorageBuffer(const ccstd::string& name, AccessType accessType, const ccstd::string& slotName);
        void addStorageImage(const ccstd::string& name, AccessType accessType, const ccstd::string& slotName);
        void addMaterialTexture(const ccstd::string& resourceName, gfx::ShaderStageFlagBit flags);
        RenderSubpassBuilder* addRenderSubpass(const ccstd::string& subpassName);
        MultisampleRenderSubpassBuilder* addMultisampleRenderSubpass(uint32_t count, uint32_t quality, const ccstd::string& subpassName);
        ComputeSubpassBuilder* addComputeSubpass(const ccstd::string& subpassName);
        void setCustomShaderStages(const ccstd::string& name, gfx::ShaderStageFlagBit stageFlags);
        fn addMaterialTexture(resourceName: const ccstd::string&) -> void => "addMaterialTexture(resourceName, gfx::ShaderStageFlagBit::VERTEX | gfx::ShaderStageFlagBit::FRAGMENT)";
        fn addComputeSubpass() -> ComputeSubpassBuilder* => "addComputeSubpass(\"\")";
    }

    interface MultisampleRenderPassBuilder : BasicMultisampleRenderPassBuilder {
        void addStorageBuffer(const ccstd::string& name, AccessType accessType, const ccstd::string& slotName);
        void addStorageImage(const ccstd::string& name, AccessType accessType, const ccstd::string& slotName);
    }

    interface ComputePassBuilder : Setter {
        void addTexture(const ccstd::string& name, const ccstd::string& slotName, gfx::Sampler* sampler, uint32_t plane);
        void addStorageBuffer(const ccstd::string& name, AccessType accessType, const ccstd::string& slotName);
        void addStorageImage(const ccstd::string& name, AccessType accessType, const ccstd::string& slotName);
        void addMaterialTexture(const ccstd::string& resourceName, gfx::ShaderStageFlagBit flags);
        ComputeQueueBuilder* addQueue(const ccstd::string& phaseName, const ccstd::string& passName);
        void setCustomShaderStages(const ccstd::string& name, gfx::ShaderStageFlagBit stageFlags);
        fn addTexture(name: const ccstd::string&, slotName: const ccstd::string&) -> void => "addTexture(name, slotName, nullptr, 0)";
        fn addTexture(name: const ccstd::string&, slotName: const ccstd::string&, sampler: gfx::Sampler*) -> void => "addTexture(name, slotName, sampler, 0)";
        fn addMaterialTexture(resourceName: const ccstd::string&) -> void => "addMaterialTexture(resourceName, gfx::ShaderStageFlagBit::COMPUTE)";
        fn addQueue() -> ComputeQueueBuilder* => "addQueue(\"default\", \"\")";
        fn addQueue(phaseName: const ccstd::string&) -> ComputeQueueBuilder* => "addQueue(phaseName, \"\")";
    }

    interface Pipeline : BasicPipeline {
        uint32_t addStorageBuffer(const ccstd::string& name, gfx::Format format, uint32_t size, ResourceResidency residency);
        uint32_t addStorageTexture(const ccstd::string& name, gfx::Format format, uint32_t width, uint32_t height, ResourceResidency residency);
        uint32_t addShadingRateTexture(const ccstd::string& name, uint32_t width, uint32_t height, ResourceResidency residency);
        void updateStorageBuffer(const ccstd::string& name, uint32_t size, gfx::Format format);
        void updateStorageTexture(const ccstd::string& name, uint32_t width, uint32_t height, gfx::Format format);
        void updateShadingRateTexture(const ccstd::string& name, uint32_t width, uint32_t height);
        RenderPassBuilder* addRenderPass(uint32_t width, uint32_t height, const ccstd::string& passName);
        MultisampleRenderPassBuilder* addMultisampleRenderPass(uint32_t width, uint32_t height, uint32_t count, uint32_t quality, const ccstd::string& passName);
        ComputePassBuilder* addComputePass(const ccstd::string& passName);
        void addUploadPass(ccstd::vector<UploadPair>& uploadPairs);
        void addMovePass(const ccstd::vector<MovePair>& movePairs);
        uint32_t addCustomBuffer(const ccstd::string& name, const gfx::BufferInfo& info, const std::string& type);
        uint32_t addCustomTexture(const ccstd::string& name, const gfx::TextureInfo& info, const std::string& type);
        fn addStorageBuffer(name: const ccstd::string&, format: gfx::Format, size: uint32_t) -> uint32_t => "addStorageBuffer(name, format, size, ResourceResidency::MANAGED)";
        fn addStorageTexture(name: const ccstd::string&, format: gfx::Format, width: uint32_t, height: uint32_t) -> uint32_t => "addStorageTexture(name, format, width, height, ResourceResidency::MANAGED)";
        fn addShadingRateTexture(name: const ccstd::string&, width: uint32_t, height: uint32_t) -> uint32_t => "addShadingRateTexture(name, width, height, ResourceResidency::MANAGED)";
        fn updateStorageBuffer(name: const ccstd::string&, size: uint32_t) -> void => "updateStorageBuffer(name, size, gfx::Format::UNKNOWN)";
        fn updateStorageTexture(name: const ccstd::string&, width: uint32_t, height: uint32_t) -> void => "updateStorageTexture(name, width, height, gfx::Format::UNKNOWN)";
    }

    interface PipelinePassBuilder {
        uint32_t getConfigOrder() const;
        uint32_t getRenderOrder() const;
    }

    interface PipelineBuilder {
        void windowResize(BasicPipeline* pipeline, scene::RenderWindow* window, scene::Camera* camera, uint32_t width, uint32_t height);
        void setup(const ccstd::vector<scene::Camera*>& cameras, BasicPipeline* pipeline);
        void onGlobalPipelineStateChanged();
    }

    struct Factory {
        static fn init(deviceIn: gfx::Device*, bufferIn: const ccstd::vector<uint8_t>&) -> RenderingModule*;
        static fn destroy(renderingModule: RenderingModule*) noexcept -> void;
        static fn createPipeline() -> Pipeline*;
    }
}
