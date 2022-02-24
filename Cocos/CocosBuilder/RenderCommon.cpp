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

void buildRenderCommon(ModuleBuilder& builder, Features features) {
    MODULE(RenderCommon,
        .mFolder = "cocos/renderer/pipeline/custom",
        .mFilePrefix = "RenderCommon",
        .mTypescriptFolder = "cocos/core/pipeline/custom",
        .mTypescriptFilePrefix = "types",
    ) {
        NAMESPACE_BEG(cc);
        NAMESPACE_BEG(render);
        
        ENUM_CLASS(UpdateFrequency) {
            ENUMS(PER_INSTANCE, PER_BATCH, PER_QUEUE, PER_PASS, COUNT);
        }

        ENUM_CLASS(ParameterType) {
            ENUMS(CONSTANTS, CBV, UAV, SRV, TABLE, SSV);
        }

        TAGS((_), Constants_);
        //TAGS((_), CBV_, UAV_, SRV_, SSV_, RTV_, DSV_, IBV_, VBV_, SOV_, Constants_, Table_);
        //VARIANT(ParameterType, (Constants_, CBV_, UAV_, SRV_, Table_, SSV_), EQUAL | LESS);

        TAGS((_), Bounded_, Unbounded_);
        VARIANT(Boundedness, (Bounded_, Unbounded_), EQUAL | LESS);

        TAGS((_), CBuffer_, Buffer_, Texture_, RWBuffer_, RWTexture_, Sampler_,
            Texture1D_, Texture1DArray_,
            Texture2D_, Texture2DArray_, Texture2DMS_, Texture2DMSArray_,
            Texture3D_, TextureCube_, TextureCubeArray_,
            RaytracingAccelerationStructure_,
            SamplerState_, SamplerComparisonState_);

        VARIANT(ResourceType, (Constants_, Buffer_, Texture1D_, Texture1DArray_, Texture2D_, Texture2DArray_, Texture2DMS_, Texture2DMSArray_, Texture3D_, TextureCube_, TextureCubeArray_, RaytracingAccelerationStructure_, SamplerState_, SamplerComparisonState_));

        TAGS((_), Typeless_,
            Float4_, Float3_, Float2_, Float1_,
            Half4_, Half3_, Half2_, Half1_,
            Fixed4_, Fixed3_, Fixed2_, Fixed1_,
            Uint4_, Uint3_, Uint2_, Uint1_,
            Int4_, Int3_, Int2_, Int1_,
            Bool4_, Bool3_, Bool2_, Bool1_);

        VARIANT(ValueType, (Typeless_, Float4_, Float3_, Float2_, Float1_, Half4_, Half3_, Half2_, Half1_, Fixed4_, Fixed3_, Fixed2_, Fixed1_, Uint4_, Uint3_, Uint2_, Uint1_, Int4_, Int3_, Int2_, Int1_, Bool4_, Bool3_, Bool2_, Bool1_));

        TAGS((_), Raster_, Compute_, Copy_, Move_, Raytrace_);

        TAGS((_), Managed_, Memoryless_, Persistent_, External_, Backbuffer_);
        VARIANT(ResourceResidency, (Managed_, Memoryless_, Persistent_, External_, Backbuffer_));

        ENUM_CLASS(QueueHint) {
            ENUMS(NONE, RENDER_OPAQUE, RENDER_CUTOUT, RENDER_TRANSPARENT, COUNT);
        }
        ENUM_CLASS(ResourceDimension) {
            ENUMS(BUFFER, TEXTURE1D, TEXTURE2D, TEXTURE3D);
        }

        STRUCT(SampleDesc) {
            PUBLIC(
                (uint32_t, mCount, 1)
                (uint32_t, mQuality, 0)
            );
        }

        ENUM_CLASS(NodeType) {
            ENUMS(INTERNAL, LEAF);
        }
        NAMESPACE_END(render);
        NAMESPACE_END(cc);
    } // module CoreRender
}

}
