#pragma once

namespace common {

constexpr int CACHE_LINE_SIZE = std::hardware_constructive_interference_size;
// Typical value for Linux and most modern CPUs
static_assert(CACHE_LINE_SIZE == 64);

}  // namespace common
