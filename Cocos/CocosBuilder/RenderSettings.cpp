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

void buildRenderSettings(ModuleBuilder& builder, Features features) {
    MODULE(RenderSettings,
        .mFolder = "cocos/renderer/pipeline/custom",
        //.mFilePrefix = "RenderSettings",
        .mTypescriptFolder = "editor/assets/default_renderpipeline/",
        .mTypescriptFilePrefix = "builtin-pipeline-types",
        .mRequires = {},
    ) {
        NAMESPACE_BEG(cc);
        NAMESPACE_BEG(render);

        STRUCT(MSAA, .mStructInterface = true) {
            PUBLIC(
                (bool, mEnabled, false)
                (gfx::SampleCount, mSampleCount, gfx::SampleCount::X4)
            );
        }

        STRUCT(HBAO, .mStructInterface = true) {
            PUBLIC(
                (bool, mEnabled, false)
                (float, mRadiusScale, 1)
                (float, mAngleBiasDegree, 10)
                (float, mBlurSharpness, 3)
                (float, mAoSaturation, 1)
                (bool, mNeedBlur, false)
            );
        }

        STRUCT(DepthOfField, .mStructInterface = true) {
            PUBLIC(
                (bool, mEnabled, false)
                ([[nullable]] IntrusivePtr<Material>, mMaterial, _)
                (float, mMinRange, 0)
                (float, mMaxRange, 50)
                (float, mBlurRadius, 1)
                (float, mIntensity, 1)
                (mutable Vec3, mFocusPos, _)

                //(float, mFocusDistance, 0)
                //(float, mFocusRange, 0)
                //(float, mBokehRadius, 1)
            );

            TS_INIT(mFocusPos, new Vec3(0, 0, 0));
        }

        STRUCT(Bloom, .mStructInterface = true) {
            PUBLIC(
                (bool, mEnabled, false)
                ([[nullable]] IntrusivePtr<Material>, mMaterial, _)
                (bool, mEnableAlphaMask, false)
                //(bool, mUseHdrIlluminance, false)
                (uint32_t, mIterations, 3)
                (float, mThreshold, 0.8)
                (float, mIntensity, 2.3)
            );
        }

        STRUCT(ColorGrading, .mStructInterface = true) {
            PUBLIC(
                (bool, mEnabled, false)
                ([[nullable]] IntrusivePtr<Material>, mMaterial, _)
                (float, mContribute, 1)
                ([[nullable]] IntrusivePtr<Texture2D>, mColorGradingMap, _)
            );
        }

        STRUCT(FSR, .mStructInterface = true) {
            PUBLIC(
                (bool, mEnabled, false)
                ([[nullable]] IntrusivePtr<Material>, mMaterial, _)
                (float, mSharpness, 0.8)
            );
        }

        STRUCT(FXAA, .mStructInterface = true) {
            PUBLIC(
                (bool, mEnabled, false)
                ([[nullable]] IntrusivePtr<Material>, mMaterial, _)
            );
        }

        STRUCT(ToneMapping, .mStructInterface = true) {
            PUBLIC(
                ([[nullable]] IntrusivePtr<Material>, mMaterial, _)
            );
        }

        //STRUCT(ForwardPipeline, .mStructInterface = true) {
        //    PUBLIC(
        //        (uint32_t, mMobileMaxSpotLightShadowMaps, 1)
        //    );
        //}

        STRUCT(PipelineSettings, .mStructInterface = true) {
            PUBLIC(
                //(bool, mEditorUseGamePipeline, false)
                //(ForwardPipeline, mForwardPipeline, _)
                (MSAA, mMSAA, _)
                //(HBAO, mHBAO, _)
                (bool, mEnableShadingScale, false)
                (float, mShadingScale, 0.5)
                (DepthOfField, mDepthOfField, _)
                (Bloom, mBloom, _)
                (ToneMapping, mToneMapping, _)
                (ColorGrading, mColorGrading, _)
                (FSR, mFsr, _)
                (FXAA, mFxaa, _)
                //([[nullable]] IntrusivePtr<Material>, mCopyMaterial, _)
            );
        }

        NAMESPACE_END(render);
        NAMESPACE_END(cc);
    }
}

}
