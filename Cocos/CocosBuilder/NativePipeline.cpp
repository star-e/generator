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
        .mRequires = { "RenderInterface", "PrivateInterface", "LayoutGraph", "RenderGraph", "Customization", "InstancedBuffer", "PrivateTypes" },
        .mHeader = R"(#include "cocos/renderer/pipeline/GlobalDescriptorSetManager.h"
#include "cocos/renderer/gfx-base/GFXRenderPass.h"
#include "cocos/renderer/gfx-base/GFXFramebuffer.h"

#ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable: 4250)
#endif
)",
        .mTail = R"(
#ifdef _MSC_VER
    #pragma warning(pop)
#endif
)" ) {
        NAMESPACE_BEG(cc);
        NAMESPACE_BEG(render);

        if (false) {
            STRUCT(NativeLayoutGraphBuilder) {
                INHERITS(LayoutGraphBuilder);
                PUBLIC(
                    (gfx::Device*, mDevice, nullptr)
                    (LayoutGraphData*, mData, nullptr)
                );
                CNTR(mDevice, mData);
            }
        }

        STRUCT(NativeRenderNode, .mFinal = false, .mFlags = NO_DEFAULT_CNTR) {
            IMPLEMENTS(RenderNode);
            PUBLIC(
                (const PipelineRuntime*, mPipelineRuntime, nullptr)
                (RenderGraph*, mRenderGraph, nullptr)
                (uint32_t, mNodeID, RenderGraph::null_vertex())
            );
            CNTR(mPipelineRuntime, mRenderGraph, mNodeID);
        }

        STRUCT(NativeSetter, .mFinal = false, .mFlags = NO_DEFAULT_CNTR) {
            INHERITS(NativeRenderNode)
            IMPLEMENTS(Setter);
            PUBLIC(
                (const LayoutGraphData*, mLayoutGraph, nullptr)
                (uint32_t, mLayoutID, LayoutGraphData::null_vertex())
            );
            MEMBER_FUNCTIONS(R"(
void setVec4ArraySize(const ccstd::string& name, uint32_t sz);
void setVec4ArrayElem(const ccstd::string& name, const cc::Vec4& vec, uint32_t id);

void setMat4ArraySize(const ccstd::string& name, uint32_t sz);
void setMat4ArrayElem(const ccstd::string& name, const cc::Mat4& mat, uint32_t id);
)");
            CNTR(mLayoutGraph, mLayoutID);
        }

        STRUCT(NativeRenderSubpassBuilderImpl, .mFinal = false, .mFlags = NO_DEFAULT_CNTR) {
            INHERITS(NativeSetter)
            IMPLEMENTS(RenderSubpassBuilder);
            CNTR_EMPTY();
        }

        STRUCT(NativeRenderQueueBuilder, .mFlags = NO_DEFAULT_CNTR) {
            INHERITS(RenderQueueBuilder, NativeSetter);
            CNTR_EMPTY();
        }

        STRUCT(NativeRenderSubpassBuilder, .mFlags = NO_DEFAULT_CNTR) {
            INHERITS(RenderSubpassBuilder, NativeRenderSubpassBuilderImpl);
            CNTR_EMPTY();
        }

        STRUCT(NativeMultisampleRenderSubpassBuilder, .mFlags = NO_DEFAULT_CNTR) {
            INHERITS(MultisampleRenderSubpassBuilder, NativeRenderSubpassBuilderImpl);
            CNTR_EMPTY();
        }

        STRUCT(NativeComputeSubpassBuilder, .mFlags = NO_DEFAULT_CNTR) {
            INHERITS(ComputeSubpassBuilder, NativeSetter);
            CNTR_EMPTY();
        }

        STRUCT(NativeRenderPassBuilder, .mFlags = NO_DEFAULT_CNTR) {
            INHERITS(RenderPassBuilder, NativeSetter);
            CNTR_EMPTY();
        }

        STRUCT(NativeMultisampleRenderPassBuilder, .mFlags = NO_DEFAULT_CNTR) {
            INHERITS(MultisampleRenderPassBuilder, NativeSetter)
            PUBLIC(
                (uint32_t, mSubpassID, RenderGraph::null_vertex())
                (uint32_t, mSubpassLayoutID, RenderGraph::null_vertex())
            );
            CNTR(mSubpassID, mSubpassLayoutID);
        }

        STRUCT(NativeComputeQueueBuilder, .mFlags = NO_DEFAULT_CNTR) {
            INHERITS(ComputeQueueBuilder, NativeSetter);
            CNTR_EMPTY();
        }

        STRUCT(NativeComputePassBuilder, .mFlags = NO_DEFAULT_CNTR) {
            INHERITS(ComputePassBuilder, NativeSetter);
            CNTR_EMPTY();
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

            STRUCT(RenderInstance/*, .mAlignment = 64*/) {
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
            
            STRUCT(RenderObject/*, .mAlignment = 16*/) {
                PUBLIC(
                    (const scene::Model*, mModel, nullptr)
                    (float, mDepth, 0.0F)
                );
                CNTR(mModel, mDepth);
            }
        }

        STRUCT(RenderInstancingQueue) {
            PUBLIC(
                (ccstd::pmr::vector<pipeline::InstancedBuffer*>, mSortedBatches, _)
                ((PmrUnorderedMap<const scene::Pass*, uint32_t>), mPassInstances, _)
                (ccstd::pmr::vector<IntrusivePtr<pipeline::InstancedBuffer>>, mInstanceBuffers, _)
            );
            MEMBER_FUNCTIONS(R"(
bool empty() const noexcept;
void clear();
void add(const scene::Pass& pass, scene::SubModel& submodel, uint32_t passID);
void sort();
void uploadBuffers(gfx::CommandBuffer *cmdBuffer) const;
void recordCommandBuffer(
    gfx::RenderPass *renderPass, gfx::CommandBuffer *cmdBuffer,
    gfx::DescriptorSet *ds = nullptr, uint32_t offset = 0,
    const ccstd::vector<uint32_t> *dynamicOffsets = nullptr) const;
)");
        }

        STRUCT(DrawInstance/*, .mAlignment = 32*/) {
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
                (uint32_t, mSubpassOrPassLayoutID, 0xFFFFFFFF)
                //(ccstd::pmr::vector<ScenePass>, mScenePassQueue, _)
                //(ccstd::pmr::vector<RenderBatchPack>, mBatchingQueue, _)
                //(ccstd::pmr::vector<uint32_t>, mInstancingQueue, _)
                //((PmrFlatMap<ScenePassHandle, PmrUniquePtr<RenderInstancePack>>), mInstancePacks, _)
            );
            CNTR(mSceneFlags, mSubpassOrPassLayoutID);
            MEMBER_FUNCTIONS(R"(
void sort();
void clear() noexcept;
bool empty() const noexcept;
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
                (bool, mDynamic, false)
                (ccstd::pmr::vector<IntrusivePtr<gfx::Buffer>>, mCurrentBuffers, _)
                (ccstd::pmr::vector<IntrusivePtr<gfx::Buffer>>, mCurrentBufferViews, _)
                (ccstd::pmr::vector<IntrusivePtr<gfx::Buffer>>, mFreeBuffers, _)
                (ccstd::pmr::vector<IntrusivePtr<gfx::Buffer>>, mFreeBufferViews, _)
            );
            MEMBER_FUNCTIONS(R"(void init(gfx::Device* deviceIn, uint32_t sz, bool bDynamic);
void syncResources();
gfx::Buffer* allocateBuffer();
)");
            CNTR(mDevice, mBufferSize, mDynamic);
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
            MEMBER_FUNCTIONS(R"(void init(gfx::Device* deviceIn, uint32_t sz, bool bDynamic);
gfx::Buffer* createFromCpuBuffer();
)");
        }

        STRUCT(ProgramResource, .mFlags = NO_COPY) {
            PUBLIC(
                ((ccstd::pmr::unordered_map<NameLocalID, UniformBlockResource>), mUniformBuffers, _)
                (DescriptorSetPool, mDescriptorSetPool, _)
            );
            MEMBER_FUNCTIONS(R"(void syncResources() noexcept;
)");
        }

        STRUCT(LayoutGraphNodeResource, .mFlags = NO_COPY) {
            PUBLIC(
                ((PmrFlatMap<NameLocalID, UniformBlockResource>), mUniformBuffers, _)
                (DescriptorSetPool, mDescriptorSetPool, _)
                ((PmrTransparentMap<ccstd::pmr::string, ProgramResource>), mProgramResources, _)
            );
            MEMBER_FUNCTIONS(R"(void syncResources() noexcept;
)");
        }

        STRUCT(QuadResource) {
            PUBLIC(
                (IntrusivePtr<gfx::Buffer>, mQuadVB, _)
                (IntrusivePtr<gfx::Buffer>, mQuadIB, _)
                (IntrusivePtr<gfx::InputAssembler>, mQuadIA, _)
            );
            CNTR(mQuadVB, mQuadIB, mQuadIA);
        }
        
        ENUM_CLASS(ResourceType) {
            ENUMS(STORAGE_BUFFER, STORAGE_IMAGE);
        }

        STRUCT(SceneResource, .mFlags = NO_COPY) {
            PUBLIC(
                ((ccstd::pmr::unordered_map<NameLocalID, ResourceType>), mResourceIndex, _)
                ((ccstd::pmr::unordered_map<NameLocalID, IntrusivePtr<gfx::Buffer>>), mStorageBuffers, _)
                ((ccstd::pmr::unordered_map<NameLocalID, IntrusivePtr<gfx::Texture>>), mStorageImages, _)
            );
        }

        // Render Context
        STRUCT(CullingKey, .mFlags = EQUAL | HASH_COMBINE) {
            PUBLIC(
                (const scene::Camera*, mCamera, nullptr)
                (const scene::Light*, mLight, nullptr)
                (bool, mCastShadow, false)
                (uint32_t, mLightLevel, 0xFFFFFFFF)
            );
        }

        STRUCT(CullingQueries) {
            PUBLIC(
                ((ccstd::pmr::unordered_map<CullingKey, uint32_t>), mCulledResultIndex, _)
            )
        }

        STRUCT(NativeRenderQueueDesc) {
            PUBLIC(
                (uint32_t, mCulledSource, 0xFFFFFFFF)
                (uint32_t, mRenderQueueTarget, 0xFFFFFFFF)
                (scene::LightType, mLightType, scene::LightType::UNKNOWN)
            );
            CNTR(mCulledSource, mRenderQueueTarget, mLightType);
        }

        STRUCT(SceneCulling, .mFlags = NO_COPY) {
            PUBLIC(
                ((ccstd::pmr::unordered_map<const scene::RenderScene*, CullingQueries>), mSceneQueries, _)
                (ccstd::pmr::vector<ccstd::vector<const scene::Model*>>, mCulledResults, _)
                (ccstd::pmr::vector<NativeRenderQueue>, mRenderQueues, _)
                ((PmrFlatMap<RenderGraph::vertex_descriptor, NativeRenderQueueDesc>), mSceneQueryIndex, _)
                (uint32_t, mNumCullingQueries, 0)
                (uint32_t, mNumRenderQueues, 0)
            );
            MEMBER_FUNCTIONS(R"(
void clear() noexcept;
void buildRenderQueues(const RenderGraph& rg, const LayoutGraphData& lg, const pipeline::PipelineSceneData& pplSceneData);
private:
uint32_t getOrCreateSceneCullingQuery(const SceneData& sceneData);
uint32_t createRenderQueue(SceneFlags sceneFlags, LayoutGraphData::vertex_descriptor subpassOrPassLayoutID);
void collectCullingQueries(const RenderGraph& rg, const LayoutGraphData& lg);
void batchCulling(const pipeline::PipelineSceneData& pplSceneData);
void fillRenderQueues(const RenderGraph& rg, const pipeline::PipelineSceneData& pplSceneData);
public:
)");
        }

        STRUCT(NativeRenderContext, .mFlags = NO_MOVE_NO_COPY | NO_DEFAULT_CNTR) {
            PUBLIC(
                (std::unique_ptr<gfx::DefaultResource>, mDefaultResource, _)
                (uint64_t, mNextFenceValue, 0)
                ((ccstd::pmr::map<uint64_t, ResourceGroup>), mResourceGroups, _)
                ((ccstd::pmr::vector<LayoutGraphNodeResource>), mLayoutGraphResources, _)
                ((ccstd::pmr::unordered_map<const scene::RenderScene*, SceneResource>), mRenderSceneResources, _)
                (QuadResource, mFullscreenQuad, _)
                (SceneCulling, mSceneCulling, _)
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
                (DescriptorSetLayoutData, mLocalLayoutData, _)
                (IntrusivePtr<gfx::DescriptorSetLayout>, mLocalDescriptorSetLayout, _)
                (IntrusivePtr<gfx::DescriptorSetLayout>, mEmptyDescriptorSetLayout, _)
                (IntrusivePtr<gfx::PipelineLayout>, mEmptyPipelineLayout, _)
                (PipelineRuntime*, mPipeline, nullptr)
                (gfx::Device*, mDevice, nullptr)
            );
            MEMBER_FUNCTIONS(R"(
void init(gfx::Device* deviceIn);
void setPipeline(PipelineRuntime* pipelineIn);
void destroy();
)");
        }

        STRUCT(PipelineCustomization) {
            PUBLIC(
                (std::shared_ptr<CustomPipelineContext>, mCurrentContext, _)
                ((PmrTransparentMap<ccstd::pmr::string, std::shared_ptr<CustomPipelineContext>>), mContexts, _)
                ((PmrTransparentMap<ccstd::pmr::string, std::shared_ptr<CustomRenderPass>>), mRenderPasses, _)
                ((PmrTransparentMap<ccstd::pmr::string, std::shared_ptr<CustomRenderSubpass>>), mRenderSubpasses, _)
                ((PmrTransparentMap<ccstd::pmr::string, std::shared_ptr<CustomComputeSubpass>>), mComputeSubpasses, _)
                ((PmrTransparentMap<ccstd::pmr::string, std::shared_ptr<CustomComputePass>>), mComputePasses, _)
                ((PmrTransparentMap<ccstd::pmr::string, std::shared_ptr<CustomRenderQueue>>), mRenderQueues, _)
                ((PmrTransparentMap<ccstd::pmr::string, std::shared_ptr<CustomRenderCommand>>), mRenderCommands, _)
            );
        }

        STRUCT(NativePipeline, .mFlags = CUSTOM_CNTR) {
            INHERITS(Pipeline);
            PUBLIC(
                (boost::container::pmr::unsynchronized_pool_resource, mUnsyncPool, _)
                //(boost::container::pmr::synchronized_pool_resource, mSyncPool, _)
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
                (ResourceGraph, mResourceGraph, _)
                (RenderGraph, mRenderGraph, _)
                (PipelineStatistics, mStatistics, _)
                (PipelineCustomization, mCustom, _)
            );
            MEMBER_FUNCTIONS(R"(
void executeRenderGraph(const RenderGraph& rg);

void addCustomContext(std::string_view name, std::shared_ptr<CustomPipelineContext> ptr);
void addCustomRenderPass(std::string_view name, std::shared_ptr<CustomRenderPass> ptr);
void addCustomRenderSubpass(std::string_view name, std::shared_ptr<CustomRenderSubpass> ptr);
void addCustomComputeSubpass(std::string_view name, std::shared_ptr<CustomComputeSubpass> ptr);
void addCustomComputePass(std::string_view name, std::shared_ptr<CustomComputePass> ptr);
void addCustomRenderQueue(std::string_view name, std::shared_ptr<CustomRenderQueue> ptr);
void addCustomRenderCommand(std::string_view name, std::shared_ptr<CustomRenderCommand> ptr);

void setCustomContext(std::string_view name);

private:
ccstd::vector<gfx::CommandBuffer*> _commandBuffers;

public:
)");
        }

        STRUCT(NativeProgramProxy) {
            INHERITS(ProgramProxy);
            PUBLIC(
                (IntrusivePtr<gfx::Shader>, mShader, _)
                (IntrusivePtr<gfx::PipelineState>, mPipelineState, _)
            );
            CNTR(mShader);
            CNTR(mShader, mPipelineState);
        }

        STRUCT(NativeRenderingModule) {
            INHERITS(RenderingModule);
            PUBLIC(
                (std::shared_ptr<NativeProgramLibrary>, mProgramLibrary, _)
            );
            CNTR(mProgramLibrary);
        }

        NAMESPACE_END(render);
        NAMESPACE_END(cc);
    }
}

}
