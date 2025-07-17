
# Profiling and Base Performance Optimization in `iex2h5`

##  Overview

This document describes the profiling strategy used to identify and optimize performance bottlenecks in the `iex2h5` application. The profiling workflow combines both **sampling-based** and **instrumentation-based** methods, using tools like Google CPU Profiler (gperftools), Valgrind Callgrind, and KCachegrind to analyze runtime behavior and uncover hotspots in decompression, symbol lookup, and matrix operations.

## Toolchain

### Profilers Used

* [`gperftools`](https://github.com/gperftools/gperftools) — sampling-based CPU profiler, thread-aware
* [`pprof-symbolize`](https://github.com/google/pprof) — converts gperftools `.prof` output into Callgrind format
* `nm`, `readelf`, and `ldd` — used to trace symbol resolution and linkage issues
* [`kcachegrind`](https://kcachegrind.sourceforge.net/) — GUI frontend for Callgrind output


## Build Configuration

### Recommended CMake flags for profiling optimized builds

```bash
CXXFLAGS="-O3 -g"
cmake -DUSE_GOOGLE_PROFILER=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo -S . -B build
cmake --build build --parallel
```

Run and collect profile data using:

```bash
./build/src/iex2h5 -n 1 /lake/iex/tops/TOPS-2017-01-03.pcap.gz # outputs './iex2h5.prof'
pprof-symbolize --callgrind ./build/src/iex2h5 iex2h5.prof > iex2h5.callgrind
kcachegrind iex2h5.callgrind
```

## Google Profiler Integration

Profiling is enabled conditionally based on the presence of `libprofiler`:

```cmake
option(USE_GOOGLE_PROFILER "Enable Google CPU profiler if found" OFF)

if(USE_GOOGLE_PROFILER AND GoogleProfiler_FOUND)
  add_definitions(-DHAVE_GOOGLE_PROFILER)
  list(APPEND LIBS ${GOOGLE_PROFILER_LIBRARY})
endif()
```

At runtime:

```cpp
#ifdef HAVE_GOOGLE_PROFILER
  ProfilerStart("iex2h5.prof");
  INFO << "<<<<<<<<<<<<< profiler started (output: iex2h5.prof) >>>>>>>>>>>>" << std::endl;
#endif

// main dispatch

#ifdef HAVE_GOOGLE_PROFILER
  ProfilerStop();
  INFO << "<<<<<<<<<<<< profiler stopped >>>>>>>>>>>>" << std::endl;
#endif
```

### **Flat Profile Summary (Optimized Build)**

| Inclusive | Self  | Called | Function                                    | Location                       |
| --------- | ----- | ------ | ------------------------------------------- | ------------------------------ |
| 2,448     | 1,730 | 2,448  | `inflate_fast_avx512`                       | `inffast_tpl.h`                |
| 928       | 811   | 1,856  | `std::ranges::__lower_bound_fn::operator`   | `ranges_algo.h`                |
| 486       | 486   | 486    | `_mm256_mask_storeu_epi8`                   | `avx512vbwintrin.h`            |
| 364       | 364   | 364    | `arma::Mat::operator[]`                     | `Mat_meat.hpp`                 |
| 331       | 331   | 331    | `zng_inflate_table`                         | `inftrees.c`                   |
| 210       | 208   | 210    | `h5::pt_t::append`                          | `H5Dappend.hpp`                |
| 1,197     | 144   | 1,197  | `io::base::consumer_t::find_or_insert`      | `consumers.hpp`                |
| 687       | 687   | 687    | `CHUNKCOPY`                                 | `chunkset_avx512.c`            |
| 122       | 122   | 122    | `iex::transport_t::transport_handler`       | `iex.hpp`                      |
| 95        | 95    | 95     | `__GI___libc_read`                          | `read.c`                       |
| 1,270     | 69    | 1,270  | `utils::base64::impl::encode`               | `base64.hpp`                   |
| 120       | 72    | 120    | `io::base::consumer_t::operator[]`          | `consumers.hpp`                |
| 72        | 72    | 72     | `__gnu_cxx::__normal_iterator::operator++`  | `stl_iterator.h`               |
| 61        | 61    | 61     | `arma::Mat::operator[]`                     | `Mat_meat.hpp`                 |
| 54        | 54    | 54     | `_bzhi_u32`                                 | `bmi2intrin.h`                 |
| 42        | 41    | 42     | `filters::ema_filter_t::update_impl`        | `filters.hpp`                  |
| 38        | 36    | 38     | `utils::base64::impl::symbol_char_to_index` | `base64.hpp`                   |
| 32        | 32    | 32     | `__gnu_cxx::__normal_iterator::operator++=` | `stl_iterator.h`               |
| 2,045     | 40    | 2,045  | `iex::transport_t::tops_v163`               | `iex.hpp`                      |
| 3,158     | 34    | 3,158  | `io::stream::gzip_t::pull`                  | `producers.hpp`                |
| 28        | 28    | 28     | `__memcpy_evex_unaligned_erms`              | `memmove-vec-unaligned-erms.S` |
| 26        | 26    | 26     | `chunkunroll_avx512`                        | `chunkset_tpl.h`               |


