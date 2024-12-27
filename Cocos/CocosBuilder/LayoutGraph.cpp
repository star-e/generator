/*
Copyright (c) 2013-2016 Chukong Technologies Inc.
Copyright (c) 2017-2022 Xiamen Yaji Software Co., Ltd.

http://www.cocos.com

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated engine source code (the "Software"), a limited,
worldwide, royalty-free, non-assignable, revocable and non-exclusive license
to use Cocos Creator solely to develop games on your target platforms. You shall
not use Cocos Creator software for developing other software or tools that's
used for developing games. You are not granted to publish, distribute,
sublicense, and/or sell copies of Cocos Creator.

The software or tools in this License Agreement are licensed, not sold.
Xiamen Yaji Software Co., Ltd. reserves all rights not expressly granted to you.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include "CocosModules.h"
#include <Cocos/AST/DSL.h>
#include <Cocos/AST/SyntaxGraphs.h>

namespace Cocos::Meta {

// clang-format off
    
void buildLayoutGraph(ModuleBuilder& builder, Features features) {
    MODULE(LayoutGraph,
        .mFolder = "cocos/renderer/pipeline/custom",
        .mFilePrefix = "LayoutGraph",
        .mJsbHeaders = R"(#include "cocos/bindings/auto/jsb_gfx_auto.h"
)",
        .mTypescriptFolder = "cocos/rendering/custom",
        .mTypescriptFilePrefix = "layout-graph",
        .mRequires = { "Gfx", "RenderCommon" },
        .mHeader = R"(#include "cocos/renderer/gfx-base/GFXDescriptorSet.h"
#include "cocos/renderer/gfx-base/GFXDescriptorSetLayout.h"
#include "cocos/renderer/gfx-base/GFXPipelineLayout.h"
)",
        .mTypescriptInclude = R"(import type { OutputArchive, InputArchive } from './archive';
import { saveUniformBlock, loadUniformBlock, saveDescriptorSetLayoutInfo, loadDescriptorSetLayoutInfo } from './serialization';

function resetDescriptorSetLayoutInfo (info: DescriptorSetLayoutInfo): void {
    info.bindings.length = 0;
}
)"
// import { equalObjectArray, equalValueArray, equalObjectMap, equalValueMap } from './utils';
    ) {
        NAMESPACE_BEG(cc);

        NAMESPACE_BEG(render);

        if (false) {
            STRUCT(DescriptorBlockIndexDx, .mFlags = LESS) {
                PUBLIC(
                    (UpdateFrequency, mUpdateFrequency, _)
                    (ParameterType, mParameterType, _)
                    (gfx::ShaderStageFlagBit, mVisibility, gfx::ShaderStageFlagBit::NONE)
                    (DescriptorTypeOrder, mDescriptorType, DescriptorTypeOrder::UNIFORM_BUFFER)
                );
                TS_INIT(mVisibility, ShaderStageFlagBit.NONE);
                TS_INIT(mDescriptorType, DescriptorTypeOrder.UNIFORM_BUFFER);
                CNTR(mUpdateFrequency, mParameterType, mVisibility, mDescriptorType);
            }

            PROJECT_TS(
                (ccstd::pmr::map<DescriptorBlockIndexDx, DescriptorBlock>),
                (Map<string, DescriptorBlock>)
            );
        }

        STRUCT(DescriptorDB) {
            PUBLIC(
                ((ccstd::pmr::map<DescriptorBlockIndex, DescriptorBlock>), mBlocks, _)
                ((ccstd::pmr::map<DescriptorGroupBlockIndex, DescriptorGroupBlock>), mGroupBlocks, _)
            );
        }

        //-----------------------------------------------------------
        // LayoutGraph
        TAGS((_), RenderStage_, RenderPhase_);

        STRUCT(RenderPhase) {
            PUBLIC(
                (PmrTransparentSet<ccstd::pmr::string>, mShaders, _)
            );
        }

        ENUM_CLASS(RenderPassType) {
            UNDERLYING_TYPE(uint32_t);
            ENUMS(SINGLE_RENDER_PASS, RENDER_PASS, RENDER_SUBPASS);
        }

        PMR_GRAPH(LayoutGraph, _, _) {
            NAMED_GRAPH(Name_);
            ALIAS_REFERENCE_GRAPH();
            ADDRESSABLE_GRAPH(mPathIndex);

            COMPONENT_GRAPH(
                (Name_, ccstd::pmr::string, mNames)
                (Descriptors_, DescriptorDB, mDescriptors)
            );

            POLYMORPHIC_GRAPH(
                (RenderStage_, RenderPassType, mStages)
                (RenderPhase_, RenderPhase, mPhases)
            );
        }

        //-----------------------------------------------------------
        // Constant
        ALIAS(UniformID, uint32_t);
        PROJECT_TS(UniformID, number);

        STRUCT(UniformData) {
            PUBLIC(
                (UniformID, mUniformID, 0xFFFFFFFF)
                (gfx::Type, mUniformType, gfx::Type::UNKNOWN)
                (uint32_t, mOffset, 0)
                (uint32_t, mSize, 0)
            );
            TS_INIT(mUniformType, Type.UNKNOWN);
            CNTR(mUniformID, mUniformType, mOffset);
        }

        STRUCT(UniformBlockData) {
            PUBLIC(
                (uint32_t, mBufferSize, 0)
                (ccstd::pmr::vector<UniformData>, mUniforms, _)
            );
        }

        //-----------------------------------------------------------
        // Descriptor
        STRUCT(NameLocalID, .mTrivial = true, .mFlags = HASH_COMBINE | EQUAL | LESS) {
            PUBLIC(
                (uint32_t, mValue, 0xFFFFFFFF)
            );
        }
        TS_PROJECT(NameLocalID, number);

        STRUCT(DescriptorData) {
            PUBLIC(
                (NameLocalID, mDescriptorID, _)
                (gfx::Type, mType, gfx::Type::UNKNOWN)
                (uint32_t, mCount, 1)
            );
            TS_INIT(mType, Type.UNKNOWN);
            CNTR(mDescriptorID, mType, mCount);
            CNTR(mDescriptorID, mType);
            CNTR(mDescriptorID);
        }

        STRUCT(DescriptorBlockData) {
            PUBLIC(
                (DescriptorTypeOrder, mType, DescriptorTypeOrder::UNIFORM_BUFFER)
                (gfx::ShaderStageFlagBit, mVisibility, gfx::ShaderStageFlagBit::NONE)
                (uint32_t, mOffset, 0)
                (uint32_t, mCapacity, 0)
                (ccstd::pmr::vector<DescriptorData>, mDescriptors, _)
            );
            TS_INIT(mType, DescriptorTypeOrder.UNIFORM_BUFFER);
            TS_INIT(mVisibility, ShaderStageFlagBit.NONE);
            CNTR(mType, mVisibility, mCapacity);
        }

        STRUCT(DescriptorSetLayoutData, .mFlags = NO_COPY) {
            PUBLIC(
                (uint32_t, mSlot, 0xFFFFFFFF)
                (uint32_t, mCapacity, 0)
                (uint32_t, mUniformBlockCapacity, 0)
                (uint32_t, mSamplerTextureCapacity, 0)
                (ccstd::pmr::vector<DescriptorBlockData>, mDescriptorBlocks, _)
                ((PmrUnorderedMap<NameLocalID, gfx::UniformBlock>), mUniformBlocks, _)
                ((PmrFlatMap<NameLocalID, uint32_t>), mBindingMap, _)
            );
            TS_INIT(mVisibility, ShaderStageFlagBit.NONE);
            CNTR(mSlot, mCapacity, mDescriptorBlocks, mUniformBlocks, mBindingMap);
        }

        STRUCT(DescriptorSetData, .mFlags = NO_COPY) {
            PUBLIC(
                (DescriptorSetLayoutData, mDescriptorSetLayoutData, _)
                (gfx::DescriptorSetLayoutInfo, mDescriptorSetLayoutInfo, _)
                ([[nullable]] IntrusivePtr<gfx::DescriptorSetLayout>, mDescriptorSetLayout, _)
                ([[nullable]] IntrusivePtr<gfx::DescriptorSet>, mDescriptorSet, _)
            );
            CNTR(mDescriptorSetLayoutData, mDescriptorSetLayout, mDescriptorSet);
        }

        STRUCT(DescriptorGroupBlockData) {
            PUBLIC(
                (DescriptorTypeOrder, mType, DescriptorTypeOrder::UNIFORM_BUFFER)
                (gfx::ShaderStageFlagBit, mVisibility, gfx::ShaderStageFlagBit::NONE)
                (AccessType, mAccessType, AccessType::READ)
                (ViewDimension, mViewDimension, ViewDimension::TEX2D)
                (gfx::Format, mFormat, gfx::Format::UNKNOWN)
                (uint32_t, mOffset, 0)
                (uint32_t, mCapacity, 0)
                (ccstd::pmr::vector<DescriptorData>, mDescriptors, _)
            );
            TS_INIT(mType, DescriptorTypeOrder.UNIFORM_BUFFER);
            TS_INIT(mVisibility, ShaderStageFlagBit.NONE);
            TS_INIT(mFormat, Format.UNKNOWN);
            CNTR(mType, mVisibility, mAccessType, mViewDimension, mFormat, mCapacity);
        }

        STRUCT(DescriptorGroupLayoutData, .mFlags = NO_COPY) {
            PUBLIC(
                (uint32_t, mSlot, 0xFFFFFFFF)
                (uint32_t, mCapacity, 0)
                (uint32_t, mUniformBlockCapacity, 0)
                (uint32_t, mSamplerTextureCapacity, 0)
                (ccstd::pmr::vector<DescriptorGroupBlockData>, mDescriptorGroupBlocks, _)
                ((PmrUnorderedMap<NameLocalID, gfx::UniformBlock>), mUniformBlocks, _)
                ((PmrFlatMap<NameLocalID, uint32_t>), mBindingMap, _)
            );
            TS_INIT(mVisibility, ShaderStageFlagBit.NONE);
            CNTR(mSlot, mCapacity, mDescriptorGroupBlocks, mUniformBlocks, mBindingMap);
        }

        STRUCT(DescriptorGroupData, .mFlags = NO_COPY) {
            PUBLIC(
                (DescriptorGroupLayoutData, mDescriptorGroupLayoutData, _)
            );
            CNTR(mDescriptorGroupLayoutData);
        }

        STRUCT(PipelineLayoutData, .mFlags = NO_COPY) {
            PUBLIC(
                ((ccstd::pmr::map<UpdateFrequency, DescriptorSetData>), mDescriptorSets, _)
                ((ccstd::pmr::map<UpdateFrequency, DescriptorGroupData>), mDescriptorGroups, _)
            );
        }

        STRUCT(ShaderBindingData, .mFlags = NO_COPY) {
            PUBLIC(
                ((PmrFlatMap<NameLocalID, uint32_t>), mDescriptorBindings, _)
            );
        }

        STRUCT(ShaderLayoutData, .mFlags = NO_COPY) {
            PUBLIC(
                ((ccstd::pmr::map<UpdateFrequency, DescriptorSetLayoutData>), mLayoutData, _)
                ((ccstd::pmr::map<UpdateFrequency, ShaderBindingData>), mBindingData, _)
            );
        }

        STRUCT(TechniqueData, .mFlags = NO_COPY) {
            PUBLIC(
                ((ccstd::pmr::vector<ShaderLayoutData>), mPasses, _)
            );
        }

        STRUCT(EffectData, .mFlags = NO_COPY) {
            PUBLIC(
                ((ccstd::pmr::map<ccstd::pmr::string, TechniqueData>), mTechniques, _)
            );
        }

        //-----------------------------------------------------------
        // Shader Program
        STRUCT(ShaderProgramData, .mFlags = NO_COPY) {
            PUBLIC(
                (PipelineLayoutData, mLayout, _)
                ([[nullable]] IntrusivePtr<gfx::PipelineLayout>, mPipelineLayout, _)
            );
        }

        //-----------------------------------------------------------
        // Descriptor Layout Graph
        STRUCT(RenderStageData, .mFlags = NO_COPY) {
            PUBLIC(
                ((PmrUnorderedMap<NameLocalID, gfx::ShaderStageFlagBit>), mDescriptorVisibility, _)
            );
        }

        STRUCT(RenderPhaseData, .mFlags = NO_COPY) {
            PUBLIC(
                (ccstd::pmr::string, mRootSignature, _)
                (ccstd::pmr::vector<ShaderProgramData>, mShaderPrograms, _)
                ((PmrTransparentMap<ccstd::pmr::string, uint32_t>), mShaderIndex, _)
                ([[nullable]] IntrusivePtr<gfx::PipelineLayout>, mPipelineLayout, _)
            );
        }

        PMR_GRAPH(LayoutGraphData, _, _, .mFlags = NO_COPY) {
            NAMED_GRAPH(Name_);
            ALIAS_REFERENCE_GRAPH();
            ADDRESSABLE_GRAPH(mPathIndex);

            COMPONENT_GRAPH(
                (Name_, ccstd::pmr::string, mNames)
                (Update_, UpdateFrequency, mUpdateFrequencies)
                (Layout_, PipelineLayoutData, mLayouts)
            );

            POLYMORPHIC_GRAPH(
                (RenderStage_, RenderStageData, mStages)
                (RenderPhase_, RenderPhaseData, mPhases)
            );
            PUBLIC(
                (ccstd::pmr::vector<ccstd::pmr::string>, mValueNames, _)
                ((PmrFlatMap<ccstd::pmr::string, NameLocalID>), mAttributeIndex, _)
                ((PmrFlatMap<ccstd::pmr::string, NameLocalID>), mConstantIndex, _)
                ((PmrFlatMap<ccstd::pmr::string, uint32_t>), mShaderLayoutIndex, _)
                ((PmrFlatMap<ccstd::pmr::string, EffectData>), mEffects, _)
            );
            PUBLIC(
                (ccstd::string, mConstantMacros, _)
            );
            MEMBER_FLAGS(mConstantMacros, NO_SERIALIZATION);
        }
        NAMESPACE_END(render);
        NAMESPACE_END(cc);
    }
}

}
