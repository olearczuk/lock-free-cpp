#pragma once

#include <atomic>
#include <cassert>
#include <memory>

#include "common.hpp"

namespace spsc_queue {

using common::CACHE_LINE_SIZE;

/**
 * @brief Single-Producer Single-Consumer (SPSC) Lock-Free Queue.
 *
 * This class implements a bounded, lock-free queue for single-producer,
 * single-consumer scenarios. It is designed for high-performance communication
 * between two threads, where one thread exclusively pushes (produces) and the
 * other exclusively pops (consumes).
 *
 * - The queue uses a ring buffer of fixed capacity (must be a power of two).
 * - All operations are wait-free for the intended SPSC use case.
 * - The queue is not safe for multiple producers or multiple consumers.
 * - Memory layout and atomic variables are aligned to cache lines to minimize
 *   false sharing and improve performance.
 *
 * @tparam T      The type of elements stored in the queue.
 * @tparam Alloc  The allocator type used for buffer allocation (default:
 * std::allocator<T>).
 */
template <typename T, typename Alloc = std::allocator<T>>
class alignas(CACHE_LINE_SIZE) SPSCQueue : private Alloc {
 private:
  uint64_t capacity_;  ///< Capacity of the ring buffer (must be power of two)
  T* ring_;            ///< Pointer to the ring buffer storage

  // Producer and consumer indices, aligned to cache lines to avoid false
  // sharing
  alignas(CACHE_LINE_SIZE) std::atomic<uint64_t> head_{0};
  alignas(CACHE_LINE_SIZE) uint64_t cached_head_{};
  alignas(CACHE_LINE_SIZE) std::atomic<uint64_t> tail_{0};
  alignas(CACHE_LINE_SIZE) uint64_t cached_tail_{};

  static_assert(std::atomic<uint64_t>::is_always_lock_free,
                "std::atomic<uint64_t> must be lock-free for SPSCQueue to work "
                "correctly");

 public:
  /**
   * @brief Constructs an SPSCQueue with the given capacity.
   * @param capacity The maximum number of elements (must be a power of two).
   * @param alloc    Optional allocator for buffer allocation.
   * @throws std::invalid_argument if capacity is not a power of two.
   */
  explicit SPSCQueue(uint64_t capacity, const Alloc& alloc = Alloc{})
      : Alloc{alloc},
        capacity_{capacity},
        ring_{std::allocator_traits<Alloc>::allocate(*this, capacity_)} {
    assert(capacity_ > 0 && "SPSCQueue capacity must be greater than 0");
    assert(ring_ != nullptr && "Failed to allocate memory for SPSCQueue");
    if (capacity_ & (capacity_ - 1)) {
      throw std::invalid_argument(
          std::format("SPSCQueue capacity {} must be a power of 2 for "
                      "efficient modulo operations",
                      capacity_));
    }
  }

  /**
   * @brief Destructor. Clears remaining elements and deallocates the buffer.
   */
  ~SPSCQueue() {
    static_assert(std::is_nothrow_destructible_v<T>,
                  "SPSCQueue requires T to be nothrow destructible");
    // Clear remaining data
    while (front() != nullptr) {
      // Keep popping until the queue is empty
      pop();
    }
    std::allocator_traits<Alloc>::deallocate(*this, ring_, capacity_);
  }

  // Disable copy and move semantics
  SPSCQueue(const SPSCQueue&) = delete;
  SPSCQueue& operator=(const SPSCQueue&) = delete;
  SPSCQueue(SPSCQueue&&) = delete;
  SPSCQueue& operator=(SPSCQueue&&) = delete;

  /**
   * @brief Pushes an item onto the queue by copying it.
   *
   * This function attempts to insert a copy of the provided item into the
   * queue. The operation is noexcept if the copy constructor of T is noexcept.
   * Requires that T is copy constructible.
   *
   * @param item The item to be copied and pushed onto the queue.
   * @return true if the item was successfully pushed; false if the queue is
   * full.
   */
  bool push(const T& item) noexcept(
      std::is_nothrow_copy_constructible<T>::value) {
    static_assert(std::is_copy_constructible<T>::value,
                  "T must be copy constructible");
    return emplace(item);
  }

