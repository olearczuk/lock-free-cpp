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

add_executable(tests test/test_seq_lock.cpp)
target_link_libraries(tests gtest gtest_main)
add_test(NAME tests COMMAND tests)

# Google Benchmark support
FetchContent_Declare(
  benchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG v1.8.3
)
FetchContent_MakeAvailable(benchmark)

add_executable(benchmarks bench/benchmark_seq_lock.cpp)
target_link_libraries(benchmarks PRIVATE benchmark::benchmark pthread)