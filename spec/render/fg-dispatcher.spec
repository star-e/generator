module FGDispatcher;
import Gfx;
import GfxResource;
import RenderGraph;
import Containers;
import Strings;
import PmrContainers;
import Maps;

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

    @requireAllocator struct ResourceAccessNode {
        resourceStatus: PmrFlatMap<ccstd::pmr::string, AccessStatus>;
    }

    struct LayoutAccess {
        prevAccess: gfx::AccessFlagBit = gfx::AccessFlagBit::NONE;
        nextAccess: gfx::AccessFlagBit = gfx::AccessFlagBit::NONE;
    }

    @requireAllocator struct AttachmentInfo {
        parentName: ccstd::pmr::string;
        attachmentIndex: uint32_t = 0;
        isResolveView: uint32_t = 0;
    }

    @requireAllocator struct FGRenderPassInfo {
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

    @requireAllocator struct RenderingInfo {
        renderpassInfo: gfx::RenderPassInfo;
        framebufferInfo: gfx::FramebufferInfo;
        clearColors: ccstd::pmr::vector<gfx::Color>;
        clearDepth: float = 0;
        clearStencil: uint8_t = 0;
    }
}
