---
hide:
  - toc
---
The `radix64.hpp` header implements a highly compact encoding scheme for symbol names, designed to pack an **8-character IEX-style symbol** and a **16-bit internal index** into a single `uint64_t`. This representation enables **high-performance lookups**, **low-memory overhead**, and **cache-friendly access patterns** — making it ideal for real-time trading systems, contract mapping tables, or HDF5 serialization. By combining base-64 encoding with bit-packing, the system achieves fast, reversible, allocation-free symbol compression, without sacrificing flexibility or clarity.

> **Terminology.** We use “**radix-64**” to mean a positional numeral system with base 64 over a chosen alphabet $\Sigma$ (6-bit digits, big-endian). This is **not** RFC 4648 Base64 encoding of bytes.


## :material-view-grid:{.icon} Overview
![memory layout](assets/radix64-encoding-layout.png#float-right-30)
Each IEX symbol is exactly 8 characters (right-padded to length with the **minimum-code** sentinel, e.g., space with code 0), drawn from a radix-64 alphabet (A–Z, 0–9, selected punctuation), and stored in a compact 64-bit key with the symbol packed **big-endian** into the upper 48 bits (8×6) and a 16-bit contract index in the lower 16 bits; this order-preserving, reversible layout is **space-efficient** (symbol + index in 8 bytes), enables **fast lookup** as a single `uint64_t` key in sorted vectors or flat hash maps, and supports **one-pass decode** without heap allocation—note that at most **8** radix-64 characters fit in 48 bits.

# :material-exponent:{.icon} Radix 64 Symbol Encoding for IEX2H5
![memory layout](assets/symbol-memory-layout.png#float-right-30)
The IEX specification uses 8-byte padded ASCII strings (left-aligned, space-padded) to represent symbols (e.g. `"AAPL    "`). Within the IEX protoicol (internally in the pcap format) they’re stored in `uint64_t`. To give you an example the memory Layout of `"APPLE   "` (Little Endian)
is `"APPLE␣␣␣"` — 8 characters, padded with spaces. The resulting `uint64_t` value (in hex):`0x202020454C505041`
```
[0x41][0x50][0x50][0x4C][0x45][0x20][0x20][0x20]
  ↑                                      ↑
 LSB                                   MSB
```
Paper vs Memory might defeat intuition leading to a number ordering confusion on little endian platforms. Humans write numbers (and symbols) **left-to-right**, from the **most significant** to **least significant** digit. Memory in little-endian CPUs stores values **from the least significant byte first**.

## :fontawesome-solid-sack-dollar:{.icon} Bit Budget Analysis (48-bit) and  radix64 Alphabet
![memory layout](assets/radix64-alphabet.png#float-left-40)
We allocate **48 bits** for symbol/string encoding, leaving **16 bits** for indexing them. Let \(b\) be the number of bits per character. To store **8 characters** in 48 bits we have  \(b = \frac{48}{8} = 6.00\) available bits per character, similarly if we wanted to store **9 characters** we have to squeeze each character into:  \(b = \frac{48}{9} \approx 5.33\) bits. Since fractional bits would make our day misarable we have to settle with \( \left\lfloor 5.33 \right\rfloor = 5\). So it is a tossup between 5 or 6 bits. 

Approaching it from the other side we empirically measured **there are 34 different characters** making up the IEX symbol alphabet. Since \( log_2(34) \approx 5.0874 \) and we strongly dislike fractional bits we need at least \( \left\lceil log_2(34) \right\rceil = 6\) bits to represent a single character.
$$
\text{encoded} = \sum_{i=0}^{n-1} \text{alphabet\_index}[i] \cdot 64^{n - 1 - i}
$$
**Conclusion:** A **base-64 alphabet** lets us encode exactly **8 characters in 48 bits** — with 30 extra characters to make it applicable for encoding tickers of other exchanges.

## :octicons-light-bulb-24:{.icon} Compact radix64 Encoding of Ticker + :material-order-alphabetical-ascending:{.icon} Index Ordering Properties
![memory layout](assets/radix64-order.png#float-right-40)

When encoding ticker symbols using 6-bit characters (i.e. a base-64 alphabet), we concatenate the character codes to form a single base-64 number. Because the encoding alphabet is lexicographically ordered, this process preserves the relative order of the original strings.
As a result, comparisons between encoded tickers can be performed using fast integer arithmetic instead of slower string-based comparisons — a key performance benefit for high-frequency symbol lookups.

The table on the right compares the original input order, decoded symbol, the index from `original[i]`, and the lexicographically sorted symbol/index from `sorted[i]`.
Correct lexicographic behavior is preserved when using the fixed radix64 encoding logic.

## :material-text-search:{.icon} Binary Search in radix64-Encoded Flat Maps

The `encode(symbol)` function preserves lexical order, making it ideal for binary search over sorted containers. For example:

```cpp
if (auto it = std::ranges::lower_bound(sorted, utils::radix64::encode("ocean   ", 0)); it != sorted.end()) {
    auto [symbol, index] = utils::radix64::decode(*it);
    assert(symbol == "ocean   ");
} else {
    TRACE << "not found..." << std::endl;
}
```

This is a **perfect match for fixed-width symbols**, such as IEX 8-character tickers.

For trimmed symbols (e.g., `"ocean"` instead of `"ocean   "`), a post-check is needed to backtrack if necessary:

```cpp
if (auto it = std::ranges::lower_bound(sorted, utils::radix64::encode("ocean")); it != sorted.end()) {
    // `it` may point to the next symbol (e.g., "planet") — backtrack and verify
    auto [symbol, index] = utils::radix64::decode(*(--it));
    if (symbol.starts_with("ocean"))
        ; // Found "ocean", possibly padded
    else
        TRACE << "not found..." << std::endl;
} else {
    TRACE << "not found..." << std::endl;
}
```

## Why “Radix-64” ? 
Short version: I called it **radix-64** because we are using a **positional numeral system in base 64** (6-bit digits) over a custom, ordered alphabet—**not** the RFC 4648 **Base64** byte codec. 


* **Definition fit.** Each character $c\in A$ is mapped via a monotone $\sigma:A\to\{0,\dots,63\}$ and encoded

$$
    E(s_0\ldots s_{N-1})=\sum_{i=0}^{N-1}\sigma(s_i)\,64^{\,N-1-i},
$$

  i.e., a base-64 **radix** expansion (big-endian 6-bit digits). That’s exactly “radix-64”.
* **Avoids ambiguity.** “Base64” typically means **RFC 4648** (bytes→ASCII using `A–Z a–z 0–9 + /` with `=` padding). Radix64 scheme: custom alphabet, order-preserving, integer-comparables. Different thing → different name.
* **Signals the key property.** Saying “radix-64” primes the reader for **order-preserving** integer compares under the usual conditions: monotone $\sigma$, fixed length or min-code padding, big-endian digits.

1. **Requirements:** 6-bit chars, lex-order == unsigned-int order, fits in 48 bits.
2. **Model:** treat chars as **digits** in base 64 with a monotone $\sigma$.
3. **Constraints:** fixed length or pad with smallest code; encode MSB-first.
4. **Verification:** first differing index $k$ ⇒ tail $<64^{N-1-k}$ ⇒ $x<_{\text{lex}}y \iff E(x)<E(y)$.
5. **Naming:** picking radix64 describes (2) precisely what the encoding does, and isn’t overloaded by (RFC) connotations → **radix-64 positional encoding**.
