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
        .mTypescriptFolder = "cocos/rendering/custom",
        .mTypescriptFilePrefix = "types",
        .mRequires = { "Gfx" },
        .mHeader = R"(#include "cocos/scene/Light.h"
#include "cocos/base/std/container/map.h"
)",
        .mTypescriptInclude = R"(import { OutputArchive, InputArchive } from './archive';
import { saveColor, loadColor, saveUniformBlock, loadUniformBlock } from './serialization';
)"
    ) {
        NAMESPACE_BEG(cc);
        NAMESPACE_BEG(render);
        
        ENUM_CLASS(UpdateFrequency) {
            ENUMS(PER_INSTANCE, PER_BATCH, PER_PHASE, PER_PASS, COUNT);
        }

        ENUM_CLASS(ParameterType) {
            ENUMS(CONSTANTS, CBV, UAV, SRV, TABLE, SSV);
        }

        TAGS((_), RasterPass_, RasterSubpass_, ComputeSubpass_, Compute_, Copy_, Move_, Raytrace_);

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
                (OPAQUE_OBJECT, 0x1)
                (CUTOUT_OBJECT, 0x2)
                (TRANSPARENT_OBJECT, 0x4)
                (SHADOW_CASTER, 0x8)
                (UI, 0x10)
                (DEFAULT_LIGHTING, 0x20)
                (VOLUMETRIC_LIGHTING, 0x40)
                (CLUSTERED_LIGHTING, 0x80)
                (PLANAR_SHADOW, 0x100)
                (GEOMETRY, 0x200)
                (PROFILER, 0x400)
                (DRAW_INSTANCING, 0x800)
                (DRAW_NON_INSTANCING, 0x1000)
                (REFLECTION_PROBE, 0x2000)
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
                (uint32_t, mSlotID, 0)
            );
            builder.setMemberFlags(vertID, "mClearColor", NOT_ELEMENT);
            builder.setMemberFlags(vertID, "mSlotID", NOT_ELEMENT);
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
                (ClearValueType, mClearValueType, ClearValueType::FLOAT_TYPE)
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
            TS_INIT(mClearFlags, ClearFlagBit.NONE);
            CNTR(mName, mAccessType, mClearFlags, mClearColor, mClearValueType);
        }

        STRUCT(LightInfo, .mFlags = JSB) {
            PUBLIC(
                ([[optional]] IntrusivePtr<scene::Light>, mLight, _)
                (uint32_t, mLevel, 0)
            );
            CNTR(mLight, mLevel);
        }

        // Descriptor
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

        STRUCT(DescriptorBlockFlattened, .mFlags = JSB) {
            PUBLIC(
                (ccstd::vector<ccstd::string>, mDescriptorNames, _)
                (ccstd::vector<ccstd::string>, mUniformBlockNames, _)
                (ccstd::vector<Descriptor>, mDescriptors, _)
                (ccstd::vector<gfx::UniformBlock>, mUniformBlocks, _)
                (uint32_t, mCapacity, 0)
                (uint32_t, mCount, 0)
            );
        }

        STRUCT(DescriptorBlockIndex, .mFlags = LESS | JSB | STRING_KEY) {
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

        // RenderGraph
        STRUCT(CopyPair, .mFlags = PMR_DEFAULT | JSB) {
            PUBLIC(
                (ccstd::pmr::string, mSource, _)
                (ccstd::pmr::string, mTarget, _)
                (uint32_t, mMipLevels, 0xFFFFFFFF)
                (uint32_t, mNumSlices, 0xFFFFFFFF)
                (uint32_t, mSourceMostDetailedMip, 0)
                (uint32_t, mSourceFirstSlice, 0)
                (uint32_t, mSourcePlaneSlice, 0)
                (uint32_t, mTargetMostDetailedMip, 0)
                (uint32_t, mTargetFirstSlice, 0)
                (uint32_t, mTargetPlaneSlice, 0)
            );
            CNTR(mSource, mTarget, mMipLevels, mNumSlices,
                mSourceMostDetailedMip, mSourceFirstSlice, mSourcePlaneSlice,
                mTargetMostDetailedMip, mTargetFirstSlice, mTargetPlaneSlice);
        }

        STRUCT(MovePair, .mFlags = PMR_DEFAULT | JSB) {
            PUBLIC(
                (ccstd::pmr::string, mSource, _)
                (ccstd::pmr::string, mTarget, _)
                (uint32_t, mMipLevels, 0xFFFFFFFF)
                (uint32_t, mNumSlices, 0xFFFFFFFF)
                (uint32_t, mTargetMostDetailedMip, 0)
                (uint32_t, mTargetFirstSlice, 0)
                (uint32_t, mTargetPlaneSlice, 0)
            );
            CNTR(mSource, mTarget, mMipLevels, mNumSlices,
                mTargetMostDetailedMip,
                mTargetFirstSlice, mTargetPlaneSlice);
        }

        STRUCT(PipelineStatistics) {
            PUBLIC(
                (uint32_t, mNumRenderPasses, 0)
                (uint32_t, mNumManagedTextures, 0)
                (uint32_t, mTotalManagedTextures, 0)
                (uint32_t, mNumUploadBuffers, 0)
                (uint32_t, mNumUploadBufferViews, 0)
                (uint32_t, mNumFreeUploadBuffers, 0)
                (uint32_t, mNumFreeUploadBufferViews, 0)
                (uint32_t, mNumDescriptorSets, 0)
                (uint32_t, mNumFreeDescriptorSets, 0)
                (uint32_t, mNumInstancingBuffers, 0)
                (uint32_t, mNumInstancingUniformBlocks, 0)
            );
        }

        NAMESPACE_END(render);
        NAMESPACE_END(cc);
    } // module CoreRender
}

}
