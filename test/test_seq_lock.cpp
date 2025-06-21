#include <gtest/gtest.h>

#include <atomic>
#include <deque>
#include <numeric>
#include <thread>

#include "../src/seq_lock.hpp"

TEST(SeqLockTest, BasicWriteRead) {
  seq_lock::SeqLock<int> lock;
  lock.write(42);
  int val = lock.read();
  ASSERT_EQ(val, 42);
}

TEST(SeqLockTest, MultipleWritesReads) {
  seq_lock::SeqLock<int> lock;
  for (int i = 0; i < 100; ++i) {
    lock.write(i);
    int val = lock.read();
    ASSERT_EQ(val, i);
  }
}

TEST(SeqLock, ConcurrentReadWrite) {
  seq_lock::SeqLock<int> lock;
  constexpr int num_iterations = 10000;
  std::atomic<bool> done{false};

  std::thread writer([&] {
    for (int i = 1; i <= num_iterations; ++i) {
      lock.write(i);
    }
    done = true;
  });

  std::thread reader([&] {
    int last_val = 0;
    while (!done) {
      int val = lock.read();
      ASSERT_GE(val, last_val);
      last_val = val;
    }
  });

  writer.join();
  reader.join();
}