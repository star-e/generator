module Customization;
import EngineTypes;
import RenderGraph;
import Gfx;
import PipelineExternals;

namespace cc::render {
    using RenderGraphVertex = RenderGraph::vertex_descriptor;

    interface Customization {
        virtual std::string_view getName() const noexcept = 0;
    }

    interface CustomPipelineContext : Customization {
        virtual IntrusivePtr<gfx::Buffer> createCustomBuffer(std::string_view type, const gfx::BufferInfo& info) = 0;
        virtual IntrusivePtr<gfx::Texture> createCustomTexture(std::string_view type, const gfx::TextureInfo& info) = 0;
        virtual void destroy() noexcept = 0;
    }

    struct CustomRenderGraphContext {
        pipelineContext: std::shared_ptr<CustomPipelineContext>;
        renderGraph: const RenderGraph* = nullptr;
        resourceGraph: const ResourceGraph* = nullptr;
        nullable primaryCommandBuffer: gfx::CommandBuffer*;
        nullable currentRenderPass: gfx::RenderPass*;
    }

    interface CustomRenderPass : Customization {
        virtual void beginRenderPass(const CustomRenderGraphContext& rg, RenderGraphVertex passID) = 0;
        virtual void endRenderPass(const CustomRenderGraphContext& rg, RenderGraphVertex passID) = 0;
    }

    interface CustomRenderSubpass : Customization {
        virtual void beginRenderSubpass(const CustomRenderGraphContext& rg, RenderGraphVertex passID) = 0;
        virtual void endRenderSubpass(const CustomRenderGraphContext& rg, RenderGraphVertex passID) = 0;
    }

    interface CustomComputeSubpass : Customization {
        virtual void beginComputeSubpass(const CustomRenderGraphContext& rg, RenderGraphVertex passID) = 0;
        virtual void endComputeSubpass(const CustomRenderGraphContext& rg, RenderGraphVertex passID) = 0;
    }

    interface CustomComputePass : Customization {
        virtual void beginComputePass(const CustomRenderGraphContext& rg, RenderGraphVertex passID) = 0;
        virtual void endComputePass(const CustomRenderGraphContext& rg, RenderGraphVertex passID) = 0;
    }

    interface CustomRenderQueue {
        virtual void beginRenderQueue(const CustomRenderGraphContext& rg, RenderGraphVertex passID) = 0;
        virtual void endRenderQueue(const CustomRenderGraphContext& rg, RenderGraphVertex passID) = 0;
    }

    interface CustomRenderCommand {
        virtual void beginRenderCommand(const CustomRenderGraphContext& rg, RenderGraphVertex passID) = 0;
        virtual void endRenderCommand(const CustomRenderGraphContext& rg, RenderGraphVertex passID) = 0;
    }
}
