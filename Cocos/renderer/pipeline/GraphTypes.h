#pragma once
#include <list>
#include <memory>
#include <type_traits>
#include <boost/optional.hpp>
#include <boost/iterator/iterator_adaptor.hpp>
#include <boost/container/pmr/polymorphic_allocator.hpp>

namespace boost {

struct use_default;

struct directed_tag;
struct undirected_tag;
struct bidirectional_tag;

struct no_property;

}

namespace cc {

template <class T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

template<class T>
using PmrList = std::list<T, boost::container::pmr::polymorphic_allocator<T>>;

namespace Impl {

struct path_t {} static constexpr path;

//--------------------------------------------------------------------
// EdgeDescriptor
//--------------------------------------------------------------------
template<class DirectedCategory, class VertexDescriptor>
struct EdgeDescriptor {
    EdgeDescriptor() = default;
    EdgeDescriptor(VertexDescriptor s, VertexDescriptor t) noexcept
        : m_source(s), m_target(t)
    {}

    void expectsNoProperty() const noexcept {
        // Expects(false);
    }
    VertexDescriptor m_source = (VertexDescriptor)-1;
    VertexDescriptor m_target = (VertexDescriptor)-1;
};

template<class VertexDescriptor>
inline bool operator==(
    const EdgeDescriptor<boost::directed_tag, VertexDescriptor>& lhs,
    const EdgeDescriptor<boost::directed_tag, VertexDescriptor>& rhs
) noexcept {
    return
        lhs.m_source == rhs.m_source &&
        lhs.m_target == rhs.m_target;
}

template<class VertexDescriptor>
inline bool operator==(
    const EdgeDescriptor<boost::bidirectional_tag, VertexDescriptor>& lhs,
    const EdgeDescriptor<boost::bidirectional_tag, VertexDescriptor>& rhs
) noexcept {
    return
        lhs.m_source == rhs.m_source &&
        lhs.m_target == rhs.m_target;
}

template<class VertexDescriptor>
inline bool operator!=(
    const EdgeDescriptor<boost::directed_tag, VertexDescriptor>& lhs,
    const EdgeDescriptor<boost::directed_tag, VertexDescriptor>& rhs
    ) noexcept {
    return !(lhs == rhs);
}

template<class VertexDescriptor>
inline bool operator!=(
    const EdgeDescriptor<boost::bidirectional_tag, VertexDescriptor>& lhs,
    const EdgeDescriptor<boost::bidirectional_tag, VertexDescriptor>& rhs
    ) noexcept {
    return !(lhs == rhs);
}

template<class DirectedCategory, class VertexDescriptor>
struct EdgeDescriptorWithProperty : EdgeDescriptor<DirectedCategory, VertexDescriptor> {
    using property_type = void;

    EdgeDescriptorWithProperty() = default;
    EdgeDescriptorWithProperty(VertexDescriptor s, VertexDescriptor t, const property_type* p) noexcept
        : EdgeDescriptor<DirectedCategory, VertexDescriptor>(s, t)
        , mEdgeProperty(const_cast<property_type*>(p))
    {}

    property_type* get_property() const noexcept {
        return mEdgeProperty;
    }

