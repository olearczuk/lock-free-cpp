#include <gtest/gtest.h>

#include <memory>
#include <numeric>
#include <print>
#include <thread>

#include "../src/spsc_queue.hpp"

using spsc_queue::SPSCQueue;

// Test that SPSCQueue panics (throws) when size is not a power of 2
TEST(SPSCQueue, ConstructorThrowsOnNonPowerOfTwo) {
  EXPECT_THROW(SPSCQueue<int> queue3(3), std::invalid_argument);
  EXPECT_THROW(SPSCQueue<int> queue5(5), std::invalid_argument);
  EXPECT_THROW(SPSCQueue<int> queue7(7), std::invalid_argument);
  EXPECT_THROW(SPSCQueue<int> queue9(9), std::invalid_argument);

  EXPECT_NO_THROW(SPSCQueue<int> queue2(2));
  EXPECT_NO_THROW(SPSCQueue<int> queue4(4));
  EXPECT_NO_THROW(SPSCQueue<int> queue8(8));
  EXPECT_NO_THROW(SPSCQueue<int> queue16(16));
}

// Test that queue works with move-only types
TEST(SPSCQueue, MoveOnlyType) {
  SPSCQueue<std::unique_ptr<int>> queue(4);

  auto ptr = std::make_unique<int>(123);
  ASSERT_TRUE(queue.push(std::move(ptr)));
  ASSERT_EQ(ptr, nullptr);

  auto out_ptr = queue.front();
  ASSERT_NE(out_ptr, nullptr);
  ASSERT_NE(*out_ptr, nullptr);
  ASSERT_EQ(**out_ptr, 123);
  queue.pop();

  // Should be empty again
  ASSERT_EQ(queue.front(), nullptr);
}

// Test that queue can handle wrap-around correctly
TEST(SPSCQueue, WrapAround) {
  SPSCQueue<int> queue(4);

  // Fill and empty the queue twice to force wrap-around
  for (int round = 0; round < 2; ++round) {
    for (int i = 0; i < 4; ++i) {
      ASSERT_TRUE(queue.push(i + round * 10));
    }
    ASSERT_FALSE(queue.push(99));  // Should be full

    for (int i = 0; i < 4; ++i) {
      int* val_ptr = queue.front();
      ASSERT_NE(val_ptr, nullptr);
      ASSERT_EQ(*val_ptr, i + round * 10);
      queue.pop();
    }
    ASSERT_EQ(queue.front(), nullptr);  // Should be empty
  }
}

// Test that pop returns false when queue is empty, even after wrap-around
TEST(SPSCQueue, PopEmptyAfterWrapAround) {
  SPSCQueue<int> queue(2);

  ASSERT_EQ(queue.front(), nullptr);
  ASSERT_TRUE(queue.push(1));

  int* val_ptr = queue.front();
  ASSERT_NE(val_ptr, nullptr);
  ASSERT_EQ(*val_ptr, 1);
  queue.pop();

  ASSERT_EQ(queue.front(), nullptr);
  ASSERT_TRUE(queue.push(2));

  val_ptr = queue.front();
  ASSERT_NE(val_ptr, nullptr);
  ASSERT_EQ(*val_ptr, 2);
  queue.pop();

  ASSERT_EQ(queue.front(), nullptr);
}

// Basic push and pop test
TEST(SPSCQueue, PushPopSingleThread) {
  SPSCQueue<int> queue(8);

  // Initially empty
  ASSERT_EQ(queue.front(), nullptr);

  // Push and pop
  ASSERT_TRUE(queue.push(42));
  int* val_ptr = queue.front();
  ASSERT_NE(val_ptr, nullptr);
  ASSERT_EQ(*val_ptr, 42);
  queue.pop();

  // Should be empty again
  ASSERT_EQ(queue.front(), nullptr);
}

// Fill and empty the queue
TEST(SPSCQueue, FillAndEmpty) {
  SPSCQueue<int> queue(4);

  // Fill the queue
  ASSERT_TRUE(queue.push(1));
  ASSERT_TRUE(queue.push(2));
  ASSERT_TRUE(queue.push(3));
  ASSERT_TRUE(queue.push(4));
  // Now full, next push should fail
  ASSERT_FALSE(queue.push(5));

  // Pop all
  int* val_ptr = queue.front();
  ASSERT_NE(val_ptr, nullptr);
  ASSERT_EQ(*val_ptr, 1);
  queue.pop();

  val_ptr = queue.front();
  ASSERT_NE(val_ptr, nullptr);
  ASSERT_EQ(*val_ptr, 2);
  queue.pop();

  val_ptr = queue.front();
  ASSERT_NE(val_ptr, nullptr);
  ASSERT_EQ(*val_ptr, 3);
  queue.pop();

  val_ptr = queue.front();
  ASSERT_NE(val_ptr, nullptr);
  ASSERT_EQ(*val_ptr, 4);
  queue.pop();
  // Now empty
  ASSERT_EQ(queue.front(), nullptr);
}

// Producer-consumer scenario
TEST(SPSCQueue, ProducerConsumerSum) {
  constexpr unsigned int N = 100000;
  SPSCQueue<int> queue(1024);

  unsigned int sum = 0;
  std::thread producer([&queue]() {
    for (unsigned int i = 0; i < N; ++i) {
      // Wait until push succeeds (queue may be full)
      while (!queue.push(i)) {
      }
    }
  });

  std::thread consumer([&queue, &sum]() {
    for (unsigned int i = 0; i < N; ++i) {
      // Wait until front is not empty
      int* val_ptr = nullptr;
      while ((val_ptr = queue.front()) == nullptr) {
      }
      ASSERT_EQ(*val_ptr, i);
      sum += *val_ptr;
      queue.pop();
    }
  });

  producer.join();
  consumer.join();

  // Check sum
  unsigned int expected_sum = (N - 1) * N / 2;
  ASSERT_EQ(sum, expected_sum);
}