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
            TS_RENAME_METHODS(
                (writeBool, b)
                (writeNumber, n)
                (writeString, s)
            );
        }

        INTERFACE(InputArchive) {
            PUBLIC_METHODS(R"(
virtual bool readBool() = 0;
virtual double readNumber() = 0;
virtual std::string_view readString() = 0;
[[skip]] virtual boost::container::pmr::memory_resource* scratch() const noexcept = 0;
)");
            TS_RENAME_METHODS(
                (readBool, b)
                (readNumber, n)
                (readString, s)
            );
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
            "Assets", "PassUtils" },
        .mHeader = R"(#include "cocos/renderer/gfx-base/GFXDef-common.h"
#include "cocos/core/ArrayBuffer.h"
#include "cocos/core/TypedArray.h"

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
[[nullable]] [[getter]] virtual pipeline::GeometryRenderer *getGeometryRenderer() const = 0;

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

        ENUM_CLASS(PipelineType, .mFlags = TS_ENUM_OBJECT) {
            UNDERLYING_TYPE(uint8_t);
            ENUMS(BASIC, STANDARD);
        }

        FLAG_CLASS(SubpassCapabilities, .mFlags = TS_ENUM_OBJECT) {
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
virtual void setSampler(const ccstd::string& name, gfx::Sampler* sampler) = 0;

virtual void setBuiltinCameraConstants(const scene::Camera* camera) = 0;
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
virtual SceneBuilder *addScene(const scene::Camera* camera, SceneFlags sceneFlags, [[optional]] scene::Light* light = nullptr, [[optional]] scene::RenderScene* scene = nullptr) = 0;
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

virtual RenderQueueBuilder *addQueue(QueueHint hint = QueueHint::NONE, const ccstd::string& phaseName = "default", const ccstd::string& passName = "") = 0;
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
//virtual RenderQueueBuilder *addQueue(QueueHint hint = QueueHint::NONE, const ccstd::string& phaseName = "default", const ccstd::string& passName = "") = 0;
//)");
//        }

        INTERFACE(BasicPipeline) {
            INHERITS(PipelineRuntime);
            PUBLIC_METHODS(R"(
[[getter]] virtual PipelineType getType() const = 0;
[[getter]] virtual PipelineCapabilities getCapabilities() const = 0;
virtual void beginSetup() = 0;
virtual void endSetup() = 0;

[[getter]] virtual bool getEnableCpuLightCulling() const = 0;
[[setter]] virtual void setEnableCpuLightCulling(bool enable) = 0;

virtual bool containsResource(const ccstd::string& name) const = 0;
virtual uint32_t addRenderWindow(const ccstd::string& name, gfx::Format format, uint32_t width, uint32_t height, scene::RenderWindow* renderWindow, const ccstd::string& depthStencilName = "") = 0;
[[deprecated]] virtual void updateRenderWindow(const ccstd::string& name, scene::RenderWindow* renderWindow, const ccstd::string& depthStencilName = "") = 0;

virtual uint32_t addRenderTarget(const ccstd::string& name, gfx::Format format, uint32_t width, uint32_t height, ResourceResidency residency = ResourceResidency::MANAGED) = 0;
virtual uint32_t addDepthStencil(const ccstd::string& name, gfx::Format format, uint32_t width, uint32_t height, ResourceResidency residency = ResourceResidency::MANAGED) = 0;

[[deprecated]] virtual void updateRenderTarget(const ccstd::string& name, uint32_t width, uint32_t height, gfx::Format format = gfx::Format::UNKNOWN) = 0;
[[deprecated]] virtual void updateDepthStencil(const ccstd::string& name, uint32_t width, uint32_t height, gfx::Format format = gfx::Format::UNKNOWN) = 0;

virtual uint32_t addBuffer(const ccstd::string &name, uint32_t size, ResourceFlags flags, ResourceResidency residency) = 0;
[[deprecated]] virtual void updateBuffer(const ccstd::string &name, uint32_t size) = 0;
virtual uint32_t addExternalTexture(const ccstd::string &name, gfx::Texture *texture, ResourceFlags flags) = 0;
[[deprecated]] virtual void updateExternalTexture(const ccstd::string &name, gfx::Texture *texture) = 0;
virtual uint32_t addTexture(const ccstd::string &name, gfx::TextureType type, gfx::Format format, uint32_t width, uint32_t height, uint32_t depth, uint32_t arraySize, uint32_t mipLevels, gfx::SampleCount sampleCount, ResourceFlags flags, ResourceResidency residency) = 0;
[[deprecated]] virtual void updateTexture(const ccstd::string &name, gfx::Format format, uint32_t width, uint32_t height, uint32_t depth, uint32_t arraySize, uint32_t mipLevels, gfx::SampleCount sampleCount) = 0;
virtual uint32_t addResource(const ccstd::string& name, ResourceDimension dimension, gfx::Format format, uint32_t width, uint32_t height, uint32_t depth, uint32_t arraySize, uint32_t mipLevels, gfx::SampleCount sampleCount, ResourceFlags flags, ResourceResidency residency) = 0;
[[deprecated]] virtual void updateResource(const ccstd::string& name, gfx::Format format, uint32_t width, uint32_t height, uint32_t depth, uint32_t arraySize, uint32_t mipLevels, gfx::SampleCount sampleCount) = 0;

virtual void beginFrame() = 0;
virtual void update(const scene::Camera* camera) = 0;
virtual void endFrame() = 0;

[[covariant]] virtual BasicRenderPassBuilder *addRenderPass(uint32_t width, uint32_t height, const ccstd::string& passName = "default") = 0;
[[beta]] [[covariant]] virtual BasicMultisampleRenderPassBuilder *addMultisampleRenderPass(uint32_t width, uint32_t height, uint32_t count, uint32_t quality, const ccstd::string& passName = "default") = 0;
[[deprecated]] virtual void addResolvePass(const ccstd::vector<ResolvePair>& resolvePairs) = 0;
virtual void addCopyPass(const ccstd::vector<CopyPair>& copyPairs) = 0;

[[deprecated]] virtual void addBuiltinReflectionProbePass(const scene::Camera *camera) = 0;

[[optional]] virtual gfx::DescriptorSetLayout *getDescriptorSetLayout(const ccstd::string& shaderName, UpdateFrequency freq) = 0;

virtual void setMat4(const ccstd::string& name, const cc::Mat4& mat) = 0;
virtual void setQuaternion(const ccstd::string& name, const cc::Quaternion& quat) = 0;
virtual void setColor(const ccstd::string& name, const gfx::Color& color) = 0;
virtual void setVec4(const ccstd::string& name, const cc::Vec4& vec) = 0;
virtual void setVec2(const ccstd::string& name, const cc::Vec2& vec) = 0;
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
virtual RenderQueueBuilder *addQueue(QueueHint hint = QueueHint::NONE, const ccstd::string& phaseName = "default", const ccstd::string& passName = "") = 0;
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

virtual ComputeQueueBuilder *addQueue(const ccstd::string& phaseName = "default", const ccstd::string& passName = "") = 0;

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

virtual ComputeQueueBuilder *addQueue(const ccstd::string& phaseName = "default", const ccstd::string& passName = "") = 0;

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

[[experimental]] virtual uint32_t addCustomBuffer(const ccstd::string& name, const gfx::BufferInfo& info, const std::string& type) = 0;
[[experimental]] virtual uint32_t addCustomTexture(const ccstd::string& name, const gfx::TextureInfo& info, const std::string& type) = 0;
)");
        }

        INTERFACE(PipelinePassBuilder) {
            PUBLIC_METHODS(R"(
virtual uint32_t getConfigOrder() const = 0;
virtual uint32_t getRenderOrder() const = 0;
)");
            TS_FUNCTIONS(R"(configCamera? (
    camera: Readonly<Camera>,
    pplConfigs: { readonly [name: string]: any },
    cameraConfigs: { [name: string]: any }): void;
windowResize? (
    ppl: BasicPipeline,
    pplConfigs: { readonly [name: string]: any },
    cameraConfigs: { readonly [name: string]: any },
    window: RenderWindow,
    camera: Camera,
    width: number,
    height: number): void;
setup? (
    ppl: BasicPipeline,
    pplConfigs: { readonly [name: string]: any },
    cameraConfigs: { readonly [name: string]: any },
    camera: Camera,
    context: { [name: string]: any },
    prevRenderPass?: BasicRenderPassBuilder): BasicRenderPassBuilder | undefined;
)");
        }

        INTERFACE(PipelineBuilder) {
            PUBLIC_METHODS(R"(
[[?]] virtual void windowResize(BasicPipeline* pipeline, scene::RenderWindow* window, scene::Camera* camera, uint32_t width, uint32_t height) = 0;

virtual void setup(const ccstd::vector<scene::Camera*>& cameras, BasicPipeline* pipeline) = 0;

[[?]] virtual void onGlobalPipelineStateChanged() = 0;
)");
        }
