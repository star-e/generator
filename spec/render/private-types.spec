module PrivateTypes;
import LayoutGraph;
import PrivateInterface;
import PipelineExternals;
import Containers;
import PmrContainers;
import Strings;
import Maps;
import Gfx;

namespace cc::render {
    @requireAllocator struct ProgramInfo {
        readonly programInfo: IProgramInfo;
        readonly shaderInfo: gfx::ShaderInfo;
        readonly attributes: ccstd::pmr::vector<gfx::Attribute>;
        readonly blockSizes: ccstd::vector<int>;
        readonly handleMap: ccstd::unordered_map<ccstd::string, uint32_t>;
        constructor(programInfo, shaderInfo, attributes, blockSizes, handleMap) required;
    }

    @requireAllocator struct ProgramGroup {
        readonly programInfos: PmrTransparentMap<ccstd::pmr::string, ProgramInfo>;
        readonly programProxies: PmrFlatMap<ccstd::pmr::string, IntrusivePtr<ProgramProxy>>;
    }
}

binding typescript {
    ProgramGroup => class { constructor(); };
}
