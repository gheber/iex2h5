# Integrating with Backtesting Systems

The HDF5 output can be easily consumed by most financial backtesting engines.

## Suggested Integration

- Read RTS datasets for OHLC generation
- Use IRTS stream for microstructure models and slippage analysis
- Load `instruments.txt` and `trading_days.txt` to align sessions

## Supported Environments

- Julia: via `HDF5.jl` or `JLD2.jl`
- Python: via `h5py`, `pandas`, or `tables`
- MATLAB: via native HDF5 I/O
- C++: via H5CPP or HDF5 C API

## Licensing & Use

You must comply with IEX license terms. See:

- [IEX Data Agreements and Forms (PDF)](spec/IEX%20Data%20Agreements%20and%20Forms.pdf)

