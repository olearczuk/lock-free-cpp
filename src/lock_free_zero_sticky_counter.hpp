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
 * - The counter is initialized to 1.
 * - incrementIfNotZero() will only increment the counter if it is not zero,
 *   and returns true if the increment succeeded.
 * - decrement() decrements the counter and returns true if the counter reached
 * zero.
 * - read() returns the current value of the counter.
 *
 * All operations use relaxed memory ordering, which is sufficient for pure
 * reference counting (not for synchronizing access to other shared data).
 */
class LockFreeZeroStickyCounter {
 private:
  std::atomic<uint64_t> counter_;

 public:
  /**
   * @brief Constructs a LockFreeZeroStickyCounter with an initial value of 1.
   */
  LockFreeZeroStickyCounter() : counter_(1) {}

  /**
   * @brief Constructs a LockFreeZeroStickyCounter with a specified initial
   * value.
   * @param initial_value The initial value of the counter.
   */
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

  /**
   * @brief Get the current value of the counter.
   * @return The current counter value.
   */
  uint64_t read() { return counter_.load(std::memory_order_relaxed); }
};

}