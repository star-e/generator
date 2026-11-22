extern module HostTypes;

namespace std {
    header "cstddef" { struct size_t; }
    header "cstdint" { struct int64_t; }
    header "string" { @string class string; }
    header "vector" { @container("array") template<T> class vector; }
    header "optional" { template<T> class optional; }
    header "variant" { template<First, Rest...> class variant; }
    header "map" { @container("map") template<K, V> class map; }
    header "set" { @container("set") template<T> class set; }
}

namespace std::filesystem {
    header "filesystem" { class path; }
}

namespace cc::codegen::graph {
    header "model/GraphSupport.h" { template<V> struct BidirectionalStorage; }
}
