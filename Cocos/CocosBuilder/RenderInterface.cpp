#include "CocosModules.h"
#include <Cocos/AST/DSL.h>
#include <Cocos/AST/SyntaxGraphs.h>

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
)"
    ) {
        NAMESPACE_BEG(cc);
        NAMESPACE_BEG(render);

        INTERFACE(OutputArchive) {
            MEMBER_FUNCTIONS(R"(
virtual void writeBool(bool value) = 0;
virtual void writeNumber(double value) = 0;
virtual void writeString(std::string_view value) = 0;
)");
        }

        INTERFACE(InputArchive) {
            MEMBER_FUNCTIONS(R"(
virtual bool readBool() = 0;
virtual double readNumber() = 0;
virtual std::string_view readString() = 0;
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
        .mToJsFilename = "renderer.i",
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
)",
        .mTypescriptFolder = "cocos/rendering/custom",
        .mTypescriptFilePrefix = "pipeline",
        .mRequires = { "Gfx", "RenderCommon", "Camera", "PipelineSceneData" },
        .mHeader = R"(#include "cocos/renderer/gfx-base/GFXDef-common.h"

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

class Model;
class RenderScene;
class RenderWindow;

} // namespace scene

} // namespace cc
)"
    ) {
        NAMESPACE_BEG(cc);
        NAMESPACE_BEG(render);

        INTERFACE(PipelineRuntime) {
            MEMBER_FUNCTIONS(R"(
virtual bool activate(gfx::Swapchain * swapchain) = 0;
virtual bool destroy() noexcept = 0;
virtual void render(const ccstd::vector<scene::Camera*>& cameras) = 0;

[[getter]] virtual gfx::Device* getDevice() const = 0;
[[skip]] virtual const MacroRecord &getMacros() const = 0;
[[getter]] virtual pipeline::GlobalDSManager *getGlobalDSManager() const = 0;
[[getter]] virtual gfx::DescriptorSetLayout *getDescriptorSetLayout() const = 0;
[[getter]] virtual gfx::DescriptorSet *getDescriptorSet() const = 0;
[[getter]] virtual const ccstd::vector<gfx::CommandBuffer*>& getCommandBuffers() const = 0;
[[getter]] virtual pipeline::PipelineSceneData *getPipelineSceneData() const = 0;
[[getter]] virtual const ccstd::string &getConstantMacros() const = 0;
[[optional]] [[getter]] virtual scene::Model *getProfiler() const = 0;
[[setter]] virtual void setProfiler([[optional]] scene::Model *profiler) = 0;
[[optional]] [[getter]] virtual pipeline::GeometryRenderer  *getGeometryRenderer() const = 0;

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
            TS_FUNCTIONS(R"(
readonly macros: MacroRecord;
)");
        }

        INTERFACE(Setter) {
            MEMBER_FUNCTIONS(R"(
virtual void setMat4(const ccstd::string& name, const cc::Mat4& mat) = 0;
virtual void setQuaternion(const ccstd::string& name, const cc::Quaternion& quat) = 0;
virtual void setColor(const ccstd::string& name, const gfx::Color& color) = 0;
virtual void setVec4(const ccstd::string& name, const cc::Vec4& vec) = 0;
virtual void setVec2(const ccstd::string& name, const cc::Vec2& vec) = 0;
virtual void setFloat(const ccstd::string& name, float v) = 0;

virtual void setBuffer(const ccstd::string& name, gfx::Buffer* buffer) = 0;
virtual void setTexture(const ccstd::string& name, gfx::Texture* texture) = 0;
virtual void setReadWriteBuffer(const ccstd::string& name, gfx::Buffer* buffer) = 0;
virtual void setReadWriteTexture(const ccstd::string& name, gfx::Texture* texture) = 0;
virtual void setSampler(const ccstd::string& name, gfx::Sampler* sampler) = 0;
)");
        }

        INTERFACE(RasterQueueBuilder) {
            INHERITS(Setter);
            MEMBER_FUNCTIONS(R"(
virtual void addSceneOfCamera(scene::Camera* camera, LightInfo light, SceneFlags sceneFlags, const ccstd::string& name) = 0;
virtual void addSceneOfCamera(scene::Camera* camera, LightInfo light, SceneFlags sceneFlags) = 0;
virtual void addScene(const ccstd::string& name, SceneFlags sceneFlags) = 0;
virtual void addFullscreenQuad(cc::Material *material, uint32_t passID, SceneFlags sceneFlags, const ccstd::string& name) = 0;
virtual void addFullscreenQuad(cc::Material *material, uint32_t passID, SceneFlags sceneFlags) = 0;
virtual void addCameraQuad(scene::Camera* camera, cc::Material *material, uint32_t passID, SceneFlags sceneFlags, const ccstd::string& name) = 0;
virtual void addCameraQuad(scene::Camera* camera, cc::Material *material, uint32_t passID, SceneFlags sceneFlags) = 0;
virtual void clearRenderTarget(const ccstd::string &name, const gfx::Color &color) = 0;
virtual void setViewport(const gfx::Viewport &viewport) = 0;
)");
        }

        INTERFACE(RasterPassBuilder) {
            INHERITS(Setter);
            MEMBER_FUNCTIONS(R"(
virtual void addRasterView(const ccstd::string& name, const RasterView& view) = 0;
virtual void addComputeView(const ccstd::string& name, const ComputeView& view) = 0;
virtual RasterQueueBuilder *addQueue(QueueHint hint, const ccstd::string& name) = 0;
virtual RasterQueueBuilder *addQueue(QueueHint hint) = 0;
virtual void addFullscreenQuad(cc::Material *material, uint32_t passID, SceneFlags sceneFlags, const ccstd::string& name) = 0;
virtual void addFullscreenQuad(cc::Material *material, uint32_t passID, SceneFlags sceneFlags) = 0;
virtual void addCameraQuad(scene::Camera* camera, cc::Material *material, uint32_t passID, SceneFlags sceneFlags, const ccstd::string& name) = 0;
virtual void addCameraQuad(scene::Camera* camera, cc::Material *material, uint32_t passID, SceneFlags sceneFlags) = 0;
virtual void setViewport(const gfx::Viewport &viewport) = 0;
)");
        }

        INTERFACE(ComputeQueueBuilder) {
            INHERITS(Setter);
            MEMBER_FUNCTIONS(R"(
virtual void addDispatch(const ccstd::string& shader, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ, const ccstd::string& name) = 0;
virtual void addDispatch(const ccstd::string& shader, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ) = 0;
)");
        }

        INTERFACE(ComputePassBuilder) {
            INHERITS(Setter);
            MEMBER_FUNCTIONS(R"(
virtual void addComputeView(const ccstd::string& name, const ComputeView& view) = 0;

virtual ComputeQueueBuilder *addQueue(const ccstd::string& name) = 0;
virtual ComputeQueueBuilder *addQueue() = 0;

virtual void addDispatch(const ccstd::string& shader, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ, const ccstd::string& name) = 0;
virtual void addDispatch(const ccstd::string& shader, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ) = 0;
)");
        }

        INTERFACE(MovePassBuilder) {
            MEMBER_FUNCTIONS(R"(
virtual void addPair(const MovePair& pair) = 0;
)");
        }

        INTERFACE(CopyPassBuilder) {
            MEMBER_FUNCTIONS(R"(
virtual void addPair(const CopyPair& pair) = 0;
)");
        }

        INTERFACE(SceneVisitor) {
            MEMBER_FUNCTIONS(R"(
[[getter]] virtual const pipeline::PipelineSceneData* getPipelineSceneData() const = 0;

virtual void setViewport(const gfx::Viewport &vp) = 0;
virtual void setScissor(const gfx::Rect &rect) = 0;
virtual void bindPipelineState(gfx::PipelineState* pso) = 0;
[[skip]] virtual void bindDescriptorSet(uint32_t set, gfx::DescriptorSet *descriptorSet, uint32_t dynamicOffsetCount, const uint32_t *dynamicOffsets) = 0;
virtual void bindInputAssembler(gfx::InputAssembler *ia) = 0;
[[skip]] virtual void updateBuffer(gfx::Buffer *buff, const void *data, uint32_t size) = 0;
virtual void draw(const gfx::DrawInfo &info) = 0;
)");
            TS_FUNCTIONS(R"(
bindDescriptorSet (set: number, descriptorSet: DescriptorSet, dynamicOffsets?: number[]): void;
updateBuffer (buffer: Buffer, data: ArrayBuffer, size?: number): void;
)");
        }

        INTERFACE(SceneTask) {
            MEMBER_FUNCTIONS(R"(
[[getter]] virtual TaskType getTaskType() const noexcept = 0;
virtual void     start() = 0;
virtual void     join() = 0;
virtual void     submit() = 0;
)");
        }

        INTERFACE(SceneTransversal) {
            MEMBER_FUNCTIONS(R"(
virtual SceneTask* transverse(SceneVisitor *visitor) const = 0;
)");
        }

        INTERFACE(LayoutGraphBuilder) {
            MEMBER_FUNCTIONS(R"(
virtual void clear() = 0;
virtual uint32_t addRenderStage(const ccstd::string& name) = 0;
virtual uint32_t addRenderPhase(const ccstd::string& name, uint32_t parentID) = 0;
virtual void addShader(const ccstd::string& name, uint32_t parentPhaseID) = 0;
virtual void addDescriptorBlock(uint32_t nodeID, const DescriptorBlockIndex& index, const DescriptorBlockFlattened& block) = 0;
virtual void addUniformBlock(uint32_t nodeID, const DescriptorBlockIndex& index, const ccstd::string& name, const gfx::UniformBlock& uniformBlock) = 0;
virtual void reserveDescriptorBlock(uint32_t nodeID, const DescriptorBlockIndex& index, const DescriptorBlockFlattened& block) = 0;
virtual int compile() = 0;

virtual ccstd::string print() const = 0;
)");
        }

        INTERFACE(Pipeline) {
            INHERITS(PipelineRuntime);
            MEMBER_FUNCTIONS(R"(
virtual void beginSetup() = 0;
virtual void endSetup() = 0;

virtual bool containsResource(const ccstd::string& name) const = 0;
virtual uint32_t addRenderTexture(const ccstd::string& name, gfx::Format format, uint32_t width, uint32_t height, scene::RenderWindow* renderWindow) = 0;
virtual uint32_t addRenderTarget(const ccstd::string& name, gfx::Format format, uint32_t width, uint32_t height, ResourceResidency residency) = 0;
virtual uint32_t addDepthStencil(const ccstd::string& name, gfx::Format format, uint32_t width, uint32_t height, ResourceResidency residency) = 0;

virtual void beginFrame() = 0;
virtual void endFrame() = 0;
virtual RasterPassBuilder *addRasterPass(uint32_t width, uint32_t height, const ccstd::string& layoutName, const ccstd::string& name) = 0;
virtual RasterPassBuilder *addRasterPass(uint32_t width, uint32_t height, const ccstd::string& layoutName) = 0;
virtual ComputePassBuilder *addComputePass(const ccstd::string& layoutName, const ccstd::string& name) = 0;
virtual ComputePassBuilder *addComputePass(const ccstd::string& layoutName) = 0;
virtual MovePassBuilder *addMovePass(const ccstd::string& name) = 0;
virtual CopyPassBuilder *addCopyPass(const ccstd::string& name) = 0;
virtual void presentAll() = 0;

virtual SceneTransversal *createSceneTransversal(const scene::Camera *camera, const scene::RenderScene *scene) = 0;
[[getter]] virtual LayoutGraphBuilder *getLayoutGraphBuilder() = 0;
[[optional]] virtual gfx::DescriptorSetLayout *getDescriptorSetLayout(const ccstd::string& shaderName, UpdateFrequency freq) = 0;
)");
        }

        INTERFACE(PipelineBuilder) {
            MEMBER_FUNCTIONS(R"(
virtual void setup(const ccstd::vector<scene::Camera*>& cameras, Pipeline* pipeline) = 0;
)");
        }

        CLASS(Factory) {
            MEMBER_FUNCTIONS(R"(
static Pipeline *createPipeline();
)");
        }

        NAMESPACE_END(render);
        NAMESPACE_END(cc);
    }
}

}
