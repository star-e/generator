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
)",
        .mTypescriptInclude = R"(import { ccclass } from '../../core/data/decorators';
import { OutputArchive } from './archive';
)"
    ) {
        NAMESPACE_BEG(cc);

        NAMESPACE_BEG(render);

        // See native/cocos/renderer/gfx-validator/DescriptorSetLayoutValidator.cpp
        //ENUM_CLASS(DescriptorTypeOrder) {
        //    ENUMS(
        //        UNIFORM_BLOCK,
        //        SAMPLER_TEXTURE,
        //        SAMPLER,
        //        TEXTURE,
        //        STORAGE_BUFFER,
        //        STORAGE_TEXTURE,
        //        SUBPASS_INPUT
        //    );
        //}
        ENUM_CLASS(DescriptorTypeOrder) {
            ENUMS(
                UNIFORM_BUFFER,
                DYNAMIC_UNIFORM_BUFFER,
                SAMPLER_TEXTURE,
                SAMPLER,
                TEXTURE,
                STORAGE_BUFFER,
                DYNAMIC_STORAGE_BUFFER,
                STORAGE_IMAGE,
                INPUT_ATTACHMENT
            );
        }

        STRUCT(Descriptor, .mFlags = JSB) {
            PUBLIC(
                (gfx::Type, mType, gfx::Type::UNKNOWN)
                (uint32_t, mCount, 1)
            );
            TS_INIT(mType, Type.UNKNOWN);
            CNTR(mType);
        }

        STRUCT(DescriptorBlock) {
            PUBLIC(
                ((ccstd::map<ccstd::string, Descriptor>), mDescriptors, _)
                ((ccstd::map<ccstd::string, gfx::UniformBlock>), mUniformBlocks, _)
                //((ccstd::map<gfx::Type, Descriptor>), mMerged, _)
                (uint32_t, mCapacity, 0)
                (uint32_t, mCount, 0)
            );
        }

        STRUCT(DescriptorBlockFlattened, .mFlags = JSB | DECORATOR) {
            PUBLIC(
                (ccstd::vector<ccstd::string>, mDescriptorNames, _)
                (ccstd::vector<ccstd::string>, mUniformBlockNames, _)
                (ccstd::vector<Descriptor>, mDescriptors, _)
                (ccstd::vector<gfx::UniformBlock>, mUniformBlocks, _)
                (uint32_t, mCapacity, 0)
                (uint32_t, mCount, 0)
            );
        }

        STRUCT(DescriptorBlockIndex, .mFlags = LESS | JSB) {
            PUBLIC(
                (UpdateFrequency, mUpdateFrequency, _)
                (ParameterType, mParameterType, _)
                (DescriptorTypeOrder, mDescriptorType, DescriptorTypeOrder::UNIFORM_BUFFER)
                (gfx::ShaderStageFlagBit, mVisibility, gfx::ShaderStageFlagBit::NONE)
            );
            TS_INIT(mVisibility, ShaderStageFlagBit.NONE);
            TS_INIT(mDescriptorType, DescriptorTypeOrder.UNIFORM_BUFFER);
            CNTR(mUpdateFrequency, mParameterType, mDescriptorType, mVisibility);
        }
        
        PROJECT_TS(
            (ccstd::pmr::map<DescriptorBlockIndex, DescriptorBlock>),
            (Map<string, DescriptorBlock>)
        );

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

        PMR_GRAPH(LayoutGraph, _, _) {
            NAMED_GRAPH(Name_);
            ALIAS_REFERENCE_GRAPH();
            ADDRESSABLE_GRAPH(mPathIndex);

            COMPONENT_GRAPH(
                (Name_, ccstd::pmr::string, mNames)
                (Descriptors_, DescriptorDB, mDescriptors)
            );

            POLYMORPHIC_GRAPH(
                (RenderStage_, uint32_t, mStages)
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
        STRUCT(NameLocalID, .mTrivial = true, .mFlags = HASH_COMBINE | EQUAL) {
            PUBLIC(
                (uint32_t, mValue, 0xFFFFFFFF)
            );
        }
        TS_PROJECT(NameLocalID, number);

        STRUCT(DescriptorData) {
            PUBLIC(
                (NameLocalID, mDescriptorID, _)
                (uint32_t, mCount, 1)
            );
            CNTR(mDescriptorID);
            CNTR(mDescriptorID, mCount);
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
                (ccstd::pmr::vector<DescriptorBlockData>, mDescriptorBlocks, _)
                ((ccstd::pmr::unordered_map<NameLocalID, gfx::UniformBlock>), mUniformBlocks, _)
            );
            TS_INIT(mVisibility, ShaderStageFlagBit.NONE);
            CNTR(mSlot, mCapacity);
        }

        STRUCT(DescriptorSetData, .mFlags = NO_COPY) {
            PUBLIC(
                (DescriptorSetLayoutData, mDescriptorSetLayoutData, _)
                (gfx::DescriptorSetLayoutInfo, mDescriptorSetLayoutInfo, _)
                ([[optional]] IntrusivePtr<gfx::DescriptorSetLayout>, mDescriptorSetLayout, _)
                ([[optional]] IntrusivePtr<gfx::DescriptorSet>, mDescriptorSet, _)
            );
            CNTR_NO_DEFAULT(mDescriptorSetLayoutData, mDescriptorSetLayout, mDescriptorSet);
        }

        STRUCT(PipelineLayoutData, .mFlags = NO_COPY) {
            PUBLIC(
                ((ccstd::pmr::map<UpdateFrequency, DescriptorSetData>), mDescriptorSets, _)
            );
        }

        //-----------------------------------------------------------
        // Shader Program
        STRUCT(ShaderProgramData, .mFlags = NO_COPY) {
            PUBLIC(
                (PipelineLayoutData, mLayout, _)
            );
        }

        //-----------------------------------------------------------
        // Descriptor Layout Graph
        STRUCT(RenderStageData, .mFlags = NO_COPY) {
            PUBLIC(
                ((ccstd::pmr::unordered_map<NameLocalID, gfx::ShaderStageFlagBit>), mDescriptorVisibility, _)
            );
        }

        STRUCT(RenderPhaseData, .mFlags = NO_COPY) {
            PUBLIC(
                (ccstd::pmr::string, mRootSignature, _)
                (ccstd::pmr::vector<ShaderProgramData>, mShaderPrograms, _)
                ((PmrTransparentMap<ccstd::pmr::string, uint32_t>), mShaderIndex, _)
            );
        }

        PMR_GRAPH(LayoutGraphData, _, _, .mFlags = NO_COPY | DECORATOR) {
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
            );
        }
        NAMESPACE_END(render);
        NAMESPACE_END(cc);
    }
}

}
