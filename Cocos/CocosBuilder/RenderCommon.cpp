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
        NAMESPACE(cc) {
            NAMESPACE(render) {
                ENUM_CLASS(UpdateFrequency) {
                    ENUMS(PerInstance, PerBatch, PerQueue, PerPass, Count);
                }

                TAGS((_), CBV_, UAV_, SRV_, SSV_, RTV_, DSV_, IBV_, VBV_, SOV_, Constants_, Table_);
                VARIANT(ParameterType, (Constants_, CBV_, UAV_, SRV_, Table_, SSV_), EQUAL | LESS);

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

                TAGS((_), Managed_, Persistent_, Backbuffer_, Memoryless_);
                VARIANT(ResourceResidency, (Managed_, Persistent_, Backbuffer_, Memoryless_));

                ENUM_CLASS(QueueHint) {
                    ENUMS(Opaque, Cutout, Transparent, Count);
                }
                ENUM_CLASS(ResourceDimension) {
                    ENUMS(Buffer, Texture1D, Texture2D, Texture3D);
                }

                STRUCT(SampleDesc) {
                    PUBLIC(
                        (uint32_t, mCount, 1)
                        (uint32_t, mQuality, 0)
                    );
                }

                ENUM_CLASS(NodeType) {
                    ENUMS(Internal, Leaf);
                }
            } // namespace render
        }
    } // module CoreRender
}

void buildRenderInterface(ModuleBuilder& builder, Features features) {
    MODULE(RenderInterface,
        .mFolder = "cocos/renderer/pipeline/custom",
        .mFilePrefix = "RenderInterface",
        .mToJsFilename = "render.ini",
        .mToJsPrefix = "render",
        .mToJsNamespace = "render",
        .mToJsCppHeaders = " cocos/bindings/auto/jsb_gfx_auto.h",
        .mTypescriptFolder = "cocos/core/pipeline/custom",
        .mTypescriptFilePrefix = "pipeline",
        .mRequires = { "Gfx", "RenderCommon", "RenderGraph" },
        .mHeader = R"(#include "renderer/gfx-base/GFXDef-common.h"

namespace cc {

class Mat4;
class Mat4;
class Quaternion;
class Color;
class Vec4;
class Vec3;
class Vec2;
class Size;
class Rect;

} // namespace cc
)",
    ) {
        NAMESPACE_BEG(cc);
        NAMESPACE_BEG(render);

        INTERFACE(Setter) {
            MEMBER_FUNCTIONS(R"(
virtual void setMat4(const std::string& name, const cc::Mat4& mat) = 0;
virtual void setCBuffer(const std::string& name, gfx::Buffer* buffer) = 0;
)");
        }

        INTERFACE(RasterPass) {
            MEMBER_FUNCTIONS(R"(
virtual void addRasterView(const std::string& name, const RasterView& view) = 0;
)");
        }

        INTERFACE(Pipeline) {
            MEMBER_FUNCTIONS(R"(
virtual uint32_t addRenderTexture(const std::string& name, gfx::Format format, uint32_t width, uint32_t height) = 0;
)");
        }

        NAMESPACE_END(render);
        NAMESPACE_END(cc);
    }
}

}
