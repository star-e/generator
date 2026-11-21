extern module Containers;
namespace ccstd {
    header "cocos/base/std/container/{name}.h" {
        @container("map") template<K, V> class unordered_map;
        @string class string;
        @container("array") template<T> class vector;
        @container("map") template<K, V> class map;
    }
}
