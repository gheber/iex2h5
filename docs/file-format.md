
# HDF5 Output File Format

The `iex2h5` application writes structured HDF5 containers optimized for time-series processing.

## Layout

```
├── instruments.txt         # symbol table (e.g., AAPL, GOOGL)
├── trading_days.txt        # timestamps of active sessions
├── time.txt                # RTS interval buckets
├── irts/                   # Irregular tick stream (packet table)
└── rts/                    # Regular time series (matrix snapshots)
```

## Compression

HDF5 output supports gzip levels from `--gzip 0` (no compression) to `--gzip 9` (maximum).

## Schema Reference

- Symbols: `std::vector<symbol_t>` in fixed-width base40 encoding
- Time-series: `arma::mat` snapshots by contract
- Tick events: Packet-table with timestamp, type, price, size, etc.

See also:

- [IEX Transport Specification v125](spec/IEX%20Transport%20Specification%20v125.pdf)
- [IEX DEEP SNAP Spec 0.05](spec/IEX%20DEEP%20SNAP%20Specification%200.05.pdf)

