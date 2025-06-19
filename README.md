
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
sudo apt install libhdf5-dev pigz
mkdir build && cmake build && cmake ../
make -j 12 && sudo make install
```

# Example Usage: Convert IEX TOPS Dataset
```
steven@jupyter:~/projects/iex2h5/src$ ./iex2h5 --help
IEX2H5 converts IEX TOPS Datasets to HDF5 Format

iex2h5 is a specialized tool for importing IEX TOPS datasets into the HDF5 data format,
enabling efficient  storage and analysis of large  financial datasets. HDF5 is a widely
used file format for handling large, complex, and hierarchical data, supported by major
programming languages including Julia, Python, MATLAB, C, C++, and Node.js.

This application allows users to convert captured packet data streams (e.g., DEEP/TOPS)
into structured HDF5 datasets for advanced analytics and seamless integration into
scientific, engineering, and financial workflows.

Usage: ./iex2h5 [--help] [--time-interval VAR] [--start VAR] [--stop VAR] [--output VAR] [--rts-path VAR] [--instruments-path VAR] [--trading-days-path VAR] [--gzip VAR] [--command VAR]

Optional arguments:
  -h, --help           shows help message 
  --time-interval      temporal interval in seconds, irts stream is converted into [nargs=0..1] [default: 10]
  --start              lower bound in UTC, considers events only after [nargs=0..1] [default: "14:30:00"]
  --stop               upper bound in UTC, considers events only before [nargs=0..1] [default: "21:00:00"]
  -o, --output         path to the HDF5 container [nargs=0..1] [default: "./iex.h5"]
  --rts-path           hdf5-group/directory for regular time interval index [nargs=0..1] [default: "/time.txt"]
  --instruments-path   hdf5-group/directory for listed [symbols|assets|financial] instruments [nargs=0..1] [default: "/instruments.txt"]
  --trading-days-path  hdf5-group/directory for active trading days [nargs=0..1] [default: "/trading_days.txt"]
  -g, --gzip           0-9 0 for no compression, 9 for highest [nargs=0..1] [default: 0]
  -c, --command        init  - intitialises hdf5 container with retrieved symbols from irts/stream
                       irts  - saves captured events as irts stream
                       rts   - converts irts to rts
                       index - scans and rebuilds trading day index
                       
 [nargs=0..1] [default: "rts"]


example:
   unpigz -c tops.pcap.gz | ./iex2h5 -g 9 --time-interval 10 --command init
   for file in repo/*.pcap.gz; do unpigz -c ${file} | iex2h5 -g 9 --command rts -o ${HOME}/iex.h5;

Copyright © <2017-2025> Varga Consulting, Toronto, ON, info@vargaconsulting.ca
```

[100]: https://iextrading.com/trading/market-data/
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

