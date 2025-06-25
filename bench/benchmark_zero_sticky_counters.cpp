#include <benchmark/benchmark.h>

#include <atomic>
#include <random>

#include "../src/lock_free_zero_sticky_counter.hpp"
#include "../src/wait_free_zero_sticky_counter.hpp"

// Ratios for increments, decrements, and reads (percentages)
// Reads fill the rest
constexpr short INC_RATIO_VERY_LOW = 5;
constexpr short INC_RATIO_LOW = 10;
constexpr short INC_RATIO_HIGH = 45;
constexpr short INC_RATIO_VERY_HIGH = 80;

constexpr short DEC_RATIO_VERY_LOW = 5;
constexpr short DEC_RATIO_LOW = 10;
constexpr short DEC_RATIO_HIGH = 45;
constexpr short DEC_RATIO_VERY_HIGH = 80;

template <typename Counter>
void BenchmarkCounter(benchmark::State& state, Counter& counter,
                      double inc_ratio, double dec_ratio) {
  std::mt19937 rng(state.thread_index());  // Different seed per thread
  std::uniform_real_distribution<> dist(0.0, 1.0);

  for (auto _ : state) {
    double op = dist(rng);
    if (op < inc_ratio) {
      counter.incrementIfNotZero();
    } else if (op < inc_ratio + dec_ratio) {
      counter.decrement();
    } else {
      benchmark::DoNotOptimize(counter.read());
    }
  }
}

template <typename Counter, short IncRatio, short DecRatio>
static void BM_Counter_Ratio(benchmark::State& state) {
  static Counter counter;
  double inc_ratio = static_cast<double>(IncRatio) / 100.0;
  double dec_ratio = static_cast<double>(DecRatio) / 100.0;
  BenchmarkCounter(state, counter, inc_ratio, dec_ratio);
}

// Low increment, low decrement (mostly reads)
BENCHMARK_TEMPLATE(BM_Counter_Ratio,
                   lock_free_zero_sticky_counter::LockFreeZeroStickyCounter,
                   INC_RATIO_LOW, DEC_RATIO_LOW)
    ->Threads(2)
    ->Threads(4)
    ->Threads(8)
    ->Threads(16)
    ->Threads(32)
    ->Name("LockFreeZeroStickyCounter/Inc10_Dec10_Read80");
BENCHMARK_TEMPLATE(BM_Counter_Ratio,
                   wait_free_zero_sticky_counter::WaitFreeZeroStickyCounter,
                   INC_RATIO_LOW, DEC_RATIO_LOW)
    ->Threads(2)
    ->Threads(4)
    ->Threads(8)
    ->Threads(16)
    ->Threads(32)
    ->Name("WaitFreeZeroStickyCounter/Inc10_Dec10_Read80");

// High increment, low decrement
BENCHMARK_TEMPLATE(BM_Counter_Ratio,
                   lock_free_zero_sticky_counter::LockFreeZeroStickyCounter,
                   INC_RATIO_HIGH, DEC_RATIO_LOW)
    ->Threads(2)
    ->Threads(4)
    ->Threads(8)
    ->Threads(16)
    ->Threads(32)
    ->Name("LockFreeZeroStickyCounter/Inc45_Dec10_Read45");
BENCHMARK_TEMPLATE(BM_Counter_Ratio,
                   wait_free_zero_sticky_counter::WaitFreeZeroStickyCounter,
                   INC_RATIO_HIGH, DEC_RATIO_LOW)
    ->Threads(2)
    ->Threads(4)
    ->Threads(8)
    ->Threads(16)
    ->Threads(32)
    ->Name("WaitFreeZeroStickyCounter/Inc45_Dec10_Read45");

// Low increment, high decrement
BENCHMARK_TEMPLATE(BM_Counter_Ratio,
                   lock_free_zero_sticky_counter::LockFreeZeroStickyCounter,
                   INC_RATIO_LOW, DEC_RATIO_HIGH)
    ->Threads(2)
    ->Threads(4)
    ->Threads(8)
    ->Threads(16)
    ->Threads(32)
    ->Name("LockFreeZeroStickyCounter/Inc10_Dec45_Read45");
BENCHMARK_TEMPLATE(BM_Counter_Ratio,
                   wait_free_zero_sticky_counter::WaitFreeZeroStickyCounter,
                   INC_RATIO_LOW, DEC_RATIO_HIGH)
    ->Threads(2)
    ->Threads(4)
    ->Threads(8)
    ->Threads(16)
    ->Threads(32)
    ->Name("WaitFreeZeroStickyCounter/Inc10_Dec45_Read45");

// High increment, high decrement (few reads)
BENCHMARK_TEMPLATE(BM_Counter_Ratio,
                   lock_free_zero_sticky_counter::LockFreeZeroStickyCounter,
                   INC_RATIO_HIGH, DEC_RATIO_HIGH)
    ->Threads(2)
    ->Threads(4)
    ->Threads(8)
    ->Threads(16)
    ->Threads(32)
    ->Name("LockFreeZeroStickyCounter/Inc45_Dec45_Read10");
BENCHMARK_TEMPLATE(BM_Counter_Ratio,
                   wait_free_zero_sticky_counter::WaitFreeZeroStickyCounter,
                   INC_RATIO_HIGH, DEC_RATIO_HIGH)
    ->Threads(2)
    ->Threads(4)
    ->Threads(8)
    ->Threads(16)
    ->Threads(32)
    ->Name("WaitFreeZeroStickyCounter/Inc45_Dec45_Read10");

BENCHMARK_MAIN();

// Increasing is clear majority
BENCHMARK_TEMPLATE(BM_Counter_Ratio,
                   lock_free_zero_sticky_counter::LockFreeZeroStickyCounter,
                   INC_RATIO_VERY_HIGH, DEC_RATIO_VERY_LOW)
    ->Threads(2)
    ->Threads(4)
    ->Threads(8)
    ->Threads(16)
    ->Threads(32)
    ->Name("LockFreeZeroStickyCounter/Inc80_Dec5_Read15");
BENCHMARK_TEMPLATE(BM_Counter_Ratio,
                   wait_free_zero_sticky_counter::WaitFreeZeroStickyCounter,
                   INC_RATIO_VERY_HIGH, DEC_RATIO_VERY_LOW)
    ->Threads(2)
    ->Threads(4)
    ->Threads(8)
    ->Threads(16)
    ->Threads(32)
    ->Name("WaitFreeZeroStickyCounter/Inc80_Dec5_Read15");

// Decreasing is clear majority
BENCHMARK_TEMPLATE(BM_Counter_Ratio,
                   lock_free_zero_sticky_counter::LockFreeZeroStickyCounter,
                   INC_RATIO_VERY_LOW, DEC_RATIO_VERY_HIGH)
    ->Threads(2)
    ->Threads(4)
    ->Threads(8)
    ->Threads(16)
    ->Threads(32)
    ->Name("LockFreeZeroStickyCounter/Inc5_Dec80_Read15");
BENCHMARK_TEMPLATE(BM_Counter_Ratio,
                   wait_free_zero_sticky_counter::WaitFreeZeroStickyCounter,
                   INC_RATIO_VERY_LOW, DEC_RATIO_VERY_HIGH)
    ->Threads(2)
    ->Threads(4)
    ->Threads(8)
    ->Threads(16)
    ->Threads(32)
    ->Name("WaitFreeZeroStickyCounter/Inc5_Dec80_Read15");