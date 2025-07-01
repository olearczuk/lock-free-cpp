#pragma once

#include <atomic>
#include <cassert>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <type_traits>

#include "common.hpp"

namespace mpmc_queue {

using common::CACHE_LINE_SIZE;

/**
 * @brief Lock-free Multi-Producer Multi-Consumer (MPMC) Bounded Queue.
 *
 * - Multiple threads can safely push and pop concurrently.
 * - The queue is bounded and uses a ring buffer of fixed capacity (must be a
 * power of two).
 * - Each slot contains a sequence number to coordinate producers and consumers.
 * - All operations are lock-free.
 *
 * @tparam T      The type of elements stored in the queue.
 * @tparam Alloc  The allocator type used for buffer allocation (default:
 * std::allocator<void>). It is then going to be rebound to allocator for Slot.
 */
template <typename T, typename Alloc = std::allocator<void>>
class MPMCQueue {
 private:
  struct Slot {
    // Sequence number to track the state of the slot.
    // When seq == i, the slot is empty.
    // When seq == i + 1, the slot contains a valid item.
    alignas(CACHE_LINE_SIZE) std::atomic<uint64_t> seq;
    static_assert(
        std::is_default_constructible_v<T>,
        "T must be default constructible for MPMCQueue to store T directly");
    alignas(CACHE_LINE_SIZE) T storage;

    // Extra % to make PADDING_SIZE 0 when SIZE % CACHE_LINE_SIZE == 0.
    // Otherwise, PADDING_SIZE = CACHE_LINE_SIZE which wastes space by adding
    // unnecessary padding.
    // Technically, char[0] is invalid according to C++ standard, however,
    // it is supported by some compilers like GCC and Clang.
    static constexpr size_t SLOT_SIZE_NO_PAD = sizeof(seq) + sizeof(storage);
    static constexpr size_t PAD_SIZE =
        (CACHE_LINE_SIZE - (SLOT_SIZE_NO_PAD % CACHE_LINE_SIZE)) %
        CACHE_LINE_SIZE;
    char pad[PAD_SIZE];
  };

  static_assert(sizeof(Slot) % CACHE_LINE_SIZE == 0,
                "Slot size must be a multiple of CACHE_LINE_SIZE");

  using SlotAllocator =
      typename std::allocator_traits<Alloc>::template rebind_alloc<Slot>;

  SlotAllocator slot_allocator_;

  uint64_t capacity_;
  alignas(CACHE_LINE_SIZE) Slot* ring_;

  alignas(CACHE_LINE_SIZE) std::atomic<uint64_t> head_{0};
  alignas(CACHE_LINE_SIZE) std::atomic<uint64_t> tail_{0};

  static_assert(std::atomic<uint64_t>::is_always_lock_free,
                "std::atomic<uint64_t> must be lock-free for MPMCQueue to work "
                "correctly");

 public:
  /**
   * @brief Constructs an MPMCQueue with the given capacity.
   * @param capacity The maximum number of elements (must be a power of two).
   * @param alloc    Optional allocator for buffer allocation.
   * @throws std::invalid_argument if capacity is not a power of two.
   */
  explicit MPMCQueue(uint64_t capacity, const Alloc& alloc = Alloc{})
      : slot_allocator_(alloc),
        capacity_{capacity},
        ring_{slot_allocator_.allocate(capacity_)} {
    if (capacity_ == 0 || (capacity_ & (capacity_ - 1))) {
      throw std::invalid_argument(
          "MPMCQueue capacity must be a power of 2 and > 0");
    }
    // Allocate slots and initialize sequence numbers
    for (uint64_t i = 0; i < capacity_; ++i) {
      new (&ring_[i]) Slot;
      ring_[i].seq.store(i, std::memory_order_relaxed);
    }
  }

  ~MPMCQueue() {
    // Destroy any remaining objects
    for (uint64_t i = 0; i < capacity_; ++i) {
      Slot& slot = ring_[i];
      uint64_t seq = slot.seq.load(std::memory_order_relaxed);
      // If slot is not empty, destroy the object
      if (seq == i + 1) {
        slot.storage.~T();
      }
      slot.~Slot();
    }
    slot_allocator_.deallocate(ring_, capacity_);
  }

  // Disable copy and move semantics
  MPMCQueue(const MPMCQueue&) = delete;
  MPMCQueue& operator=(const MPMCQueue&) = delete;
  MPMCQueue(MPMCQueue&&) = delete;
  MPMCQueue& operator=(MPMCQueue&&) = delete;

  /**
   * @brief Pushes an item into the queue by moving it.
   * @param item The item to be moved into the queue.
   * @return true if the item was successfully pushed; false if the queue is
   * full.
   */
  bool push(T&& item) { return emplace(std::move(item)); }

  /**
   * @brief Pushes an item onto the queue by copying it.
   * @param item The item to be copied and pushed onto the queue.
   * @return true if the item was successfully pushed; false if the queue is
   * full.
   */
  bool push(const T& item) { return emplace(item); }

  /**
   * @brief Constructs a new element in-place at the end of the queue.
   * @tparam Args Types of arguments to forward to T's constructor.
   * @param args Arguments to forward to the constructor of T.
   * @return true if the element was successfully constructed and added to the
   * queue.
   * @return false if the queue is full and the element could not be added.
   */
  template <typename... Args>
  bool emplace(Args&&... args) {
    uint64_t pos = tail_.load(std::memory_order_relaxed);
    for (;;) {
      Slot& slot = ring_[pos & (capacity_ - 1)];
      uint64_t seq = slot.seq.load(std::memory_order_acquire);
      auto cmp = seq <=> pos;
      if (cmp == 0) {
        if (tail_.compare_exchange_weak(pos, pos + 1,
                                        std::memory_order_relaxed)) {
          // Construct in-place
          new (&slot.storage) T(std::forward<Args>(args)...);
          slot.seq.store(pos + 1, std::memory_order_release);
          return true;
        }
      } else if (cmp < 0) {
        return false;  // Queue is full
      } else {
        pos = tail_.load(std::memory_order_relaxed);
      }
    }
  }

  /**
   * @brief Attempts to pop an item from the queue.
   * @param out Reference to store the popped item.
   * @return true if an item was popped, false if the queue is empty.
   */
  bool pop(T& out) {
    uint64_t pos = head_.load(std::memory_order_relaxed);
    while (true) {
      Slot& slot = ring_[pos & (capacity_ - 1)];
      uint64_t seq = slot.seq.load(std::memory_order_acquire);
      auto cmp = seq <=> pos + 1;
      if (cmp == 0) {
        if (head_.compare_exchange_weak(pos, pos + 1,
                                        std::memory_order_relaxed)) {
          // Move out
          out = std::move(slot.storage);
          slot.seq.store(pos + capacity_, std::memory_order_release);
          return true;
        }
      } else if (cmp < 0) {
        return false;  // Queue is empty
      } else {
        pos = head_.load(std::memory_order_relaxed);
      }
    }
  }
};

}  // namespace mpmc_queue