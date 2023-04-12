#pragma once
#include <Cocos/AST/BuilderTypes.h>

namespace Cocos::Meta {

void buildCocosCommon(ModuleBuilder& builder, Features features);

void buildLayoutGraph(ModuleBuilder& builder, Features features);
void buildRenderCommon(ModuleBuilder& builder, Features features);
void buildRenderGraph(ModuleBuilder& builder, Features features);
void buildCustomization(ModuleBuilder& builder, Features features);

void buildArchiveInterface(ModuleBuilder& builder, Features features);
void buildRenderInterface(ModuleBuilder& builder, Features features);
void buildPrivateInterface(ModuleBuilder& builder, Features features);
void buildRenderCompiler(ModuleBuilder& builder, Features features);

// Private
void buildPrivateTypes(ModuleBuilder& builder, Features features);

// Native
void buildNativePipeline(ModuleBuilder& builder, Features features);

void buildFGDispatcher(ModuleBuilder& builder, Features features);

void buildExecutorExample(ModuleBuilder& builder, Features features);
void buildRenderExample(ModuleBuilder& builder, Features features);

}
