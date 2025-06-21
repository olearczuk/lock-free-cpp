#include "../src/seq_lock.hpp"

#include <benchmark/benchmark.h>

#include <atomic>
#include <mutex>
#include <random>
#include <shared_mutex>

// Instantiate benchmarks for different write ratios
constexpr short SMALL_WRITE_PERCENTAGE = 5;
constexpr short LARGE_WRITE_PERCENTAGE = 90;

template <typename Lock>
void BenchmarkRW(benchmark::State& state, Lock& lock, double write_ratio) {
  std::mt19937 rng(0);
  std::uniform_real_distribution<> dist(0.0, 1.0);

  int counter = 0;
  for (auto _ : state) {
    // only 1 thread writes, others read
    if (state.thread_index() == 0) {
      if (dist(rng) < write_ratio) {
        lock.write(++counter);
      }
    } else {
      auto d = lock.read();
      benchmark::DoNotOptimize(d);
    }
  }
}

template <template <typename> class Lock, short WriteRatioPercentage>
static void BM_Lock_WR(benchmark::State& state) {
  static Lock<int> lock;
  double write_ratio = (double)WriteRatioPercentage / 100.;
  BenchmarkRW(state, lock, write_ratio);
}

template <typename T>
class SharedMutexLock {
 public:
  SharedMutexLock() : value_(T()) {}

  T read() {
    std::shared_lock lock(mutex_);
    return value_;
  }

  void write(T value) {
    std::unique_lock lock(mutex_);
    value_ = value;
  }

 private:
  T value_;
  std::shared_mutex mutex_;
};

template <typename T>
class MutexLock {
 public:
  MutexLock() : value_(T()) {}

  T read() {
    std::lock_guard lock(mutex_);
    return value_;
  }

  void write(int value) {
    std::lock_guard lock(mutex_);
    value_ = value;
  }

 private:
  T value_;
  std::mutex mutex_;
};

// Small write percentage benchmarks
BENCHMARK_TEMPLATE(BM_Lock_WR, seq_lock::SeqLock, SMALL_WRITE_PERCENTAGE)
    ->Threads(2)
    ->Threads(4)
    ->Threads(8)
    ->Threads(16)
    ->Name("seq_lock/SeqLock/SmallWritePercentage");
BENCHMARK_TEMPLATE(BM_Lock_WR, SharedMutexLock, SMALL_WRITE_PERCENTAGE)
    ->Threads(2)
    ->Threads(4)
    ->Threads(8)
    ->Threads(16)
    ->Name("seq_lock/SharedMutex/SmallWritePercentage");
BENCHMARK_TEMPLATE(BM_Lock_WR, MutexLock, SMALL_WRITE_PERCENTAGE)
    ->Threads(2)
    ->Threads(4)
    ->Threads(8)
    ->Threads(16)
    ->Name("seq_lock/Mutex/SmallWritePercentage");

// // Large write percentage benchmarks
BENCHMARK_TEMPLATE(BM_Lock_WR, seq_lock::SeqLock, LARGE_WRITE_PERCENTAGE)
    ->Threads(2)
    ->Threads(4)
    ->Threads(8)
    ->Threads(16)
    ->Name("seq_lock/SeqLock/SmallWritePercentage");
BENCHMARK_TEMPLATE(BM_Lock_WR, SharedMutexLock, LARGE_WRITE_PERCENTAGE)
    ->Threads(2)
    ->Threads(4)
    ->Threads(8)
    ->Threads(16)
    ->Name("seq_lock/SharedMutex/SmallWritePercentage");
BENCHMARK_TEMPLATE(BM_Lock_WR, MutexLock, LARGE_WRITE_PERCENTAGE)
    ->Threads(2)
    ->Threads(4)
    ->Threads(8)
    ->Threads(16)
    ->Name("seq_lock/Mutex/SmallWritePercentage");

BENCHMARK_MAIN();