
# Data Consumers

This project uses a modular consumer architecture to handle incoming decoded packets.

## Consumer Types

- **RTS Consumer**: Aggregates packets into regular time intervals (e.g., 10s) and emits snapshot matrices.
- **IRTS Consumer**: Appends best bid/offer and trade events into an HDF5 packet table stream.

## Module Design

All consumers inherit from `io::base::consumer_t`, and implement:

```cpp
virtual void trade_report(...);
virtual void quote_update(...);
virtual void begin_day(...);
virtual void end_day(...);
````

For implementation details, see `src/consumers.hpp`.

## Reference Protocols

This design maps to:

* [IEX TOPS Specification v164 (PDF)](spec/IEX%20TOPS%20Specification%20v164.pdf)
* [IEX DEEP Specification v105 (PDF)](spec/IEX%20DEEP%20Specification%20v105.pdf)


