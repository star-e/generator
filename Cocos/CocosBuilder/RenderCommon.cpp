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

        TAGS((_), Raster_, Compute_, Copy_, Move_, Raytrace_);

        ENUM_CLASS(ResourceResidency) {
            ENUMS(MANAGED, MEMORYLESS, PERSISTENT, EXTERNAL, BACKBUFFER);
        }

        ENUM_CLASS(QueueHint) {
            ENUMS(NONE, RENDER_OPAQUE, RENDER_CUTOUT, RENDER_TRANSPARENT);
        }

        ENUM_CLASS(ResourceDimension) {
            ENUMS(BUFFER, TEXTURE1D, TEXTURE2D, TEXTURE3D);
        }
        
        FLAG_CLASS(ResourceFlags) {
            FLAGS(
                (NONE, 0)
                (UNIFORM, 0x1)
                (INDIRECT, 0x2)
                (STORAGE, 0x4)
                (SAMPLED, 0x8)
                (COLOR_ATTACHMENT, 0x10)
                (DEPTH_STENCIL_ATTACHMENT, 0x20)
                (INPUT_ATTACHMENT, 0x40)
            );
        }

        TAGS((_), Buffer_, Texture_);

        NAMESPACE_END(render);
        NAMESPACE_END(cc);
    } // module CoreRender
}

}
