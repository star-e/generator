#include "CocosModules.h"
#include <Cocos/AST/DSL.h>
#include <Cocos/AST/SyntaxGraphs.h>

// clang-format off

namespace Cocos::Meta {

void buildArchiveInterface(ModuleBuilder& builder, Features features) {
    MODULE(ArchiveInterface,
        .mFolder = "cocos/renderer/pipeline/custom",
        .mFilePrefix = "Archive",
        .mToJsFilename = "",
        .mToJsPrefix = "",
        .mToJsNamespace = "",
        .mToJsCppHeaders = "",
        .mToJsUsingNamespace = "",
        .mToJsConfigs = "",
        .mTypescriptFolder = "cocos/rendering/custom",
        .mTypescriptFilePrefix = "archive",
        .mRequires = {},
        .mHeader = R"(#include <string_view>
#include <boost/container/pmr/memory_resource.hpp>
)"
    ) {
        NAMESPACE_BEG(cc);
        NAMESPACE_BEG(render);

        INTERFACE(OutputArchive) {
            PUBLIC_METHODS(R"(
virtual void writeBool(bool value) = 0;
virtual void writeNumber(double value) = 0;
virtual void writeString(std::string_view value) = 0;
[[skip]] virtual boost::container::pmr::memory_resource* scratch() const noexcept = 0;
)");
        }

        INTERFACE(InputArchive) {
            PUBLIC_METHODS(R"(
virtual bool readBool() = 0;
virtual double readNumber() = 0;
virtual std::string_view readString() = 0;
[[skip]] virtual boost::container::pmr::memory_resource* scratch() const noexcept = 0;
)");
        }

        NAMESPACE_END(render);
        NAMESPACE_END(cc);
    }
}

void buildRenderInterface(ModuleBuilder& builder, Features features) {
    MODULE(RenderInterface,
        .mFolder = "cocos/renderer/pipeline/custom",
        .mFilePrefix = "RenderInterface",
        .mToJsFilename = "render.i",
        .mToJsPrefix = "render",
        .mToJsNamespace = "render",
        .mToJsCppHeaders = R"(#include "bindings/auto/jsb_render_auto.h"
#include "bindings/auto/jsb_scene_auto.h"
#include "bindings/auto/jsb_gfx_auto.h"
#include "bindings/auto/jsb_assets_auto.h"
#include "renderer/pipeline/GeometryRenderer.h"
#include "renderer/pipeline/GlobalDescriptorSetManager.h"
)",
        .mToJsUsingNamespace = R"(
using namespace cc;
using namespace cc::render;
)",
        .mToJsConfigs = R"(%module_macro(CC_USE_GEOMETRY_RENDERER) cc::render::PipelineRuntime::geometryRenderer;

// ----- Release Returned Cpp Object in GC Section ------
%release_returned_cpp_object_in_gc(cc::render::BasicRenderPassBuilder::addQueue);
%release_returned_cpp_object_in_gc(cc::render::BasicPipeline::addRenderPass);
%release_returned_cpp_object_in_gc(cc::render::BasicPipeline::addMultisampleRenderPass);
%release_returned_cpp_object_in_gc(cc::render::RenderSubpassBuilder::addQueue);
%release_returned_cpp_object_in_gc(cc::render::ComputeSubpassBuilder::addQueue);
%release_returned_cpp_object_in_gc(cc::render::RenderPassBuilder::addRenderSubpass);
%release_returned_cpp_object_in_gc(cc::render::RenderPassBuilder::addMultisampleRenderSubpass);
%release_returned_cpp_object_in_gc(cc::render::RenderPassBuilder::addComputeSubpass);
%release_returned_cpp_object_in_gc(cc::render::ComputePassBuilder::addQueue);
%release_returned_cpp_object_in_gc(cc::render::RenderQueueBuilder::addScene);
%release_returned_cpp_object_in_gc(cc::render::Pipeline::addRenderPass);
%release_returned_cpp_object_in_gc(cc::render::Pipeline::addComputePass);
)",
        .mTypescriptFolder = "cocos/rendering/custom",
        .mTypescriptFilePrefix = "pipeline",
        .mRequires = { "Gfx", "RenderCommon", "Camera", "PipelineSceneData",
            "Assets", "PassUtils", "Customization" },
        .mHeader = R"(#include "cocos/renderer/gfx-base/GFXDef-common.h"