  /**
   * @brief Pushes an item into the queue by moving it.
   *
   * This function attempts to insert an item into the queue using move
   * semantics. It requires that the type T is move constructible. The operation
   * is noexcept if T's move constructor is noexcept.
   *
   * @param item The item to be moved into the queue.
   * @return true if the item was successfully pushed into the queue, false
   * otherwise.
   */
  bool push(T&& item) noexcept(std::is_nothrow_move_constructible<T>::value) {
    static_assert(std::is_move_constructible<T>::value,
                  "T must be move constructible");
    return emplace(std::move(item));
  }

  /**
   * @brief Returns a pointer to the front element in the queue.
   *
   * @note The returned pointer may be nullptr if the queue is empty.
   * @return T* Pointer to the front element, or nullptr if the queue is empty.
   * @nodiscard The return value should not be ignored.
   * @remark This function is noexcept and does not throw exceptions.
   */
  [[nodiscard]] T* front() noexcept {
    auto head = head_.load(std::memory_order_acquire);
    // Only update cached_tail_ when the queue seems to be empty
    if (empty(head, cached_tail_)) {
      cached_tail_ = tail_.load(std::memory_order_relaxed);
      if (empty(head, cached_tail_)) {
        return nullptr;  // Queue is empty
      }
    }
    return &ring_[index(head)];
  }

  /**
   * @brief Removes the front element from the queue.
   *
   * This function destroys the object at the current head position and advances
   * the head pointer. It must only be called after front() has returned a
   * non-nullptr value, indicating that the queue is not empty.
   *
   * @note The type T must be nothrow destructible.
   * @note This function is noexcept and thread-safe for single-producer,
   * single-consumer usage.
   *
   * @pre The queue must not be empty.
   * @post The front element is removed and its destructor is called.
   */
  void pop() noexcept {
    static_assert(std::is_nothrow_destructible<T>::value,
                  "T must be nothrow destructible");
    auto head = head_.load(std::memory_order_acquire);
    assert(!empty(head, cached_tail_) &&
           "Can only call pop() after front() has returned a non-nullptr");

    ring_[index(head)].~T();  // Call destructor on the item
    head_.store(head + 1, std::memory_order_release);
  }

 private:
  /**
   * @brief Returns the number of elements in the queue.
   */
  [[nodiscard]] uint64_t size(uint64_t head, uint64_t tail) const {
    return tail - head;
  }

  /**
   * @brief Checks if the queue is full.
   */
  [[nodiscard]] bool full(uint64_t head, uint64_t tail) const {
    return size(head, tail) == capacity_;
  }

  /**
   * @brief Checks if the queue is empty.
   */
  [[nodiscard]] bool empty(uint64_t head, uint64_t tail) const {
    return size(head, tail) == 0;
  }

  /**
   * @brief Computes the index in the ring buffer for a given position.
   *        Assumes capacity_ is a power of two.
   */
  [[nodiscard]] uint64_t index(uint64_t pos) const {
    return pos & (capacity_ - 1);
  }

  /**
   * @brief Constructs a new element in-place at the end of the queue.
   *
   * This method attempts to construct a new element of type T in-place at the
   * current tail position of the queue using the provided arguments. If the
   * queue is full, the method returns false and no element is constructed.
   * Otherwise, the element is constructed and the tail is advanced.
   *
   * @tparam Args Types of arguments to forward to T's constructor.
   * @param args Arguments to forward to the constructor of T.
   * @return true if the element was successfully constructed and added to the
   * queue.
   * @return false if the queue is full and the element could not be added.
   * @note This method is noexcept if T is nothrow-constructible from Args&&...
   */
  template <typename... Args>
  bool emplace(Args&&... args) noexcept(
      std::is_nothrow_constructible<T, Args&&...>::value) {
    static_assert(std::is_constructible<T, Args&&...>::value,
                  "T must be constructible with Args&&...");

    auto tail = tail_.load(std::memory_order_acquire);
    // Only update cached_head_ when the queue seems to be full
    if (full(cached_head_, tail)) {
      cached_head_ = head_.load(std::memory_order_relaxed);
      if (full(cached_head_, tail)) {
        return false;  // Queue is full
      }
    }

    new (&ring_[index(tail)]) T(std::forward<Args>(args)...);
    tail_.store(tail + 1, std::memory_order_release);
    return true;  // Item successfully pushed
  }
};

}  // namespace spsc_queue