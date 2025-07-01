#include <gtest/gtest.h>

#include <atomic>
#include <memory>
#include <thread>
#include <vector>

#include "../src/mpmc_queue.hpp"

using mpmc_queue::MPMCQueue;

// Test that MPMCQueue panics (throws) when size is not a power of 2
TEST(MPMCQueue, ConstructorThrowsOnNonPowerOfTwo) {
  EXPECT_THROW(MPMCQueue<int> queue3(3), std::invalid_argument);
  EXPECT_THROW(MPMCQueue<int> queue5(5), std::invalid_argument);
  EXPECT_THROW(MPMCQueue<int> queue7(7), std::invalid_argument);
  EXPECT_THROW(MPMCQueue<int> queue9(9), std::invalid_argument);

  EXPECT_NO_THROW(MPMCQueue<int> queue2(2));
  EXPECT_NO_THROW(MPMCQueue<int> queue4(4));
  EXPECT_NO_THROW(MPMCQueue<int> queue8(8));
  EXPECT_NO_THROW(MPMCQueue<int> queue16(16));
}

// Test that queue works with move-only types
TEST(MPMCQueue, MoveOnlyType) {
  MPMCQueue<std::unique_ptr<int>> queue(4);

  auto ptr = std::make_unique<int>(123);
  ASSERT_TRUE(queue.push(std::move(ptr)));
  ASSERT_EQ(ptr, nullptr);

  std::unique_ptr<int> out_ptr;
  ASSERT_TRUE(queue.pop(out_ptr));
  ASSERT_NE(out_ptr, nullptr);
  ASSERT_EQ(*out_ptr, 123);

  // Should be empty again
  ASSERT_FALSE(queue.pop(out_ptr));
}

// Test that queue can handle wrap-around correctly
TEST(MPMCQueue, WrapAround) {
  MPMCQueue<int> queue(4);

  // Fill and empty the queue twice to force wrap-around
  for (int round = 0; round < 2; ++round) {
    for (int i = 0; i < 4; ++i) {
      ASSERT_TRUE(queue.push(i + round * 10));
    }
    ASSERT_FALSE(queue.push(99));  // Should be full

    for (int i = 0; i < 4; ++i) {
      int value = -1;
      ASSERT_TRUE(queue.pop(value));
      ASSERT_EQ(value, i + round * 10);
    }
    int dummy;
    ASSERT_FALSE(queue.pop(dummy));  // Should be empty
  }
}

// Test that pop returns false when queue is empty, even after wrap-around
TEST(MPMCQueue, PopEmptyAfterWrapAround) {
  MPMCQueue<int> queue(2);

  int value;
  ASSERT_FALSE(queue.pop(value));
  ASSERT_TRUE(queue.push(1));
  ASSERT_TRUE(queue.pop(value));
  ASSERT_EQ(value, 1);
  ASSERT_FALSE(queue.pop(value));
  ASSERT_TRUE(queue.push(2));
  ASSERT_TRUE(queue.pop(value));
  ASSERT_EQ(value, 2);
  ASSERT_FALSE(queue.pop(value));
}

// Basic push and pop test
TEST(MPMCQueue, PushPopSingleThread) {
  MPMCQueue<int> queue(8);

  int value;
  // Initially empty
  ASSERT_FALSE(queue.pop(value));

  // Push and pop
  ASSERT_TRUE(queue.push(42));
  ASSERT_TRUE(queue.pop(value));
  ASSERT_EQ(value, 42);

  // Should be empty again
  ASSERT_FALSE(queue.pop(value));
}

// Fill and empty the queue
TEST(MPMCQueue, FillAndEmpty) {
  MPMCQueue<int> queue(4);

  // Fill the queue
  ASSERT_TRUE(queue.push(1));
  ASSERT_TRUE(queue.push(2));
  ASSERT_TRUE(queue.push(3));
  ASSERT_TRUE(queue.push(4));
  // Now full, next push should fail
  ASSERT_FALSE(queue.push(5));

  // Pop all
  int value;
  ASSERT_TRUE(queue.pop(value));
  ASSERT_EQ(value, 1);
  ASSERT_TRUE(queue.pop(value));
  ASSERT_EQ(value, 2);
  ASSERT_TRUE(queue.pop(value));
  ASSERT_EQ(value, 3);
  ASSERT_TRUE(queue.pop(value));
  ASSERT_EQ(value, 4);
  // Now empty
  ASSERT_FALSE(queue.pop(value));
}

// Multi-producer, multi-consumer scenario
TEST(MPMCQueue, MultiProducerMultiConsumerSum) {
  constexpr unsigned int N = 100000;
  constexpr unsigned int PRODUCERS = 4;
  constexpr unsigned int CONSUMERS = 4;
  MPMCQueue<int> queue(1024);

  std::atomic<unsigned int> produced{0};
  std::atomic<unsigned int> consumed{0};
  std::atomic<unsigned int> sum{0};

  std::vector<std::thread> producers, consumers;

  for (unsigned int p = 0; p < PRODUCERS; ++p) {
    producers.emplace_back([&]() {
      for (;;) {
        unsigned int i = produced.fetch_add(1);
        if (i >= N) break;
        while (!queue.push(i)) {
        }
      }
    });
  }

  for (unsigned int c = 0; c < CONSUMERS; ++c) {
    consumers.emplace_back([&]() {
      int value, local_sum = 0;
      for (;;) {
        unsigned int idx = consumed.load();
        if (idx >= N) break;
        if (queue.pop(value)) {
          local_sum += value;
          consumed.fetch_add(1);
        }
      }
      sum.fetch_add(local_sum);
    });
  }

  for (auto& t : producers) t.join();
  for (auto& t : consumers) t.join();

  unsigned int expected_sum = (N - 1) * N / 2;
  ASSERT_EQ(sum.load(), expected_sum);
}