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

namespace cc {

namespace scene {

class ReflectionProbe;

} // namespace scene

} // namespace cc
)",
        .mTypescriptInclude = R"(import type { OutputArchive, InputArchive } from './archive';
import { saveUniformBlock, loadUniformBlock } from './serialization';
)"
    ) {
        NAMESPACE_BEG(cc);
        NAMESPACE_BEG(render);
        
        ENUM_CLASS(UpdateFrequency, .mFlags = TS_NAME | TS_ENUM_OBJECT) {
            ENUMS(PER_INSTANCE, PER_BATCH, PER_PHASE, PER_PASS, COUNT);
        }

        ENUM_CLASS(ParameterType) {
            ENUMS(CONSTANTS, CBV, UAV, SRV, TABLE, SSV);
        }

        TAGS((_), RasterPass_, RasterSubpass_, ComputeSubpass_, Compute_, Resolve_, Copy_, Move_, Raytrace_);

        ENUM_CLASS(ResourceResidency, .mFlags = TS_ENUM_OBJECT) {
            ENUMS(MANAGED, MEMORYLESS, PERSISTENT, EXTERNAL, BACKBUFFER);
        }

        ENUM_CLASS(QueueHint, .mFlags = TS_ENUM_OBJECT) {
            ENUMS(NONE, OPAQUE, MASK, BLEND);
            builder.addEnumElement(vertID, "RENDER_OPAQUE", "OPAQUE", true);
            builder.addEnumElement(vertID, "RENDER_CUTOUT", "MASK", true);
            builder.addEnumElement(vertID, "RENDER_TRANSPARENT", "BLEND", true);
        }

        ENUM_CLASS(ResourceDimension, .mFlags = TS_ENUM_OBJECT) {
            ENUMS(BUFFER, TEXTURE1D, TEXTURE2D, TEXTURE3D);
        }
        
        FLAG_CLASS(ResourceFlags, .mFlags = TS_ENUM_OBJECT) {
            FLAGS(
                (NONE, 0)
                (UNIFORM, 0x1)
                (INDIRECT, 0x2)
                (STORAGE, 0x4)
                (SAMPLED, 0x8)
                (COLOR_ATTACHMENT, 0x10)
                (DEPTH_STENCIL_ATTACHMENT, 0x20)
                (INPUT_ATTACHMENT, 0x40)
                (SHADING_RATE, 0x80)
                (TRANSFER_SRC, 0x100)
                (TRANSFER_DST, 0x200)
            );
        }

        TAGS((_), Buffer_, Texture_);

        ENUM_CLASS(TaskType) {
            ENUMS(SYNC, ASYNC);
        }

        FLAG_CLASS(SceneFlags, .mFlags = TS_ENUM_OBJECT) {
            UNDERLYING_TYPE(uint32_t);
            FLAGS(
                (NONE, 0)
                (OPAQUE, 0x1)
                (MASK, 0x2)
                (BLEND, 0x4)
                (OPAQUE_OBJECT, OPAQUE)
                (CUTOUT_OBJECT, MASK)
                (TRANSPARENT_OBJECT, BLEND)
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
                (GPU_DRIVEN, 0x4000)
                (NON_BUILTIN, 0x8000)
                (ALL, 0xFFFFFFFF)
            );
        }

        ENUM_CLASS(LightingMode) {
            UNDERLYING_TYPE(uint32_t);
            ENUMS(NONE, DEFAULT, CLUSTERED);
        }

        // RenderGraph
        ENUM_CLASS(AttachmentType) {
            ENUMS(RENDER_TARGET, DEPTH_STENCIL, SHADING_RATE);
        }

        ENUM_CLASS(AccessType, .mFlags = TS_ENUM_OBJECT) {
            ENUMS(READ, READ_WRITE, WRITE);
        }

        ENUM_CLASS(ClearValueType) {
            ENUMS(NONE, FLOAT_TYPE, INT_TYPE);
        }

        STRUCT(LightInfo, .mFlags = JSB | POOL_OBJECT) {
            PUBLIC(
                ([[nullable]] IntrusivePtr<scene::Light>, mLight, _)
                ([[nullable]] scene::ReflectionProbe*, mProbe, nullptr)
                (uint32_t, mLevel, 0)
                (bool, mCulledByLight, false)
            );
            CNTR(mLight, mLevel, mCulledByLight, mProbe);
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
        ENUM_CLASS(DescriptorTypeOrder, .mFlags = TS_NAME) {
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

        STRUCT(DescriptorBlockIndex, .mFlags = LESS | JSB | STRING_KEY | SKIP_RESET) {
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
        FLAG_CLASS(ResolveFlags) {
            FLAGS(
                (NONE, 0)
                (COLOR, 1 << 0)
                (DEPTH, 1 << 1)
                (STENCIL, 1 << 2)
            );
        }

        STRUCT(ResolvePair, .mFlags = PMR_DEFAULT | JSB | POOL_OBJECT) {
            PUBLIC(
                (ccstd::pmr::string, mSource, _)
                (ccstd::pmr::string, mTarget, _)
                (ResolveFlags, mResolveFlags, ResolveFlags::NONE)
                (gfx::ResolveMode, mMode, gfx::ResolveMode::SAMPLE_ZERO)
                (gfx::ResolveMode, mMode1, gfx::ResolveMode::SAMPLE_ZERO)
            );
            CNTR(mSource, mTarget, mResolveFlags, mMode, mMode1);
        }

        STRUCT(CopyPair, .mFlags = PMR_DEFAULT | JSB | POOL_OBJECT) {
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

        STRUCT(UploadPair, .mFlags = PMR_DEFAULT | JSB | POOL_OBJECT | NO_COPY | NO_SERIALIZATION) {
            PUBLIC(
                (ccstd::vector<uint8_t>, mSource, _)
                (ccstd::pmr::string, mTarget, _)
                (uint32_t, mMipLevels, 0xFFFFFFFF)
                (uint32_t, mNumSlices, 0xFFFFFFFF)
                (uint32_t, mTargetMostDetailedMip, 0)
                (uint32_t, mTargetFirstSlice, 0)
                (uint32_t, mTargetPlaneSlice, 0)
            );
            CNTR(mSource, mTarget, mMipLevels, mNumSlices,
                mTargetMostDetailedMip, mTargetFirstSlice, mTargetPlaneSlice);
        }

        STRUCT(MovePair, .mFlags = PMR_DEFAULT | JSB | POOL_OBJECT) {
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
