#include "CocosModules.h"
#include <Cocos/AST/DSL.h>
#include <Cocos/AST/SyntaxGraphs.h>

// clang-format off

namespace Cocos::Meta {
    
void buildRenderInterfaceDocument(ModuleBuilder& builder) {
    NAMESPACE_BEG(cc);
    NAMESPACE_BEG(render);

    COMMENT(PipelineRuntime, R"(@en PipelineRuntime is the runtime of both classical and custom pipelines.
It is used internally and should not be called directly.
@zh PipelineRuntime是经典管线以及自定义管线的运行时。
属于内部实现，用户不应直接调用。
)") {
        
    }

    NAMESPACE_END(render);
    NAMESPACE_END(cc);
}

}
