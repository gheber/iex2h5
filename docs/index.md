# IEX2H5 Documentation

Welcome to the **IEX2H5** project — a high-performance C++ application designed to convert IEX stock market data, captured in raw Ethernet frames, into structured and query-friendly HDF5 format.

## Project Overview

IEX2H5 processes historical datasets from the [Investors Exchange (IEX)](https://iextrading.com), spanning from **December 2016 to present**. These datasets, captured as Ethernet packet traces, are available under the IEX free license terms. This project converts those packet captures into HDF5, enabling streamlined use in:

- **Backtesting frameworks**
- **Algorithmic trading systems**
- **Scientific and financial time-series analytics**

It supports two primary modes:

- **RTS (Regular Time Series)**: Aggregated snapshots over fixed intervals (e.g., 10 seconds)
- **IRTS (Irregular Time Series)**: High-fidelity tick streams for best bid/offer and trade events

## Key Features

- Parses **IEX TOPS** (Top of Book) market data from raw `.pcap.gz` files
- Writes output in compressed **HDF5** format with schema optimized for timeseries
- Provides `--init`, `--irts`, `--rts`, and `--index` commands for flexible workflows
- Lightweight C++ implementation using `H5CPP`, `Armadillo`, and Boost
- Compatible with Julia, Python, MATLAB, and C/C++ for downstream consumption

## Quickstart

```bash
sudo apt install libhdf5-dev pigz
mkdir build && cd build
cmake ../ && make -j
sudo make install
````

Convert a file:

```bash
unpigz -c tops.pcap.gz | ./iex2h5 -g 9 --command rts -o ~/iex.h5
```
