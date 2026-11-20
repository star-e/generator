#pragma once

#include <boost/container/pmr/global_resource.hpp>
#include <boost/container/pmr/memory_resource.hpp>
#include <new>

// Fail each allocation in turn, including those made by nested PMR containers.
struct InjectedFailure final : std::bad_alloc {};

class FailingResource final : public boost::container::pmr::memory_resource {
public:
    int allocationsBeforeFailure{-1};
private:
    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        if (allocationsBeforeFailure == 0) throw InjectedFailure{};
        if (allocationsBeforeFailure > 0) --allocationsBeforeFailure;
        return boost::container::pmr::new_delete_resource()->allocate(bytes, alignment);
    }
    void do_deallocate(void* p, std::size_t bytes, std::size_t alignment) override {
        boost::container::pmr::new_delete_resource()->deallocate(p, bytes, alignment);
    }
    bool do_is_equal(const memory_resource& other) const noexcept override { return this == &other; }
};