// [[?]] virtual void gameWindowOrientationChange(BasicPipeline* pipeline, scene::RenderWindow* window, uint32_t orientation) = 0;

        INTERFACE(RenderingModule) {
            PUBLIC_METHODS(R"(
virtual uint32_t getPassID(const ccstd::string& name) const = 0;
virtual uint32_t getSubpassID(uint32_t passID, const ccstd::string& name) const = 0;
virtual uint32_t getPhaseID(uint32_t subpassOrPassID, const ccstd::string& name) const = 0;
)");
        }
//
//    ENUM_CLASS(DataType) {
//        UNDERLYING_TYPE(uint32_t);
//        builder.setEnumOutputAll(vertID, true);
//        ENUMS(
//            UNKNOWN,
//
//            DATA_TYPE,
//            STRING,
//            VOID_POINTER,
//            BOOL,
//
//            STRING_LIST,
//            DATA_TYPE_LIST,
//            PARAMETER_LIST,
//
//            FUNCTION_POINTER,
//            MEMORY_DELETER,
//            STATUS_CALLBACK,
//            FRAME_COMPLETION_CALLBACK,
//
//            LIBRARY,
//            DEVICE,
//            OBJECT,
//            ARRAY,
//            ARRAY1D,
//            ARRAY2D,
//            ARRAY3D,
//            CAMERA,
//            FRAME,
//            GEOMETRY,
//            GROUP,
//            INSTANCE,
//            LIGHT,
//            MATERIAL,
//            RENDERER,
//            SURFACE,
//            SAMPLER,
//            SPATIAL_FIELD,
//            VOLUME,
//            WORLD
//        );
//        const std::string_view dataTypes[] = {
//            "INT8", "UINT8", "INT16", "UINT16", "INT32", "UINT32", "INT64", "UINT64",
//            "FIXED8", "UFIXED8", "FIXED16", "UFIXED16", "FIXED32", "UFIXED32", "FIXED64", "UFIXED64",
//            "FLOAT16", "FLOAT32", "FLOAT64"
//        };
//        for (const auto& type : dataTypes) {
//            builder.addEnumElement(vertID, type, "");
//            builder.addEnumElement(vertID, std::string(type) + "_VEC2", "");
//            builder.addEnumElement(vertID, std::string(type) + "_VEC3", "");
//            builder.addEnumElement(vertID, std::string(type) + "_VEC4", "");
//        }
//
//        ENUMS2(
//            (UFIXED8_RGBA_SRGB, 2003)
//            (UFIXED8_RGB_SRGB, 2002)
//            (UFIXED8_RA_SRGB, 2001)
//            (UFIXED8_R_SRGB, 2000)
//        );
//
//        const std::string_view boxTypes[] = {
//            "INT32", "FLOAT32", "FLOAT64"
//        };
//        for (const auto& type : boxTypes) {
//            builder.addEnumElement(vertID, std::string(type) + "_BOX1", "");
//            builder.addEnumElement(vertID, std::string(type) + "_BOX2", "");
//            builder.addEnumElement(vertID, std::string(type) + "_BOX3", "");
//            builder.addEnumElement(vertID, std::string(type) + "_BOX4", "");
//        }
//
//        ENUMS(
//            UINT64_REGION1,
//            UINT64_REGION2,
//            UINT64_REGION3,
//            UINT64_REGION4,
//            FLOAT32_MAT2,
//            FLOAT32_MAT3,
//            FLOAT32_MAT4,
//            FLOAT32_MAT2x3,
//            FLOAT32_MAT3x4,
//            FLOAT32_QUAT_IJKW
//        );
//
//        SET_ENUM_VALUE(DATA_TYPE, 100);
//        SET_ENUM_VALUE(STRING_LIST, 150);
//        SET_ENUM_VALUE(FUNCTION_POINTER, 200);
//        SET_ENUM_VALUE(LIBRARY, 500);
//        SET_ENUM_VALUE(INT8, 1000);
//        SET_ENUM_VALUE(INT32_BOX1, 2004);
//        SET_ENUM_VALUE(FLOAT64_BOX1, 2208);
//        SET_ENUM_VALUE(UINT64_REGION1, 2104);
//        SET_ENUM_VALUE(FLOAT32_MAT2, 2012);
//    }
//
//    STRUCT(Parameter) {
//        PUBLIC(
//            (ccstd::string, mName, _)
//            (DataType, mType, DataType::UNKNOWN)
//        );
//    }
//
//    ENUM_CLASS(WaitMask) {
//        UNDERLYING_TYPE(uint32_t);
//        ENUMS(NO_WAIT, WAIT);
//    }
//
//    STRUCT(BufferView) {
//        PUBLIC(
//            ([[nullable]] IntrusivePtr<gfx::Buffer>, mBuffer, _)
//            (uint64_t, mBufferOffset, 0)
//            (uint64_t, mSizeInBytes, 0)
//            (uint64_t, mStrideInBytes, 0)
//        );
//    }
//
//    INTERFACE(RenderObject) {
//        INHERITS(RefCounted);
//        PUBLIC_METHODS(R"(
//virtual void destroy() noexcept = 0;
//)");
//    }
//
//    INTERFACE(RenderGeometry) {
//        INHERITS(RenderObject);
//        PUBLIC_METHODS(R"(
//virtual void setPrimitiveColor(DataType type, const BufferView& color) = 0;
//virtual void setPrimitiveAttribute0(DataType type, const BufferView& attribute) = 0;
//virtual void setPrimitiveAttribute1(DataType type, const BufferView& attribute) = 0;
//virtual void setPrimitiveAttribute2(DataType type, const BufferView& attribute) = 0;
//virtual void setPrimitiveAttribute3(DataType type, const BufferView& attribute) = 0;
//virtual void setPrimitiveId(Uint32Array id) = 0;
//)");
//    }
//
//    INTERFACE(RenderGeometryCone) {
//        INHERITS(RenderGeometry);
//        PUBLIC_METHODS(R"(
//virtual void setVertexPosition(const BufferView& position) = 0;
//virtual void setVertexRadius(const BufferView& radius) = 0;
//virtual void setVertexCap(const BufferView& cap) = 0;
//virtual void setVertexColor(DataType type, const BufferView& color) = 0;
//virtual void setVertexAttribute0(DataType type, const BufferView& attribute) = 0;
//virtual void setVertexAttribute1(DataType type, const BufferView& attribute) = 0;
//virtual void setVertexAttribute2(DataType type, const BufferView& attribute) = 0;
//virtual void setVertexAttribute3(DataType type, const BufferView& attribute) = 0;
//virtual void setPrimitiveIndex(Uint32Array index) = 0;
//virtual void setCaps(const ccstd::string& caps) = 0;
//)");
//    }
//
//    INTERFACE(RenderGeometryCurve) {
//        INHERITS(RenderGeometry);
//        PUBLIC_METHODS(R"(
//virtual void setVertexPosition(const BufferView& position) = 0;
//virtual void setVertexRadius(const BufferView& radius) = 0;
//virtual void setVertexColor(DataType type, const BufferView& color) = 0;
//virtual void setVertexAttribute0(DataType type, const BufferView& attribute) = 0;
//virtual void setVertexAttribute1(DataType type, const BufferView& attribute) = 0;
//virtual void setVertexAttribute2(DataType type, const BufferView& attribute) = 0;
//virtual void setVertexAttribute3(DataType type, const BufferView& attribute) = 0;
//virtual void setPrimitiveIndex(Uint32Array index) = 0;
//virtual void setRadius(float radius) = 0;
//)");
//    }
//    
//    INTERFACE(RenderGeometryCylinder) {
//        INHERITS(RenderGeometry);
//        PUBLIC_METHODS(R"(
//virtual void setVertexPosition(const BufferView& position) = 0;
//virtual void setVertexCap(const BufferView& cap) = 0;
//virtual void setVertexColor(DataType type, const BufferView& color) = 0;
//virtual void setVertexAttribute0(DataType type, const BufferView& attribute) = 0;
//virtual void setVertexAttribute1(DataType type, const BufferView& attribute) = 0;
//virtual void setVertexAttribute2(DataType type, const BufferView& attribute) = 0;
//virtual void setVertexAttribute3(DataType type, const BufferView& attribute) = 0;
//virtual void setPrimitiveIndex(Uint32Array index) = 0;
//virtual void setPrimitiveRadius(const BufferView& radius) = 0;
//virtual void setRadius(float radius) = 0;
//virtual void setCaps(const ccstd::string& caps) = 0;
//)");
//    }
//
//    INTERFACE(RenderGeometryQuad) {
//        INHERITS(RenderGeometry);
//        PUBLIC_METHODS(R"(
//virtual void setVertexPosition(const BufferView& position) = 0;
//virtual void setVertexNormal(DataType type, const BufferView& normal) = 0;
//virtual void setVertexTangent(DataType type, const BufferView& tangent) = 0;
//virtual void setVertexColor(DataType type, const BufferView& color) = 0;
//virtual void setVertexAttribute0(DataType type, const BufferView& attribute) = 0;
//virtual void setVertexAttribute1(DataType type, const BufferView& attribute) = 0;
//virtual void setVertexAttribute2(DataType type, const BufferView& attribute) = 0;
//virtual void setVertexAttribute3(DataType type, const BufferView& attribute) = 0;
//virtual void setPrimitiveIndex(Uint32Array index) = 0;
//)");
//    }
//
//    INTERFACE(RenderGeometrySphere) {
//        INHERITS(RenderGeometry);
//        PUBLIC_METHODS(R"(
//virtual void setVertexPosition(const BufferView& position) = 0;
//virtual void setVertexRadius(const BufferView& radius) = 0;
//virtual void setVertexColor(DataType type, const BufferView& color) = 0;
//virtual void setVertexAttribute0(DataType type, const BufferView& attribute) = 0;
//virtual void setVertexAttribute1(DataType type, const BufferView& attribute) = 0;
//virtual void setVertexAttribute2(DataType type, const BufferView& attribute) = 0;
//virtual void setVertexAttribute3(DataType type, const BufferView& attribute) = 0;
//virtual void setPrimitiveIndex(Uint32Array index) = 0;
//virtual void setRadius(float radius) = 0;
//)");
//    }
//
//    INTERFACE(RenderGeometryTriangle) {
//        INHERITS(RenderGeometry);
//        PUBLIC_METHODS(R"(
//virtual void setVertexPosition(const BufferView& position) = 0;
//virtual void setVertexNormal(DataType type, const BufferView& normal) = 0;
//virtual void setVertexTangent(DataType type, const BufferView& tangent) = 0;
//virtual void setVertexColor(DataType type, const BufferView& color) = 0;
//virtual void setVertexAttribute0(DataType type, const BufferView& attribute) = 0;
//virtual void setVertexAttribute1(DataType type, const BufferView& attribute) = 0;
//virtual void setVertexAttribute2(DataType type, const BufferView& attribute) = 0;
//virtual void setVertexAttribute3(DataType type, const BufferView& attribute) = 0;
//virtual void setPrimitiveIndex(const BufferView& index) = 0;
//)");
//    }
//
//    INTERFACE(RenderSampler) {
//        INHERITS(RenderObject);
//    }
//
//    INTERFACE(RenderSamplerImage1D) {
//        INHERITS(RenderSampler);
//        PUBLIC_METHODS(R"(
//virtual void setInAttribute(const ccstd::string& inAttribute) = 0;
//virtual void setInTransform(const Mat4& inTransform) = 0;
//virtual void setInOffset(const Vec4& inOffset) = 0;
//virtual void setImage(DataType type, IntrusivePtr<gfx::Texture> image) = 0;
//virtual void setFilter(const ccstd::string& filter) = 0;
//virtual void setWrapMode(const ccstd::string& wrapMode) = 0;
//virtual void setOutTransform(const Mat4& outTransform) = 0;
//virtual void setOutOffset(const Vec4& outOffset) = 0;
//)");
//    }
//
//    INTERFACE(RenderSamplerImage2D) {
//        INHERITS(RenderSampler);
//        PUBLIC_METHODS(R"(
//virtual void setInAttribute(const ccstd::string& inAttribute) = 0;
//virtual void setInTransform(const Mat4& inTransform) = 0;
//virtual void setInOffset(const Vec4& inOffset) = 0;
//virtual void setImage(DataType type, IntrusivePtr<gfx::Texture> image) = 0;
//virtual void setFilter(const ccstd::string& filter) = 0;
//virtual void setWrapMode1(const ccstd::string& wrapMode) = 0;
//virtual void setWrapMode2(const ccstd::string& wrapMode) = 0;
//virtual void setOutTransform(const Mat4& outTransform) = 0;
//virtual void setOutOffset(const Vec4& outOffset) = 0;
//)");
//    }
//
//    INTERFACE(RenderSamplerImage3D) {
//        INHERITS(RenderSampler);
//        PUBLIC_METHODS(R"(
//virtual void setInAttribute(const ccstd::string& inAttribute) = 0;
//virtual void setInTransform(const Mat4& inTransform) = 0;
//virtual void setInOffset(const Vec4& inOffset) = 0;
//virtual void setImage(DataType type, IntrusivePtr<gfx::Texture> image) = 0;
//virtual void setFilter(const ccstd::string& filter) = 0;
//virtual void setWrapMode1(const ccstd::string& wrapMode) = 0;
//virtual void setWrapMode2(const ccstd::string& wrapMode) = 0;
//virtual void setWrapMode3(const ccstd::string& wrapMode) = 0;
//virtual void setOutTransform(const Mat4& outTransform) = 0;
//virtual void setOutOffset(const Vec4& outOffset) = 0;
//)");
//    }
//
//    INTERFACE(RenderSamplerPrimitive) {
//        INHERITS(RenderSampler);
//        PUBLIC_METHODS(R"(
//virtual void setArray(DataType type, Uint8Array array) = 0;
//virtual void setInOffset(uint64_t inOffset) = 0;
//)");
//    }
//
//    INTERFACE(RenderSamplerTransform) {
//        INHERITS(RenderSampler);
//        PUBLIC_METHODS(R"(
//virtual void setInAttribute(const ccstd::string& inAttribute) = 0;
//virtual void setOutTransform(const Mat4& outTransform) = 0;
//virtual void setOutOffset(const Vec4& outOffset) = 0;
//)");
//    }
//
//    INTERFACE(RenderMaterial) {
//        INHERITS(RenderObject);
//    }
//
//    INTERFACE(RenderMaterialMatte) {
//        INHERITS(RenderMaterial);
//        PUBLIC_METHODS(R"(
//virtual void setColor(IntrusivePtr<RenderSampler> color) = 0;
//virtual void setColorValue(const Vec3& color) = 0;
//virtual void setOpacity(IntrusivePtr<RenderSampler> opacity) = 0;
//virtual void setOpacityValue(float opacity) = 0;
//virtual void setAlphaMode(const ccstd::string& alphaMode) = 0;
//virtual void setAlphaCutoff(float alphaCutoff) = 0;
//)");
//    }
//
//    INTERFACE(RenderMaterialPhysicallyBased) {
//        INHERITS(RenderMaterial);
//        PUBLIC_METHODS(R"(
//virtual void setBaseColor(IntrusivePtr<RenderSampler> baseColor) = 0;
//virtual void setBaseColorValue(const Vec3& baseColor) = 0;
//virtual void setOpacity(IntrusivePtr<RenderSampler> opacity) = 0;
//virtual void setOpacityValue(float opacity) = 0;
//virtual void setMetallic(IntrusivePtr<RenderSampler> metallic) = 0;
//virtual void setMetallicValue(float metallic) = 0;
//virtual void setRoughness(IntrusivePtr<RenderSampler> roughness) = 0;
//virtual void setRoughnessValue(float roughness) = 0;
//
//virtual void setNormal(const IntrusivePtr<RenderSampler>& normal) = 0;
//
//virtual void setEmissive(IntrusivePtr<RenderSampler> emissive) = 0;
//virtual void setEmissiveValue(const Vec3& emissive) = 0;
//
//virtual void setOcclusion(const IntrusivePtr<RenderSampler>& occlusion) = 0;
//
//virtual void setAlphaMode(const ccstd::string& alphaMode) = 0;
//virtual void setAlphaCutoff(float alphaCutoff) = 0;
//
//virtual void setSpecular(IntrusivePtr<RenderSampler> specular) = 0;
//virtual void setSpecularValue(float specular) = 0;
//virtual void setSpecularColor(IntrusivePtr<RenderSampler> specular) = 0;
//virtual void setSpecularColorValue(const Vec3& specular) = 0;
//
//virtual void setClearcoat(IntrusivePtr<RenderSampler> clearcoat) = 0;
//virtual void setClearcoatValue(float clearcoat) = 0;
//virtual void setClearcoatRoughness(IntrusivePtr<RenderSampler> clearcoatRoughness) = 0;
//virtual void setClearcoatRoughnessValue(float clearcoatRoughness) = 0;
//virtual void setClearcoatNormal(const IntrusivePtr<RenderSampler>& clearcoatNormal) = 0;
//
//virtual void setTransmission(IntrusivePtr<RenderSampler> transmission) = 0;
//virtual void setTransmissionValue(float transmission) = 0;
//
//virtual void setIor(float ior) = 0;
//
//virtual void setThickness(IntrusivePtr<RenderSampler> thickness) = 0;
//virtual void setThicknessValue(float thickness) = 0;
//
//virtual void setAttenuationDistance(float attenuationDistance) = 0;
//virtual void setAttenuationColor(const Vec3& attenuationColor) = 0;
//
//virtual void setSheenColor(IntrusivePtr<RenderSampler> sheenColor) = 0;
//virtual void setSheenColorValue(const Vec3& sheenColor) = 0;
//virtual void setSheenRoughness(IntrusivePtr<RenderSampler> sheenRoughness) = 0;
//virtual void setSheenRoughnessValue(float sheenRoughness) = 0;
//
//virtual void setIridescence(IntrusivePtr<RenderSampler> iridescence) = 0;
//virtual void setIridescenceValue(float iridescence) = 0;
//virtual void setIridescenceIor(float iridescenceIor) = 0;
//virtual void setIridescenceThickness(IntrusivePtr<RenderSampler> iridescenceThickness) = 0;
//virtual void setIridescenceThicknessValue(float iridescenceThickness) = 0;
//)");
//    }
//
//    INTERFACE(RenderVolume) {
//        INHERITS(RenderObject);
//    }
//    
//    INTERFACE(RenderSpatialField) {
//        INHERITS(RenderObject);
//    }
//
//    INTERFACE(RenderSpatialFieldStructuredRegular) {
//        INHERITS(RenderSpatialField);
//        PUBLIC_METHODS(R"(
//virtual void setData(DataType type, IntrusivePtr<gfx::Texture> data) = 0;
//virtual void setOrigin(const Vec3& origin) = 0;
//virtual void setSpacing(const Vec3& spacing) = 0;
//virtual void setFilter(const ccstd::string& filter) = 0;
//)");
//    }
//
//    INTERFACE(RenderVolumeTransferFunction1D) {
//        INHERITS(RenderVolume);
//        PUBLIC_METHODS(R"(
//virtual void setValue(const IntrusivePtr<RenderSpatialField>& value) = 0;
//virtual void setValueRange(float rangeMin, float rangeMax) = 0;
//virtual void setColor(DataType type, IntrusivePtr<gfx::Texture> color) = 0;
//virtual void setOpacity(Float32Array opacity) = 0;
//virtual void setOpacityValue(float opacity) = 0;
//virtual void setUnitDistance(float unitDistance) = 0;
//)");
//    }
//
//    INTERFACE(RenderLight) {
//        INHERITS(RenderObject);
//        PUBLIC_METHODS(R"(
//virtual void setColor(const Vec3& color) = 0;
//virtual void setVisible(bool visible) = 0;
//)");
//    }
//
//    INTERFACE(RenderLightDirectional) {
//        INHERITS(RenderLight);
//        PUBLIC_METHODS(R"(
//virtual void setDirection(const Vec3& direction) = 0;
//virtual void setIrradiance(float irradiance) = 0;
//virtual void setAngularDiameter(float angularDiameter) = 0;
//virtual void setRadiance(float radiance) = 0;
//)");
//    }
//
//    INTERFACE(RenderLightHDRI) {
//        INHERITS(RenderLight);
//        PUBLIC_METHODS(R"(
//virtual void setUp(const Vec3& up) = 0;
//virtual void setDirection(const Vec3& direction) = 0;
//virtual void setRadiance(Float32Array radiance) = 0;
//virtual void setLayout(const ccstd::string& layout) = 0;
//virtual void setScale(float scale) = 0;
//)");
//    }
//
//    INTERFACE(RenderLightPoint) {
//        INHERITS(RenderLight);
//        PUBLIC_METHODS(R"(
//virtual void setPosition(const Vec3& position) = 0;
//virtual void setIntensity(float intensity) = 0;
//virtual void setPower(float power) = 0;
//virtual void setRadius(float radius) = 0;
//virtual void setRadiance(float radiance) = 0;
//)");
//    }
//
//    INTERFACE(RenderLightQuad) {
//        INHERITS(RenderLight);
//        PUBLIC_METHODS(R"(
//virtual void setPosition(const Vec3& position) = 0;
//virtual void setEdge1(const Vec3& edge1) = 0;
//virtual void setEdge2(const Vec3& edge2) = 0;
//virtual void setIntensity(float intensity) = 0;
//virtual void setPower(float power) = 0;
//virtual void setRadiance(float radiance) = 0;
//virtual void setSide(const ccstd::string& side) = 0;
//virtual void setIntensityDistribution(DataType type, Float32Array intensityDistribution) = 0;
//)");
//    }
//
//    INTERFACE(RenderLightRing) {
//        INHERITS(RenderLight);
//        PUBLIC_METHODS(R"(
//virtual void setPosition(const Vec3& position) = 0;
//virtual void setDirection(const Vec3& direction) = 0;
//virtual void setOpeningAngle(float openingAngle) = 0;
//virtual void setFalloffAngle(float falloffAngle) = 0;
//virtual void setIntensity(float intensity) = 0;
//virtual void setPower(float power) = 0;
//virtual void setRadius(float radius) = 0;
//virtual void setInnerRadius(float innerRadius) = 0;
//virtual void setRadiance(float radiance) = 0;
//virtual void setIntensityDistribution(DataType type, Float32Array intensityDistribution) = 0;
//virtual void setC0(const Vec3& c0) = 0;
//)");
//    }
//    
//    INTERFACE(RenderSurface) {
//        INHERITS(RenderObject);
//        PUBLIC_METHODS(R"(
//virtual void setGeometry(const IntrusivePtr<RenderGeometry>& geometry) = 0;
//virtual void setMaterial(const IntrusivePtr<RenderMaterial>& material) = 0;
//virtual void setId(uint32_t id) = 0;
//)");
//    }
//    
//    INTERFACE(RenderGroup) {
//        INHERITS(RenderObject);
//        PUBLIC_METHODS(R"(
//virtual void setSurface(ccstd::vector<IntrusivePtr<RenderSurface>>&& surface) = 0;
//virtual void setVolume(ccstd::vector<IntrusivePtr<RenderVolume>>&& volume) = 0;
//virtual void setLight(ccstd::vector<IntrusivePtr<RenderLight>>&& light) = 0;
//
//virtual geometry::AABB getBounds(WaitMask waitMask) const = 0;
//)");
//    }
//
//    INTERFACE(RenderInstance) {
//        INHERITS(RenderObject);
//        PUBLIC_METHODS(R"(
//virtual void setGroup(const IntrusivePtr<RenderGroup>& group) = 0;
//virtual void setTransform(const Mat4& transform) = 0;
//
//virtual geometry::AABB getBounds(WaitMask waitMask) const = 0;
//)");
//    }
//// virtual void setId(uint32_t id) = 0;
//
//    INTERFACE(RenderInstanceTransform) {
//        INHERITS(RenderInstance);
//    }
//
//    INTERFACE(RenderInstanceMotionTransform) {
//        INHERITS(RenderInstance);
//        PUBLIC_METHODS(R"(
//virtual void setMotionTransform(Float32Array transform) = 0;
//virtual void setTime(float timeMin, float timeMax) = 0;
//)");
//    }
//
//    INTERFACE(RenderInstanceMotionScaleRotationTranslation) {
//        INHERITS(RenderInstance);
//        PUBLIC_METHODS(R"(
//virtual void setMotionScale(Float32Array scale) = 0;
//virtual void setMotionRotation(Float32Array rotation) = 0;
//virtual void setMotionTranslation(Float32Array translation) = 0;
//virtual void setTime(float timeMin, float timeMax) = 0;
//)");
//    }
//    
//    INTERFACE(RenderWorld) {
//        INHERITS(RenderObject);
//        PUBLIC_METHODS(R"(
//virtual bool isEmpty() const = 0;
//
//virtual void setInstance(ccstd::vector<IntrusivePtr<RenderInstance>>&& instance) = 0;
//virtual void setSurface(ccstd::vector<IntrusivePtr<RenderSurface>>&& surface) = 0;
//virtual void setVolume(ccstd::vector<IntrusivePtr<RenderVolume>>&& volume) = 0;
//virtual void setLight(ccstd::vector<IntrusivePtr<RenderLight>>&& light) = 0;
//
//virtual void addInstance(const IntrusivePtr<RenderInstance>& instance) = 0;
//virtual void addSurface(const IntrusivePtr<RenderSurface>& surface) = 0;
//
//virtual void removeInstance(const IntrusivePtr<RenderInstance>& instance) = 0;
//virtual void removeSurface(const IntrusivePtr<RenderSurface>& surface) = 0;
//
//virtual void addInstances(ccstd::vector<IntrusivePtr<RenderInstance>>&& instance) = 0;
//virtual void addSurfaces(ccstd::vector<IntrusivePtr<RenderSurface>>&& surface) = 0;
//
//virtual void removeInstances(const ccstd::vector<IntrusivePtr<RenderInstance>>& instance) = 0;
//virtual void removeSurfaces(const ccstd::vector<IntrusivePtr<RenderSurface>>& surface) = 0;
//
//virtual geometry::AABB getBounds(WaitMask waitMask) const = 0;
//)");
//    }
//
//    INTERFACE(RenderCamera) {
//        INHERITS(RenderObject);
//        PUBLIC_METHODS(R"(
//virtual void setPosition(const Vec3& position) = 0;
//virtual void setDirection(const Vec3& direction) = 0;
//virtual void setUp(const Vec3& up) = 0;
//virtual void setImageRegion(const Vec4& imageRegion) = 0;
//virtual void setApertureRadius(float apertureRadius) = 0;
//virtual void setFocusDistance(float focusDistance) = 0;
//virtual void setShutter(const Vec2& shutter) = 0;
//)");
//    }
//
//    INTERFACE(RenderCameraPerspective) {
//        INHERITS(RenderCamera);
//        PUBLIC_METHODS(R"(
//virtual void setFovy(float fovy) = 0;
//virtual void setAspect(float aspect) = 0;
//virtual void setNear(float near) = 0;
//virtual void setFar(float far) = 0;
//)");
//    }
//
//    INTERFACE(RenderCameraOmnidirectional) {
//        INHERITS(RenderCamera);
//        PUBLIC_METHODS(R"(
//virtual void setLayout(const ccstd::string& layout) = 0;
//)");
//    }
//
//    INTERFACE(RenderCameraOrthographic) {
//        INHERITS(RenderCamera);
//        PUBLIC_METHODS(R"(
//virtual void setAspect(float aspect) = 0;
//virtual void setHeight(float height) = 0;
//virtual void setNear(float near) = 0;
//virtual void setFar(float far) = 0;
//)");
//    }
//    
//    INTERFACE(Renderer) {
//        INHERITS(RenderObject);
//        PUBLIC_METHODS(R"(
//virtual void setBackground(const Vec4& background) = 0;
//virtual void setAmbientColor(const Vec3& ambientColor) = 0;
//virtual void setAmbientRadiance(float ambientRadiance) = 0;
//
//virtual const ccstd::vector<ccstd::string>& getExtension(WaitMask waitMask) const = 0;
//)");
//    }
//    
//    INTERFACE(RenderDevice) {
//        INHERITS(RenderObject);
//        PUBLIC_METHODS(R"(
//[[getter]] virtual int32_t getVersion() const noexcept = 0;
//[[getter]] virtual uint64_t getGeometryMaxIndex() const = 0;
//[[getter]] virtual ccstd::vector<ccstd::string> getExtension() const = 0;
//virtual ccstd::vector<ccstd::string> getObjectSubtypes(DataType objectType) const = 0;
//
//virtual IntrusivePtr<RenderCamera> createCamera(const ccstd::string& subtype) = 0;
//virtual IntrusivePtr<Renderer> createRenderer(const ccstd::string& subtype) = 0;
//virtual IntrusivePtr<RenderWorld> createWorld() = 0;
//virtual IntrusivePtr<RenderInstance> createInstance(const ccstd::string& subtype) = 0;
//virtual IntrusivePtr<RenderGroup> createGroup() = 0;
//virtual IntrusivePtr<RenderLight> createLight(const ccstd::string& subtype) = 0;
//virtual IntrusivePtr<RenderSurface> createSurface() = 0;
//virtual IntrusivePtr<RenderGeometry> createGeometry(const ccstd::string& subtype) = 0;
//virtual IntrusivePtr<RenderSampler> createSampler(const ccstd::string& subtype) = 0;
//virtual IntrusivePtr<RenderMaterial> createMaterial(const ccstd::string& subtype) = 0;
//virtual IntrusivePtr<RenderVolume> createVolume(const ccstd::string& subtype) = 0;
//virtual IntrusivePtr<RenderSpatialField> createSpatialField(const ccstd::string& subtype) = 0;
//)");
//    }
//// virtual IntrusivePtr<RenderFrame> createFrame() = 0;
//// virtual Property getObjectInfo(DataType objectType, const ccstd::string& objectSubtype, const ccstd::string& infoName, DataType infoType) const = 0;
//// virtual Property getParameterInfo(DataType objectType, const ccstd::string& objectSubtype, const ccstd::string& parameterName, DataType parameterType, const ccstd::string& infoName, DataType infoType) const = 0;

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