#include "cocos/core/ArrayBuffer.h"

namespace cc {

class Mat4;
class Mat4;
class Quaternion;
class Vec4;
class Vec3;
class Vec2;

namespace pipeline {

class GlobalDSManager;
class PipelineSceneData;
class GeometryRenderer;

} // namespace pipeline

namespace scene {

class DirectionalLight;
class SphereLight;
class SpotLight;
class PointLight;
class RangedDirectionalLight;
class Model;
class RenderScene;
class RenderWindow;

} // namespace scene

namespace render {

constexpr bool ENABLE_SUBPASS = true;
constexpr bool ENABLE_GPU_DRIVEN = false;

} // namespace render

} // namespace cc
)"
    ) {
        NAMESPACE_BEG(cc);
        NAMESPACE_BEG(render);

        INTERFACE(PipelineRuntime) {
            PUBLIC_METHODS(R"(
virtual bool activate(gfx::Swapchain * swapchain) = 0;
virtual bool destroy() noexcept = 0;
virtual void render(const ccstd::vector<scene::Camera*>& cameras) = 0;

[[getter]] virtual gfx::Device* getDevice() const = 0;
[[getter]] virtual const MacroRecord &getMacros() const = 0;
[[getter]] virtual pipeline::GlobalDSManager *getGlobalDSManager() const = 0;
[[getter]] virtual gfx::DescriptorSetLayout *getDescriptorSetLayout() const = 0;
[[getter]] virtual gfx::DescriptorSet *getDescriptorSet() const = 0;
[[getter]] virtual const ccstd::vector<gfx::CommandBuffer*>& getCommandBuffers() const = 0;
[[getter]] virtual pipeline::PipelineSceneData *getPipelineSceneData() const = 0;
[[getter]] virtual const ccstd::string &getConstantMacros() const = 0;
[[nullable]] [[getter]] virtual scene::Model *getProfiler() const = 0;
[[setter]] virtual void setProfiler([[nullable]] scene::Model *profiler) = 0;
[[nullable]] [[getter]] virtual pipeline::GeometryRenderer  *getGeometryRenderer() const = 0;

[[getter]] virtual float getShadingScale() const = 0;
[[setter]] virtual void setShadingScale(float scale) = 0;

virtual const ccstd::string& getMacroString(const ccstd::string& name) const = 0;
virtual int32_t getMacroInt(const ccstd::string& name) const = 0;
virtual bool getMacroBool(const ccstd::string& name) const = 0;

virtual void setMacroString(const ccstd::string& name, const ccstd::string& value) = 0;
virtual void setMacroInt(const ccstd::string& name, int32_t value) = 0;
virtual void setMacroBool(const ccstd::string& name, bool value) = 0;

virtual void onGlobalPipelineStateChanged() = 0;

[[skip]] virtual void setValue(const ccstd::string& name, int32_t value) = 0;
[[skip]] virtual void setValue(const ccstd::string& name, bool value) = 0;

[[skip]] virtual bool isOcclusionQueryEnabled() const = 0;

[[skip]] virtual void resetRenderQueue(bool reset) = 0;
[[skip]] virtual bool isRenderQueueReset() const = 0;
)");
        }
                
        ENUM_CLASS(PipelineType) {
            ENUMS(BASIC, STANDARD);
        }

        FLAG_CLASS(SubpassCapabilities) {
            UNDERLYING_TYPE(uint32_t);
            FLAGS(
                (NONE, 0)
                (INPUT_DEPTH_STENCIL, 1 << 0)
                (INPUT_COLOR, 1 << 1)
                (INPUT_COLOR_MRT, 1 << 2)
                (HETEROGENEOUS_SAMPLE_COUNT, 1 << 3)
            );
        }

        STRUCT(PipelineCapabilities) {
            PUBLIC(
                (SubpassCapabilities, mSubpass, SubpassCapabilities::NONE)
            );
        }

        INTERFACE(RenderNode) {
            PUBLIC_METHODS(R"(
[[getter]] virtual ccstd::string getName() const = 0;
[[setter]] virtual void setName(const ccstd::string& name) = 0;
[[experimental]] virtual void setCustomBehavior(const ccstd::string& name) = 0;
)");
        }

        INTERFACE(Setter) {
            INHERITS(RenderNode);
            PUBLIC_METHODS(R"(
virtual void setMat4(const ccstd::string& name, const cc::Mat4& mat) = 0;
virtual void setQuaternion(const ccstd::string& name, const cc::Quaternion& quat) = 0;
virtual void setColor(const ccstd::string& name, const gfx::Color& color) = 0;
virtual void setVec4(const ccstd::string& name, const cc::Vec4& vec) = 0;
virtual void setVec2(const ccstd::string& name, const cc::Vec2& vec) = 0;
virtual void setFloat(const ccstd::string& name, float v) = 0;
virtual void setArrayBuffer(const ccstd::string& name, const ArrayBuffer* arrayBuffer) = 0;

virtual void setBuffer(const ccstd::string& name, gfx::Buffer* buffer) = 0;
virtual void setTexture(const ccstd::string& name, gfx::Texture* texture) = 0;
[[deprecated]] virtual void setReadWriteBuffer(const ccstd::string& name, gfx::Buffer* buffer) = 0;
[[deprecated]] virtual void setReadWriteTexture(const ccstd::string& name, gfx::Texture* texture) = 0;
virtual void setSampler(const ccstd::string& name, gfx::Sampler* sampler) = 0;

virtual void setBuiltinCameraConstants(const scene::Camera* camera) = 0;
virtual void setBuiltinShadowMapConstants(const scene::DirectionalLight* light) = 0;
virtual void setBuiltinDirectionalLightConstants(const scene::DirectionalLight* light, const scene::Camera* camera) = 0;
virtual void setBuiltinSphereLightConstants(const scene::SphereLight* light, const scene::Camera* camera) = 0;
virtual void setBuiltinSpotLightConstants(const scene::SpotLight* light, const scene::Camera* camera) = 0;
virtual void setBuiltinPointLightConstants(const scene::PointLight* light, const scene::Camera* camera) = 0;
virtual void setBuiltinRangedDirectionalLightConstants(const scene::RangedDirectionalLight* light, const scene::Camera* camera) = 0;
virtual void setBuiltinDirectionalLightFrustumConstants(const scene::Camera* camera, const scene::DirectionalLight* light, uint32_t csmLevel = 0) = 0;
virtual void setBuiltinSpotLightFrustumConstants(const scene::SpotLight* light) = 0;
)");
        }

        INTERFACE(SceneBuilder) {
            INHERITS(Setter);
            PUBLIC_METHODS(R"(
virtual void useLightFrustum(IntrusivePtr<scene::Light> light, uint32_t csmLevel = 0, [[optional]] const scene::Camera* optCamera = nullptr) = 0;
)");
        }

        INTERFACE(RenderQueueBuilder) {
            INHERITS(Setter);
            PUBLIC_METHODS(R"(
[[deprecated]] virtual void addSceneOfCamera(scene::Camera* camera, LightInfo light, SceneFlags sceneFlags = SceneFlags::NONE) = 0;
virtual SceneBuilder *addScene(const scene::Camera* camera, SceneFlags sceneFlags, [[optional]] scene::Light* light = nullptr) = 0;
virtual void addFullscreenQuad(cc::Material *material, uint32_t passID, SceneFlags sceneFlags = SceneFlags::NONE) = 0;
virtual void addCameraQuad(scene::Camera* camera, cc::Material *material, uint32_t passID, SceneFlags sceneFlags = SceneFlags::NONE) = 0;
virtual void clearRenderTarget(const ccstd::string &name, const gfx::Color &color = {}) = 0;
virtual void setViewport(const gfx::Viewport &viewport) = 0;
[[experimental]] virtual void addCustomCommand(std::string_view customBehavior) = 0;
)");
        }

        INTERFACE(BasicRenderPassBuilder) {
            INHERITS(Setter);
            PUBLIC_METHODS(R"(
virtual void addRenderTarget(const ccstd::string& name, gfx::LoadOp loadOp = gfx::LoadOp::CLEAR, gfx::StoreOp storeOp = gfx::StoreOp::STORE, const gfx::Color& color = {}) = 0;
virtual void addDepthStencil(const ccstd::string& name, gfx::LoadOp loadOp = gfx::LoadOp::CLEAR, gfx::StoreOp storeOp = gfx::StoreOp::STORE, float depth = 1, uint8_t stencil = 0, gfx::ClearFlagBit clearFlags = gfx::ClearFlagBit::DEPTH_STENCIL) = 0;
virtual void addTexture(const ccstd::string& name, const ccstd::string& slotName, [[optional]] gfx::Sampler* sampler = nullptr, uint32_t plane = 0) = 0;

virtual RenderQueueBuilder *addQueue(QueueHint hint = QueueHint::NONE, const ccstd::string& phaseName = "default") = 0;
virtual void setViewport(const gfx::Viewport &viewport) = 0;
[[deprecated]] virtual void setVersion(const ccstd::string& name, uint64_t version) = 0;
[[getter]] virtual bool getShowStatistics() const = 0;
[[setter]] virtual void setShowStatistics(bool enable) = 0;
)");
        }

        INTERFACE(BasicMultisampleRenderPassBuilder) {
            INHERITS(BasicRenderPassBuilder);
            PUBLIC_METHODS(R"(
virtual void resolveRenderTarget(const ccstd::string& source, const ccstd::string& target) = 0;
virtual void resolveDepthStencil(const ccstd::string& source, const ccstd::string& target, gfx::ResolveMode depthMode = gfx::ResolveMode::SAMPLE_ZERO, gfx::ResolveMode stencilMode = gfx::ResolveMode::SAMPLE_ZERO) = 0;
)");
        }
//
//        INTERFACE(BuiltinReflectionProbePassBuilder) {
//            INHERITS(Setter);
//            PUBLIC_METHODS(R"(
//virtual RenderQueueBuilder *addQueue(QueueHint hint = QueueHint::NONE, const ccstd::string& phaseName = "default") = 0;
//)");
//        }

        INTERFACE(BasicPipeline) {
            INHERITS(PipelineRuntime);
            PUBLIC_METHODS(R"(
[[getter]] virtual PipelineType getType() const = 0;
[[getter]] virtual PipelineCapabilities getCapabilities() const = 0;
virtual void beginSetup() = 0;
virtual void endSetup() = 0;

virtual bool containsResource(const ccstd::string& name) const = 0;
virtual uint32_t addRenderWindow(const ccstd::string& name, gfx::Format format, uint32_t width, uint32_t height, scene::RenderWindow* renderWindow) = 0;
virtual void updateRenderWindow(const ccstd::string& name, scene::RenderWindow* renderWindow) = 0;

virtual uint32_t addRenderTarget(const ccstd::string& name, gfx::Format format, uint32_t width, uint32_t height, ResourceResidency residency = ResourceResidency::MANAGED) = 0;
virtual uint32_t addDepthStencil(const ccstd::string& name, gfx::Format format, uint32_t width, uint32_t height, ResourceResidency residency = ResourceResidency::MANAGED) = 0;

virtual void updateRenderTarget(const ccstd::string& name, uint32_t width, uint32_t height, gfx::Format format = gfx::Format::UNKNOWN) = 0;
virtual void updateDepthStencil(const ccstd::string& name, uint32_t width, uint32_t height, gfx::Format format = gfx::Format::UNKNOWN) = 0;

virtual uint32_t addResource(const ccstd::string& name, ResourceDimension dimension, gfx::Format format, uint32_t width, uint32_t height, uint32_t depth, uint32_t arraySize, uint32_t mipLevels, gfx::SampleCount sampleCount, ResourceFlags flags, ResourceResidency residency) = 0;
virtual void updateResource(const ccstd::string& name, gfx::Format format, uint32_t width, uint32_t height, uint32_t depth, uint32_t arraySize, uint32_t mipLevels, gfx::SampleCount sampleCount) = 0;
virtual uint32_t addTexture(const ccstd::string &name, gfx::TextureType type, gfx::Format format, uint32_t width, uint32_t height, uint32_t depth, uint32_t arraySize, uint32_t mipLevels, gfx::SampleCount sampleCount, ResourceFlags flags, ResourceResidency residency) = 0;
virtual void updateTexture(const ccstd::string &name, gfx::Format format, uint32_t width, uint32_t height, uint32_t depth, uint32_t arraySize, uint32_t mipLevels, gfx::SampleCount sampleCount) = 0;
virtual uint32_t addBuffer(const ccstd::string &name, uint32_t size, ResourceFlags flags, ResourceResidency residency) = 0;
virtual void updateBuffer(const ccstd::string &name, uint32_t size) = 0;
virtual uint32_t addExternalTexture(const ccstd::string &name, gfx::Texture *texture, ResourceFlags flags) = 0;
virtual void updateExternalTexture(const ccstd::string &name, gfx::Texture *texture) = 0;

virtual void beginFrame() = 0;
virtual void update(const scene::Camera* camera) = 0;
virtual void endFrame() = 0;

[[covariant]] virtual BasicRenderPassBuilder *addRenderPass(uint32_t width, uint32_t height, const ccstd::string& passName = "default") = 0;
[[beta]] [[covariant]] virtual BasicMultisampleRenderPassBuilder *addMultisampleRenderPass(uint32_t width, uint32_t height, uint32_t count, uint32_t quality, const ccstd::string& passName = "default") = 0;
[[deprecated]] virtual void addResolvePass(const ccstd::vector<ResolvePair>& resolvePairs) = 0;
virtual void addCopyPass(const ccstd::vector<CopyPair>& copyPairs) = 0;

virtual void addBuiltinReflectionProbePass(const scene::Camera *camera) = 0;

[[optional]] virtual gfx::DescriptorSetLayout *getDescriptorSetLayout(const ccstd::string& shaderName, UpdateFrequency freq) = 0;
)");
        }

        INTERFACE(RenderSubpassBuilder) {
            INHERITS(Setter);
            PUBLIC_METHODS(R"(
virtual void addRenderTarget(const ccstd::string& name, AccessType accessType, const ccstd::string& slotName = "", gfx::LoadOp loadOp = gfx::LoadOp::CLEAR, gfx::StoreOp storeOp = gfx::StoreOp::STORE, const gfx::Color& color = {}) = 0;
virtual void addDepthStencil(const ccstd::string& name, AccessType accessType, const ccstd::string& depthSlotName = "", const ccstd::string& stencilSlotName = "", gfx::LoadOp loadOp = gfx::LoadOp::CLEAR, gfx::StoreOp storeOp = gfx::StoreOp::STORE, float depth = 1, uint8_t stencil = 0, gfx::ClearFlagBit clearFlags = gfx::ClearFlagBit::DEPTH_STENCIL) = 0;
virtual void addTexture(const ccstd::string& name, const ccstd::string& slotName, [[optional]] gfx::Sampler* sampler = nullptr, uint32_t plane = 0) = 0;
virtual void addStorageBuffer(const ccstd::string& name, AccessType accessType, const ccstd::string& slotName) = 0;
virtual void addStorageImage(const ccstd::string& name, AccessType accessType, const ccstd::string& slotName) = 0;

virtual void setViewport(const gfx::Viewport &viewport) = 0;
virtual RenderQueueBuilder *addQueue(QueueHint hint = QueueHint::NONE, const ccstd::string& phaseName = "default") = 0;
[[getter]] virtual bool getShowStatistics() const = 0;
[[setter]] virtual void setShowStatistics(bool enable) = 0;

[[experimental]] virtual void setCustomShaderStages(const ccstd::string& name, gfx::ShaderStageFlagBit stageFlags) = 0;
)");
        }
//virtual void addStorageBuffer(const ccstd::string& name, AccessType accessType, const ccstd::string& slotName, ClearValueType clearType = ClearValueType::NONE, const ClearValue& clearValue = {}) = 0;
//virtual void addStorageImage(const ccstd::string& name, AccessType accessType, const ccstd::string& slotName, ClearValueType clearType = ClearValueType::NONE, const ClearValue& clearValue = {}) = 0;

        INTERFACE(MultisampleRenderSubpassBuilder) {
            INHERITS(RenderSubpassBuilder);
            PUBLIC_METHODS(R"(
virtual void resolveRenderTarget(const ccstd::string& source, const ccstd::string& target) = 0;
virtual void resolveDepthStencil(const ccstd::string& source, const ccstd::string& target, gfx::ResolveMode depthMode = gfx::ResolveMode::SAMPLE_ZERO, gfx::ResolveMode stencilMode = gfx::ResolveMode::SAMPLE_ZERO) = 0;
)");
        }

        INTERFACE(ComputeQueueBuilder) {
            INHERITS(Setter);
            PUBLIC_METHODS(R"(
virtual void addDispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ, cc::Material *material = nullptr, uint32_t passID = 0) = 0;
)");
        }

        INTERFACE(ComputeSubpassBuilder) {
            INHERITS(Setter);
            PUBLIC_METHODS(R"(
virtual void addRenderTarget(const ccstd::string& name, const ccstd::string& slotName) = 0;
virtual void addTexture(const ccstd::string& name, const ccstd::string& slotName, [[optional]] gfx::Sampler* sampler = nullptr, uint32_t plane = 0) = 0;
virtual void addStorageBuffer(const ccstd::string& name, AccessType accessType, const ccstd::string& slotName) = 0;
virtual void addStorageImage(const ccstd::string& name, AccessType accessType, const ccstd::string& slotName) = 0;

virtual ComputeQueueBuilder *addQueue(const ccstd::string& phaseName = "default") = 0;

[[experimental]] virtual void setCustomShaderStages(const ccstd::string& name, gfx::ShaderStageFlagBit stageFlags) = 0;
)");
        }

        INTERFACE(RenderPassBuilder) {
            INHERITS(BasicRenderPassBuilder);
            PUBLIC_METHODS(R"(
virtual void addStorageBuffer(const ccstd::string& name, AccessType accessType, const ccstd::string& slotName) = 0;
virtual void addStorageImage(const ccstd::string& name, AccessType accessType, const ccstd::string& slotName) = 0;
[[beta]] virtual void addMaterialTexture(const ccstd::string& resourceName, gfx::ShaderStageFlagBit flags = gfx::ShaderStageFlagBit::VERTEX | gfx::ShaderStageFlagBit::FRAGMENT) = 0;

[[beta]] virtual RenderSubpassBuilder *addRenderSubpass(const ccstd::string& subpassName) = 0;
[[beta]] virtual MultisampleRenderSubpassBuilder *addMultisampleRenderSubpass(uint32_t count, uint32_t quality, const ccstd::string& subpassName) = 0;
[[experimental]] virtual ComputeSubpassBuilder *addComputeSubpass(const ccstd::string& subpassName = "") = 0;

[[experimental]] virtual void setCustomShaderStages(const ccstd::string& name, gfx::ShaderStageFlagBit stageFlags) = 0;
)");
        }

        INTERFACE(MultisampleRenderPassBuilder) {
            INHERITS(BasicMultisampleRenderPassBuilder);
            PUBLIC_METHODS(R"(
virtual void addStorageBuffer(const ccstd::string& name, AccessType accessType, const ccstd::string& slotName) = 0;
virtual void addStorageImage(const ccstd::string& name, AccessType accessType, const ccstd::string& slotName) = 0;
)");
        }

        INTERFACE(ComputePassBuilder) {
            INHERITS(Setter);
            PUBLIC_METHODS(R"(
virtual void addTexture(const ccstd::string& name, const ccstd::string& slotName, [[optional]] gfx::Sampler* sampler = nullptr, uint32_t plane = 0) = 0;
virtual void addStorageBuffer(const ccstd::string& name, AccessType accessType, const ccstd::string& slotName) = 0;
virtual void addStorageImage(const ccstd::string& name, AccessType accessType, const ccstd::string& slotName) = 0;
[[beta]] virtual void addMaterialTexture(const ccstd::string& resourceName, gfx::ShaderStageFlagBit flags = gfx::ShaderStageFlagBit::COMPUTE) = 0;

virtual ComputeQueueBuilder *addQueue(const ccstd::string& phaseName = "default") = 0;

[[experimental]] virtual void setCustomShaderStages(const ccstd::string& name, gfx::ShaderStageFlagBit stageFlags) = 0;
)");
        }

