#include "bindings/jswrapper/SeApi.h"
#include "cocos/renderer/pipeline/custom/RenderingModule.h"
#include "renderer/pipeline/custom/RenderCommonTypes.h"
#include "renderer/pipeline/custom/RenderInterfaceTypes.h"
#include <cstdlib>
#include <iostream>

namespace cc::render {
namespace {

struct PassCalls {
    unsigned count{};
    bool valid{true};

    template<class Record, class Check>
    void receive(const ccstd::vector<Record>& pairs, Check check) {
        ++count;
        if (count == 1) {
            valid = pairs.empty();
            return;
        }
        const auto* resource = boost::container::pmr::get_default_resource();
        valid = valid && count == 2 && pairs.size() == 2 &&
                pairs[0].get_allocator().resource() == resource &&
                pairs[1].get_allocator().resource() == resource && check(pairs[0], pairs[1]);
    }

    bool passed(const char* method) const {
        if (count == 2 && valid) return true;
        std::cerr << method << ": expected an empty array followed by two ordered PMR records\n";
        return false;
    }
};

template<class Record>
bool hasTargetRange(const Record& pair, uint32_t mips, uint32_t slices,
                    uint32_t firstMip, uint32_t firstSlice, uint32_t plane) {
    return pair.mipLevels == mips && pair.numSlices == slices && pair.targetMostDetailedMip == firstMip &&
           pair.targetFirstSlice == firstSlice && pair.targetPlaneSlice == plane;
}

class PassPipeline final : public Pipeline {
public:
    PassCalls copy, resolve, move, upload;

