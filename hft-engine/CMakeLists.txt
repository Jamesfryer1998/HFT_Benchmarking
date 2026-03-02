cmake_minimum_required(VERSION 3.20)
project(hft_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Optimization flags
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")

# Default to Release build
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Fetch Google Benchmark
include(FetchContent)
FetchContent_Declare(
    benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.8.3
)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Disable benchmark tests" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "Disable gtest tests" FORCE)
FetchContent_MakeAvailable(benchmark)

# Main engine library
add_library(hft_engine_lib
    src/order_book.cpp
    src/matching_engine.cpp
    src/feed_simulator.cpp
)
target_include_directories(hft_engine_lib PUBLIC src)

# Main executable
add_executable(hft_engine
    src/main.cpp
)
target_link_libraries(hft_engine PRIVATE hft_engine_lib)

# Benchmark executable
add_executable(hft_benchmarks
    benchmarks/bench_order_book.cpp
    benchmarks/bench_latency.cpp
)
target_link_libraries(hft_benchmarks PRIVATE hft_engine_lib benchmark::benchmark)

# Test executable
add_executable(hft_tests
    tests/test_matching.cpp
)
target_link_libraries(hft_tests PRIVATE hft_engine_lib)
