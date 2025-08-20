
## :material-archive-outline:{.icon} **Archiving Tick Data** — Store once, query forever (fast, lossless, compressed)
```bash
iex2h5 -o ~/iex.h5 -c irts ~/data/*.pcap.gz
```
This command extracts the tick stream from one or more IEX TOPS datasets — whether gzip-compressed or not — and stores it in an HDF5 file. Within the container, datasets are organized by trading date under the `/irts` group. These archived streams can later be reloaded using `iex2h5` and transformed into RTS price matrices, OHLC series, or other derived formats — all without data loss or reprocessing.


## :fontawesome-solid-money-bills:{.icon} **From Ticks to Prices** — resample to bars/OHLCV/RTS with one command.
```bash
iex2h5 -o ~/rts.h5 -c rts --time-interval 00:01:00 
  --date-range 2025-01-01:2025-01-31  --time-range 14:30:00-21:00:00 ~/iex.h5
```
This command converts raw tick data into regularly spaced price matrices. It resamples trades, bids, asks, and volumes into fixed time intervals (e.g. 1-minute bars) over the specified date and time range. For example, a full trading day with 1-minute intervals yields 390 rows per instrument.

Columns correspond to instruments — including inactive ones — and their positions are **stable over time**, thanks to the incremental instrument database. To drop unused columns, you can either:

* Apply a **permutation vector** in post-processing, or
* Use a **custom instrument map** during sampling (functionally equivalent to permuting the output matrix)

## :fontawesome-brands-nfc-symbol: **Managing Contracts (Symbols ⇄ IDs)** (TBD/not yet implemented)


## :material-clock-start:{.icon} **Quickstart Demo** — Run IEX2H5 in 60 Seconds and Explore the Layout
```bash
iex2h5 -o ~/iex.h5 -c irts ~/data/*.pcap.gz
```
This is the fastest way to get started: just point to your downloaded PCAP files and fire.
By default, IEX2H5 extracts all tick-level data from the IEX stream and stores it in an HDF5 container — combining **archival tick stream storage** with **regular interval price matrices** in one go. It’s perfect for exploration or sharing small but complete datasets with others.

## :simple-blockbench:{.icon} **Performance Benchmarks** — tick-stream ingest & query vs other formats.
![memory layout](assets/demo-pcap-csv.png#float-right-70)
The IEX2H5 tool provides a powerful benchmarking framework for evaluating the performance of tick-stream ingestion and querying across multiple output formats—HDF5, CSV, JSON, and REDIS. It accepts raw or gzipped PCAP files as input and produces normalized tick streams in each target format, enabling direct comparison of storage footprint, I/O throughput, and query responsiveness. 
![memory layout](assets/struct-csv.png#float-right-30)

While originally intended to highlight performance differences between formats, IEX2H5’s flexible CLI makes it a convenient converter as well. With simple invocations like iex2h5 -o ticks.h5 -c irts *.pcap, users can perform one-shot conversions from PCAP to any supported backend, including pushing real-time streams to Redis for downstream consumption.
The output format is deduced from `-o` or `--output` option depending how it ends `.h5 | .csv | .json` triggers the respective formats, whereas redis url must start with `redis://` here is the full url format: `redis://[[username:]password@]host[:port][/db]`

### :fontawesome-solid-wrench:{.icon} **Profiling** — ballpark or measure performance with built-in tools

To benchmark the performance of `iex2h5`, a built-in profiling mode is available using **gperftools**. To enable it, build the project with profiling support by setting appropriate flags: compile with `CXXFLAGS="-O3 -g"` and configure CMake using `-DUSE_GOOGLE_PROFILER=ON` along with `RelWithDebInfo` build type. Then build the project normally with `cmake --build build --parallel`.

Once compiled, you can run a profiling session with a representative dataset:

```bash
./build/src/iex2h5 -n 1 /lake/iex/tops/TOPS-2017-01-03.pcap.gz
```

This generates a profiling output file `iex2h5.prof`, which can be converted to Callgrind format using:

```bash
pprof-symbolize --callgrind ./build/src/iex2h5 iex2h5.prof > iex2h5.callgrind
kcachegrind iex2h5.callgrind
```

Profiling support is conditionally enabled in the CMake configuration when `libprofiler` is found:

```cmake
option(USE_GOOGLE_PROFILER "Enable Google CPU profiler if found" OFF)
```

When both `USE_GOOGLE_PROFILER` and `GoogleProfiler_FOUND` are true, `HAVE_GOOGLE_PROFILER` is defined and linked, allowing runtime hooks:

```cpp
#ifdef HAVE_GOOGLE_PROFILER
  ProfilerStart("iex2h5.prof");
  // main workload
  ProfilerStop();
#endif
```
This lightweight sampling profiler is thread-aware and integrates with `pprof-symbolize` to generate interactive traces for `kcachegrind`. See [gperftools](https://github.com/gperftools/gperftools) and [pprof](https://github.com/google/pprof) for more on these tools.

## :material-location-exit:{.icon} **Stopping/Exiting** running software
Either hitting  ++ctrl+c++ or sending sending the signals below with `killall -SIGINT iex2h5` 

```cpp
	std::signal(SIGINT, signal_handler); std::signal(SIGTERM, signal_handler);
	std::signal(SIGHUP, signal_handler); std::signal(SIGQUIT, signal_handler);
```


[100]: https://github.com/gperftools/gperftools
[101]: https://github.com/google/pprof
[102]: (https://kcachegrind.sourceforge.net/