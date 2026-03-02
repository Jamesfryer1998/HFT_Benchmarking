#pragma once

#include <cstddef>
#include <memory>
#include <vector>
#include <optional>

namespace hft {

// Simple fixed-size memory pool for Order objects
// Avoids dynamic allocation in hot path
template<typename T>
class MemoryPool {
public:
    explicit MemoryPool(size_t capacity = 1'000'000)
        : capacity_(capacity)
        , pool_(capacity)
        , free_list_()
    {
        free_list_.reserve(capacity);
        // Initialize free list with all indices
        for (size_t i = 0; i < capacity; ++i) {
            free_list_.push_back(i);
        }
    }

    // Allocate an object from the pool
    [[nodiscard]] std::optional<T*> allocate() noexcept {
        if (free_list_.empty()) {
            return std::nullopt; // Pool exhausted
        }

        size_t index = free_list_.back();
        free_list_.pop_back();
        return &pool_[index];
    }

    // Deallocate an object back to the pool
    void deallocate(T* ptr) noexcept {
        if (ptr == nullptr) return;

        size_t index = ptr - pool_.data();
        if (index < capacity_) {
            free_list_.push_back(index);
        }
    }

    // Construct object in-place in the pool
    template<typename... Args>
    [[nodiscard]] std::optional<T*> construct(Args&&... args) noexcept {
        auto ptr_opt = allocate();
        if (!ptr_opt) return std::nullopt;

        T* ptr = *ptr_opt;
        new (ptr) T(std::forward<Args>(args)...);
        return ptr;
    }

    // Destroy and deallocate object
    void destroy(T* ptr) noexcept {
        if (ptr) {
            ptr->~T();
            deallocate(ptr);
        }
    }

    [[nodiscard]] size_t available() const noexcept { return free_list_.size(); }
    [[nodiscard]] size_t capacity() const noexcept { return capacity_; }
    [[nodiscard]] size_t used() const noexcept { return capacity_ - free_list_.size(); }

private:
    size_t capacity_;
    std::vector<T> pool_;
    std::vector<size_t> free_list_;
};

} // namespace hft
