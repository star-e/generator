#pragma once

#include "gfx-base/GFXDevice.h"
#include <cstdlib>

namespace cc::gfx::test {

class TestDevice : public Device {
private:
    DescriptorSetLayout* createDescriptorSetLayout() override { std::abort(); }
    DescriptorSet* createDescriptorSet() override { std::abort(); }
    bool doInit(const DeviceInfo&) override { std::abort(); }
    void doDestroy() override { std::abort(); }
    void frameSync() override { std::abort(); }
    void acquire(Swapchain* const*, uint32_t) override { std::abort(); }
    void present() override { std::abort(); }
    void copyBuffersToTexture(const uint8_t* const*, Texture*, const BufferTextureCopy*, uint32_t) override { std::abort(); }
    void copyTextureToBuffers(Texture*, uint8_t* const*, const BufferTextureCopy*, uint32_t) override { std::abort(); }
    void getQueryPoolResults(QueryPool*) override { std::abort(); }
    CommandBuffer* createCommandBuffer(const CommandBufferInfo&, bool) override { std::abort(); }
    Queue* createQueue() override { std::abort(); }
    QueryPool* createQueryPool() override { std::abort(); }
    Swapchain* createSwapchain() override { std::abort(); }
    Buffer* createBuffer() override { std::abort(); }
    Texture* createTexture() override { std::abort(); }
    Shader* createShader() override { std::abort(); }
    InputAssembler* createInputAssembler() override { std::abort(); }
    RenderPass* createRenderPass() override { std::abort(); }
    Framebuffer* createFramebuffer() override { std::abort(); }
    PipelineLayout* createPipelineLayout() override { std::abort(); }
    PipelineState* createPipelineState() override { std::abort(); }
};

} // namespace cc::gfx::test