    //std::strong_ordering operator<=>(const EdgeDescriptorWithProperty& rhs) const noexcept {
    //    return mEdgeProperty <=> rhs.mEdgeProperty;
    //}
    //bool operator==(const EdgeDescriptorWithProperty& rhs) const noexcept {
    //    return mEdgeProperty == rhs.mEdgeProperty;
    //}
    property_type* mEdgeProperty = nullptr;
};

template<class DirectedCategory, class VertexDescriptor>
inline bool operator==(
    const EdgeDescriptorWithProperty<DirectedCategory, VertexDescriptor>& lhs,
    const EdgeDescriptorWithProperty<DirectedCategory, VertexDescriptor>& rhs
) noexcept {
    return lhs.mEdgeProperty == rhs.mEdgeProperty;
}

template<class DirectedCategory, class VertexDescriptor>
inline bool operator!=(
    const EdgeDescriptorWithProperty<DirectedCategory, VertexDescriptor>& lhs,
    const EdgeDescriptorWithProperty<DirectedCategory, VertexDescriptor>& rhs
    ) noexcept {
    return !(lhs == rhs);
}

template<class DirectedCategory, class VertexDescriptor>
inline bool operator<(
    const EdgeDescriptorWithProperty<DirectedCategory, VertexDescriptor>& lhs,
    const EdgeDescriptorWithProperty<DirectedCategory, VertexDescriptor>& rhs
) noexcept {
    return lhs.mEdgeProperty < rhs.mEdgeProperty;
}

//--------------------------------------------------------------------
// StoredEdge
//--------------------------------------------------------------------
template<class VertexDescriptor>
class StoredEdge {
public:
    StoredEdge(VertexDescriptor target) noexcept
        : m_target(target)
    {}
    const VertexDescriptor& get_target() const noexcept {
        return m_target;
    }
    VertexDescriptor& get_target() noexcept {
        return m_target;
    }
    //auto operator<=>(const StoredEdge&) const noexcept = default;

    VertexDescriptor m_target;
};

template<class VertexDescriptor>
inline bool operator==(
    const StoredEdge<VertexDescriptor>& lhs,
    const StoredEdge<VertexDescriptor>& rhs
) noexcept {
    return lhs.m_target == rhs.m_target;
}

template<class VertexDescriptor>
inline bool operator!=(
    const StoredEdge<VertexDescriptor>& lhs,
    const StoredEdge<VertexDescriptor>& rhs
) noexcept {
    return !(lhs == rhs);
}

template<class VertexDescriptor>
inline bool operator<(
    const StoredEdge<VertexDescriptor>& lhs,
    const StoredEdge<VertexDescriptor>& rhs
) noexcept {
    return lhs.m_target < rhs.m_target;
}

template<class VertexDescriptor, class EdgeProperty>
class StoredEdgeWithProperty : public StoredEdge<VertexDescriptor> {
public:
    StoredEdgeWithProperty(VertexDescriptor target, const EdgeProperty& p)
        : StoredEdge<VertexDescriptor>(target)
        , mProperty(new EdgeProperty(p))
    {}
    StoredEdgeWithProperty(VertexDescriptor target, std::unique_ptr<EdgeProperty>&& ptr)
        : StoredEdge<VertexDescriptor>(target)
        , mProperty(std::move(ptr))
    {}
    StoredEdgeWithProperty(VertexDescriptor target)
        : StoredEdge<VertexDescriptor>(target)
    {}

    StoredEdgeWithProperty(StoredEdgeWithProperty&&) = default;
    StoredEdgeWithProperty& operator=(StoredEdgeWithProperty&&) = default;

    EdgeProperty& get_property() noexcept {
        Expects(mProperty);
        return *mProperty;
    }
    const EdgeProperty& get_property() const noexcept {
        Expects(mProperty);
        return *mProperty;
    }
    std::unique_ptr<EdgeProperty> mProperty;
};

template<class VertexDescriptor, class EdgeListIter, class EdgeProperty = boost::no_property>
class StoredEdgeWithEdgeIter : public StoredEdge<VertexDescriptor> {
public:
    StoredEdgeWithEdgeIter(VertexDescriptor v, EdgeListIter iter) noexcept
        : StoredEdge<VertexDescriptor>(v)
        , mEdgeListIter(iter)
    {}
    StoredEdgeWithEdgeIter(VertexDescriptor v) noexcept
        : StoredEdge<VertexDescriptor>(v)
    {}

    EdgeListIter get_iter() const noexcept {
        return mEdgeListIter;
    }
    EdgeProperty& get_property() noexcept {
        return this->mEdgeListIter->get_property();
    }
    const EdgeProperty& get_property() const noexcept {
        return this->mEdgeListIter->get_property();
    }
protected:
    EdgeListIter mEdgeListIter = {};
};

template<class VertexDescriptor, class EdgeVec, class EdgeProperty = boost::no_property>
class StoredEdgeWithRandomAccessEdgeIter : public StoredEdge<VertexDescriptor> {
public:
    StoredEdgeWithRandomAccessEdgeIter(
        VertexDescriptor v, typename EdgeVec::iterator i, EdgeVec* edge_vec
    ) noexcept
        : StoredEdge<VertexDescriptor>(v)
        , mID(i - edge_vec->begin())
        , mVector(edge_vec)
    {}

