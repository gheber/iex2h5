---
hide:
  - toc
---
## 📜 Third-Party Library Credits

| Library             | Purpose                                                                     | License       | Author / Maintainer                    |
|---------------------|-----------------------------------------------------------------------------|---------------|----------------------------------------|
| **h5cpp**           | High-level HDF5 C++ interface for IRTS/RTS persistence and custom filters   | MIT           | Steven Varga                           |
| **hdf5**            | Core binary format and low-level C API for structured time-series storage   | BSD-like      | The HDF Group, NCSA                    |
| **libzng**          | Efficient decompression of `.pcap.gz` files using zlib-compatible API       | Zlib          | Jean-loup Gailly, Mark Adler           |
| **armadillo**       | Matrix operations for RTS construction and signal postprocessing            | Apache 2.0    | Conrad Sanderson, Ryan Curtin          |
| **argparse**        | Argument parsing for CLI interface (`iex2h5` flags and options)             | MIT           | Daniel Bayer (p-ranav)                 |
| **date**            | Time parsing, timezone handling, and chrono compatibility utilities         | MIT           | Howard Hinnant                         |
| **fmt**             | Formatting diagnostics and benchmark output with `std::format` compatibility| MIT           | Victor Zverovich                       |
| **doctest**         | Unit testing framework for validating HDF5 read/write and time slicing logic| MIT           | Viktor Kirilov                         |
| **hiredis**         | Low-level Redis C client for writing tick snapshots to Redis streams         | BSD 3-Clause  | Salvatore Sanfilippo, Pieter Noordhuis |
| **redis-plus-plus** | Modern C++ interface over hiredis for contract cache and key-value syncing  | Apache 2.0    | Qi Gao (sewenew)                       |
