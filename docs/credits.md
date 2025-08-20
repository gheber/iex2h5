---
hide:
  - toc
---

| Library                | Purpose                                                                     | License       | Author / Maintainer                    |
|------------------------|-----------------------------------------------------------------------------|---------------|----------------------------------------|
| [h5cpp][100]           | High-level HDF5 C++ interface for IRTS/RTS persistence and custom filters   | MIT           | Steven Varga                           |
| [hdf5][101]            | Core binary format and low-level C API for structured time-series storage   | BSD-like      | The HDF Group, NCSA                    |
| [libzng][102]          | Efficient decompression of `.pcap.gz` files using zlib-compatible API       | Zlib          | Jean-loup Gailly, Mark Adler           |
| [armadillo][103]       | Matrix operations for RTS construction and signal postprocessing            | Apache 2.0    | Conrad Sanderson, Ryan Curtin          |
| [argparse][104]        | Argument parsing for CLI interface (`iex2h5` flags and options)             | MIT           | Daniel Bayer (p-ranav)                 |
| [date][105]            | Time parsing, timezone handling, and chrono compatibility utilities         | MIT           | Howard Hinnant                         |
| [fmt][106]             | Formatting diagnostics and benchmark output with `std::format` compatibility| MIT           | Victor Zverovich                       |
| [doctest][107]         | Unit testing framework for validating HDF5 read/write and time slicing logic| MIT           | Viktor Kirilov                         |
| [hiredis][108]         | Low-level Redis C client for writing tick snapshots to Redis streams        | BSD 3-Clause  | Salvatore Sanfilippo, Pieter Noordhuis |
| [redis-plus-plus][109] | Modern C++ interface over hiredis for contract cache and key-value syncing  | Apache 2.0    | Qi Gao (sewenew)                       |

[100]: https://github.com/steven-varga/h5cpp
[101]: https://github.com/HDFGroup/hdf5
[102]: https://github.com/zlib-ng/zlib-ng
[103]: https://gitlab.com/conradsnicta/armadillo-code
[104]: https://github.com/p-ranav/argparse
[105]: https://github.com/HowardHinnant/date
[106]: https://github.com/fmtlib/fmt
[107]: https://github.com/doctest/doctest
[108]: https://github.com/redis/hiredis
[109]: https://github.com/sewenew/redis-plus-plus