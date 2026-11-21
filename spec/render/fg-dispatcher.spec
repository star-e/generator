module FGDispatcher;
import Gfx;
import RenderGraph;
import LayoutGraph;
import Containers;
import Strings;
import PmrContainers;
import Maps;
import Sets;

namespace cc::render {
    struct NullTag {}

    struct ResourceLifeRecord {
        start: uint32_t = 0;
        end: uint32_t = 0;
    }

    struct LeafStatus {
        isExternal: bool = false;
        needCulling: bool = false;
    }

    struct AccessStatus {
        accessFlag: gfx::AccessFlagBit = gfx::AccessFlagBit::NONE;
        range: gfx::ResourceRange;
    }

    struct ResourceAccessNode {
        resourceStatus: PmrFlatMap<ccstd::pmr::string, AccessStatus>;
    }

    struct LayoutAccess {
        prevAccess: gfx::AccessFlagBit = gfx::AccessFlagBit::NONE;
        nextAccess: gfx::AccessFlagBit = gfx::AccessFlagBit::NONE;
    }

    struct AttachmentInfo {
        parentName: ccstd::pmr::string;
        attachmentIndex: uint32_t = 0;
        isResolveView: uint32_t = 0;
    }

    struct FGRenderPassInfo {
        colorAccesses: ccstd::vector<LayoutAccess>;
        dsAccess: LayoutAccess;
        dsResolveAccess: LayoutAccess;
        rpInfo: gfx::RenderPassInfo;
        orderedViews: ccstd::pmr::vector<ccstd::pmr::string>;
        viewIndex: PmrTransparentMap<ccstd::pmr::string, AttachmentInfo>;
        resolveCount: uint32_t = 0;
        uniqueRasterViewCount: uint32_t = 0;
    }

    struct Barrier {
        resourceID: ResourceGraph::Vertex = 0xFFFFFFFF;
        type: gfx::BarrierType = gfx::BarrierType::FULL;
        nullable barrier: gfx::GFXObject*;
        beginVert: RenderGraph::Vertex = 0xFFFFFFFF;
        endVert: RenderGraph::Vertex = 0xFFFFFFFF;
        beginStatus: AccessStatus;
        endStatus: AccessStatus;
    }

    struct BarrierNode {
        frontBarriers: ccstd::vector<Barrier>;
        rearBarriers: ccstd::vector<Barrier>;
    }

    struct SliceNode {
        full: bool = false;
        mips: ccstd::vector<uint32_t>;
    }

    struct TextureNode {
        full: bool = false;
        slices: ccstd::vector<SliceNode>;
    }

    struct ResourceNode {
        full: bool = false;
        planes: ccstd::vector<TextureNode>;
    }

    @tag struct PassID_ {}
    @tag struct PassNode_ {}
    @tag struct RenderPassInfo_ {}
    @tag struct Barrier_ {}
    @tag struct DescID_ {}

    @immovable graph ResourceAccessGraph {
        vertex_descriptor Vertex = uint32_t;
        vertex_storage: indexed;
        vertex_list_graph;
        mutable_graph;
        incidence_graph { out_edges: vector; }
        bidirectional_graph { in_edges: vector; }
        component PassID_: RenderGraph::Vertex { member: passID; }
        component PassNode_: ResourceAccessNode { member: passResource; }
        component RenderPassInfo_: FGRenderPassInfo { member: rpInfo; }
        component Barrier_: BarrierNode { member: barrier; }
        component_index { component: PassID_; }

        resourceNames: ccstd::pmr::vector<ccstd::pmr::string>;
        resourceIndex: PmrUnorderedStringMap<ccstd::pmr::string, uint32_t>;
        presentPassID: Vertex = 0xFFFFFFFF;
        leafPasses: PmrFlatMap<Vertex, LeafStatus>;
        culledPasses: PmrFlatSet<Vertex>;
        resourceLifeRecord: PmrFlatMap<ccstd::pmr::string, ResourceLifeRecord>;
        topologicalOrder: ccstd::pmr::vector<Vertex>;
        resourceAccess: PmrTransparentMap<ccstd::pmr::string, PmrFlatMap<uint32_t, AccessStatus>>;
        movedTarget: PmrFlatMap<ccstd::pmr::string, PmrFlatMap<ccstd::pmr::string, ccstd::pmr::string>>;
        movedSourceStatus: PmrFlatMap<ccstd::pmr::string, AccessStatus>;
        movedTargetStatus: PmrFlatMap<ccstd::pmr::string, ResourceNode>;

        Vertex getCurrentID() const noexcept { return num_vertices(); }
        boost::container::pmr::memory_resource* resource() const noexcept { return get_allocator().resource(); }
        LayoutAccess getAccess(ccstd::pmr::string name, RenderGraph::Vertex vertID);
    }

