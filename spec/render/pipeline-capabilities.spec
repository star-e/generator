module PipelineCapabilities;
import Containers;

namespace cc::render {
    enum class PipelineType : uint8_t { BASIC, STANDARD }

    flags SubpassCapabilities : uint32_t {
        NONE = 0,
        INPUT_DEPTH_STENCIL = 1 << 0,
        INPUT_COLOR = 1 << 1,
        INPUT_COLOR_MRT = 1 << 2,
        HETEROGENEOUS_SAMPLE_COUNT = 1 << 3,
    }

    struct PipelineCapabilities {
        subpass: SubpassCapabilities = SubpassCapabilities::NONE;
    }

    interface RenderingModule {
        uint32_t getPassID(const ccstd::string& name) const;
        uint32_t getSubpassID(uint32_t passID, const ccstd::string& name) const;
        uint32_t getPhaseID(uint32_t subpassOrPassID, const ccstd::string& name) const;
    }
}

binding typescript {
    PipelineCapabilities => class { constructor(); };
}
