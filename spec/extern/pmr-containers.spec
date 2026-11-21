extern module PmrContainers;
namespace ccstd::pmr {
    header "cocos/base/std/container/{name}.h" {
        @pmr @container("array") template<T> class vector;
        @pmr @container("map") template<K, V> class map;
        @pmr @container("map") template<K, V> class unordered_map;
    }
}
