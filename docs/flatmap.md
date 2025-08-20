---
hide:
  - toc
---
In the IEX2H5 system — where data arrives as a **high-frequency stream of ticks** across many instruments — fast and efficient symbol lookup is essential. To achieve this, we use a **flat map**: a sorted vector of key–value pairs that provides both speed and simplicity, especially under heavy read workloads.

![prices](assets/hashmap-comparison.png#float-right-50)
The performance comparison chart shown here is based on data provided by [Martin Leitner-Ankerl][100].

### :material-earth-box:{.icon} What Is a Flat Map?

A flat map is a lightweight associative container implemented as a **sorted array** of `(key, value)` pairs. Lookups are performed via **binary search** using `std::lower_bound`, making it ideal for scenarios with frequent reads and relatively few insertions.

A typical example might look like:`std::vector<std::pair<std::string, uint16_t>> flat_map;` However, our implementation is even simpler and faster:`
std::vector<uint64_t> flat_map;` 
Here, each `uint64_t` value encodes both the **symbol** and its **internal index** using a compact base64 scheme. This reduces memory usage, improves cache locality, and enables zero-allocation binary search over fixed-width keys. The result is a fast, predictable, and cache-friendly structure that outperforms traditional hash maps or tree maps for symbol resolution — especially when dealing with thousands of instruments and millions of ticks per session.

### :material-chat-question:{.icon} Why Flat Maps Work Well Here

* **Cache-friendly**: contiguous memory layout → faster lookups and better prefetching
* **Zero heap churn**: avoids frequent node allocation like `std::map` or `std::unordered_map`
* **Predictable performance**: `O(log N)` lookups, `O(N)` insertions (batchable)
* **Deterministic**: no hash functions, no resizing, no hash collisions
* **Sorting = fast bulk traversal**: great for periodic summaries and compressions

For workloads with a few thousand symbols and frequent reads (but relatively rare insertions), flat maps often **outperform hash maps** in real-world performance.

### :fontawesome-solid-hashtag:{.icon} Why Not Use Perfect Hashing?

In theory, a **perfect hash map** — one that maps known symbols directly to index slots with no collisions — would be ideal for constant-time lookups.

But in practice:

> **We can’t build a perfect hash upfront, because we don’t know all the symbols ahead of time.**

In IEX feeds, symbols are **not disseminated before trading starts** — the stream begins with activity (trades or quotes), and new symbols may appear at any time. This makes it impossible to construct a fixed mapping ahead of time without either:

* a separate discovery phase (which delays startup), or
* backpatching (which breaks streaming assumptions)

Flat maps let us build the mapping **on the fly**, adding symbols as they first appear, while preserving fast lookup and ordered access.


[100]:https://martin.ankerl.com/2019/04/01/hashmap-benchmarks-03-01-result-InsertHugeInt/