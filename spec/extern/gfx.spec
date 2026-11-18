module Gfx;
namespace cc::gfx {
    enum class ResolveMode : uint32_t {
        NONE,
        SAMPLE_ZERO,
        AVERAGE,
        MIN,
        MAX,
    }
}
