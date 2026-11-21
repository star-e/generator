extern module Sets;
cpp { header "cocos/renderer/pipeline/custom/details/Set.h"; }

namespace cc {
    @pmr @container("set") template<T> class PmrFlatSet;
    @pmr @container("set") template<T> class PmrUnorderedSet;
    @pmr @container("set") template<T> class PmrTransparentSet;
}
