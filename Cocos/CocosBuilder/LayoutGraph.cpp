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

void buildLayoutGraph(ModuleBuilder& builder, Features features) {
    MODULE(LayoutGraph,
        .mFolder = "cocos/renderer/pipeline/custom",
        .mFilePrefix = "LayoutGraph",
        .mTypescriptFolder = "cocos/core/pipeline/custom",
        .mTypescriptFilePrefix = "layout-graph",
        .mRequires = { "Gfx", "RenderCommon" },
    ) {
        NAMESPACE_BEG(cc);
        NAMESPACE_BEG(render);

        ENUM_CLASS(DescriptorIndex) {
            ENUMS(
                UNIFORM_BLOCK,
                SAMPLER_TEXTURE,
                SAMPLER,
                TEXTURE,
                STORAGE_BUFFER,
                STORAGE_TEXTURE,
                SUBPASS_INPUT
            );
        }

        STRUCT(UniformBlockDB) {
            PUBLIC(
                ((PmrTransparentMap<PmrString, gfx::Uniform>), mValues, _)
            );
        }

        STRUCT(Descriptor) {
            PUBLIC(
                (gfx::Type, mType, gfx::Type::UNKNOWN)
                (uint32_t, mCount, 1)
            );
            TS_INIT(mType, Type.UNKNOWN);
            CNTR(mType);
        }

        STRUCT(DescriptorBlock) {
            PUBLIC(
                ((PmrTransparentMap<PmrString, Descriptor>), mDescriptors, _)
                ((PmrTransparentMap<PmrString, UniformBlockDB>), mUniformBlocks, _)
                //(uint32_t, mCapacity, 0)
                //(uint32_t, mStart, 0)
                //(uint32_t, mCount, 0)
            );
        }
        
        STRUCT(DescriptorBlockIndex, .mFlags = LESS) {
            PUBLIC(
                (UpdateFrequency, mUpdateFrequency, _)
                (ParameterType, mParameterType, _)
                (DescriptorIndex, mDescriptorType, DescriptorIndex::UNIFORM_BLOCK)
                (gfx::ShaderStageFlagBit, mVisibility, gfx::ShaderStageFlagBit::NONE)
            );
            TS_INIT(mVisibility, ShaderStageFlagBit.NONE);
            TS_INIT(mDescriptorType, DescriptorIndex.UNIFORM_BLOCK);
            CNTR(mUpdateFrequency, mParameterType, mDescriptorType, mVisibility);
        }
        
        PROJECT_TS(
            (PmrMap<DescriptorBlockIndex, DescriptorBlock>),
            (Map<string, DescriptorBlock>)
        );

        STRUCT(DescriptorBlockIndexDx, .mFlags = LESS) {
            PUBLIC(
                (UpdateFrequency, mUpdateFrequency, _)
                (ParameterType, mParameterType, _)
                (gfx::ShaderStageFlagBit, mVisibility, gfx::ShaderStageFlagBit::NONE)
                (DescriptorIndex, mDescriptorType, DescriptorIndex::UNIFORM_BLOCK)
            );
            TS_INIT(mVisibility, ShaderStageFlagBit.NONE);
            TS_INIT(mDescriptorType, DescriptorIndex.UNIFORM_BLOCK);
            CNTR(mUpdateFrequency, mParameterType, mVisibility, mDescriptorType);
        }

        PROJECT_TS(
            (PmrMap<DescriptorBlockIndexDx, DescriptorBlock>),
            (Map<string, DescriptorBlock>)
        );

        STRUCT(DescriptorDB) {
            PUBLIC(
                ((PmrMap<DescriptorBlockIndex, DescriptorBlock>), mBlocks, _)
            );
        }

        //-----------------------------------------------------------
        // LayoutGraph
        TAGS((_), RenderStage_, RenderPhase_);

        STRUCT(RenderPhase) {
            PUBLIC(
                (PmrTransparentSet<PmrString>, mShaders, _)
            );
        }

        PMR_GRAPH(LayoutGraph, _, _) {
            NAMED_GRAPH(Name_);
            ALIAS_REFERENCE_GRAPH();
            ADDRESSABLE_GRAPH(mPathIndex);

            COMPONENT_GRAPH(
                (Name_, PmrString, mNames)
                (Descriptors_, DescriptorDB, mDescriptors)
            );

            POLYMORPHIC_GRAPH(
                (RenderStage_, uint32_t, mStages)
                (RenderPhase_, RenderPhase, mPhases)
            );
        }

        //-----------------------------------------------------------
        // Constant
        STRUCT(UniformData) {
            PUBLIC(
                (gfx::Type, mType, gfx::Type::UNKNOWN)
                (uint32_t, mValueID, 0xFFFFFFFF)
            );
            TS_INIT(mType, Type.UNKNOWN);
            CNTR(mType, mValueID);
        }

        STRUCT(UniformBlockData) {
            PUBLIC(
                (uint32_t, mSize, 0)
                (boost::container::pmr::vector<UniformData>, mValues, _)
            );
        }

        //-----------------------------------------------------------
        // Descriptor
        STRUCT(DescriptorData) {
            PUBLIC(
                (uint32_t, mID, 0xFFFFFFFF)
                (gfx::Type, mType, gfx::Type::UNKNOWN)
                (uint32_t, mCount, 1)
            );
            TS_INIT(mType, Type.UNKNOWN);
            CNTR(mID, mType);
        }

        STRUCT(DescriptorBlockData) {
            PUBLIC(
                (DescriptorIndex, mType, _)
                (uint32_t, mCapacity, 0)
                (boost::container::pmr::vector<DescriptorData>, mDescriptors, _)
            );
            CNTR(mType, mCapacity);
        }

        STRUCT(DescriptorTableData) {
            PUBLIC(
                (uint32_t, mSlot, 0xFFFFFFFF)
                (uint32_t, mCapacity, 0)
                (boost::container::pmr::vector<DescriptorBlockData>, mDescriptorBlocks, _)
                ((PmrFlatMap<uint32_t, UniformBlockData>), mUniformBlocks, _)
            );
            TS_INIT(mVisibility, ShaderStageFlagBit.NONE);
            CNTR(mSlot, mCapacity);
        }

        STRUCT(DescriptorSetData) {
            PUBLIC(
                ((PmrFlatMap<gfx::ShaderStageFlagBit, DescriptorTableData>), mTables, _)
            );
        }

        STRUCT(PipelineLayoutData) {
            PUBLIC(
                ((PmrFlatMap<UpdateFrequency, DescriptorSetData>), mDescriptorSets, _)
            );
        }

        //-----------------------------------------------------------
        // Shader Program
        STRUCT(ShaderProgramData) {
            PUBLIC(
                (PipelineLayoutData, mLayout, _)
            );
        }

        //-----------------------------------------------------------
        // Descriptor Layout Graph
        STRUCT(RenderPhaseData) {
            PUBLIC(
                (PmrString, mRootSignature, _)
                (boost::container::pmr::vector<ShaderProgramData>, mShaderPrograms, _)
                ((PmrTransparentMap<PmrString, uint32_t>), mShaderIndex, _)
            );
        }

        PMR_GRAPH(LayoutGraphData, _, _) {
            NAMED_GRAPH(Name_);
            ALIAS_REFERENCE_GRAPH();
            ADDRESSABLE_GRAPH(mPathIndex);

            COMPONENT_GRAPH(
                (Name_, PmrString, mNames)
                (Update_, UpdateFrequency, mUpdateFrequencies)
                (Layout_, PipelineLayoutData, mLayouts)
            );

            POLYMORPHIC_GRAPH(
                (RenderStage_, uint32_t, mStages)
                (RenderPhase_, RenderPhaseData, mPhases)
            );
        }
        NAMESPACE_END(render);
        NAMESPACE_END(cc);
    }
}

}