    typename EdgeVec::iterator get_iter() const noexcept {
        Expects(mVector);
        return mVector->begin() + mID;
    }
    EdgeProperty& get_property() noexcept {
        Expects(this->mVector);
        return (*this->mVector)[this->mID].get_property();
    }
    const EdgeProperty& get_property() const noexcept {
        Expects(this->mVector);
        return (*this->mVector)[this->mID].get_property();
    }
protected:
    size_t mID = (size_t)-1;
    EdgeVec* mVector = nullptr;
};

//--------------------------------------------------------------------
// VertexIterator
//--------------------------------------------------------------------
template<class BaseIter, class VertexDescriptor, class Difference>
struct VertexIter : boost::iterator_adaptor<
    VertexIter<BaseIter, VertexDescriptor, Difference>,
    BaseIter, VertexDescriptor, boost::use_default, VertexDescriptor, Difference
> {
    using Base = boost::iterator_adaptor<
        VertexIter<BaseIter, VertexDescriptor, Difference>,
        BaseIter, VertexDescriptor, boost::use_default, VertexDescriptor, Difference
    >;

    VertexIter() = default;
    VertexIter(const BaseIter& i) noexcept
        : Base(i) {}

    VertexDescriptor dereference() const noexcept {
        return VertexDescriptor{ &(*this->base()) };
    }
};

template<class BaseIter, class VertexDescriptor, class Difference>
struct VertexMapPtrIter : boost::iterator_adaptor<
    VertexMapPtrIter<BaseIter, VertexDescriptor, Difference>,
    BaseIter, VertexDescriptor, boost::use_default, VertexDescriptor, Difference
> {
    using Base = boost::iterator_adaptor<
        VertexMapPtrIter<BaseIter, VertexDescriptor, Difference>,
        BaseIter, VertexDescriptor, boost::use_default, VertexDescriptor, Difference
    >;

    VertexMapPtrIter() = default;
    VertexMapPtrIter(const BaseIter& i) noexcept
        : Base(i) {}

    VertexDescriptor dereference() const noexcept {
        return VertexDescriptor{ this->base()->second.get() };
    }
};

//--------------------------------------------------------------------
// OutEdgeIterator
//--------------------------------------------------------------------
template<class BaseIter, class VertexDescriptor, class EdgeDescriptor, class Difference>
struct OutEdgeIter : boost::iterator_adaptor<
    OutEdgeIter<BaseIter, VertexDescriptor, EdgeDescriptor, Difference>,
    BaseIter, EdgeDescriptor, boost::use_default, EdgeDescriptor, Difference
> {
    using Base = boost::iterator_adaptor<
        OutEdgeIter<BaseIter, VertexDescriptor, EdgeDescriptor, Difference>,
        BaseIter, EdgeDescriptor, boost::use_default, EdgeDescriptor, Difference
    >;

    OutEdgeIter() = default;
    OutEdgeIter(const BaseIter& i, const VertexDescriptor& src) noexcept
        : Base(i), mSource(src)
    {}

    EdgeDescriptor dereference() const noexcept {
        // this->base() return out edge list iterator
        return EdgeDescriptor{
            mSource, (*this->base()).get_target()
        };
    }
    VertexDescriptor mSource = {};
};

template<class BaseIter, class VertexDescriptor, class EdgeDescriptor, class Difference>
struct OutPropertyEdgeIter : boost::iterator_adaptor<
    OutPropertyEdgeIter<BaseIter, VertexDescriptor, EdgeDescriptor, Difference>,
    BaseIter, EdgeDescriptor, boost::use_default, EdgeDescriptor, Difference
> {
    using Base = boost::iterator_adaptor<
        OutPropertyEdgeIter<BaseIter, VertexDescriptor, EdgeDescriptor, Difference>,
        BaseIter, EdgeDescriptor, boost::use_default, EdgeDescriptor, Difference
    >;

    OutPropertyEdgeIter() = default;
    OutPropertyEdgeIter(const BaseIter& i, const VertexDescriptor& src) noexcept
        : Base(i), mSource(src)
    {}

    EdgeDescriptor dereference() const noexcept {
        // this->base() return out edge list iterator
        return EdgeDescriptor{
            mSource, (*this->base()).get_target(), &(*this->base()).get_property()
        };
    }
    VertexDescriptor mSource = {};
};

//--------------------------------------------------------------------
// InEdgeIterator
//--------------------------------------------------------------------
template<class BaseIter, class VertexDescriptor, class EdgeDescriptor, class Difference>
struct InEdgeIter : boost::iterator_adaptor<
    InEdgeIter<BaseIter, VertexDescriptor, EdgeDescriptor, Difference>,
    BaseIter, EdgeDescriptor, boost::use_default, EdgeDescriptor, Difference
> {
    using Base = boost::iterator_adaptor<
        InEdgeIter<BaseIter, VertexDescriptor, EdgeDescriptor, Difference>,
        BaseIter, EdgeDescriptor, boost::use_default, EdgeDescriptor, Difference
    >;

    InEdgeIter() = default;
    InEdgeIter(const BaseIter& i, const VertexDescriptor& src) noexcept
        : Base(i), mSource(src)
    {}

    EdgeDescriptor dereference() const noexcept {
        return EdgeDescriptor{
            (*this->base()).get_target(), mSource
        };
    }
    VertexDescriptor mSource = {};
};

template<class BaseIter, class VertexDescriptor, class EdgeDescriptor, class Difference>
struct InPropertyEdgeIter : boost::iterator_adaptor<
    InPropertyEdgeIter<BaseIter, VertexDescriptor, EdgeDescriptor, Difference>,
    BaseIter, EdgeDescriptor, boost::use_default, EdgeDescriptor, Difference
> {
    using Base = boost::iterator_adaptor<
        InPropertyEdgeIter<BaseIter, VertexDescriptor, EdgeDescriptor, Difference>,
        BaseIter, EdgeDescriptor, boost::use_default, EdgeDescriptor, Difference
    >;

    InPropertyEdgeIter() = default;
    InPropertyEdgeIter(const BaseIter& i, const VertexDescriptor& src) noexcept
        : Base(i), mSource(src)
    {}

    EdgeDescriptor dereference() const noexcept {
        return EdgeDescriptor{
            (*this->base()).get_target(), mSource, &this->base()->get_property()
        };
    }
    VertexDescriptor mSource = {};
};

//--------------------------------------------------------------------
// EdgeIterator
//--------------------------------------------------------------------
// UndirectedEdgeIter (Bidirectional || Undirected)
template<class EdgeIter, class EdgeDescriptor, class Difference>
struct UndirectedEdgeIter : boost::iterator_adaptor<
    UndirectedEdgeIter<EdgeIter, EdgeDescriptor, Difference>,
    EdgeIter, EdgeDescriptor, boost::use_default, EdgeDescriptor, Difference
> {
    using Base = boost::iterator_adaptor<
        UndirectedEdgeIter<EdgeIter, EdgeDescriptor, Difference>,
        EdgeIter, EdgeDescriptor, boost::use_default, EdgeDescriptor, Difference
    >;

    UndirectedEdgeIter() = default;
    explicit UndirectedEdgeIter(EdgeIter i) noexcept : Base(i) {}

    EdgeDescriptor dereference() const noexcept {
        return EdgeDescriptor{
            (*this->base()).m_source, (*this->base()).m_target, &this->base()->get_property()
        };
    }
};

// DirectedEdgeIterator
template<class VertexIterator, class OutEdgeIterator, class Graph>
class DirectedEdgeIterator {
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = typename OutEdgeIterator::value_type;
    using reference = typename OutEdgeIterator::reference;
    using pointer = typename OutEdgeIterator::pointer;
    using difference_type = typename OutEdgeIterator::difference_type;
    using distance_type = difference_type;

