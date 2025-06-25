#pragma once

#include <atomic>

namespace wait_free_zero_sticky_counter {

/**
 * @brief A wait-free, zero-sticky reference counter.
 *
 * This class implements a wait-free reference counter that never goes below
 * zero. It is designed for scenarios similar to reference counting in smart
 * pointers, where you want to track the number of active references to a shared
 * resource in a wait-free manner.
 *
 * - The counter is initialized to 1.
 * - incrementIfNotZero() will only increment the counter if it is not zero,
 *   and returns true if the increment succeeded.
 * - decrement() decrements the counter and returns true if the counter reached
 *   zero.
 * - read() returns the current value of the counter.
 *
 * All operations use relaxed memory ordering, which is sufficient for pure
 * reference counting (not for synchronizing access to other shared data).
 * This implementation uses special bit patterns to ensure wait-freedom and
 * to handle the "sticky zero" property.
 */
class WaitFreeZeroStickyCounter {
 private:
  // flag signalling value has been set to 0
  static constexpr uint64_t zero = 1ull << 63;
  // flag signalling zero flag has been set by decrement()
  // used by decrement() to know it should "take credit" for it
  static constexpr uint64_t helped = 1ull << 62;

  std::atomic<uint64_t> counter_;

 public:
  /**
   * @brief Constructs a WaitFreeZeroStickyCounter with an initial value of 1.
   */
  WaitFreeZeroStickyCounter() : counter_(1) {}

  /**
   * @brief Constructs a WaitFreeZeroStickyCounter with a specified initial
   * value.
   * @param initial_value The initial value of the counter.
   */
  explicit WaitFreeZeroStickyCounter(uint64_t initial_value)
      : counter_(initial_value) {}

  /**
   * @brief Increment the counter if it is not zero.
   * @return true if the increment succeeded, false if the counter was zero.
   */
  bool incrementIfNotZero() {
    return (counter_.fetch_add(1, std::memory_order_relaxed) & zero) == 0;
  }

  /**
   * @brief Decrement the counter.
   * @return true if the counter reached zero after decrementing, false
   * otherwise.
   */
  bool decrement() {
    if (counter_.fetch_sub(1, std::memory_order_relaxed) == 1) {
      auto val = counter_.load(std::memory_order_relaxed);
      // Set to zero using flag.
      // It is possible that value is set to 0 but before
      // compare_exchange_strong succeeds, incrementIfNotZero() changes it back
      // to 1 so the value is not zero.
      // That's fine because from external point of view it's exactly the same
      // as if increment occurred before decrement
      if (counter_.compare_exchange_strong(val, zero)) {
        return true;
      }
      // Setting to zero failed - perhaps read() set it to zero - exchange to
      // zero to "take credit".
      // In case of multiple decrements, we want only one want to take credit
      // for it, thus atomic operation again.
      else if ((val & helped) &&
               counter_.exchange(zero, std::memory_order_relaxed) & helped) {
        return true;
      }
    }
    return false;
  }

  /**
   * @brief Get the current value of the counter.
   * @return The current counter value.
   */
  uint64_t read() {
    auto val = counter_.load(std::memory_order_relaxed);
    // Value has been changed to 0 - try to help decrement() in setting it
    // to zero.
    // Add additional helped flag so decrement() can take credit for it.
    if (val == 0 && counter_.compare_exchange_strong(val, zero | helped)) {
      return 0;
    }
    // simply check if zero flag is set
    else {
      return val & zero ? 0 : val;
    }
  }
};

}  // namespace wait_free_zero_sticky_counter