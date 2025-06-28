#pragma once

namespace common {

#if defined(__cpp_lib_hardware_interference_size)
constexpr int CACHE_LINE_SIZE = std::hardware_constructive_interference_size;
#else
// Typical value for Linux and most modern CPUs
constexpr int CACHE_LINE_SIZE = 64;
#endif

}  // namespace common