//        INTERFACE(SceneVisitor) {
//            PUBLIC_METHODS(R"(
//[[getter]] virtual const pipeline::PipelineSceneData* getPipelineSceneData() const = 0;
//
//virtual void setViewport(const gfx::Viewport &vp) = 0;
//virtual void setScissor(const gfx::Rect &rect) = 0;
//virtual void bindPipelineState(gfx::PipelineState* pso) = 0;
//[[skip]] virtual void bindDescriptorSet(uint32_t set, gfx::DescriptorSet *descriptorSet, uint32_t dynamicOffsetCount, const uint32_t *dynamicOffsets) = 0;
//virtual void bindInputAssembler(gfx::InputAssembler *ia) = 0;
//[[skip]] virtual void updateBuffer(gfx::Buffer *buff, const void *data, uint32_t size) = 0;
//virtual void draw(const gfx::DrawInfo &info) = 0;
//)");
//            TS_FUNCTIONS(R"(
//bindDescriptorSet (set: number, descriptorSet: DescriptorSet, dynamicOffsets?: number[]): void;
//updateBuffer (buffer: Buffer, data: ArrayBuffer, size?: number): void;
//)");
//        }
//
//        INTERFACE(SceneTask) {
//            PUBLIC_METHODS(R"(
//[[getter]] virtual TaskType getTaskType() const noexcept = 0;
//virtual void     start() = 0;
//virtual void     join() = 0;
//virtual void     submit() = 0;
//)");
//        }
//
//        INTERFACE(SceneTransversal) {
//            PUBLIC_METHODS(R"(
//virtual SceneTask* transverse(SceneVisitor *visitor) const = 0;
//)");
//        }

        INTERFACE(Pipeline) {
            INHERITS(BasicPipeline);
            PUBLIC_METHODS(R"(
virtual uint32_t addStorageBuffer(const ccstd::string& name, gfx::Format format, uint32_t size, ResourceResidency residency = ResourceResidency::MANAGED) = 0;
virtual uint32_t addStorageTexture(const ccstd::string& name, gfx::Format format, uint32_t width, uint32_t height, ResourceResidency residency = ResourceResidency::MANAGED) = 0;
[[experimental]] virtual uint32_t addShadingRateTexture(const ccstd::string& name, uint32_t width, uint32_t height, ResourceResidency residency = ResourceResidency::MANAGED) = 0;

virtual void updateStorageBuffer(const ccstd::string& name, uint32_t size, gfx::Format format = gfx::Format::UNKNOWN) = 0;
virtual void updateStorageTexture(const ccstd::string& name, uint32_t width, uint32_t height, gfx::Format format = gfx::Format::UNKNOWN) = 0;
virtual void updateShadingRateTexture(const ccstd::string& name, uint32_t width, uint32_t height) = 0;

virtual RenderPassBuilder *addRenderPass(uint32_t width, uint32_t height, const ccstd::string& passName) = 0;
virtual MultisampleRenderPassBuilder *addMultisampleRenderPass(uint32_t width, uint32_t height, uint32_t count, uint32_t quality, const ccstd::string& passName) = 0;
virtual ComputePassBuilder *addComputePass(const ccstd::string& passName) = 0;
[[beta]] virtual void addUploadPass(ccstd::vector<UploadPair>& uploadPairs) = 0;
virtual void addMovePass(const ccstd::vector<MovePair>& movePairs) = 0;
virtual void addBuiltinGpuCullingPass(const scene::Camera* camera, const std::string& hzbName = "", [[optional]] const scene::Light* light = nullptr) = 0;
virtual void addBuiltinHzbGenerationPass(const std::string& sourceDepthStencilName, const std::string& targetHzbName) = 0;

[[experimental]] virtual uint32_t addCustomBuffer(const ccstd::string& name, const gfx::BufferInfo& info, const std::string& type) = 0;
[[experimental]] virtual uint32_t addCustomTexture(const ccstd::string& name, const gfx::TextureInfo& info, const std::string& type) = 0;
)");
        }

        INTERFACE(PipelineBuilder) {
            PUBLIC_METHODS(R"(
virtual void setup(const ccstd::vector<scene::Camera*>& cameras, BasicPipeline* pipeline) = 0;

[[?]] virtual void onGlobalPipelineStateChanged() = 0;
)");
        }

        INTERFACE(RenderingModule) {
            PUBLIC_METHODS(R"(
virtual uint32_t getPassID(const ccstd::string& name) const = 0;
virtual uint32_t getSubpassID(uint32_t passID, const ccstd::string& name) const = 0;
virtual uint32_t getPhaseID(uint32_t subpassOrPassID, const ccstd::string& name) const = 0;
)");
        }

        CLASS(Factory, .mExport = false) {
            MEMBER_FUNCTIONS(R"(static RenderingModule* init(gfx::Device* deviceIn, const ccstd::vector<unsigned char>& bufferIn);
static void destroy(RenderingModule* renderingModule) noexcept;
static Pipeline *createPipeline();
)");
        }

        NAMESPACE_END(render);
        NAMESPACE_END(cc);
    }
}

