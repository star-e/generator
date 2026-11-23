#pragma once

#include <boost/container/pmr/global_resource.hpp>
#include <boost/container/pmr/memory_resource.hpp>
#include <cstddef>
#include <new>

// Fail each allocation in turn, including those made by nested PMR containers.
struct InjectedFailure final : std::bad_alloc {};

class FailingResource final : public boost::container::pmr::memory_resource {
public:
    int allocationsBeforeFailure{-1};
    std::size_t outstandingAllocations() const noexcept { return outstanding_; }
private:
    std::size_t outstanding_{};
    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        if (allocationsBeforeFailure == 0) throw InjectedFailure{};
        if (allocationsBeforeFailure > 0) --allocationsBeforeFailure;
        auto* memory = boost::container::pmr::new_delete_resource()->allocate(bytes, alignment);
        ++outstanding_;
        return memory;
    }
    void do_deallocate(void* p, std::size_t bytes, std::size_t alignment) override {
        boost::container::pmr::new_delete_resource()->deallocate(p, bytes, alignment);
        --outstanding_;
    }
    bool do_is_equal(const memory_resource& other) const noexcept override { return this == &other; }
};
