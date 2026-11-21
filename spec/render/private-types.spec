module PrivateTypes;
import LayoutGraph;
import PrivateInterface;
import PipelineExternals;
import Containers;
import PmrContainers;
import Strings;
import Maps;
import Gfx;
import EngineTypes;

namespace cc::render {
    struct ProgramInfo {
        readonly programInfo: IProgramInfo;
        readonly shaderInfo: gfx::ShaderInfo;
        readonly attributes: ccstd::pmr::vector<gfx::Attribute>;
        readonly blockSizes: ccstd::vector<int>;
        readonly handleMap: ccstd::unordered_map<ccstd::string, uint32_t>;
        @cppOnly constructor();
        constructor(programInfo, shaderInfo, attributes, blockSizes, handleMap);
    }

    struct ProgramGroup {
        readonly programInfos: PmrTransparentMap<ccstd::pmr::string, ProgramInfo>;
        readonly programProxies: PmrFlatMap<ccstd::pmr::string, IntrusivePtr<ProgramProxy>>;
    }
}
