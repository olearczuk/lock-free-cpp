 # Lock-free Data Structures and Algorithms
 - [SeqLock](https://en.wikipedia.org/wiki/Seqlock) - [src/seq_lock.hpp](src/seq_lock.hpp)
    - [Can Seqlocks Get Along With Programming Language Memory Models?](https://web.archive.org/web/20210506174408/https://www.hpl.hp.com/techreports/2012/HPL-2012-68.pdf) — *Hans-J. Boehm*

## Tests and benchmarks
All tests are in [tests](tests/) subdirectory and all benchmarks are in [benchmarks](benchmarks/) subdirectory.
```bash
cmake -E make_directory build && cd build && cmake .. && make
# Run tests
./tests
# Run benchmarks
./benchmarks
```
