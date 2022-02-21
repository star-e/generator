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
        .mRequires = { "Gfx", "RenderCommon", "RenderGraph" },
        .mHeader = R"(#include "cocos/renderer/gfx-base/GFXDef-common.h"

namespace cc {

class Mat4;
class Mat4;
class Quaternion;
class Color;
class Vec4;
class Vec3;
class Vec2;

namespace pipeline {

class PipelineSceneData;

} // namespace pipeline

} // namespace cc
)",
    ) {
        NAMESPACE_BEG(cc);
        NAMESPACE_BEG(render);

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
            MEMBER_FUNCTIONS(R"(
virtual void addSceneOfCamera(scene::Camera* camera, const std::string& name) = 0;
virtual void addSceneOfCamera(scene::Camera* camera) = 0;
virtual void addScene(const std::string& name) = 0;
virtual void addFullscreenQuad(const std::string& shader, const std::string& name) = 0;
virtual void addFullscreenQuad(const std::string& shader) = 0;
)");
        }

        INTERFACE(RasterPassBuilder) {
            MEMBER_FUNCTIONS(R"(
virtual void addRasterView(const std::string& name, const RasterView& view) = 0;
virtual void addComputeView(const std::string& name, const ComputeView& view) = 0;
virtual RasterQueueBuilder* addQueue(QueueHint hint, const std::string& layoutName, const std::string& name) = 0;
virtual RasterQueueBuilder* addQueue(QueueHint hint, const std::string& layoutName) = 0;
virtual RasterQueueBuilder* addQueue(QueueHint hint) = 0;
virtual void addFullscreenQuad(const std::string& shader, const std::string& layoutName, const std::string& name) = 0;
virtual void addFullscreenQuad(const std::string& shader, const std::string& layoutName) = 0;
virtual void addFullscreenQuad(const std::string& shader) = 0;
)");
        }

        INTERFACE(ComputeQueueBuilder) {
            MEMBER_FUNCTIONS(R"(
virtual void addDispatch(const std::string& shader, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ, const std::string& layoutName, const std::string& name) = 0;
virtual void addDispatch(const std::string& shader, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ, const std::string& layoutName) = 0;
virtual void addDispatch(const std::string& shader, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ) = 0;
)");
        }

        INTERFACE(ComputePassBuilder) {
            MEMBER_FUNCTIONS(R"(
virtual void addComputeView(const std::string& name, const ComputeView& view) = 0;

virtual ComputeQueueBuilder* addQueue(const std::string& layoutName, const std::string& name) = 0;
virtual ComputeQueueBuilder* addQueue(const std::string& layoutName) = 0;
virtual ComputeQueueBuilder* addQueue() = 0;

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

        INTERFACE(Pipeline) {
            MEMBER_FUNCTIONS(R"(
virtual uint32_t addRenderTexture(const std::string& name, gfx::Format format, uint32_t width, uint32_t height) = 0;
virtual uint32_t addRenderTarget(const std::string& name, gfx::Format format, uint32_t width, uint32_t height) = 0;
virtual uint32_t addDepthStencil(const std::string& name, gfx::Format format, uint32_t width, uint32_t height) = 0;
virtual void beginFrame(pipeline::PipelineSceneData* pplScene) = 0;
virtual void endFrame() = 0;
virtual RasterPassBuilder* addRasterPass(uint32_t width, uint32_t height, const std::string& layoutName, const std::string& name) = 0;
virtual RasterPassBuilder* addRasterPass(uint32_t width, uint32_t height, const std::string& layoutName) = 0;
virtual ComputePassBuilder* addComputePass(const std::string& layoutName, const std::string& name) = 0;
virtual ComputePassBuilder* addComputePass(const std::string& layoutName) = 0;
virtual MovePassBuilder* addMovePass(const std::string& name) = 0;
virtual CopyPassBuilder* addCopyPass(const std::string& name) = 0;
)");
        }

        NAMESPACE_END(render);
        NAMESPACE_END(cc);
    }
}

}
