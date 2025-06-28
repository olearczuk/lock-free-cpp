cmake_minimum_required(VERSION 3.28)
project(lock-free VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)

add_executable(lock_free main.cpp)

enable_testing()

# Google Test support
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
FetchContent_MakeAvailable(googletest)

file(GLOB TEST_SOURCES test/*.cpp)
add_executable(tests ${TEST_SOURCES})
target_link_libraries(tests gtest gtest_main)
add_test(NAME tests COMMAND tests)

# Google Benchmark support
FetchContent_Declare(
  benchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG v1.8.3
)
FetchContent_MakeAvailable(benchmark)

add_executable(bench_seq_lock bench/benchmark_seq_lock.cpp)
add_executable(bench_zero_sticky_counters bench/benchmark_zero_sticky_counters.cpp)
add_executable(bench_spsc_queue bench/benchmark_spsc_queue.cpp)
target_link_libraries(bench_seq_lock PRIVATE benchmark::benchmark pthread)
target_link_libraries(bench_zero_sticky_counters PRIVATE benchmark::benchmark pthread)
target_link_libraries(bench_spsc_queue PRIVATE benchmark::benchmark pthread)