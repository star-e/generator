#include "CocosModules.h"
#include <Cocos/AST/DSL.h>
#include <Cocos/AST/SyntaxGraphs.h>

// clang-format off

namespace Cocos::Meta {
    
void buildRenderInterfaceDocument(ModuleBuilder& builder) {
    NAMESPACE_BEG(cc);
    NAMESPACE_BEG(render);

    //COMMENT(PipelineRuntime) {
    //    
    //}

    NAMESPACE_END(render);
    NAMESPACE_END(cc);
}

}