    void addCopyPass(const ccstd::vector<CopyPair>& pairs) override {
        copy.receive(pairs, [](const CopyPair& a, const CopyPair& b) {
            return a.source == "color" && a.target == "copy" && hasTargetRange(a, 2, 3, 4, 5, 6) &&
                   a.sourceMostDetailedMip == 7 && a.sourceFirstSlice == 8 && a.sourcePlaneSlice == 9 &&
                   b.source.empty() && b.target == "copy-defaults" && hasTargetRange(b, UINT32_MAX, UINT32_MAX, 0, 0, 0) &&
                   b.sourceMostDetailedMip == 0 && b.sourceFirstSlice == 0 && b.sourcePlaneSlice == 0;
        });
    }
    void addResolvePass(const ccstd::vector<ResolvePair>& pairs) override {
        resolve.receive(pairs, [](const ResolvePair& a, const ResolvePair& b) {
            return a.source == "msaa" && a.target == "resolved" &&
                   a.resolveFlags == (ResolveFlags::DEPTH | ResolveFlags::STENCIL) &&
                   a.mode == gfx::ResolveMode::AVERAGE && a.mode1 == gfx::ResolveMode::MAX &&
                   b.source.empty() && b.target == "resolve-defaults" && b.resolveFlags == ResolveFlags::NONE &&
                   b.mode == gfx::ResolveMode::SAMPLE_ZERO && b.mode1 == gfx::ResolveMode::SAMPLE_ZERO;
        });
    }
    void addMovePass(const ccstd::vector<MovePair>& pairs) override {
        move.receive(pairs, [](const MovePair& a, const MovePair& b) {
            return a.source == "history" && a.target == "move" && hasTargetRange(a, 2, 3, 4, 5, 6) &&
                   b.source.empty() && b.target == "move-defaults" && hasTargetRange(b, UINT32_MAX, UINT32_MAX, 0, 0, 0);
        });
    }
    void addUploadPass(ccstd::vector<UploadPair>& pairs) override {
        upload.receive(pairs, [](const UploadPair& a, const UploadPair& b) {
            return a.source == ccstd::vector<uint8_t>{0, 128, 255} && a.target == "upload" &&
                   hasTargetRange(a, 2, 3, 4, 5, 6) && b.source.empty() && b.target == "upload-defaults" &&
                   hasTargetRange(b, UINT32_MAX, UINT32_MAX, 0, 0, 0);
        });
    }

private:
    // Any other virtual call means the JS wrapper dispatched to the wrong method.
    bool activate(gfx::Swapchain*) override { std::abort(); }
    bool destroy() noexcept override { std::abort(); }
    void render(const ccstd::vector<scene::Camera*>&) override { std::abort(); }
    gfx::Device* getDevice() const override { std::abort(); }
    const cc::MacroRecord& getMacros() const override { std::abort(); }
    pipeline::GlobalDSManager* getGlobalDSManager() const override { std::abort(); }
    gfx::DescriptorSetLayout* getDescriptorSetLayout() const override { std::abort(); }
    gfx::DescriptorSet* getDescriptorSet() const override { std::abort(); }
    const ccstd::vector<gfx::CommandBuffer*>& getCommandBuffers() const override { std::abort(); }
    pipeline::PipelineSceneData* getPipelineSceneData() const override { std::abort(); }
    const ccstd::string& getConstantMacros() const override { std::abort(); }
    scene::Model* getProfiler() const override { std::abort(); }
    void setProfiler(scene::Model*) override { std::abort(); }
    pipeline::GeometryRenderer* getGeometryRenderer() const override { std::abort(); }
    float getShadingScale() const override { std::abort(); }
    void setShadingScale(float) override { std::abort(); }
    const ccstd::string& getMacroString(const ccstd::string&) const override { std::abort(); }
    int32_t getMacroInt(const ccstd::string&) const override { std::abort(); }
    bool getMacroBool(const ccstd::string&) const override { std::abort(); }
    void setMacroString(const ccstd::string&, const ccstd::string&) override { std::abort(); }
    void setMacroInt(const ccstd::string&, int32_t) override { std::abort(); }
    void setMacroBool(const ccstd::string&, bool) override { std::abort(); }
    void onGlobalPipelineStateChanged() override { std::abort(); }
    void setValue(const ccstd::string&, int32_t) override { std::abort(); }
    void setValue(const ccstd::string&, bool) override { std::abort(); }
    bool isOcclusionQueryEnabled() const override { std::abort(); }
    void resetRenderQueue(bool) override { std::abort(); }
    bool isRenderQueueReset() const override { std::abort(); }
    PipelineType getType() const override { std::abort(); }
    PipelineCapabilities getCapabilities() const override { std::abort(); }
    void beginSetup() override { std::abort(); }
    void endSetup() override { std::abort(); }
    bool getEnableCpuLightCulling() const override { std::abort(); }
    void setEnableCpuLightCulling(bool) override { std::abort(); }
    bool containsResource(const ccstd::string&) const override { std::abort(); }
    uint32_t addRenderWindow(const ccstd::string&, gfx::Format, uint32_t, uint32_t, scene::RenderWindow*, const ccstd::string&) override { std::abort(); }
    void updateRenderWindow(const ccstd::string&, scene::RenderWindow*, const ccstd::string&) override { std::abort(); }
    uint32_t addRenderTarget(const ccstd::string&, gfx::Format, uint32_t, uint32_t, ResourceResidency) override { std::abort(); }
    uint32_t addDepthStencil(const ccstd::string&, gfx::Format, uint32_t, uint32_t, ResourceResidency) override { std::abort(); }
    void updateRenderTarget(const ccstd::string&, uint32_t, uint32_t, gfx::Format) override { std::abort(); }
    void updateDepthStencil(const ccstd::string&, uint32_t, uint32_t, gfx::Format) override { std::abort(); }
    uint32_t addBuffer(const ccstd::string&, uint32_t, ResourceFlags, ResourceResidency) override { std::abort(); }
    void updateBuffer(const ccstd::string&, uint32_t) override { std::abort(); }
    uint32_t addExternalTexture(const ccstd::string&, gfx::Texture*, ResourceFlags) override { std::abort(); }
    void updateExternalTexture(const ccstd::string&, gfx::Texture*) override { std::abort(); }
    uint32_t addTexture(const ccstd::string&, gfx::TextureType, gfx::Format, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, gfx::SampleCount, ResourceFlags, ResourceResidency) override { std::abort(); }
    void updateTexture(const ccstd::string&, gfx::Format, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, gfx::SampleCount) override { std::abort(); }
    uint32_t addResource(const ccstd::string&, ResourceDimension, gfx::Format, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, gfx::SampleCount, ResourceFlags, ResourceResidency) override { std::abort(); }
    void updateResource(const ccstd::string&, gfx::Format, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, gfx::SampleCount) override { std::abort(); }
    void beginFrame() override { std::abort(); }
    void update(const scene::Camera*) override { std::abort(); }
    void endFrame() override { std::abort(); }
    void addBuiltinReflectionProbePass(const scene::Camera*) override { std::abort(); }
    gfx::DescriptorSetLayout* getDescriptorSetLayout(const ccstd::string&, UpdateFrequency) override { std::abort(); }
    void setMat4(const ccstd::string&, const cc::Mat4&) override { std::abort(); }
    void setQuaternion(const ccstd::string&, const cc::Quaternion&) override { std::abort(); }
    void setColor(const ccstd::string&, const gfx::Color&) override { std::abort(); }
    void setVec4(const ccstd::string&, const cc::Vec4&) override { std::abort(); }
    void setVec2(const ccstd::string&, const cc::Vec2&) override { std::abort(); }
    void setFloat(const ccstd::string&, float) override { std::abort(); }
    void setArrayBuffer(const ccstd::string&, const cc::ArrayBuffer*) override { std::abort(); }
    void setBuffer(const ccstd::string&, gfx::Buffer*) override { std::abort(); }
    void setTexture(const ccstd::string&, gfx::Texture*) override { std::abort(); }
    void setSampler(const ccstd::string&, gfx::Sampler*) override { std::abort(); }
    void setBuiltinCameraConstants(const scene::Camera*) override { std::abort(); }
    void setBuiltinDirectionalLightConstants(const scene::DirectionalLight*, const scene::Camera*) override { std::abort(); }
    void setBuiltinSphereLightConstants(const scene::SphereLight*, const scene::Camera*) override { std::abort(); }
    void setBuiltinSpotLightConstants(const scene::SpotLight*, const scene::Camera*) override { std::abort(); }
    void setBuiltinPointLightConstants(const scene::PointLight*, const scene::Camera*) override { std::abort(); }
    void setBuiltinRangedDirectionalLightConstants(const scene::RangedDirectionalLight*, const scene::Camera*) override { std::abort(); }
    void setBuiltinDirectionalLightFrustumConstants(const scene::Camera*, const scene::DirectionalLight*, uint32_t) override { std::abort(); }
    void setBuiltinSpotLightFrustumConstants(const scene::SpotLight*) override { std::abort(); }
    uint32_t addStorageBuffer(const ccstd::string&, gfx::Format, uint32_t, ResourceResidency) override { std::abort(); }
    uint32_t addStorageTexture(const ccstd::string&, gfx::Format, uint32_t, uint32_t, ResourceResidency) override { std::abort(); }
    uint32_t addShadingRateTexture(const ccstd::string&, uint32_t, uint32_t, ResourceResidency) override { std::abort(); }
    void updateStorageBuffer(const ccstd::string&, uint32_t, gfx::Format) override { std::abort(); }
    void updateStorageTexture(const ccstd::string&, uint32_t, uint32_t, gfx::Format) override { std::abort(); }
    void updateShadingRateTexture(const ccstd::string&, uint32_t, uint32_t) override { std::abort(); }
    RenderPassBuilder* addRenderPass(uint32_t, uint32_t, const ccstd::string&) override { std::abort(); }
    MultisampleRenderPassBuilder* addMultisampleRenderPass(uint32_t, uint32_t, uint32_t, uint32_t, const ccstd::string&) override { std::abort(); }
    ComputePassBuilder* addComputePass(const ccstd::string&) override { std::abort(); }
    uint32_t addCustomBuffer(const ccstd::string&, const gfx::BufferInfo&, const std::string&) override { std::abort(); }
    uint32_t addCustomTexture(const ccstd::string&, const gfx::TextureInfo&, const std::string&) override { std::abort(); }
};

PassPipeline& testPipeline() {
    static PassPipeline pipeline;
    return pipeline;
}

} // namespace

// The binding tests do not initialize a device or a native rendering module.
ProgramLibrary* getProgramLibrary() { return nullptr; }
RenderingModule* getRenderingModule() { return nullptr; }
RenderingModule* Factory::init(gfx::Device*, const ccstd::vector<unsigned char>&) { std::abort(); }
void Factory::destroy(RenderingModule*) noexcept { std::abort(); }
Pipeline* Factory::createPipeline() { return &testPipeline(); }

bool checkPassArrays(se::ScriptEngine& engine) {
    if (!engine.evalString(R"(
        (() => {
            const p = render.Factory.createPipeline();
            const range = {mipLevels: 2, numSlices: 3, targetMostDetailedMip: 4, targetFirstSlice: 5, targetPlaneSlice: 6};
            p.addCopyPass([]);
            p.addCopyPass([{...range, source: "color", target: "copy", sourceMostDetailedMip: 7,
                            sourceFirstSlice: 8, sourcePlaneSlice: 9}, {target: "copy-defaults"}]);
            p.addResolvePass([]);
            p.addResolvePass([{source: "msaa", target: "resolved", resolveFlags: 6, mode: 2, mode1: 4},
                              {target: "resolve-defaults"}]);
            p.addMovePass([]);
            p.addMovePass([{...range, source: "history", target: "move"}, {target: "move-defaults"}]);
            p.addUploadPass([]);
            const bytes = new Uint8Array([77, 0, 128, 255, 33]);
            p.addUploadPass([{...range, source: bytes.subarray(1, 4), target: "upload"}, {target: "upload-defaults"}]);
        })()
    )")) return false;

    const auto& p = testPipeline();
    return p.copy.passed("addCopyPass") && p.resolve.passed("addResolvePass") &&
           p.move.passed("addMovePass") && p.upload.passed("addUploadPass");
}

} // namespace cc::render
