#pragma once

#include <atomic>
#include <thread>
#include <type_traits>

#include "common.hpp"

namespace seq_lock {

using common::CACHE_LINE_SIZE;

/**
 * @brief A lock-free sequence lock (seqlock) for synchronizing access to a
 * single value between multiple readers and a single writer.
 *
 * SeqLock is designed for scenarios with many readers and a single writer,
 * providing efficient, lock-free reads and wait-free writes. It uses a sequence
 * counter to detect concurrent modifications, allowing readers to retry if a
 * write occurs during a read.
 *
 * @tparam T The type of value to protect. Must be trivially and nothrow copy
 * assignable.
 *
 * Usage:
 *   - Multiple threads can call read() concurrently.
 *   - Only one thread should call write() at a time.
 *
 * @note The size of SeqLock is padded to a multiple of CACHE_LINE_SIZE to
 * prevent false sharing.
 */
template <typename T>
class alignas(CACHE_LINE_SIZE) SeqLock {
  static_assert(
      std::is_trivially_copy_assignable_v<T>,
      "SeqLock can only be used with trivially copy assignable types");
  static_assert(
      std::is_nothrow_copy_assignable_v<T>,
      "SeqLock can only be used with types that are nothrow copy assignable");

 private:
  T value_;
  std::atomic<std::size_t> seq_;
  // Padding to prevent false sharing
  constexpr static int SIZE = sizeof(T) + sizeof(decltype(seq_));
  // Extra % to make PADDING_SIZE 0 when SIZE % CACHE_LINE_SIZE == 0.
  // Otherwise, it PADDING_SIZE = CACHE_LINE_SIZE which wastes space by adding
  // unnecessary padding.
  // Technically, char[0] is invalid according to C++ standard, however,
  // it is supported by some compilers like GCC and Clang.
  constexpr static int PADDING_SIZE =
      (CACHE_LINE_SIZE - SIZE % CACHE_LINE_SIZE) % CACHE_LINE_SIZE;
  char padding_[PADDING_SIZE];

  static_assert((SIZE + PADDING_SIZE) % CACHE_LINE_SIZE == 0,
                "SeqLock size must be a multiple of CACHE_LINE_SIZE");

 public:
  /**
   * @brief Constructs a SeqLock with a default-initialized value and sequence
   * counter set to 0.
   */
  SeqLock() : value_(T()), seq_(0) {}

  /**
   * @brief Atomically reads the protected value.
   *
   * This function may retry if a concurrent write is detected. It guarantees
   * that the returned value was not modified during the read operation.
   *
   * @return A copy of the protected value.
   */
  [[nodiscard]] T read() {
    while (true) {
      auto s1 = seq_.load(std::memory_order_acquire);

      // Makes sure value_ is read before seq_ is checked again
      T v = value_;
      std::atomic_signal_fence(std::memory_order_acq_rel);

      auto s2 = seq_.load(std::memory_order_relaxed);
      if (s1 == s2 && s1 % 2 == 0) {
        return v;
      }

      // Yield to avoid busy-waiting
      std::this_thread::yield();
    }
  }

  /**
   * @brief Atomically writes a new value, making it visible to readers.
   *
   * The write operation increments the sequence counter before and after
   * updating the value, allowing readers to detect concurrent modifications.
   *
   * @param value The new value to store.
   */
  void write(const T& value) {
    auto s1 = seq_.load(std::memory_order_relaxed);
    seq_.store(s1 + 1, std::memory_order_relaxed);

    // Makes sure value_ is written after initial increment of seq_
    std::atomic_signal_fence(std::memory_order_acq_rel);
    value_ = value;
    seq_.store(s1 + 2, std::memory_order_release);
  }
};

}  // namespace seq_lock