void buildPrivateInterface(ModuleBuilder& builder, Features features) {
    MODULE(PrivateInterface,
        .mFolder = "cocos/renderer/pipeline/custom",
        .mFilePrefix = "Private",
        .mToJsFilename = "",
        .mToJsPrefix = "",
        .mToJsNamespace = "",
        .mToJsCppHeaders = "",
        .mToJsUsingNamespace = "",
        .mToJsConfigs = "",
        .mTypescriptFolder = "cocos/rendering/custom",
        .mTypescriptFilePrefix = "private",
        .mRequires = { "RenderInterface", "ProgramLib" },
        .mHeader = R"(
)") {
        NAMESPACE_BEG(cc);
        NAMESPACE_BEG(render);

        INTERFACE(ProgramProxy) {
            INHERITS(RefCounted);
            PUBLIC_METHODS(R"(
[[getter]] virtual const ccstd::string& getName() const noexcept = 0;
[[getter]] virtual gfx::Shader* getShader() const noexcept = 0;
)");
        }

        INTERFACE(ProgramLibrary) {
            PUBLIC_METHODS(R"(
virtual void addEffect(const EffectAsset* effectAsset) = 0;
virtual void precompileEffect(gfx::Device* device, EffectAsset* effectAsset) = 0;
virtual ccstd::string getKey(uint32_t phaseID, const ccstd::string& programName, const MacroRecord& defines) const = 0;
virtual IntrusivePtr<gfx::PipelineLayout> getPipelineLayout(gfx::Device* device, uint32_t phaseID, const ccstd::string& programName) = 0;
virtual const gfx::DescriptorSetLayout& getMaterialDescriptorSetLayout(gfx::Device* device, uint32_t phaseID, const ccstd::string& programName) = 0;
virtual const gfx::DescriptorSetLayout& getLocalDescriptorSetLayout(gfx::Device* device, uint32_t phaseID, const ccstd::string& programName) = 0;
virtual const IProgramInfo& getProgramInfo(uint32_t phaseID, const ccstd::string& programName) const = 0;
virtual const gfx::ShaderInfo& getShaderInfo(uint32_t phaseID, const ccstd::string& programName) const = 0;
[[nullable]] virtual ProgramProxy* getProgramVariant(gfx::Device* device, uint32_t phaseID, const ccstd::string& name, MacroRecord& defines, [[optional]] const ccstd::pmr::string* key = nullptr) = 0;
[[skip]] virtual gfx::PipelineState* getComputePipelineState(gfx::Device* device, uint32_t phaseID, const ccstd::string& name, MacroRecord& defines, [[optional]] const ccstd::pmr::string* key = nullptr) = 0;
virtual const ccstd::vector<int>& getBlockSizes(uint32_t phaseID, const ccstd::string& programName) const = 0;
virtual const ccstd::unordered_map<ccstd::string, uint32_t>& getHandleMap(uint32_t phaseID, const ccstd::string& programName) const = 0;

virtual uint32_t getProgramID(uint32_t phaseID, const ccstd::pmr::string& programName) = 0;
virtual uint32_t getDescriptorNameID(const ccstd::pmr::string& name) = 0;
virtual const ccstd::pmr::string& getDescriptorName(uint32_t nameID) = 0;
)");
        }

        NAMESPACE_END(render);
        NAMESPACE_END(cc);
    }
}

}
