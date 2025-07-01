# Lock-free Data Structures and Algorithms

[![Build and Run Tests][build-run-tests-badge]][build-run-tests-url]

[build-run-tests-badge]: https://github.com/olearczuk/lock-free-cpp/actions/workflows/build_run_tests.yml/badge.svg
[build-run-tests-url]: https://github.com/olearczuk/lock-free-cpp/actions/workflows/build_run_tests.yml

## Implemented data structures
<details>
<summary><a href="https://en.wikipedia.org/wiki/Seqlock">SeqLock</a> - <a href="src/seq_lock.hpp">src/seq_lock.hpp</a></summary>

[Can Seqlocks Get Along With Programming Language Memory Models](https://web.archive.org/web/20210506174408/https://www.hpl.hp.com/techreports/2012/HPL-2012-68.pdf) — *Hans-J. Boehm*

</details>

<details>
<summary>Zero Sticky Counter</summary>

Sticky counter is a concurrent counter that never goes below zero. Once the counter reaches zero, it becomes "sticky" and can not be incremented ago.<br/>
Inspiration comes from [Introduction to Wait-free Algorithms in C++ Programming - Daniel Anderson - CppCon 2024](https://www.youtube.com/watch?v=kPh8pod0-gk&list=PLr05g8IRfRd6kAxBpmpGsijzlVLCuuPqZ)

### LockFreeZeroStickyCounter - [src/lock_free_zero_sticky_counter.hpp](src/lock_free_zero_sticky_counter.hpp)
It avoids locks by using atomic operations, making it suitable for high-performance, multi-threaded environments.

### WaitFreeZeroStickyCounter - [src/wait_free_zero_sticky_counter.hpp](src/wait_free_zero_sticky_counter.hpp)
Same as `LockFreeZeroStickyCounter` but it's wait-free instead.

</details>

<details>
<summary>Single Producer Single Consumer Queue - <a href="src/spsc_queue.hpp">src/spsc_queue.hpp</a></summary>

Lock-free implementation of Single Producer Single Consumer Queue.<br/>
Inspiration comes from [Single Producer Single Consumer Lock-free FIFO From the Ground Up - Charles Frasch - CppCon 2023](https://www.youtube.com/watch?v=K3P_Lmq6pw0&list=PLr05g8IRfRd51NGMQ-9X_BuTHHhzro5P0) as well as [Erik Rigtorp's SPSCQueue implementation](https://github.com/rigtorp/SPSCQueue/tree/master)

</details>

<details>
<summary>Multi Producer Multi Consumer Queue - <a href="src/mpmc_queue.hpp">src/mpmc_queue.hpp</a></summary>

Lock-free implementation of Multi Producer Multi Consumer Queue.<br/>
Inspiration comes from [User API and C++ Implementation of a Multi Producer, Multi Consumer, Lock Free, Atomic Queue - Erez Strauss - CppCon 2024
](https://www.youtube.com/watch?v=bjz_bMNNWRk), my implementation is a more basic version of Erez's implementation.
</details>

## Tests and benchmarks
All tests are in [tests](tests/) subdirectory and all benchmarks are in [benchmarks](benchmarks/) subdirectory.
```bash
cmake -E make_directory build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make
# Run tests
./tests
# Run benchmarks
# SeqLock vs mutex and shared mutex
./bench_seq_lock
# LockFreeZeroStickyCounter vs WaitFreeZeroStickyCounter
./bench_zero_sticky_counters
# SPSCQueue vs mutex-based vs Erik Rigtorp's SPSCQueue implementation
./bench_spsc_queue
# MPMCQueue vs mutex-based
./bench_mpmc_queue
```