    DirectedEdgeIterator() = default;
    template<class G>
    DirectedEdgeIterator(VertexIterator b, VertexIterator c, VertexIterator e, const G& g) noexcept
        : mBegin(b), mCurr(c), mEnd(e), mG(&g)
    {
        if (mCurr != mEnd) {
            while (mCurr != mEnd && out_degree(*mCurr, *mG) == 0)
                ++mCurr;
            if (mCurr != mEnd)
                mEdges = out_edges(*mCurr, *mG);
        }
    }

    DirectedEdgeIterator& operator++() noexcept {
        ++mEdges->first;
        if (mEdges->first == mEdges->second) {
            ++mCurr;
            while (mCurr != mEnd && out_degree(*mCurr, *mG) == 0)
                ++mCurr;
            if (mCurr != mEnd)
                mEdges = out_edges(*mCurr, *mG);
        }
        return *this;
    }
    DirectedEdgeIterator operator++(int) noexcept {
        DirectedEdgeIterator tmp = *this;
        ++(*this);
        return tmp;
    }
    value_type operator*() const noexcept {
        return *mEdges->first;
    }
    bool operator==(const DirectedEdgeIterator& x) const noexcept {
        return mCurr == x.mCurr && (mCurr == mEnd || mEdges->first == x.mEdges->first);
    }
    bool operator!=(const DirectedEdgeIterator& x) const noexcept {
        return mCurr != x.mCurr || (mCurr != mEnd && mEdges->first != x.mEdges->first);
    }
protected:
    VertexIterator mBegin = {};
    VertexIterator mCurr = {};
    VertexIterator mEnd = {};
    boost::optional<std::pair<OutEdgeIterator, OutEdgeIterator>> mEdges;
    const Graph* mG = nullptr;
};

//--------------------------------------------------------------------
// EdgeListGraph
//--------------------------------------------------------------------
template<class VertexDescriptor, class EdgeProperty = boost::no_property>
struct ListEdge {
    ListEdge(VertexDescriptor s, VertexDescriptor t)
        : m_source(s), m_target(t)
    {}
    
