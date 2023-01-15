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

void buildNativePipeline(ModuleBuilder& builder, Features features) {
    MODULE(NativePipeline,
        .mFolder = "cocos/renderer/pipeline/custom",
        .mFilePrefix = "NativePipeline",
        .mRequires = { "RenderInterface", "PrivateInterface", "LayoutGraph", "RenderGraph", "InstancedBuffer", "PrivateTypes" },
        .mHeader = R"(#include "cocos/renderer/pipeline/GlobalDescriptorSetManager.h"
#include "cocos/renderer/gfx-base/GFXRenderPass.h"
#include "cocos/renderer/gfx-base/GFXFramebuffer.h"
)"
    ) {
        NAMESPACE_BEG(cc);
        NAMESPACE_BEG(render);

        STRUCT(NativeLayoutGraphBuilder) {
            INHERITS(LayoutGraphBuilder);
            PUBLIC(
                (gfx::Device*, mDevice, nullptr)
                (LayoutGraphData*, mData, nullptr)
            );
            CNTR(mDevice, mData);
        }

        STRUCT(NativeRasterQueueBuilder) {
            INHERITS(RasterQueueBuilder);
            PUBLIC(
                (const PipelineRuntime*, mPipelineRuntime, nullptr)
                (RenderGraph*, mRenderGraph, nullptr)
                (const LayoutGraphData*, mLayoutGraph, nullptr)
                (uint32_t, mQueueID, RenderGraph::null_vertex())
            );
            CNTR(mPipelineRuntime, mRenderGraph, mQueueID, mLayoutGraph);
        }

        STRUCT(NativeRasterPassBuilder) {
            INHERITS(RasterPassBuilder);
            PUBLIC(
                (const PipelineRuntime*, mPipelineRuntime, nullptr)
                (RenderGraph*, mRenderGraph, nullptr)
                (const LayoutGraphData*, mLayoutGraph, nullptr)
                (uint32_t, mPassID, RenderGraph::null_vertex())
                (uint32_t, mLayoutID, LayoutGraphData::null_vertex())
            );
            CNTR(mPipelineRuntime, mRenderGraph, mPassID, mLayoutGraph, mLayoutID);
        }

        STRUCT(NativeComputeQueueBuilder) {
            INHERITS(ComputeQueueBuilder);
            PUBLIC(
                (RenderGraph*, mRenderGraph, nullptr)
                (const LayoutGraphData*, mLayoutGraph, nullptr)
                (uint32_t, mQueueID, RenderGraph::null_vertex())
            );
            CNTR(mRenderGraph, mQueueID, mLayoutGraph);
        }

        STRUCT(NativeComputePassBuilder) {
            INHERITS(ComputePassBuilder);
            PUBLIC(
                (RenderGraph*, mRenderGraph, nullptr)
                (const LayoutGraphData*, mLayoutGraph, nullptr)
                (uint32_t, mPassID, RenderGraph::null_vertex())
                (uint32_t, mLayoutID, LayoutGraphData::null_vertex())
            );
            CNTR(mRenderGraph, mPassID, mLayoutGraph, mLayoutID);
        }

        STRUCT(NativeMovePassBuilder) {
            INHERITS(MovePassBuilder);
            PUBLIC(
                (RenderGraph*, mRenderGraph, nullptr)
                (uint32_t, mPassID, RenderGraph::null_vertex())
            );
            CNTR(mRenderGraph, mPassID);
        }

        STRUCT(NativeCopyPassBuilder) {
            INHERITS(CopyPassBuilder);
            PUBLIC(
                (RenderGraph*, mRenderGraph, nullptr)
                (uint32_t, mPassID, RenderGraph::null_vertex())
            );
            CNTR(mRenderGraph, mPassID);
        }

        STRUCT(NativeSceneTransversal) {
            INHERITS(SceneTransversal);
            PUBLIC(
                (const scene::Camera*, mCamera, nullptr)
                (const scene::RenderScene*, mScene, nullptr)
            );
            CNTR(mCamera, mScene);
        }

        //STRUCT(NativeRenderViewDesc) {
        //    PUBLIC(
        //        (AccessType, mAccessType, _)
        //        (framegraph::TextureHandle, mHandle, _)
        //    );
        //    CNTR(mAccessType, mHandle);
        //}

        //STRUCT(NativePassData) {
        //    PUBLIC(
        //        (ccstd::pmr::vector<NativeRenderViewDesc>, mOutputViews, _)
        //    );
        //}

        STRUCT(PersistentRenderPassAndFramebuffer) {
            PUBLIC(
                (IntrusivePtr<gfx::RenderPass>, mRenderPass, _)
                (IntrusivePtr<gfx::Framebuffer>, mFramebuffer, _)
                (ccstd::pmr::vector<gfx::Color>, mClearColors, _)
                (float, mClearDepth, 0)
                (uint8_t, mClearStencil, 0)
                (int32_t, mRefCount, 1)
            );
        }
        
        if (false) {
            STRUCT(ScenePassHandle, .mFlags = LESS) {
                PUBLIC(
                    (const scene::Pass*, mHandle, nullptr)
                );
            }

            STRUCT(ScenePassQueue, .mFlags = NO_MOVE_NO_COPY) {
                PUBLIC(
                    (ccstd::pmr::vector<ScenePass>, mQueue, _)
                );
            }

            STRUCT(RenderInstance, .mAlignment = 64) {
                PUBLIC(
                    (uint32_t, mCount, 0)
                    (uint32_t, mCapacity, 0)
                    (gfx::Buffer*, mVertexBuffer, nullptr)
                    (uint8_t*, mData, nullptr)
                    (gfx::InputAssembler*, mInputAssembler, nullptr)
                    (uint32_t, mStride, 0)
                    (uint32_t, mBufferOffset, 0)
                    (gfx::Shader*, mShader, nullptr)
                    (gfx::DescriptorSet*, mDescriptorSet, nullptr)
                    (gfx::Texture*, mLightmap, nullptr)
                );
            }

            STRUCT(RenderInstancePack, .mFlags = NO_COPY) {
                PUBLIC(
                    (ccstd::pmr::vector<RenderInstance>, mInstances, _)
                );
            }

            STRUCT(RenderBatch, .mFlags = NO_COPY) {
                PUBLIC(
                    (ccstd::pmr::vector<gfx::Buffer*>, mVertexBuffers, _)
                    (ccstd::pmr::vector<uint8_t*>, mVertexBufferData, _)
                    (gfx::Buffer*, mIndexBuffer, nullptr)
                    (float*, mIndexBufferData, nullptr)
                    (uint32_t, mVertexBufferCount, 0)
                    (uint32_t, mMergeCount, 0)
                    (gfx::InputAssembler*, mInputAssembler, nullptr)
                    (ccstd::pmr::vector<uint8_t>, mUniformBufferData, _)
                    (gfx::Buffer*, mUniformBuffer, nullptr)
                    (gfx::DescriptorSet*, mDescriptorSet, nullptr)
                    (const scene::Pass*, mScenePass, nullptr)
                    (gfx::Shader*, mShader, nullptr)
                );
            }

            STRUCT(RenderBatchPack, .mFlags = NO_COPY) {
                PUBLIC(
                    (ccstd::pmr::vector<PmrUniquePtr<RenderBatch>>, mBatches, _)
                    (ccstd::pmr::vector<uint32_t>, mBufferOffset, _)
                );
            }
            
            STRUCT(RenderObject, .mAlignment = 16) {
                PUBLIC(
                    (const scene::Model*, mModel, nullptr)
                    (float, mDepth, 0.0F)
                );
                CNTR(mModel, mDepth);
            }
        }

        STRUCT(RenderInstancingQueue) {
            PUBLIC(
                (PmrUnorderedSet<pipeline::InstancedBuffer*>, mBatches, _)
                (ccstd::pmr::vector<pipeline::InstancedBuffer*>, mSortedBatches, _)
            );
            MEMBER_FUNCTIONS(R"(
void add(pipeline::InstancedBuffer &instancedBuffer);
void sort();
void uploadBuffers(gfx::CommandBuffer *cmdBuffer) const;
void recordCommandBuffer(
    gfx::RenderPass *renderPass, gfx::CommandBuffer *cmdBuffer,
    gfx::DescriptorSet *ds = nullptr, uint32_t offset = 0,
    const ccstd::vector<uint32_t> *dynamicOffsets = nullptr) const;
)");
        }
        
        STRUCT(DrawInstance, .mAlignment = 32) {
            PUBLIC(
                (const scene::SubModel*, mSubModel, nullptr)
                (uint32_t, mPriority, 0)
                (uint32_t, mHash, 0)
                (float, mDepth, 0)
                (uint32_t, mShaderID, 0)
                (uint32_t, mPassIndex, 0)
            );
        }

        STRUCT(RenderDrawQueue) {
            PUBLIC(
                (ccstd::pmr::vector<DrawInstance>, mInstances, _)
            );
            MEMBER_FUNCTIONS(R"(
void add(const scene::Model& model, float depth, uint32_t subModelIdx, uint32_t passIdx);
void sortOpaqueOrCutout();
void sortTransparent();
void recordCommandBuffer(gfx::Device *device, const scene::Camera *camera,
    gfx::RenderPass *renderPass, gfx::CommandBuffer *cmdBuffer,
    uint32_t subpassIndex) const;
)");
        }

        STRUCT(NativeRenderQueue, .mFlags = NO_COPY) {
            PUBLIC(
                //(ccstd::pmr::vector<RenderObject>, mRenderObjects, _)
                (RenderDrawQueue, mOpaqueQueue, _)
                (RenderDrawQueue, mTransparentQueue, _)
                (RenderInstancingQueue, mOpaqueInstancingQueue, _)
                (RenderInstancingQueue, mTransparentInstancingQueue, _)
                (SceneFlags, mSceneFlags, SceneFlags::NONE)
                //(ccstd::pmr::vector<ScenePass>, mScenePassQueue, _)
                //(ccstd::pmr::vector<RenderBatchPack>, mBatchingQueue, _)
                //(ccstd::pmr::vector<uint32_t>, mInstancingQueue, _)
                //((PmrFlatMap<ScenePassHandle, PmrUniquePtr<RenderInstancePack>>), mInstancePacks, _)
            );
            CNTR(mSceneFlags);
            MEMBER_FUNCTIONS(R"(
void sort();
)");
        }

        STRUCT(DefaultSceneVisitor) {
            INHERITS(SceneVisitor);
            PUBLIC(
                (ccstd::pmr::string, mName, _)
            );
        }

        STRUCT(DefaultForwardLightingTransversal) {
            INHERITS(SceneTransversal);
            PUBLIC(
                (ccstd::pmr::string, mName, _)
            );
        }
        
        STRUCT(ResourceGroup, .mFlags = NO_MOVE_NO_COPY | CUSTOM_DTOR) {
            PUBLIC(
                (PmrUnorderedSet<IntrusivePtr<pipeline::InstancedBuffer>>, mInstancingBuffers, _)
            );
        }

        STRUCT(BufferPool, .mFlags = NO_COPY) {
            PUBLIC(
                (gfx::Device*, mDevice, nullptr)
                (uint32_t, mBufferSize, 0)
                (ccstd::pmr::vector<IntrusivePtr<gfx::Buffer>>, mCurrentBuffers, _)
                (ccstd::pmr::vector<IntrusivePtr<gfx::Buffer>>, mFreeBuffers, _)
            );
            MEMBER_FUNCTIONS(R"(void init(gfx::Device* deviceIn, uint32_t sz);
void syncResources();
gfx::Buffer* allocateBuffer();
)");
            CNTR(mDevice, mBufferSize);
        }

        STRUCT(DescriptorSetPool, .mFlags = NO_COPY) {
            PUBLIC(
                (gfx::Device*, mDevice, nullptr)
                (IntrusivePtr<gfx::DescriptorSetLayout>, mSetLayout, _)
                (ccstd::pmr::vector<IntrusivePtr<gfx::DescriptorSet>>, mCurrentDescriptorSets, _)
                (ccstd::pmr::vector<IntrusivePtr<gfx::DescriptorSet>>, mFreeDescriptorSets, _)
            );
            MEMBER_FUNCTIONS(R"(void init(gfx::Device* deviceIn, IntrusivePtr<gfx::DescriptorSetLayout> layout);
void syncDescriptorSets();
const gfx::DescriptorSet& getCurrentDescriptorSet() const;
gfx::DescriptorSet& getCurrentDescriptorSet();
gfx::DescriptorSet* allocateDescriptorSet();
)");
            CNTR(mDevice, mSetLayout);
        }

        STRUCT(UniformBlockResource, .mFlags = NO_COPY) {
            PUBLIC(
                (ccstd::pmr::vector<char>, mCpuBuffer, _)
                (BufferPool, mBufferPool, _)
            );
            MEMBER_FUNCTIONS(R"(void init(gfx::Device* deviceIn, uint32_t sz);
)");
        }

        STRUCT(LayoutGraphNodeResource, .mFlags = NO_COPY) {
            PUBLIC(
                ((PmrFlatMap<NameLocalID, UniformBlockResource>), mUniformBuffers, _)
                (DescriptorSetPool, mDescriptorSetPool, _)
            );
        }

        STRUCT(NativeRenderContext, .mFlags = NO_MOVE_NO_COPY | NO_DEFAULT_CNTR) {
            PUBLIC(
                (std::unique_ptr<gfx::DefaultResource>, mDefaultResource, _)
                ((ccstd::pmr::unordered_map<RasterPass, PersistentRenderPassAndFramebuffer>), mRenderPasses, _)
                ((ccstd::pmr::map<uint64_t, ResourceGroup>), mResourceGroups, _)
                ((ccstd::pmr::vector<LayoutGraphNodeResource>), mLayoutGraphResources, _)
                //(ccstd::pmr::vector<PmrUniquePtr<NativeRenderQueue>>, mFreeRenderQueues, _)
                //(ccstd::pmr::vector<PmrUniquePtr<RenderInstancePack>>, mFreeInstancePacks, _)
                (uint64_t, mNextFenceValue, 0)
            );
            MEMBER_FUNCTIONS(R"(
void clearPreviousResources(uint64_t finishedFenceValue) noexcept;
)");
            CNTR(mDefaultResource);
        }

        STRUCT(NativeProgramLibrary) {
            INHERITS(ProgramLibrary);
            PUBLIC(
                (LayoutGraphData, mLayoutGraph, _)
                ((PmrFlatMap<uint32_t, ProgramGroup>), mPhases, _)
                (boost::container::pmr::unsynchronized_pool_resource, mUnsycPool, _)
                (bool, mMergeHighFrequency, false)
                (bool, mFixedLocal, true)
                (IntrusivePtr<gfx::DescriptorSetLayout>, mEmptyDescriptorSetLayout, _)
                (IntrusivePtr<gfx::PipelineLayout>, mEmptyPipelineLayout, _)
                (PipelineRuntime*, mPipeline, nullptr)
                (gfx::Device*, mDevice, nullptr)
            );
            MEMBER_FUNCTIONS(R"(
void init(gfx::Device* deviceIn);
void destroy();
)");
        }

        STRUCT(NativePipeline, .mFlags = CUSTOM_CNTR) {
            INHERITS(Pipeline);
            PUBLIC(
                (boost::container::pmr::unsynchronized_pool_resource, mUnsyncPool, _)
                (gfx::Device*, mDevice, nullptr)
                (gfx::Swapchain*, mSwapchain, nullptr)
                (MacroRecord, mMacros, _)
                (ccstd::string, mConstantMacros, _)
                (std::unique_ptr<pipeline::GlobalDSManager>, mGlobalDSManager, _)
                (ccstd::pmr::string, mName, _)
                (NativeProgramLibrary*, mProgramLibrary, _)
                (scene::Model*, mProfiler, nullptr)
                (LightingMode, mLightingMode, LightingMode::DEFAULT)
                (IntrusivePtr<pipeline::PipelineSceneData>, mPipelineSceneData, _)
                (NativeRenderContext, mNativeContext, _)
                (LayoutGraphData, mDummyLayoutGraph, _)
                (ResourceGraph, mResourceGraph, _)
                (RenderGraph, mRenderGraph, _)
            );
            MEMBER_FUNCTIONS(R"(
void executeRenderGraph(const RenderGraph& rg);
private:
ccstd::vector<gfx::CommandBuffer*> _commandBuffers;

public:
)");
        }

        STRUCT(NativeProgramProxy) {
            INHERITS(ProgramProxy);
            PUBLIC(
                (IntrusivePtr<gfx::Shader>, mShader, _)
            );
            CNTR(mShader);
        }

        STRUCT(NativeRenderingModule) {
            INHERITS(RenderingModule);
            PUBLIC(
                (std::shared_ptr<NativeProgramLibrary>, mProgramLibrary, _)
            );
            CNTR(mProgramLibrary);
        }

        STRUCT(NativeSetter, .mFlags = NO_MOVE_NO_COPY | NO_DEFAULT_CNTR) {
            INHERITS(Setter);
            PUBLIC(
                (const LayoutGraphData&, mLayoutGraph, _)
                (RenderData&, mRenderData, _)
            );
            CNTR(mLayoutGraph, mRenderData);
            MEMBER_FUNCTIONS(R"(
void setVec4ArraySize(const ccstd::string& name, uint32_t sz);
void setVec4ArrayElem(const ccstd::string& name, const cc::Vec4& vec, uint32_t id);

void setMat4ArraySize(const ccstd::string& name, uint32_t sz);
void setMat4ArrayElem(const ccstd::string& name, const cc::Mat4& mat, uint32_t id);
)");
        }

        NAMESPACE_END(render);
        NAMESPACE_END(cc);
    }
}

}
