#pragma once

namespace common {

// Assume 64 bytes for cache line size (typical for Linux and modern CPUs)
constexpr int CACHE_LINE_SIZE = 64;

}  // namespace common