    ListEdge(VertexDescriptor s, VertexDescriptor t, EdgeProperty&& p)
        : m_source(s), m_target(t)
        , mProperty(std::move(p))
    {}

    ListEdge(VertexDescriptor s, VertexDescriptor t, const EdgeProperty& p)
        : m_source(s), m_target(t)
        , mProperty(p)
    {}

    template<class... T>
    ListEdge(VertexDescriptor s, VertexDescriptor t, T&&... args)
        : m_source(s), m_target(t)
        , mProperty(std::forward<T>(args)...)
    {}

    EdgeProperty& get_property() noexcept { return mProperty; }
    const EdgeProperty& get_property() const noexcept { return mProperty; }

    VertexDescriptor m_source = {};
    VertexDescriptor m_target = {};
    EdgeProperty mProperty;
};

//template<class VertexDescriptor, PmrAllocatorUserClass_ EdgeProperty>
template<class VertexDescriptor, class EdgeProperty>
struct PmrListEdge {
    using allocator_type = boost::container::pmr::polymorphic_allocator<char>;
    allocator_type get_allocator() const noexcept {
        return allocator_type{ mProperty.get_allocator().resource() };
    }
    // cntrs
    PmrListEdge(VertexDescriptor s, VertexDescriptor t, const allocator_type& alloc)
        : m_source(s), m_target(t)
        , mProperty(alloc)
    {}
    
    PmrListEdge(VertexDescriptor s, VertexDescriptor t, EdgeProperty&& p, const allocator_type& alloc)
        : m_source(s), m_target(t)
        , mProperty(std::move(p), alloc)
    {}

    PmrListEdge(VertexDescriptor s, VertexDescriptor t, const EdgeProperty& p, const allocator_type& alloc)
        : m_source(s), m_target(t)
        , mProperty(p, alloc)
    {}

