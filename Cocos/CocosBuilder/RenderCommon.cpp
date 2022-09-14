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

#ifdef OPAQUE
#undef OPAQUE
#endif

#ifdef TRANSPARENT
#undef TRANSPARENT
#endif

// clang-format off

void buildRenderCommon(ModuleBuilder& builder, Features features) {
    MODULE(RenderCommon,
        .mFolder = "cocos/renderer/pipeline/custom",
        .mFilePrefix = "RenderCommon",
        .mJsbHeaders = R"(#include "cocos/bindings/auto/jsb_gfx_auto.h"
#include "cocos/bindings/auto/jsb_scene_auto.h"
)",
        .mTypescriptFolder = "cocos/core/pipeline/custom",
        .mTypescriptFilePrefix = "types",
        .mRequires = { "Gfx" },
        .mHeader = R"(#include "cocos/scene/Light.h"
)",
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

        ENUM_CLASS(TaskType) {
            ENUMS(SYNC, ASYNC);
        }

        FLAG_CLASS(SceneFlags) {
            UNDERLYING_TYPE(uint32_t);
            FLAGS(
                (NONE, 0)
                (OPAQUE_OBJECT, 1)
                (CUTOUT_OBJECT, 2)
                (TRANSPARENT_OBJECT, 4)
                (SHADOW_CASTER, 8)
                (UI, 16)
                (DEFAULT_LIGHTING, 32)
                (VOLUMETRIC_LIGHTING, 64)
                (CLUSTERED_LIGHTING, 128)
                (PLANAR_SHADOW, 256)
                (GEOMETRY, 512)
                (PROFILER, 1024)
                (ALL, 0xFFFFFFFF)
            );
        }

        ENUM_CLASS(LightingMode) {
            UNDERLYING_TYPE(uint32_t);
            ENUMS(NONE, DEFAULT, CLUSTERED);
        }

        // RenderGraph
        ENUM_CLASS(AttachmentType) {
            ENUMS(RENDER_TARGET, DEPTH_STENCIL);
        }
        ENUM_CLASS(AccessType) {
            ENUMS(READ, READ_WRITE, WRITE);
        }

        STRUCT(RasterView, .mFlags = JSB | PMR_DEFAULT) {
            PUBLIC(
                (ccstd::pmr::string, mSlotName, _)
                (AccessType, mAccessType, AccessType::WRITE)
                (AttachmentType, mAttachmentType, _)
                (gfx::LoadOp, mLoadOp, gfx::LoadOp::LOAD)
                (gfx::StoreOp, mStoreOp, gfx::StoreOp::STORE)
                (gfx::ClearFlagBit, mClearFlags, gfx::ClearFlagBit::ALL)
                (gfx::Color, mClearColor, _)
            );
            builder.setMemberFlags(vertID, "mClearColor", NOT_ELEMENT);
            TS_INIT(mAccessType, AccessType.WRITE);
            TS_INIT(mLoadOp, LoadOp.LOAD);
            TS_INIT(mStoreOp, StoreOp.STORE);
            TS_INIT(mClearFlags, ClearFlagBit.ALL);
            CNTR(mSlotName, mAccessType, mAttachmentType, mLoadOp, mStoreOp, mClearFlags, mClearColor);
        }

        ENUM_CLASS(ClearValueType) {
            ENUMS(FLOAT_TYPE, INT_TYPE);
        }

        STRUCT(ComputeView, .mFlags = JSB | PMR_DEFAULT) {
            PUBLIC(
                (ccstd::pmr::string, mName, _)
                (AccessType, mAccessType, AccessType::READ)
                (gfx::ClearFlagBit, mClearFlags, gfx::ClearFlagBit::NONE)
                (gfx::Color, mClearColor, _)
                (ClearValueType, mClearValueType, _)
            );
            builder.setMemberFlags(vertID, "mClearColor", NOT_ELEMENT);
            MEMBER_FUNCTIONS(R"(
bool isRead() const {
    return accessType != AccessType::WRITE;
}
bool isWrite() const {
    return accessType != AccessType::READ;
}
)");
            TS_INIT(mAccessType, AccessType.READ);
            TS_INIT(mClearFlags, ClearFlagBit.NONE);
        }

        STRUCT(LightInfo, .mFlags = JSB) {
            PUBLIC(
                ([[optional]] IntrusivePtr<scene::Light>, mLight, _)
                (uint32_t, mLevel, 0)
            );
            CNTR(mLight, mLevel);
        }

        NAMESPACE_END(render);
        NAMESPACE_END(cc);
    } // module CoreRender
}

}
