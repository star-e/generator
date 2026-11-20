extern module Maps;
namespace cc {
    @pmr @container("map") template<K, V> class PmrTransparentMap;
    @pmr @container("map") template<K, V> class PmrUnorderedMap;
    @pmr @container("map") template<K, V> class PmrUnorderedStringMap;
    @pmr @container("map") template<K, V> class PmrFlatMap;
}
