#include "CocosModules.h"
#include <Cocos/AST/DSL.h>
#include <Cocos/AST/SyntaxGraphs.h>

namespace Cocos::Meta {

void buildRenderInterface(ModuleBuilder& builder, Features features) {
    MODULE(RenderInterface,
        .mFolder = "cocos/renderer/pipeline/custom",
        .mFilePrefix = "RenderInterface",
        .mToJsFilename = "render.ini",
        .mToJsPrefix = "render",
        .mToJsNamespace = "render",
        .mToJsCppHeaders = " cocos/bindings/auto/jsb_gfx_auto.h cocos/bindings/auto/jsb_scene_auto.h",
        .mTypescriptFolder = "cocos/core/pipeline/custom",
        .mTypescriptFilePrefix = "pipeline",
        .mRequires = { "Gfx", "RenderCommon", "RenderGraph", "Assets" },
        .mHeader = R"(#include "cocos/renderer/gfx-base/GFXDef-common.h"

namespace cc {

class Mat4;
class Mat4;
class Quaternion;
class Color;
class Vec4;
class Vec3;
class Vec2;

class EffectAsset;

namespace pipeline {

class GlobalDSManager;
class PipelineSceneData;

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
virtual void render(const std::vector<const scene::Camera*>& cameras) = 0;

[[getter]] virtual const MacroRecord           &getMacros() const = 0;
[[getter]] virtual pipeline::GlobalDSManager   &getGlobalDSManager() const = 0;
[[getter]] virtual gfx::DescriptorSetLayout    &getDescriptorSetLayout() const = 0;
[[getter]] virtual pipeline::PipelineSceneData &getPipelineSceneData() const = 0;
[[getter]] virtual const std::string           &getConstantMacros() const = 0;
[[nullable]] [[getter]] virtual scene::Model                *getProfiler() const = 0;
[[nullable]] [[setter]] virtual void                         setProfiler(scene::Model *profiler) const = 0;

[[getter]] virtual float getShadingScale() const = 0;
[[setter]] virtual void  setShadingScale(float scale) = 0;

virtual void onGlobalPipelineStateChanged() = 0;
)");
        }

        INTERFACE(DescriptorHierarchy) {
            MEMBER_FUNCTIONS(R"(
virtual void addEffect(EffectAsset* asset) = 0;
)");
        }

        INTERFACE(Setter) {
            MEMBER_FUNCTIONS(R"(
virtual void setMat4(const std::string& name, const cc::Mat4& mat) = 0;
virtual void setQuaternion(const std::string& name, const cc::Quaternion& quat) = 0;
virtual void setColor(const std::string& name, const cc::Color& color) = 0;
virtual void setVec4(const std::string& name, const cc::Vec4& vec) = 0;
virtual void setVec2(const std::string& name, const cc::Vec2& vec) = 0;
virtual void setFloat(const std::string& name, float v) = 0;

virtual void setBuffer(const std::string& name, gfx::Buffer* buffer) = 0;
virtual void setTexture(const std::string& name, gfx::Texture* texture) = 0;
virtual void setReadWriteBuffer(const std::string& name, gfx::Buffer* buffer) = 0;
virtual void setReadWriteTexture(const std::string& name, gfx::Texture* texture) = 0;
virtual void setSampler(const std::string& name, gfx::Sampler* sampler) = 0;
)");
        }

        INTERFACE(RasterQueueBuilder) {
            INHERITS(Setter);
            MEMBER_FUNCTIONS(R"(
virtual void addSceneOfCamera(scene::Camera* camera, const std::string& name) = 0;
virtual void addSceneOfCamera(scene::Camera* camera) = 0;
virtual void addScene(const std::string& name) = 0;
virtual void addFullscreenQuad(const std::string& shader, const std::string& name) = 0;
virtual void addFullscreenQuad(const std::string& shader) = 0;
)");
        }

        INTERFACE(RasterPassBuilder) {
            INHERITS(Setter);
            MEMBER_FUNCTIONS(R"(
virtual void                addRasterView(const std::string& name, const RasterView& view) = 0;
virtual void                addComputeView(const std::string& name, const ComputeView& view) = 0;
virtual RasterQueueBuilder *addQueue(QueueHint hint, const std::string& layoutName, const std::string& name) = 0;
virtual RasterQueueBuilder *addQueue(QueueHint hint, const std::string& layoutName) = 0;
virtual RasterQueueBuilder *addQueue(QueueHint hint) = 0;
virtual void                addFullscreenQuad(const std::string& shader, const std::string& layoutName, const std::string& name) = 0;
virtual void                addFullscreenQuad(const std::string& shader, const std::string& layoutName) = 0;
virtual void                addFullscreenQuad(const std::string& shader) = 0;
)");
        }

        INTERFACE(ComputeQueueBuilder) {
            INHERITS(Setter);
            MEMBER_FUNCTIONS(R"(
virtual void addDispatch(const std::string& shader, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ, const std::string& layoutName, const std::string& name) = 0;
virtual void addDispatch(const std::string& shader, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ, const std::string& layoutName) = 0;
virtual void addDispatch(const std::string& shader, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ) = 0;
)");
        }

        INTERFACE(ComputePassBuilder) {
            INHERITS(Setter);
            MEMBER_FUNCTIONS(R"(
virtual void addComputeView(const std::string& name, const ComputeView& view) = 0;

virtual ComputeQueueBuilder *addQueue(const std::string& layoutName, const std::string& name) = 0;
virtual ComputeQueueBuilder *addQueue(const std::string& layoutName) = 0;
virtual ComputeQueueBuilder *addQueue() = 0;

virtual void addDispatch(const std::string& shader, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ, const std::string& layoutName, const std::string& name) = 0;
virtual void addDispatch(const std::string& shader, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ, const std::string& layoutName) = 0;
virtual void addDispatch(const std::string& shader, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ) = 0;
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
virtual void setViewport(const gfx::Viewport &vp) = 0;
virtual void setScissor(const gfx::Rect &rect) = 0;
virtual void bindPipelineState(gfx::PipelineState* pso) = 0;
virtual void bindDescriptorSet(uint32_t set, gfx::DescriptorSet *descriptorSet, uint32_t dynamicOffsetCount, const uint32_t *dynamicOffsets) = 0;
virtual void bindInputAssembler(gfx::InputAssembler *ia) = 0;
[[skip]] virtual void updateBuffer(gfx::Buffer *buff, const void *data, uint32_t size) = 0;
virtual void draw(const gfx::DrawInfo &info) = 0;
)");
            TS_FUNCTIONS(R"(
public abstract updateBuffer (buffer: Buffer, data: ArrayBuffer, size?: number): void;
)");
        }

        INTERFACE(SceneTask) {
            MEMBER_FUNCTIONS(R"(
[[getter]] virtual TaskType getTaskType() const noexcept = 0;
virtual void     start() = 0;
virtual void     join() = 0;
)");
        }

        INTERFACE(SceneTransversal) {
            MEMBER_FUNCTIONS(R"(
virtual SceneTask* transverse(SceneVisitor *visitor) const = 0;
)");
        }

        INTERFACE(Pipeline) {
            //INHERITS(PipelineRuntime);
            MEMBER_FUNCTIONS(R"(
virtual uint32_t            addRenderTexture(const std::string& name, gfx::Format format, uint32_t width, uint32_t height, scene::RenderWindow* renderWindow) = 0;
virtual uint32_t            addRenderTarget(const std::string& name, gfx::Format format, uint32_t width, uint32_t height, ResourceResidency residency) = 0;
virtual uint32_t            addDepthStencil(const std::string& name, gfx::Format format, uint32_t width, uint32_t height, ResourceResidency residency) = 0;
virtual void                beginFrame(pipeline::PipelineSceneData* pplScene) = 0;
virtual void                endFrame() = 0;
virtual RasterPassBuilder  *addRasterPass(uint32_t width, uint32_t height, const std::string& layoutName, const std::string& name) = 0;
virtual RasterPassBuilder  *addRasterPass(uint32_t width, uint32_t height, const std::string& layoutName) = 0;
virtual ComputePassBuilder *addComputePass(const std::string& layoutName, const std::string& name) = 0;
virtual ComputePassBuilder *addComputePass(const std::string& layoutName) = 0;
virtual MovePassBuilder    *addMovePass(const std::string& name) = 0;
virtual CopyPassBuilder    *addCopyPass(const std::string& name) = 0;
virtual void                addPresentPass(const std::string& name, const std::string& swapchainName) = 0;

virtual SceneTransversal *createSceneTransversal(const scene::Camera *camera, const scene::RenderScene *scene) = 0;
)");
        }

        CLASS(Factory) {
            MEMBER_FUNCTIONS(R"(
static Pipeline            *createPipeline();
static DescriptorHierarchy *createDescriptorHierarchy();
)");
        }

        NAMESPACE_END(render);
        NAMESPACE_END(cc);
    }
}

}
