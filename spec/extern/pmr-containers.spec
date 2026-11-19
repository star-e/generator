extern module PmrContainers;
namespace ccstd::pmr {
    @pmr @container("array") template<T> class vector;
    @pmr @container("map") template<K, V> class map;
}
