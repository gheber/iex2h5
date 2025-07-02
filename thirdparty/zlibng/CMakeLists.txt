# This file is part of the IEX2H5 project and is licensed under the MIT License.
#
# Copyright © 2017–2025 Varga Consulting, Toronto, ON, Canada 🇨🇦
# Contact: info@vargaconsulting.ca

set(name zng)
set(libname lib${name}_static)
project(${libname} LANGUAGES C)

set(WITH_GZFILEOP ON CACHE BOOL "Enable gzFile functions" FORCE)
set(ZLIB_COMPAT OFF CACHE BOOL "Enable zlib compatible API" FORCE)
set(WITH_OPTIM ON CACHE BOOL "Enable SIMD optimizations" FORCE)
set(WITH_NEW_STRATEGIES ON CACHE BOOL "Enable new compression strategies" FORCE)
set(WITH_RUNTIME_CPU_DETECTION ON CACHE BOOL "Enable runtime CPU detection" FORCE)

set(ZLIB_ENABLE_SYMBOL_PREFIX OFF CACHE BOOL "Enable symbol prefixing" FORCE)
#set(ZLIB_SYMBOL_PREFIX "iex_" CACHE STRING "Symbol prefix" FORCE)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Disable shared library build" FORCE)
set(WITH_NATIVE_INSTRUCTIONS OFF CACHE BOOL "Use -march=native" FORCE)
set(WITH_FUZZERS OFF CACHE BOOL "Disable fuzzing tools" FORCE)
set(WITH_BENCHMARKS OFF CACHE BOOL "Disable benchmarks" FORCE)
set(WITH_BENCHMARK_APPS OFF CACHE BOOL "Disable benchmark applications" FORCE)
set(WITH_GTEST OFF CACHE BOOL "Disable GTest" FORCE)
set(ZLIB_ENABLE_TESTS OFF CACHE BOOL "Disable zlib tests" FORCE)
set(ZLIBNG_ENABLE_TESTS OFF CACHE BOOL "Disable zlib-ng tests" FORCE)

# Optional: enforce strictness or invalid inflate behavior
set(WITH_INFLATE_STRICT OFF CACHE BOOL "Strict inflate distance checks" FORCE)
set(WITH_INFLATE_ALLOW_INVALID_DIST OFF CACHE BOOL "Allow zero-fill for invalid distances" FORCE)
set(WITH_REDUCED_MEM OFF CACHE BOOL "Reduce memory usage (slower)" FORCE)

set(current_version_dir "v2.2.4")
add_subdirectory(${current_version_dir})
add_library(libzng_static ALIAS zlib)