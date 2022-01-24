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
        .mFolder = "cocos/renderer/pipeline",
        .mFilePrefix = "LayoutGraph",
        .mTypescriptFolder = "cocos/core/pipeline",
        .mTypescriptFilePrefix = "layout-graph",
        .mRequires = { "RenderCommon" },
    ) {
        NAMESPACE(cc) {
            NAMESPACE(render) {
                //-----------------------------------------------------------
                // Constant
                STRUCT(Constant) {
                    PUBLIC(
                        (ValueType, mType, _)
                        (uint32_t, mValueID, 0xFFFFFFFF)
                    );
                }

                STRUCT(ConstantBuffer) {
                    PUBLIC(
                        (uint32_t, mSize, 0)
                        (boost::container::pmr::vector<Constant>, mConstants, _)
                    );
                }

                //-----------------------------------------------------------
                // Descriptor
                VARIANT(DescriptorType, (CBuffer_, RWBuffer_, RWTexture_, Buffer_, Texture_, Sampler_), LESS | EQUAL);

                STRUCT(DescriptorBlock) {
                    PUBLIC(
                        (DescriptorType, mType, _)
                        (uint32_t, mCapacity, 0)
                        (boost::container::pmr::vector<uint32_t>, mAttributeIDs, _)
                    );
                }

                STRUCT(DescriptorArray) {
                    PUBLIC(
                        (uint32_t, mCapacity, 0)
                        (uint32_t, mAttributeID, 0xFFFFFFFF)
                    );
                }

                STRUCT(UnboundedDescriptor) {
                    PUBLIC(
                        (DescriptorType, mType, _)
                        (boost::container::pmr::vector<DescriptorArray>, mDescriptors, _)
                    );
                }

                STRUCT(DescriptorTable) {
                    PUBLIC(
                        (uint32_t, mSlot, 0)
                        (uint32_t, mCapacity, 0)
                        (boost::container::pmr::vector<DescriptorBlock>, mBlocks, _)
                    );
                }

                STRUCT(DescriptorSet){
                    PUBLIC(
                        (boost::container::pmr::vector<DescriptorTable>, mTables, _)
                        (UnboundedDescriptor, mUnbounded, _)
                    );
                }

                STRUCT(LayoutData) {
                    PUBLIC(
                        ((PmrMap<ParameterType, ConstantBuffer>), mConstantBuffers, _)
                        ((PmrMap<ParameterType, DescriptorSet>), mDescriptorSets, _)
                    );
                }
                //-----------------------------------------------------------
                // Shader Program
                STRUCT(ShaderProgramData) {
                    PUBLIC(
                        ((PmrMap<UpdateFrequency, LayoutData>), mLayouts, _)
                    );
                }

                //-----------------------------------------------------------
                // Descriptor Layout Graph
                STRUCT(GroupNodeData) {
                    PUBLIC(
                        (NodeType, mNodeType, _)
                    );
                    CNTR(mNodeType);
                }

                STRUCT(ShaderNodeData) {
                    PUBLIC(
                        (std::pmr::string, mRootSignature, _)
                        (boost::container::pmr::vector<ShaderProgramData>, mShaderPrograms, _)
                        ((PmrMap<std::pmr::string, uint32_t>), mShaderIndex, _)
                    );
                }

                TAGS((_), Group_, Shader_);

                PMR_GRAPH(LayoutGraph, _, _) {
                    NAMED_GRAPH(Name_);
                    ALIAS_REFERENCE_GRAPH();
                    ADDRESSABLE_GRAPH(mPathIndex);

                    COMPONENT_GRAPH(
                        (Name_, std::pmr::string, mName)
                        (Update_, UpdateFrequency, mUpdateFrequencies)
                        (Layout_, LayoutData, mLayouts)
                    );

                    POLYMORPHIC_GRAPH(
                        (Group_, GroupNodeData, mGroupNodes)
                        (Shader_, ShaderNodeData, mShaderNodes)
                    );
                }
            }
        }
    }
}

}
