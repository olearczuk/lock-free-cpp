 ![Build and Run Tests](https://github.com/olearczuk/lock-free-cpp/actions/workflows/build_run_tests.yml/badge.svg?branch=main)
 # Lock-free Data Structures and Algorithms
 ### [SeqLock](https://en.wikipedia.org/wiki/Seqlock) - [src/seq_lock.hpp](src/seq_lock.hpp)
[Can Seqlocks Get Along With Programming Language Memory Models](https://web.archive.org/web/20210506174408/https://www.hpl.hp.com/techreports/2012/HPL-2012-68.pdf) — *Hans-J. Boehm*
### Zero Sticky Counter
Sticky counter is a concurrent counter that never goes below zero. Once the counter reaches zero, it becomes "sticky" and can not be incremented ago.<br/>
Inspiration comes from [Introduction to Wait-free Algorithms in C++ Programming - Daniel Anderson - CppCon 2024](https://www.youtube.com/watch?v=kPh8pod0-gk&list=PLr05g8IRfRd6kAxBpmpGsijzlVLCuuPqZ)
 ### LockFreeZeroStickyCounter - [src/lock_free_zero_sticky_counter](src/lock_free_zero_sticky_counter)
It avoids locks by using atomic operations, making it suitable for high-performance, multi-threaded environments.
 ### WaitFreeZeroStickyCounter - [src/wait_free_zero_sticky_counter](src/wait_free_zero_sticky_counter)
 Same as `LockFreeZeroStickyCounter` but it's wait-free instead.

## Tests and benchmarks
All tests are in [tests](tests/) subdirectory and all benchmarks are in [benchmarks](benchmarks/) subdirectory.
```bash
cmake -E make_directory build && cd build && cmake .. && make
# Run tests
./tests
# Run benchmarks
# SeqLock vs mutex and shared mutex
./bench_seq_lock
# LockFreeZeroStickyCounter vs WaitFreeZeroStickyCounter
./bench_zero_sticky_counters
```
