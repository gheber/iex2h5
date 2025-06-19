
# Profiling and Base Performance Optimization in `iex2h5`

##  Overview

This article documents the profiling journey of the `iex2h5` application, focusing on identifying and optimizing base performance bottlenecks using **Valgrind Callgrind** and **KCachegrind**.


##  Tools Used

- [`perf`](https://perf.wiki.kernel.org/)
- [`valgrind --tool=callgrind`](http://valgrind.org/)
- [`kcachegrind`](https://kcachegrind.sourceforge.net/html/Home.html)
- Flat profile, call graph, and callee map analysis


##  Key Hotspots Identified
### 1. `std::chrono::operator<=>`

```cpp
auto std::chrono::operator<=><long, std::ratio<1l, 1000000000l>, long, std::ratio<1l, 1000000000l> >
```

- **Inclusive:** ~2.9B cycles
- **Self:** ~2.9B cycles
- **Calls:** ~57M
- Appears in:
  ```cpp
  if (now > today + this->start) ...
  if (now > today + this->stop)  ...
  if (now - last_time >= this->heart_beat_interval) ...
  ```

### 2. `std::chrono::duration::count()`

```cpp
std::chrono::duration<long, std::nano>::count() const
```

- **Self:** ~1.8B cycles
- **Calls:** ~231M
- Pure accessor, but called from every comparison

### 3. `iex::transport_t::transport_handler(...)`

- **Inclusive:** ~56B cycles
- **Self:** ~3B cycles
- **Calls:** ~19M

This is the driver of the event loop, and indirectly responsible for millions of time comparisons and filter applications.

### 4. `std::unordered_map` Internals

- `std::_Hashtable`, `_Hash_node`, and `std::__detail` symbols total:
  - **Inclusive:** ~8.3B cycles
  - High lookup and insert volume due to symbol mapping

## Performance Analysis

- **Hot paths** show excessive use of `std::chrono::duration` and related operators.
- Cost comes from abstraction layering: templates, type casting, virtual calls, and inline barriers.

##  Optimization Strategies

###  Replace Time Comparisons

Original:

```cpp
if (now > today + this->start) ...
```

Optimized:

```cpp
int64_t now_ns = now.time_since_epoch().count();
int64_t start_ns = today.time_since_epoch().count() + this->start.count();
if (now_ns > start_ns) ...
```

###  Use Raw Timestamps in Hot Loops

Avoid instantiating `std::chrono::time_point` inside per-tick message processing.

###  Flatten Container Usage

Replace `std::unordered_map<K,V>` with `std::vector<uint64_t>` + binary search where keyspace is dense and flat.


## Lessons Learned

| Observation | Recommendation |
|-------------|----------------|
| High cost of `std::chrono` ops | Use `int64_t` timestamps |
| Operator overloads not optimized | Avoid templated comparisons in hot path |
| Containers impact performance | Consider flat maps for known contract ID range |
| Filters and EMA computation | Profiled but not yet optimized — low hanging fruit later |


## Conclusion

This profiling session helped uncover unexpected hotspots in high-level constructs. By peeling away abstraction where it matters (timing logic, container lookups), substantial CPU time can be reclaimed. Profiling is not just about finding bugs — it's about finding what matters.

*Authored by Steven Varga, June 2025*
