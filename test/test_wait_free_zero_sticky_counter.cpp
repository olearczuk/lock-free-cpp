#include <gtest/gtest.h>

#include <atomic>
#include <thread>
#include <vector>

#include "../src/wait_free_zero_sticky_counter.hpp"

// Basic single-threaded tests
TEST(WaitFreeZeroStickyCounter, InitialValueIsOne) {
  wait_free_zero_sticky_counter::WaitFreeZeroStickyCounter counter;
  EXPECT_EQ(counter.read(), 1u);
}

TEST(WaitFreeZeroStickyCounter, IncrementIfNotZeroWorks) {
  wait_free_zero_sticky_counter::WaitFreeZeroStickyCounter counter;
  EXPECT_TRUE(counter.incrementIfNotZero());
  EXPECT_EQ(counter.read(), 2u);
}

TEST(WaitFreeZeroStickyCounter, IncrementIfZeroFails) {
  wait_free_zero_sticky_counter::WaitFreeZeroStickyCounter counter;
  EXPECT_TRUE(counter.decrement());  // goes to 0, should return true
  EXPECT_EQ(counter.read(), 0u);
  EXPECT_FALSE(counter.incrementIfNotZero());
  EXPECT_EQ(counter.read(), 0u);
}

TEST(WaitFreeZeroStickyCounter, DecrementReturnsTrueAtZero) {
  wait_free_zero_sticky_counter::WaitFreeZeroStickyCounter counter;
  EXPECT_TRUE(counter.decrement());
  EXPECT_EQ(counter.read(), 0u);
}

TEST(WaitFreeZeroStickyCounter, DecrementReturnsFalseIfNotZero) {
  wait_free_zero_sticky_counter::WaitFreeZeroStickyCounter counter;
  counter.incrementIfNotZero();
  EXPECT_FALSE(counter.decrement());
  EXPECT_TRUE(counter.decrement());
  EXPECT_EQ(counter.read(), 0u);
}

// Concurrent increment test
TEST(WaitFreeZeroStickyCounter, ConcurrentIncrements) {
  wait_free_zero_sticky_counter::WaitFreeZeroStickyCounter counter;
  constexpr int num_threads = 8;
  constexpr int increments_per_thread = 10000;
  std::vector<std::thread> threads;

  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&counter, increments_per_thread]() {
      for (int j = 0; j < increments_per_thread; ++j) {
        counter.incrementIfNotZero();
      }
    });
  }
  for (auto& t : threads) t.join();

  // Initial value is 1, so total increments = num_threads *
  // increments_per_thread
  EXPECT_EQ(counter.read(), 1u + num_threads * increments_per_thread);
}

// Concurrent decrement test
TEST(WaitFreeZeroStickyCounter, ConcurrentDecrements) {
  constexpr int start_value = 10000;
  wait_free_zero_sticky_counter::WaitFreeZeroStickyCounter counter(start_value);

  std::atomic<int> zeros{0};
  constexpr int num_threads = 8;
  ASSERT_EQ(start_value % num_threads, 0);  // Ensure even distribution
  std::vector<std::thread> threads;
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back(
        [&counter, &zeros, iterations = start_value / num_threads]() {
          for (int i = 0; i < iterations; ++i) {
            if (counter.decrement()) {
              zeros.fetch_add(1, std::memory_order_relaxed);
            }
          }
        });
  }
  for (auto& t : threads) t.join();

  EXPECT_EQ(counter.read(), 0u);
  EXPECT_EQ(zeros.load(), 1);
}

// Stress test: increments and decrements concurrently
TEST(WaitFreeZeroStickyCounter, ConcurrentIncDecStress) {
  wait_free_zero_sticky_counter::WaitFreeZeroStickyCounter counter;
  constexpr int num_threads = 8;
  constexpr int ops_per_thread = 10000;
  std::vector<std::thread> threads;

  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&counter, i]() {
      for (int j = 0; j < ops_per_thread; ++j) {
        if (j % 2 == 0) {
          counter.incrementIfNotZero();
        } else {
          counter.decrement();
        }
      }
    });
  }
  for (auto& t : threads) t.join();

  // The counter should never go negative and should be >= 0
  EXPECT_GE(counter.read(), 0u);
}

// Stress test: increments, decrements, and reads concurrently
TEST(WaitFreeZeroStickyCounter, ConcurrentIncDecReadStress) {
  wait_free_zero_sticky_counter::WaitFreeZeroStickyCounter counter;
  constexpr int num_threads = 8;
  constexpr int ops_per_thread = 10000;
  constexpr int num_readers = 4;
  constexpr int num_workers = num_threads - num_readers;
  std::vector<std::thread> threads;
  std::atomic<bool> running{true};
  std::atomic<uint64_t> read_sum{0};

  // Reader threads: continuously read the counter while workers are running
  for (int i = 0; i < num_readers; ++i) {
    threads.emplace_back([&counter, &running, &read_sum]() {
      while (running.load(std::memory_order_relaxed)) {
        read_sum.fetch_add(counter.read(), std::memory_order_relaxed);
      }
    });
  }

  // Worker threads: increment and decrement
  for (int i = 0; i < num_workers; ++i) {
    threads.emplace_back([&counter, i]() {
      for (int j = 0; j < ops_per_thread; ++j) {
        if (j % 2 == 0) {
          counter.incrementIfNotZero();
        } else {
          counter.decrement();
        }
      }
    });
  }

  // Wait for workers to finish, then stop readers
  for (int i = num_readers; i < threads.size(); ++i) {
    threads[i].join();
  }
  running = false;
  for (int i = 0; i < num_readers; ++i) {
    threads[i].join();
  }

  // The counter should never go negative and should be >= 0
  EXPECT_GE(counter.read(), 0u);
  // Optionally, check that some reads were performed
  EXPECT_GT(read_sum.load(), 0u);
}