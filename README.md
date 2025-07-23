
[![CI](https://github.com/vargaconsulting/iex2h5/actions/workflows/ci.yml/badge.svg)](https://github.com/vargaconsulting/iex2h5/actions/workflows/ci.yml)
[![codecov](https://codecov.io/gh/vargaconsulting/iex2h5/branch/main/graph/badge.svg)](https://codecov.io/gh/vargaconsulting/iex2h5)
[![MIT License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)
[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.15677290.svg)](https://doi.org/10.5281/zenodo.15677290)
[![GitHub release](https://img.shields.io/github/v/release/vargaconsulting/iex2h5.svg)](https://github.com/vargaconsulting/iex2h5/releases)
[![Documentation](https://img.shields.io/badge/docs-stable-blue)](https://vargaconsulting.github.io/iex2h5)

# IEX2H5: IEX TOPS Dataset to HDF5 Converter
A high-performance C++ utility for converting [IEX Transport Protocol (IEX-TP)][100] packet captures into structured HDF5 datasets, suitable for financial analytics, scientific computation, and time-series processing.

## Build Matrix

| OS / Compiler | GCC 13      | GCC 14      | GCC 15      | Clang 17      | Clang 18      | Clang 19      |Clang 20       |
|---------------|-------------|-------------|-------------|---------------|---------------|---------------|---------------|
| Ubuntu 22.04  |![gcc13][200]|![gcc14][201]|![gcc15][202]|![clang17][250]|![clang18][251]|![clang19][252]|![clang20][253]|
| Ubuntu 24.04  |![gcc13][300]|![gcc14][301]|![gcc15][302]|![clang17][350]|![clang18][351]|![clang19][352]|![clang20][353]|

## 📦 Installation
```bash
sudo apt install build-essential cmake
cmake -DCMAKE_BUILD_TYPE=Release -S . -B build
cmake --build build --parallel
sudo cmake --install build
```

# Example Usage: Convert IEX TOPS Dataset
```
steven@gauss:~/projects/iex2h5/build$ iex2h5 --help
IEX2H5 converts IEX TOPS Datasets to HDF5 Format

iex2h5 is a specialized tool for importing IEX TOPS datasets into the HDF5 format,enabling efficient storage and analysis of large-scale financial data. HDF5 is a widelyadopted format for managing hierarchical, structured data and is supported across majorenvironments such as Julia, Python, MATLAB, C++, and Node.js.

This tool allows users to convert captured packet data streams (e.g. IEX DEEP/TOPS) intostructured HDF5 datasets for quantitative analysis, visualization, and integration withscientific, engineering, or trading workflows.
Usage: ./iex2h5 [--help] [--version] [--time-interval VAR] [--time-range VAR] [--date-range VAR] [--output VAR] [--rts-path VAR] [--instruments-path VAR] [--trading-days-path VAR] [--gzip VAR] [--convert VAR] [--third-party-licenses] [remaining]...

Positional arguments:
  remaining               [nargs: 0 or more] 

Optional arguments:
  -h, --help              shows help message 
  --version               Print version information 
  --time-interval         temporal interval in hh::mm::ss format, irts stream is converted into [nargs=0..1] [default: "00:01:00"]
  --time-range            Time window in UTC, specified as START-END (e.g. 14:30:00-21:00:00). Events outside this range are ignored. [nargs=0..1] [default: "14:30:00-21:00:00"]
  --date-range            Inclusive trading date range in format START:END (e.g. 2016-12-01:2020-01-01). Use 'today' as a valid END value. [nargs=0..1] [default: "2016-12-01:today"]
  -o, --output            path to the HDF5 container [nargs=0..1] [default: "./iex.h5"]
  --rts-path              HDF5 path for regular time index [nargs=0..1] [default: "/time.txt"]
  --instruments-path      HDF5 path for instrument (symbol) list [nargs=0..1] [default: "/instruments.txt"]
  --trading-days-path     HDF5 path for trading day index [nargs=0..1] [default: "/trading_days.txt"]
  -g, --gzip              Compression level (0 = none, 9 = maximum) [nargs=0..1] [default: 1]
  -c, --convert           Which conversion pipeline to run: rts | irts | none | all [nargs=0..1] [default: "all"]
  --third-party-licenses  Print license(s) for a third-party library (or 'all | license 01 [, license 02, ...]') 


Examples:
   ./iex2h5 -o ~/iex.h5 -c irts ~/data/202{4,5}-{04,05}-??.pcap.gz # Convert gzipped PCAP files to IRTS (brace expansion and globs supported)
   ./iex2h5 -o rts.h5  --time-interval 00:00:10 -c rts iex.h5      # Load IRTS from HDF5 and convert to RTS matrices at 10-seconds intervals
   ./iex2h5 -o ~/iex.h5 -c irts ~/data/**/*.pcap.gz                # Convert gzipped PCAP files to IRTS tickdata and store in HDF5 format
   ./iex2h5 -o rts.h5  --time-interval 00:05:00 -c rts *.pcap.gz   # Load IRTS from HDF5 and convert to RTS matrices at 5-minutes intervals

[iex2h5] Market data © IEX — Investors Exchange. Attribution required. See https://iextrading.com
Copyright © 2017–2025 Varga Consulting, Toronto, ON, Canada   info@vargaconsulting.ca

```

### Notice:
“[Data provided][100] for free by IEX. By accessing or using IEX Historical Data, you agree to the [IEX Historical Data Terms of Use][101].”

[100]: https://iextrading.com/trading/market-data/
[101]: https://www.iexexchange.io/legal/hist-data-terms
[200]: https://vargaconsulting.github.io/iex2h5/badges/ubuntu-22.04-gcc-13.svg
[201]: https://vargaconsulting.github.io/iex2h5/badges/ubuntu-22.04-gcc-14.svg
[202]: https://vargaconsulting.github.io/iex2h5/badges/ubuntu-22.04-gcc-15.svg
[300]: https://vargaconsulting.github.io/iex2h5/badges/ubuntu-24.04-gcc-13.svg
[301]: https://vargaconsulting.github.io/iex2h5/badges/ubuntu-24.04-gcc-14.svg
[302]: https://vargaconsulting.github.io/iex2h5/badges/ubuntu-24.04-gcc-15.svg
[250]: https://vargaconsulting.github.io/iex2h5/badges/ubuntu-22.04-clang-17.svg
[251]: https://vargaconsulting.github.io/iex2h5/badges/ubuntu-22.04-clang-18.svg
[252]: https://vargaconsulting.github.io/iex2h5/badges/ubuntu-22.04-clang-19.svg
[253]: https://vargaconsulting.github.io/iex2h5/badges/ubuntu-22.04-clang-20.svg
[350]: https://vargaconsulting.github.io/iex2h5/badges/ubuntu-24.04-clang-17.svg
[351]: https://vargaconsulting.github.io/iex2h5/badges/ubuntu-24.04-clang-18.svg
[352]: https://vargaconsulting.github.io/iex2h5/badges/ubuntu-24.04-clang-19.svg
[353]: https://vargaconsulting.github.io/iex2h5/badges/ubuntu-24.04-clang-20.svg
[400]: https://vargaconsulting.github.io/iex2h5/badges/macos-13-gcc-13.svg
[401]: https://vargaconsulting.github.io/iex2h5/badges/macos-13-gcc-14.svg
[402]: https://vargaconsulting.github.io/iex2h5/badges/macos-13-gcc-15.svg
[450]: https://vargaconsulting.github.io/iex2h5/badges/macos-13-clang-17.svg
[451]: https://vargaconsulting.github.io/iex2h5/badges/macos-13-clang-18.svg
[452]: https://vargaconsulting.github.io/iex2h5/badges/macos-13-clang-19.svg
[453]: https://vargaconsulting.github.io/iex2h5/badges/macos-13-clang-20.svg