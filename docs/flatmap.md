# Base40-Compressed Symbol Encoding with Embedded Index (64-bit Key Format)

This document outlines a compact and efficient encoding scheme for representing IEX 8-character symbols along with a 14-bit internal index using a single `uint64_t`. The design is ideal for high-performance symbol table lookups, memory-efficient storage, and cache-friendly data access patterns.


## Overview

Each IEX symbol is:

- Exactly **8 characters**, space-padded on the right
- Comprised of a limited set of uppercase letters, digits, and a few special characters
- Represented in ASCII as 8 bytes (64 bits)

This encoding compresses the symbol into **50 bits using base40**, leaving **14 bits** for an internal index — all packed into a single 64-bit unsigned integer.

## Encoding Layout

| Field             | Bits | Description                          |
|------------------|------|--------------------------------------|
| Base40 Symbol     | 50   | Encoded from up to 9 characters      |
| Internal Index ID | 14   | Custom ID (up to 16,383 values)      |
| **Total**         | 64   | Fits in `uint64_t`                   |

## Base40 Alphabet

The chosen alphabet supports IEX-compatible symbols and includes:

```

ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-\_␣  ← total: 40

````

Where `'␣'` is the space character.

Each character maps to an integer in `[0, 39]`.

## Encoding Logic

- Symbols are mapped to base-40 digits, packed into the upper 50 bits
- The internal index is stored in the lower 14 bits
- The encoded value is stored in a `uint64_t`

```cpp
uint64_t encode(std::string_view symbol, uint16_t index);
std::string decode_symbol(uint64_t packed);
uint16_t decode_index(uint64_t packed);
````

* If fewer than 9 base40 characters are provided, trailing positions are filled with `' '` (index 39)
* Maximum of 9 base40 characters can be encoded within 50 bits

## Advantages

*  **Space-efficient**: 1 symbol + 1 index in 8 bytes
*  **Fast lookup**: use as a direct key in sorted vectors or flat hash maps
*  **One-pass decode**: no heap allocation or complex parsing
*  **Reversible**: lossless compression + recovery of symbol and index
*  **Index-embedded keys**: excellent for symbol-to-contract mapping

## Caveats

* Symbols must be validated to contain only allowed base40 characters
* Lexicographical sorting may require care due to base40 digit mapping
* Cannot encode arbitrary Unicode or lowercase letters

## Example Usage

```cpp
using compressed_symbol_t = uint64_t;

compressed_symbol_t key = encode("AAPL", 1337);

std::string sym = decode_symbol(key);     // "AAPL"
uint16_t idx = decode_index(key);         // 1337
```

##  Applications

* Trading system symbol → contract ID mapping
* Fast in-memory symbol resolution tables
* Compact serialization of symbol-index pairs

*Varga Consulting · 2025 · MIT License*

```
