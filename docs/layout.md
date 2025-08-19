---
hide:
  - toc
---

![prices](assets/data-layout.png#float-right-50)
The `iex2h5` system organizes market data into a structured **HDF5 container**, which behaves like a logical file system — grouping time-series datasets into typed, self-describing directories.

It supports two primary representations of the market:

* **IRTS (Irregular Time Series):** tick-by-tick bid, ask, and trade events
* **RTS (Regular Time Series):** resampled OHLC-style snapshots at fixed intervals

While the example diagram shows **both IRTS and RTS stored in a single HDF5 file**, this is mostly for convenience and demonstration purposes. In real-world, high-throughput deployments (e.g. streaming tick data from multiple sources 24/7), this is **not recommended** without an extra layer due to HDF5’s **Single-Writer, Multiple-Reader (SWMR)** constraint.

🗃 **Recommended Practice:** Store **IRTS** (the raw tick stream) in compressed, append-only containers  → e.g. one file per exchange say: `iex.h5, dydx.h5, binance.h5, ...` then generate **RTS** datasets *on demand* from the IRTS archive say: `delta-neutral-strategy-backtest.h5` 

This design balances long-term archival with real-time analytics, and ensures data remains accessible across languages and platforms without compromising write performance.

### Structure of the RTS Price Matrix
![prices](assets/matrix.png#float-left-30)
The prices matrix shown above has 4290 rows and 10 columns. Each row marks the end of a fixed-duration trading interval — typically aligned with the Close in an OHLC series — while each column corresponds to a different stock or instrument. Column-to-symbol mappings can be reverse-resolved using the accompanying instruments.txt index file. Most cells contain regular price values, but some could be marked as NaN, which stands for “Not a Number.”

In numerical computing, `NaN` (short for *Not a Number*) is a special value used to represent missing or undefined data. This is especially common in financial time series — for example, when a stock hasn’t traded during a given interval, or its first quote hasn’t appeared yet. Instead of filling these gaps with zeros or placeholders, `NaN` makes the absence explicit. For highly liquid symbols like `NVDA`, `AMZN`, `SPY`, `GOOGL`, `AMD`, or `AAPL`, you're unlikely to see `NaN`s — these stocks trade continuously. But for thinly traded names, delisted symbols, or stale contract IDs still present in the dataset, `NaN` values will show up, indicating periods with no valid market data.

In general, trades occur between the bid and ask prices. The `price` matrix reflects this relationship: when both bid and ask quotes are present, a **mid-price** is computed by the filtering mechanism and recorded as the representative price. However, these synthetic midpoints are **not included** in the trading statistics — this ensures that only actual executed trades contribute to volume or activity metrics, making it easier to filter out illiquid or infrequently traded instruments.

### Tickdata stream

![prices](assets/tick-struct.png#float-right-30)
The `tick_t` structure represents the most granular level of market activity in the IEX2H5 system — capturing individual bid/ask quotes and trades as they happen. Each tick includes a high-precision **timestamp** (as recorded by the exchange), the **price**, **size**, and a compact **contract ID** that identifies the security. The final field, `flags`, is a bitfield union that efficiently encodes metadata about the event — indicating whether it’s a bid or ask quote, a trade, or a level removal from the order book.

In practice, this tick data is stored as a vector of structs — conceptually similar to working with a large `std::vector<tick_t>` in C++. This vector is serialized using HDF5’s compound datatype support and stored in compressed blocks called **chunks**. This binary layout preserves the full fidelity of the market stream, making it suitable for **market replay**, **order flow analysis**, and **fine-grained statistical modeling**. With over 245 million ticks in the example below, **efficient storage and compression** become essential — and that's exactly what IEX2H5 delivers through its compact layout and `gzip` filtering.

```
245503474-element Vector{@NamedTuple{time::UInt64, price::Float32, size::UInt32, contract_id::UInt16, flags::UInt16}}:
 (time = 0x185896be8c2b291c, price = 629.45996, size = 0x00000065, contract_id = 0x029d, flags = 0x0004)
 (time = 0x185896be8c2b291c, price = 629.39996, size = 0x00000064, contract_id = 0x029d, flags = 0x0001)
 (time = 0x185896be8c2c14b1, price = 206.76999, size = 0x00000078, contract_id = 0x01bf, flags = 0x0004)
 ⋮
 (time = 0x1858ac06c1e9485a, price = 38.45, size = 0x0000012c, contract_id = 0x0d86, flags = 0x0004)
```

### 📈 `/stats/` — Intraday Trade Statistics
Summarized metrics for each trading day, useful for sanity checks, feature extraction, or visualization.

| Dataset           | Description                           | Motivation                           |
| ----------------- | ------------------------------------- |--------------------------------------|
| `event_count`     | Total events seen on the wire         | Filter out slow lazy stocks: asks and bids are included | 
| `first_trade`     | Timestamp of first trade (per symbol) | Filter out stocks by intra day time     |
| `last_trade`      | Timestamp of last trade (per symbol)  |                                         |
| `trade_count`     | Total trades (per symbol)             | Filter out intrequently traded stocks   |
| `trade_size`      | Average trade size                    | Indicator for low or high traded volume |


## 🗜️ Compression

The `iex2h5` tool supports **gzip compression levels** from `--gzip 0` (no compression) to `--gzip 9` (maximum). Typical results:

* **IRTS datasets** see a **3–5× space reduction**
* **Compared to PCAP**, total savings can reach **6–10×**, as irrelevant protocol data is stripped away

This makes **HDF5 a vastly more efficient archival format** than raw `pcap.gz`, especially for long-term retention or transfer between systems. For teams with large-scale cloud storage needs, a commercial version is in development offering **up to 100× compression**. This unlocks:

* Dramatic savings on object storage costs (S3, GCS, Azure)
* Faster network sync and replication

> 📬 Want early access? [Contact me](mailto:info@vargaconsulting.ca) to discuss tailored storage pipelines for your use case.

See also:

- [IEX Transport Specification v125](spec/IEX%20Transport%20Specification%20v125.pdf)
- [IEX DEEP SNAP Spec 0.05](spec/IEX%20DEEP%20SNAP%20Specification%200.05.pdf)

