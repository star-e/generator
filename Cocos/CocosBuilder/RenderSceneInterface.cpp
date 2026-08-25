#include "CocosModules.h"
#include <Cocos/AST/DSL.h>
#include <Cocos/AST/SyntaxGraphs.h>

// clang-format off

namespace Cocos::Meta {

void buildRenderSceneInterface(ModuleBuilder& builder, Features features) {
    MODULE(RenderSceneInterface,
        .mFolder = "cocos/renderer/scene",
        .mFilePrefix = "RenderSceneInterface",
        .mToJsFilename = "render-scene.i",
        .mToJsPrefix = "scene",
        .mToJsNamespace = "render",
        .mToJsCppHeaders = R"(#include "bindings/auto/jsb_render_scene_auto.h"
)",
        .mToJsUsingNamespace = R"(
using namespace cc;
using namespace cc::render;
)",
        .mToJsConfigs = R"()",
        .mTypescriptFolder = "cocos/rendering/scene",
        .mTypescriptFilePrefix = "render-scene",
        .mRequires = {},
        .mHeader = R"(#include "cocos/base/RefCounted.h"
)"
    ) {
        NAMESPACE_BEG(cc);
        NAMESPACE_BEG(render);

        INTERFACE(RenderScene) {
            INHERITS(RefCounted);
            PUBLIC_METHODS(R"(
virtual bool empty() const = 0;

[[getter]] virtual uint32_t getNumNodes() const = 0;
)");
        }

        NAMESPACE_END(render);
        NAMESPACE_END(cc);
    }
}

}
