#pragma once
#include <Cocos/AST/BuilderTypes.h>

namespace Cocos::Meta {

void buildLayoutGraph(ModuleBuilder& builder, Features features);
void buildRenderCommon(ModuleBuilder& builder, Features features);
void buildRenderGraph(ModuleBuilder& builder, Features features);
void buildRenderCompiler(ModuleBuilder& builder, Features features);
void buildRenderExecutor(ModuleBuilder& builder, Features features);

void buildRenderExample(ModuleBuilder& builder, Features features);

}
