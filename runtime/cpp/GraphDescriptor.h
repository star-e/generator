#pragma once

namespace cc::graph {

template<class Vertex>
struct EdgeDescriptor {
    Vertex source;
    Vertex target;

    constexpr bool operator==(const EdgeDescriptor& rhs) const noexcept(noexcept(source == rhs.source && target == rhs.target)) {
        return source == rhs.source && target == rhs.target;
    }
    constexpr bool operator!=(const EdgeDescriptor& rhs) const noexcept(noexcept(*this == rhs)) {
        return !(*this == rhs);
    }
};

} // namespace cc::graph