    @immovable graph RelationGraph {
        vertex_descriptor Vertex = uint32_t;
        vertex_storage: indexed;
        vertex_list_graph;
        mutable_graph { remove_vertex; }
        incidence_graph { out_edges: vector; }
        bidirectional_graph { in_edges: vector; }
        component DescID_: ResourceAccessGraph::Vertex { member: descID; }
        component_index { component: DescID_; }

        Vertex getCurrentID() const noexcept { return num_vertices(); }
        boost::container::pmr::memory_resource* resource() const noexcept { return get_allocator().resource(); }
    }

    struct RenderingInfo {
        renderpassInfo: gfx::RenderPassInfo;
        framebufferInfo: gfx::FramebufferInfo;
        clearColors: ccstd::pmr::vector<gfx::Color>;
        clearDepth: float = 0;
        clearStencil: uint8_t = 0;
    }

    @immovable struct FrameGraphDispatcher {
        resourceAccessGraph: ResourceAccessGraph;
        resourceGraph: ResourceGraph&;
        renderGraph: const RenderGraph&;
        layoutGraph: const LayoutGraphData&;
        nullable scratch: boost::container::pmr::memory_resource*;
        relationGraph: RelationGraph;
        _enablePassReorder: bool = false;
        _enableAutoBarrier: bool = true;
        _enableMemoryAliasing: bool = false;
        _accessGraphBuilt: bool = false;
        _paralellExecWeight: float = 0;

        constructor(resourceGraph, renderGraph, layoutGraph, scratch);

        void enablePassReorder(bool enable);
        void setParalellWeight(float paralellExecWeight);
        void enableMemoryAliasing(bool enable);
        void run();
        const BarrierNode& getBarrier(RenderGraph::Vertex u) const;
        const ResourceAccessNode& getAccessNode(RenderGraph::Vertex u) const;
        const gfx::RenderPassInfo& getRenderPassInfo(RenderGraph::Vertex u) const;
        RenderingInfo getRenderPassAndFrameBuffer(RenderGraph::Vertex u, const ResourceGraph& resg) const;
        LayoutAccess getResourceAccess(ResourceGraph::Vertex r, RenderGraph::Vertex p) const;
        ResourceGraph::Vertex realResourceID(const ccstd::pmr::string& name) const;
        PmrFlatMap<NameLocalID, ResourceGraph::Vertex> buildDescriptorIndex(
            const PmrTransparentMap<ccstd::pmr::string, ccstd::pmr::vector<ComputeView>>& computeViews,
            const PmrTransparentMap<ccstd::pmr::string, RasterView>& rasterViews,
            boost::container::pmr::memory_resource* scratch) const;
        PmrFlatMap<NameLocalID, ResourceGraph::Vertex> buildDescriptorIndex(
            const PmrTransparentMap<ccstd::pmr::string, ccstd::pmr::vector<ComputeView>>& computeViews,
            boost::container::pmr::memory_resource* scratch) const;
        void buildDescriptorIndex(
            const PmrTransparentMap<ccstd::pmr::string, ccstd::pmr::vector<ComputeView>>& computeViews,
            const PmrTransparentMap<ccstd::pmr::string, RasterView>& rasterViews,
            PmrFlatMap<NameLocalID, ResourceGraph::Vertex>& resourceIndex) const;
        void buildDescriptorIndex(
            const PmrTransparentMap<ccstd::pmr::string, ccstd::pmr::vector<ComputeView>>& computeViews,
            PmrFlatMap<NameLocalID, ResourceGraph::Vertex>& resourceIndex) const;
    }
}
