# Usage Guide

The `iex2h5` utility provides a command-line interface for converting IEX packet captures into HDF5.

## Basic Commands

```bash
./iex2h5 --command init
./iex2h5 --command irts
./iex2h5 --command rts
./iex2h5 --command index
````

## Options

* `--time-interval`: Time bucket width in seconds (used in RTS mode)
* `--start/--stop`: UTC time filters for trading hours
* `--gzip`: Compression level (0 = none, 9 = max)
* `--output`: Path to generated HDF5 file

## Workflow Examples

```bash
# Initialize file with symbol information
unpigz -c tops.pcap.gz | ./iex2h5 --command init

# Convert all pcap files into RTS time series
for f in repo/*.pcap.gz; do
  unpigz -c $f | ./iex2h5 --command rts -g 9 -o ~/iex.h5
done
```

