#pragma once

#include <atomic>

namespace zero_sticky_counter {

/**
 * @brief A lock-free, zero-sticky reference counter.
 *
 * This class implements a lock-free reference counter that never goes below
 * zero. It is designed for scenarios similar to reference counting in smart
 * pointers, where you want to track the number of active references to a shared
 * resource.
 *
 * All operations use relaxed memory ordering, which is sufficient for pure
 * reference counting (not for synchronizing access to other shared data).
 */
class LockFreeZeroStickyCounter {
 private:
  std::atomic<uint64_t> counter_;

 public:
  LockFreeZeroStickyCounter() : counter_(1) {}

  explicit LockFreeZeroStickyCounter(uint64_t initial_value)
      : counter_(initial_value) {}

  /**
   * @brief Increment the counter if it is not zero.
   * @return true if the increment succeeded, false if the counter was
   * zero.
   */
  bool incrementIfNotZero() {
    auto value = counter_.load(std::memory_order_relaxed);
    while (value != 0) {
      if (counter_.compare_exchange_weak(value, value + 1,
                                         std::memory_order_relaxed,
                                         std::memory_order_relaxed)) {
        return true;
      }
    }
    return false;
  }

  /**
   * @brief Decrement the counter.
   * @return true if the counter reached zero after decrementing, false
   * otherwise.
   */
  bool decrement() {
    return counter_.fetch_sub(1, std::memory_order_relaxed) == 1;
  }

  uint64_t read() { return counter_.load(std::memory_order_relaxed); }
};

}  // namespace zero_sticky_counter
