#include <benchmark/benchmark.h>

#include <cassert>
#include <mutex>
#include <print>
#include <queue>
#include <thread>
#include <vector>

#include "../src/mpmc_queue.hpp"

// Naive mutex-protected MPMC queue
template <typename T>
class MutexMPMCQueue {
 public:
  explicit MutexMPMCQueue(size_t capacity) : capacity_(capacity) {}

  bool push(T item) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.size() >= capacity_) return false;
    queue_.push(std::move(item));
    return true;
  }

  bool pop(T& out) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.empty()) return false;
    out = std::move(queue_.front());
    queue_.pop();
    return true;
  }

 private:
  std::queue<T> queue_;
  size_t capacity_;
  std::mutex mutex_;
};

// Single-threaded push/pop
template <typename Queue>
static void BM_MPMCQueue_SingleThreaded(benchmark::State& state) {
  int N = 1024;
  Queue queue(N);
  for (auto _ : state) {
    for (int i = 0; i < N; ++i) {
      queue.push(i);
    }
    int sum = 0;
    int value;
    for (int i = 0; i < N; ++i) {
      assert(queue.pop(value));
      sum += value;
    }
    assert(sum == N * (N - 1) / 2);
  }
}
BENCHMARK_TEMPLATE(BM_MPMCQueue_SingleThreaded, mpmc_queue::MPMCQueue<int>)
    ->Name("LockFree_MPMCQueue/SingleThreaded");
BENCHMARK_TEMPLATE(BM_MPMCQueue_SingleThreaded, MutexMPMCQueue<int>)
    ->Name("Mutex_MPMCQueue/SingleThreaded");

// Multi-producer, multi-consumer
template <typename Queue, unsigned int N, unsigned int PRODUCERS,
          unsigned int CONSUMERS>
static void BM_MPMCQueue_MultiProducerMultiConsumer(benchmark::State& state) {
  Queue queue(1024);
  for (auto _ : state) {
    std::vector<std::thread> producers, consumers;

    for (unsigned int p = 0; p < PRODUCERS; ++p) {
      producers.emplace_back([&queue]() {
        for (unsigned int i = 0; i < N; ++i) {
          while (!queue.push(i));
        }

        // Signal end of production
        // Make sure that each consumers gets at least one signal
        // This is a simple heuristic to ensure that all consumers can finish
        for (unsigned int i = 0; i < (CONSUMERS + PRODUCERS - 1) / PRODUCERS;
             ++i) {
          while (!queue.push(-1));
        }
      });
    }

    std::atomic<unsigned int> sum{0};
    for (unsigned int c = 0; c < CONSUMERS; ++c) {
      consumers.emplace_back([&sum, &queue]() {
        int value, local_sum = 0;
        while (true) {
          if (queue.pop(value)) {
            if (value == -1) {
              break;  // End of production signal
            }
            local_sum += value;
          }
        }
        sum.fetch_add(local_sum);
      });
    }

    for (auto& t : producers) t.join();
    for (auto& t : consumers) t.join();

    // Process remaining items in the queue
    int val;
    while (queue.pop(val)) {
      if (val != -1) {
        sum.fetch_add(val);
      }
    }

    assert(sum.load() == PRODUCERS * (N - 1) * N / 2);
  }
}

// 4 producers, 4 consumers, 10000 items
BENCHMARK_TEMPLATE(BM_MPMCQueue_MultiProducerMultiConsumer,
                   mpmc_queue::MPMCQueue<int>, 10'000, 4, 4)
    ->Name("LockFree_MPMCQueue/MPMC/10000/4P4C")
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_MPMCQueue_MultiProducerMultiConsumer, MutexMPMCQueue<int>,
                   10'000, 4, 4)
    ->Name("Mutex_MPMCQueue/MPMC/10000/4P4C")
    ->Unit(benchmark::kMillisecond);

// 4 producers, 8 consumers, 10000 items
BENCHMARK_TEMPLATE(BM_MPMCQueue_MultiProducerMultiConsumer,
                   mpmc_queue::MPMCQueue<int>, 10'000, 4, 8)
    ->Name("LockFree_MPMCQueue/MPMC/10000/4P8C")
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_MPMCQueue_MultiProducerMultiConsumer, MutexMPMCQueue<int>,
                   10'000, 4, 8)
    ->Name("Mutex_MPMCQueue/MPMC/10000/4P8C")
    ->Unit(benchmark::kMillisecond);

// 4 producers, 16 consumers, 10000 items
BENCHMARK_TEMPLATE(BM_MPMCQueue_MultiProducerMultiConsumer,
                   mpmc_queue::MPMCQueue<int>, 10'000, 4, 16)
    ->Name("LockFree_MPMCQueue/MPMC/10000/4P16C")
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_MPMCQueue_MultiProducerMultiConsumer, MutexMPMCQueue<int>,
                   10'000, 4, 16)
    ->Name("Mutex_MPMCQueue/MPMC/10000/4P16C")
    ->Unit(benchmark::kMillisecond);

// 4 producers, 32 consumers, 10000 items
BENCHMARK_TEMPLATE(BM_MPMCQueue_MultiProducerMultiConsumer,
                   mpmc_queue::MPMCQueue<int>, 10'000, 4, 32)
    ->Name("LockFree_MPMCQueue/MPMC/10000/4P32C")
    ->Unit(benchmark::kMillisecond);
BENCHMARK_TEMPLATE(BM_MPMCQueue_MultiProducerMultiConsumer, MutexMPMCQueue<int>,
                   10'000, 4, 32)
    ->Name("Mutex_MPMCQueue/MPMC/10000/4P32C")
    ->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();