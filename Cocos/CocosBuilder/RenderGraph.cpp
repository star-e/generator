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

void buildRenderGraph(ModuleBuilder& builder, Features features) {
    MODULE(RenderGraph,
        .mFolder = "cocos/renderer/pipeline/custom",
        .mFilePrefix = "RenderGraph",
        .mTypescriptFolder = "cocos/core/pipeline/custom",
        .mTypescriptFilePrefix = "render-graph",
        .mRequires = { "RenderCommon", "Gfx", "Camera", "PipelineSceneData" },
        .mHeader = R"(#include "renderer/gfx-base/GFXBuffer.h"
#include "renderer/gfx-base/GFXTexture.h"
#include "renderer/gfx-base/states/GFXSampler.h"
)",
        .mTypescriptInclude = R"(import { Mat4 } from '../../math';
import { legacyCC } from '../../global-exports';
import { RenderScene } from '../../renderer/scene';
)") {
        NAMESPACE_BEG(cc);
        NAMESPACE_BEG(render);
        FLAG_CLASS(ResourceFlags) {
            FLAGS(
                (None, 0)
                (AllowRenderTarget, 0x1)
                (AllowDepthStencil, 0x2)
                (AllowUnorderedAccess, 0x4)
                (DenyShaderResource, 0x8)
                (AllowCrossAdapter, 0x10)
                (AllowSimultaneousAccess, 0x20)
                (VideoDecodeReferenceOnly, 0x40)
            );
        }

        ENUM_CLASS(TextureLayout) {
            ENUMS(Unknown, RowMajor, UndefinedSwizzle, StandardSwizzle);
        }

        STRUCT(ResourceDesc) {
            PUBLIC(
                (ResourceDimension, mDimension, _)
                (uint32_t, mAlignment, 0)
                (uint32_t, mWidth, 0)
                (uint32_t, mHeight, 0)
                (uint16_t, mDepthOrArraySize, 0)
                (uint16_t, mMipLevels, 0)
                (gfx::Format, mFormat, gfx::Format::UNKNOWN)
                (gfx::SampleCount, mSampleCount, gfx::SampleCount::ONE)
                (TextureLayout, mLayout, _)
                (ResourceFlags, mFlags, _)
            );
            TS_INIT(mFormat, Format.UNKNOWN);
            TS_INIT(mSampleCount, SampleCount.ONE);
        }

        STRUCT(ResourceTraits) {
            PUBLIC(
                (ResourceResidency, mResidency, _)
            );
            CNTR(mResidency);
            MEMBER_FUNCTIONS(R"(
bool hasSideEffects() const noexcept {
    return boost::variant2::holds_alternative<Persistent_>(mResidency) ||
           boost::variant2::holds_alternative<Backbuffer_>(mResidency);
}
)");
        }

        PMR_GRAPH(ResourceGraph, _, _) {
            NAMED_GRAPH(Name_);
            COMPONENT_GRAPH(
                (Name_, PmrString, mNames)
                (Desc_, ResourceDesc, mDescs)
                (Traits_, ResourceTraits, mTraits)
            );
            COMPONENT_BIMAP(PmrUnorderedMap, mValueIndex, Name_);
        }

        //STRUCT(NodeValue) {
        //    PUBLIC(
        //        (std::pmr::u8string, mNodeName, _)
        //        (PmrString, mValueName, _)
        //    );
        //}
        //PROJECT_TS((PmrTransparentMap<NodeValue, uint32_t>), (Map<string, number>));

        //PMR_GRAPH(NodeGraph, NodeValue, _) {
        //    PUBLIC(
        //        ((PmrTransparentMap<NodeValue, uint32_t>), mIndex, _)
        //    );
        //}
                                
        //STRUCT(ShaderParameterIndex) {
        //    PUBLIC(
        //        (UpdateFrequency, mUpdateFrequency, _)
        //        (ParameterType, mParameterType, ParameterType.Table)
        //    );
        //}

        //STRUCT(ShaderParameter) {
        //    PUBLIC(
        //        (ShaderParameterIndex, mIndex, _)
        //        (ResourceType, mResourceType, _)
        //        (ValueType, mValueType, _)
        //        (uint32_t, mNumDescriptors, 1)
        //        (PmrString, mValue, "")
        //    );
        //}

        ENUM_CLASS(AttachmentType) {
            ENUMS(RenderTarget, DepthStencil);
        }
        ENUM_CLASS(AccessType) {
            ENUMS(Read, ReadWrite, Write);
        }

        STRUCT(RasterView) {
            PUBLIC(
                (PmrString, mSlotName, _)
                (AccessType, mAccessType, AccessType::Write)
                (AttachmentType, mAttachmentType, _)
                (gfx::LoadOp, mLoadOp, gfx::LoadOp::LOAD)
                (gfx::StoreOp, mStoreOp, gfx::StoreOp::STORE)
                (gfx::ClearFlagBit, mClearFlags, gfx::ClearFlagBit::ALL)
                (gfx::Color, mClearColor, _)
            );
            TS_INIT(mAccessType, AccessType.Write);
            TS_INIT(mLoadOp, LoadOp.LOAD);
            TS_INIT(mStoreOp, StoreOp.STORE);
            TS_INIT(mClearFlags, ClearFlagBit.ALL);
            CNTR(mSlotName, mAccessType, mAttachmentType, mLoadOp, mStoreOp, mClearFlags, mClearColor);
        }

        ENUM_CLASS(ClearValueType) {
            ENUMS(Float, Int);
        }

        STRUCT(ComputeView) {
            PUBLIC(
                (PmrString, mName, _)
                (AccessType, mAccessType, AccessType::Read)
                (gfx::ClearFlagBit, mClearFlags, gfx::ClearFlagBit::NONE)
                (gfx::Color, mClearColor, _)
                (ClearValueType, mClearValueType, _)
            );
            MEMBER_FUNCTIONS(R"(
bool isRead() const {
    return mAccessType != AccessType::Write;
}
bool isWrite() const {
    return mAccessType != AccessType::Read;
}
)");
            TS_INIT(mAccessType, AccessType.Read);
            TS_INIT(mClearFlags, ClearFlagBit.NONE);
        }

        STRUCT(RasterSubpass) {
            PUBLIC(
                ((PmrTransparentMap<PmrString, RasterView>), mRasterViews, _)
                ((PmrTransparentMap<PmrString, boost::container::pmr::vector<ComputeView>>), mComputeViews, _)
            );
        }

        PMR_GRAPH(SubpassGraph, _, _) {
            NAMED_GRAPH(Name_);
            COMPONENT_GRAPH(
                (Name_, PmrString, mNames)
                (Subpass_, RasterSubpass, mSubpasses)
            );
        }
                
        STRUCT(RasterPassData) {
            PUBLIC(
                ((PmrTransparentMap<PmrString, RasterView>), mRasterViews, _)
                ((PmrTransparentMap<PmrString, boost::container::pmr::vector<ComputeView>>), mComputeViews, _)
                (SubpassGraph, mSubpassGraph, _)
            );
        }

        STRUCT(ComputePassData) {
            PUBLIC(
                ((PmrTransparentMap<PmrString, boost::container::pmr::vector<ComputeView>>), mComputeViews, _)
            );
        }

        STRUCT(CopyPair) {
            PUBLIC(
                (PmrString, mSource, _)
                (PmrString, mTarget, _)
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

        STRUCT(CopyPassData) {
            PUBLIC(
                (boost::container::pmr::vector<CopyPair>, mCopyPairs, _)
            );
        }

        STRUCT(MovePair) {
            PUBLIC(
                (PmrString, mSource, _)
                (PmrString, mTarget, _)
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

        STRUCT(MovePassData) {
            PUBLIC(
                (boost::container::pmr::vector<MovePair>, mMovePairs, _)
            );
        }

        STRUCT(RaytracePassData) {
            PUBLIC(
                ((PmrTransparentMap<PmrString, boost::container::pmr::vector<ComputeView>>), mComputeViews, _)
            );
        }

        TAGS((_), Queue_, Scene_, Dispatch_, Blit_, Present_);

        STRUCT(RenderQueueData) {
            PUBLIC(
                (QueueHint, mHint, _)
            );
            CNTR(mHint);
        }

        STRUCT(SceneData) {
            PUBLIC(
                (PmrString, mName, _)
                (scene::Camera*, mCamera, nullptr)
                (boost::container::pmr::vector<PmrString>, mScenes, _)
            );
            CNTR(mName);
        }

        STRUCT(Dispatch) {
            PUBLIC(
                (PmrString, mShader, _)
                (uint32_t, mThreadGroupCountX, 0)
                (uint32_t, mThreadGroupCountY, 0)
                (uint32_t, mThreadGroupCountZ, 0)
            );
            CNTR(mShader, mThreadGroupCountX, mThreadGroupCountY, mThreadGroupCountZ);
        }

        STRUCT(Blit) {
            PUBLIC(
                (PmrString, mShader, _)
            );
            CNTR(mShader);
        }

        STRUCT(PresentPassData) {
            PUBLIC(
                (PmrString, mResourceName, _)
                (uint32_t, mSyncInterval, 0)
                (uint32_t, mFlags, 0)
            );
            CNTR(mResourceName, mSyncInterval, mFlags);
        }

        STRUCT(RenderData, .mFlags = NO_COPY) {
            PUBLIC(
                ((PmrUnorderedMap<uint32_t, boost::container::pmr::vector<uint8_t>>), mConstants, _)
                ((PmrUnorderedMap<uint32_t, std::unique_ptr<gfx::Buffer>>), mBuffers, _)
                ((PmrUnorderedMap<uint32_t, std::unique_ptr<gfx::Texture>>), mTextures, _)
                ((PmrUnorderedMap<uint32_t, std::unique_ptr<gfx::Sampler>>), mSamplers, _)
            );
        }

        PMR_GRAPH(RenderGraph, _, _, .mFlags = NO_COPY) {
            NAMED_GRAPH(Name_);
            REFERENCE_GRAPH();

            COMPONENT_GRAPH(
                (Name_, PmrString, mNames)
                (Layout_, PmrString, mLayoutNodes)
                (Data_, RenderData, mData)
            );

            POLYMORPHIC_GRAPH(
                (Raster_, RasterPassData, mRasterPasses)
                (Compute_, ComputePassData, mComputePasses)
                (Copy_, CopyPassData, mCopyPasses)
                (Move_, MovePassData, mMovePasses)
                (Present_, PresentPassData, mPresentPasses)
                (Raytrace_, RaytracePassData, mRaytracePasses)
                (Queue_, RenderQueueData, mRenderQueues)
                (Scene_, SceneData, mScenes)
                (Blit_, Blit, mBlits)
                (Dispatch_, Dispatch, mDispatches)
            );
            PUBLIC(
                ((PmrUnorderedMap<PmrString, uint32_t>), mIndex, _)
            );
        }

        INTERFACE(Setter, .mFlags = NO_DEFAULT_CNTR) {
            PRIVATE(
                (RenderData&, mData, _)
            );
            CNTR_NO_DEFAULT(mData);
            TS_FUNCTIONS(R"(setMat4 (name: string, mat: Mat4): void {

}
setMatrix4x4 (name: string, data: number[]): void {

}
setMatrix3x4 (name: string, data: number[]): void {

}
setFloat4 (name: string, data: number[]): void {

}
setFloat2 (name: string, data: number[]): void {

}
setFloat (name: string, data: number): void {

}
setInt4 (name: string, data: number[]): void {

}
setInt2 (name: string, data: number[]): void {

}
setInt (name: string, data: number): void {

}
setUint4 (name: string, data: number[]): void {

}
setUint2 (name: string, data: number[]): void {

}
setUint (name: string, data: number): void {

}
setCBuffer (name: string, buffer: Buffer): void {

}
setBuffer (name: string, buffer: Buffer): void {

}
setTexture (name: string, texture: Texture): void {

}
setRWBuffer (name: string, buffer: Buffer): void {

}
setRWTexture (name: string, texture: Texture): void {

}
setSampler (name: string, sampler: Sampler): void {

}
protected _setCameraValues (camera: Readonly<Camera>, cfg: Readonly<PipelineSceneData>, scene: Readonly<RenderScene>) {
    this.setMat4('cc_matView', camera.matView);
    this.setMat4('cc_matViewInv', camera.node.worldMatrix);
    this.setMat4('cc_matProj', camera.matProj);
    this.setMat4('cc_matProjInv', camera.matProjInv);
    this.setMat4('cc_matViewProj', camera.matViewProj);
    this.setMat4('cc_matViewProjInv', camera.matViewProjInv);
    this.setFloat4('cc_cameraPos', [camera.position.x, camera.position.y, camera.position.z, 0.0]);
    this.setFloat4('cc_screenScale', [cfg.shadingScale, cfg.shadingScale, 1.0 / cfg.shadingScale, 1.0 / cfg.shadingScale]);
    this.setFloat4('cc_exposure', [camera.exposure, 1.0 / camera.exposure, cfg.isHDR ? 1.0 : 0.0, 0.0]);

    const mainLight = scene.mainLight;
    if (mainLight) {
        this.setFloat4('cc_mainLitDir', [mainLight.direction.x, mainLight.direction.y, mainLight.direction.z, 0.0]);
        let r = mainLight.color.x;
        let g = mainLight.color.y;
        let b = mainLight.color.z;
        if (mainLight.useColorTemperature) {
            r *= mainLight.colorTemperatureRGB.x;
            g *= mainLight.colorTemperatureRGB.y;
            b *= mainLight.colorTemperatureRGB.z;
        }
        let w = mainLight.illuminance;
        if (cfg.isHDR) {
            w *= camera.exposure;
        }
        this.setFloat4('cc_mainLitColor', [r, g, b, w]);
    } else {
        this.setFloat4('cc_mainLitDir', [0, 0, 1, 0]);
        this.setFloat4('cc_mainLitColor', [0, 0, 0, 0]);
    }

    const ambient = cfg.ambient;
    const skyColor = ambient.skyColor;
    if (cfg.isHDR) {
        skyColor.w = ambient.skyIllum * camera.exposure;
    } else {
        skyColor.w = ambient.skyIllum;
    }
    this.setFloat4('cc_ambientSky', [skyColor.x, skyColor.y, skyColor.z, skyColor.w]);
    this.setFloat4('cc_ambientGround', [ambient.groundAlbedo.x, ambient.groundAlbedo.y, ambient.groundAlbedo.z, ambient.groundAlbedo.w]);

    const fog = cfg.fog;
    const colorTempRGB = fog.colorArray;
    this.setFloat4('cc_fogColor', [colorTempRGB.x, colorTempRGB.y, colorTempRGB.z, colorTempRGB.w]);
    this.setFloat4('cc_fogBase', [fog.fogStart, fog.fogEnd, fog.fogDensity, 0.0]);
    this.setFloat4('cc_fogAdd', [fog.fogTop, fog.fogRange, fog.fogAtten, 0.0]);
    this.setFloat4('cc_nearFar', [camera.nearClip, camera.farClip, 0.0, 0.0]);
    this.setFloat4('cc_viewPort', [camera.viewport.x, camera.viewport.y, camera.viewport.z, camera.viewport.w]);
}
)");
        }

        STRUCT(RasterQueue, .mFlags = NO_DEFAULT_CNTR) {
            INHERITS(Setter);
            PRIVATE(
                (RenderGraph&, mRenderGraph, _)
                (const uint32_t, mVertID, 0xFFFFFFFF)
                (RenderQueueData&, mQueue, _)
                (pipeline::PipelineSceneData&, mPipeline, _)
            );
            CNTR_NO_DEFAULT(mRenderGraph, mVertID, mQueue, mPipeline);
            TS_FUNCTIONS(R"(addSceneOfCamera (camera: Camera, name = 'Camera'): RasterQueue {
    const sceneData = new SceneData(name);
    sceneData.camera = camera;
    this._renderGraph.addVertex<RenderGraphValue.scene>(
        RenderGraphValue.scene, sceneData, name, '', new RenderData(), this._vertID,
    );
    super._setCameraValues(camera, this._pipeline,
        camera.scene ? camera.scene : legacyCC.director.getScene().renderScene);
    return this;
}
addScene (sceneName: string): RasterQueue {
    const sceneData = new SceneData(sceneName);
    this._renderGraph.addVertex<RenderGraphValue.scene>(
        RenderGraphValue.scene, sceneData, sceneName, '', new RenderData(), this._vertID,
    );
    return this;
}
addFullscreenQuad (shader: string, name = 'Quad'): RasterQueue {
    this._renderGraph.addVertex<RenderGraphValue.blit>(
        RenderGraphValue.blit, new Blit(shader), name, '', new RenderData(), this._vertID,
    );
    return this;
}
)");
        }
                                                
        STRUCT(RasterPass, .mFlags = NO_DEFAULT_CNTR) {
            INHERITS(Setter);
            PRIVATE(
                (RenderGraph&, mRenderGraph, _)
                (const uint32_t, mVertID, 0xFFFFFFFF)
                (RasterPassData&, mPass, _)
                (pipeline::PipelineSceneData&, mPipeline, _)
            );
            CNTR_NO_DEFAULT(mRenderGraph, mVertID, mPass, mPipeline);
            TS_FUNCTIONS(R"(addRasterView (name: string, view: RasterView) {
    this._pass.rasterViews.set(name, view);
}
addComputeView (name: string, view: ComputeView) {
    if (this._pass.computeViews.has(name)) {
        this._pass.computeViews.get(name)?.push(view);
    } else {
        this._pass.computeViews.set(name, [view]);
    }
}
addQueue (hint: QueueHint = QueueHint.Opaque, layoutName = '', name = 'Queue') {
    if (layoutName === '') {
        switch (hint) {
        case QueueHint.Opaque:
            layoutName = 'Opaque';
            break;
        case QueueHint.Cutout:
            layoutName = 'Cutout';
            break;
        case QueueHint.Transparent:
            layoutName = 'Transparent';
            break;
        default:
            throw Error('cannot infer layoutName from QueueHint');
        }
    }
    const queue = new RenderQueueData(hint);
    const data = new RenderData();
    const queueID = this._renderGraph.addVertex<RenderGraphValue.queue>(
        RenderGraphValue.queue, queue, name, layoutName, data, this._vertID,
    );
    return new RasterQueue(data, this._renderGraph, queueID, queue, this._pipeline);
}
addFullscreenQuad (shader: string, layoutName = '', name = 'Quad') {
    this._renderGraph.addVertex<RenderGraphValue.blit>(
        RenderGraphValue.blit,
        new Blit(shader),
        name, layoutName, new RenderData(), this._vertID,
    );
}
)");
        }

        STRUCT(ComputeQueue, .mFlags = NO_DEFAULT_CNTR) {
            INHERITS(Setter);
            PRIVATE(
                (RenderGraph&, mRenderGraph, _)
                (const uint32_t, mVertID, 0xFFFFFFFF)
                (RenderQueueData&, mQueue, _)
                (pipeline::PipelineSceneData&, mPipeline, _)
            );
            CNTR_NO_DEFAULT(mRenderGraph, mVertID, mQueue, mPipeline);
            TS_FUNCTIONS(R"(addDispatch (shader: string,
    threadGroupCountX: number,
    threadGroupCountY: number,
    threadGroupCountZ: number,
    layoutName = '',
    name = 'Dispatch') {
    this._renderGraph.addVertex<RenderGraphValue.dispatch>(
        RenderGraphValue.dispatch,
        new Dispatch(shader, threadGroupCountX, threadGroupCountY, threadGroupCountZ),
        name, layoutName, new RenderData(), this._vertID,
    );
}
)");
        }

        STRUCT(ComputePass, .mFlags = NO_DEFAULT_CNTR) {
            INHERITS(Setter);
            PRIVATE(
                (RenderGraph&, mRenderGraph, _)
                (const uint32_t, mVertID, 0xFFFFFFFF)
                (ComputePassData&, mPass, _)
                (pipeline::PipelineSceneData&, mPipeline, _)
            );
            CNTR_NO_DEFAULT(mRenderGraph, mVertID, mPass, mPipeline);
            TS_FUNCTIONS(R"(addComputeView (name: string, view: ComputeView) {
    if (this._pass.computeViews.has(name)) {
        this._pass.computeViews.get(name)?.push(view);
    } else {
        this._pass.computeViews.set(name, [view]);
    }
}
addDispatch (shader: string,
    threadGroupCountX: number,
    threadGroupCountY: number,
    threadGroupCountZ: number,
    layoutName = '',
    name = 'Dispatch') {
    this._renderGraph.addVertex<RenderGraphValue.dispatch>(
        RenderGraphValue.dispatch,
        new Dispatch(shader, threadGroupCountX, threadGroupCountY, threadGroupCountZ),
        name, layoutName, new RenderData(), this._vertID,
    );
}
)");
        }

        STRUCT(MovePass, .mFlags = NO_DEFAULT_CNTR) {
            PRIVATE(
                (RenderGraph&, mRenderGraph, _)
                (const uint32_t, mVertID, 0xFFFFFFFF)
                (MovePassData&, mPass, _)
            );
            CNTR_NO_DEFAULT(mRenderGraph, mVertID, mPass);
            TS_FUNCTIONS(R"(addMove (pair: MovePair) {
    this._pass.movePairs.push(pair);
}
)");
        }
                
        STRUCT(CopyPass, .mFlags = NO_DEFAULT_CNTR) {
            PRIVATE(
                (RenderGraph&, mRenderGraph, _)
                (const uint32_t, mVertID, 0xFFFFFFFF)
                (CopyPassData&, mPass, _)
            );
            CNTR_NO_DEFAULT(mRenderGraph, mVertID, mPass);
            TS_FUNCTIONS(R"(addCopy (pair: CopyPair) {
    this._pass.copyPairs.push(pair);
}
)");
        }
        NAMESPACE_END(render);
        NAMESPACE_END(cc);
    }
}

} // namespace Cocos::Meta
