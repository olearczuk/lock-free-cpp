#include <benchmark/benchmark.h>

#include <cassert>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>

#include "../src/rigtorp_spsc_queue.hpp"
#include "../src/spsc_queue.hpp"

// Naive mutex-protected SPSC queue with the same interface
template <typename T>
class MutexSPSCQueue {
 public:
  explicit MutexSPSCQueue(size_t capacity) : capacity_(capacity) {}

  bool push(T item) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.size() >= capacity_) return false;
    queue_.push(std::move(item));
    return true;
  }

  T* front() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.empty()) return nullptr;  // Return nullptr if empty
    return &queue_.front();
  }

  void pop() {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.pop();
  }

 private:
  std::queue<T> queue_;
  size_t capacity_;
  std::mutex mutex_;
};

// Benchmark: single-threaded push/pop
template <typename Queue>
static void BM_SPSCQueue_SingleThreaded(benchmark::State& state) {
  int N = 1024;
  Queue queue(N);
  for (auto _ : state) {
    for (int i = 0; i < N; ++i) {
      queue.push(i);
    }

    int item;
    int sum = 0;
    for (int i = 0; i < N; ++i) {
      if (auto val = queue.front(); val != nullptr) {
        sum += *val;
        queue.pop();
      }
    }
    assert(sum == N * (N - 1) / 2);  // Check if sum is correct
  }
}
BENCHMARK_TEMPLATE(BM_SPSCQueue_SingleThreaded, spsc_queue::SPSCQueue<int>)
    ->Name("LockFree_SPSCQueue/SingleThreaded");
BENCHMARK_TEMPLATE(BM_SPSCQueue_SingleThreaded, rigtorp::SPSCQueue<int>)
    ->Name("RigTorp_SPSCQueue/SingleThreaded");
BENCHMARK_TEMPLATE(BM_SPSCQueue_SingleThreaded, MutexSPSCQueue<int>)
    ->Name("Mutex_SPSCQueue/SingleThreaded");

// Benchmark: producer-consumer (2 threads)
template <typename Queue, unsigned int N>
static void BM_SPSCQueue_ProducerConsumer(benchmark::State& state) {
  Queue queue(1024);
  // Producing thread
  for (auto _ : state) {
    std::thread producer([&]() {
      for (unsigned int i = 0; i < N; ++i) {
        // Slightly different flow for rigtorp SPSCQueue
        if constexpr (std::is_same_v<Queue, rigtorp::SPSCQueue<int>>) {
          queue.push(i);
        } else {
          while (!queue.push(i)) {
          }
        }
      }
    });

    // Consuming thread
    unsigned int sum = 0;
    std::thread consumer([&]() {
      unsigned int count = 0;
      while (count < N) {
        if (auto val_ptr = queue.front(); val_ptr != nullptr) {
          sum += *val_ptr;
          ++count;
          queue.pop();
        }
      }
    });
    producer.join();
    consumer.join();
    assert(sum == N * (N - 1) / 2);  // Check if sum is correct
  }
}
// 10000 items - significantly more than the queue capacity
BENCHMARK_TEMPLATE(BM_SPSCQueue_ProducerConsumer, spsc_queue::SPSCQueue<int>,
                   10000)
    ->Name("LockFree_SPSCQueue/ProducerConsumer/10000");
BENCHMARK_TEMPLATE(BM_SPSCQueue_ProducerConsumer, rigtorp::SPSCQueue<int>,
                   10000)
    ->Name("RigTorp_SPSCQueue/ProducerConsumer/10000");
BENCHMARK_TEMPLATE(BM_SPSCQueue_ProducerConsumer, MutexSPSCQueue<int>, 10000)
    ->Name("Mutex_SPSCQueue/ProducerConsumer/10000");

// 5000 items - still more than queue capacity
BENCHMARK_TEMPLATE(BM_SPSCQueue_ProducerConsumer, spsc_queue::SPSCQueue<int>,
                   5000)
    ->Name("LockFree_SPSCQueue/ProducerConsumer/5000");
BENCHMARK_TEMPLATE(BM_SPSCQueue_ProducerConsumer, rigtorp::SPSCQueue<int>, 5000)
    ->Name("RigTorp_SPSCQueue/ProducerConsumer/5000");
BENCHMARK_TEMPLATE(BM_SPSCQueue_ProducerConsumer, MutexSPSCQueue<int>, 5000)
    ->Name("Mutex_SPSCQueue/ProducerConsumer/5000");

// 1024 items - equal to queue capacity
BENCHMARK_TEMPLATE(BM_SPSCQueue_ProducerConsumer, spsc_queue::SPSCQueue<int>,
                   1024)
    ->Name("LockFree_SPSCQueue/ProducerConsumer/1000");
BENCHMARK_TEMPLATE(BM_SPSCQueue_ProducerConsumer, rigtorp::SPSCQueue<int>, 1024)
    ->Name("RigTorp_SPSCQueue/ProducerConsumer/1000");
BENCHMARK_TEMPLATE(BM_SPSCQueue_ProducerConsumer, MutexSPSCQueue<int>, 1024)
    ->Name("Mutex_SPSCQueue/ProducerConsumer/1000");

// 500 items - half the queue capacity
BENCHMARK_TEMPLATE(BM_SPSCQueue_ProducerConsumer, spsc_queue::SPSCQueue<int>,
                   500)
    ->Name("LockFree_SPSCQueue/ProducerConsumer/500");
BENCHMARK_TEMPLATE(BM_SPSCQueue_ProducerConsumer, rigtorp::SPSCQueue<int>, 500)
    ->Name("RigTorp_SPSCQueue/ProducerConsumer/500");
BENCHMARK_TEMPLATE(BM_SPSCQueue_ProducerConsumer, MutexSPSCQueue<int>, 500)
    ->Name("Mutex_SPSCQueue/ProducerConsumer/500");

// 100 items - significantly less than queue capacity
BENCHMARK_TEMPLATE(BM_SPSCQueue_ProducerConsumer, spsc_queue::SPSCQueue<int>,
                   100)
    ->Name("LockFree_SPSCQueue/ProducerConsumer/100");
BENCHMARK_TEMPLATE(BM_SPSCQueue_ProducerConsumer, rigtorp::SPSCQueue<int>, 100)
    ->Name("RigTorp_SPSCQueue/ProducerConsumer/100");
BENCHMARK_TEMPLATE(BM_SPSCQueue_ProducerConsumer, MutexSPSCQueue<int>, 100)
    ->Name("Mutex_SPSCQueue/ProducerConsumer/100");

BENCHMARK_MAIN();