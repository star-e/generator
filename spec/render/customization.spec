module Customization;
import RenderGraph;
import GfxResource;
import PipelineExternals;

namespace cc::render {
    interface Customization {
        std::string_view getName() const noexcept;
    }

    interface CustomPipelineContext : Customization {
        IntrusivePtr<gfx::Buffer> createCustomBuffer(std::string_view type, const gfx::BufferInfo& info);
        IntrusivePtr<gfx::Texture> createCustomTexture(std::string_view type, const gfx::TextureInfo& info);
        void destroy() noexcept;
    }

    struct CustomRenderGraphContext {
        pipelineContext: std::shared_ptr<CustomPipelineContext>;
        renderGraph: const RenderGraph* = nullptr;
        resourceGraph: const ResourceGraph* = nullptr;
        nullable primaryCommandBuffer: gfx::CommandBuffer*;
        nullable currentRenderPass: gfx::RenderPass*;
    }

    interface CustomRenderPass : Customization {
        void beginRenderPass(const CustomRenderGraphContext& rg, RenderGraph::Vertex passID);
        void endRenderPass(const CustomRenderGraphContext& rg, RenderGraph::Vertex passID);
    }

    interface CustomRenderSubpass : Customization {
        void beginRenderSubpass(const CustomRenderGraphContext& rg, RenderGraph::Vertex passID);
        void endRenderSubpass(const CustomRenderGraphContext& rg, RenderGraph::Vertex passID);
    }

    interface CustomComputeSubpass : Customization {
        void beginComputeSubpass(const CustomRenderGraphContext& rg, RenderGraph::Vertex passID);
        void endComputeSubpass(const CustomRenderGraphContext& rg, RenderGraph::Vertex passID);
    }

    interface CustomComputePass : Customization {
        void beginComputePass(const CustomRenderGraphContext& rg, RenderGraph::Vertex passID);
        void endComputePass(const CustomRenderGraphContext& rg, RenderGraph::Vertex passID);
    }

    interface CustomRenderQueue {
        void beginRenderQueue(const CustomRenderGraphContext& rg, RenderGraph::Vertex passID);
        void endRenderQueue(const CustomRenderGraphContext& rg, RenderGraph::Vertex passID);
    }

    interface CustomRenderCommand {
        void beginRenderCommand(const CustomRenderGraphContext& rg, RenderGraph::Vertex passID);
        void endRenderCommand(const CustomRenderGraphContext& rg, RenderGraph::Vertex passID);
    }
}
