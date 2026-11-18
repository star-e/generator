extern module CocosModelSupport;
@integer(signed=false, bits=32) struct uint32_t;
@integer(signed=true, bits=64) struct int64_t;
@integer(signed=false, bits=64) struct uint64_t;
namespace std {
    @string @comparison("==") struct string;
    @container("array") @comparison("==") template<T> class vector;
    @container("optional") template<T> class optional;
    @container("variant") @comparison("==") template<Ts...> class variant;
    @container("map") template<K,V> class map;
}
namespace cc {
    namespace codegen {
        struct SourceLocation;
    }
}