    template<class... T>
    PmrListEdge(VertexDescriptor s, VertexDescriptor t, T&&... args)
        : m_source(s), m_target(t)
        , mProperty(std::forward<T>(args)...)
    {}

    // move/copy cntrs
    PmrListEdge(PmrListEdge&& rhs, const allocator_type& alloc)
        : m_source(std::move(rhs.m_source))
        , m_target(std::move(rhs.m_target))
        , mProperty(std::move(rhs.mProperty), alloc)
    {}
    PmrListEdge(const PmrListEdge& rhs, const allocator_type& alloc)
        : m_source(rhs.m_source)
        , m_target(rhs.m_target)
        , mProperty(rhs.mProperty, alloc)
    {}

    PmrListEdge(const PmrListEdge&) = delete;

    EdgeProperty& get_property() noexcept { return mProperty; }
    const EdgeProperty& get_property() const noexcept { return mProperty; }

    VertexDescriptor m_source = {};
    VertexDescriptor m_target = {};
    EdgeProperty mProperty;
};

// Polymorphic Graph
template <class Tag, class Handle>
struct ValueHandle : Tag {
    ValueHandle() noexcept = default;
    ValueHandle(ValueHandle&& rhs) noexcept
        : mValue(std::move(rhs.mValue)) {}
    ValueHandle(const ValueHandle& rhs) noexcept
        : mValue(rhs.mValue) {}
    ValueHandle& operator=(ValueHandle&& rhs) noexcept {
        mValue = std::move(rhs.mValue);
        return *this;
    }
    ValueHandle& operator=(const ValueHandle& rhs) noexcept {
        mValue = rhs.mValue;
        return *this;
    }

    ValueHandle(const Handle& handle) noexcept
        : mValue(handle) {}
    ValueHandle(Handle&& handle) noexcept
        : mValue(std::move(handle)) {}
    template <class... Args>
    ValueHandle(Args&&... args) noexcept
        : mValue(std::forward<Args>(args)...) {}

    Handle mValue = {};
};

// Reference Graph
// OwnershipIterator (Bidirectional, !EdgeProperty)
template <class VertexIterator, class OutEdgeIterator, class Graph>
class OwnershipIterator {
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = typename OutEdgeIterator::value_type;
    using reference = typename OutEdgeIterator::reference;
    using pointer = typename OutEdgeIterator::pointer;
    using difference_type = typename OutEdgeIterator::difference_type;
    using distance_type = difference_type;

    OwnershipIterator() = default;
    template <class G>
    OwnershipIterator(VertexIterator b, VertexIterator c, VertexIterator e, const G& g) noexcept
        : mBegin(b)
        , mCurr(c)
        , mEnd(e)
        , mG(&g) {
        if (mCurr != mEnd) {
            while (mCurr != mEnd && num_children(*mCurr, *mG) == 0)
                ++mCurr;
            if (mCurr != mEnd)
                mEdges = children(*mCurr, *mG);
        }
    }

    OwnershipIterator& operator++() noexcept {
        ++mEdges->first;
        if (mEdges->first == mEdges->second) {
            ++mCurr;
            while (mCurr != mEnd && num_children(*mCurr, *mG) == 0)
                ++mCurr;
            if (mCurr != mEnd)
                mEdges = children(*mCurr, *mG);
        }
        return *this;
    }
    OwnershipIterator operator++(int) noexcept {
        OwnershipIterator tmp = *this;
        ++(*this);
        return tmp;
    }
    value_type operator*() const noexcept {
        return *mEdges->first;
    }
    bool operator==(const OwnershipIterator& x) const noexcept {
        return mCurr == x.mCurr && (mCurr == mEnd || mEdges->first == x.mEdges->first);
    }
    bool operator!=(const OwnershipIterator& x) const noexcept {
        return mCurr != x.mCurr || (mCurr != mEnd && mEdges->first != x.mEdges->first);
    }

protected:
    VertexIterator mBegin = {};
    VertexIterator mCurr = {};
    VertexIterator mEnd = {};
    boost::optional<std::pair<OutEdgeIterator, OutEdgeIterator>> mEdges;
    const Graph* mG = nullptr;
};

}

}
