#pragma once
#include <Cocos/AST/BuilderTypes.h>

namespace Cocos::Meta {

void buildRenderGraph(ModuleBuilder& builder);
void buildRenderExecutor(ModuleBuilder& builder);

}
