# Generics: Zero-Overhead Container Utilities

The `generics.hpp` module provides lightweight, template-based utilities that eliminate repetitive boilerplate when working with large sets of numerical or matrix containers in `iex2h5`.

It enables **broadcast operations** over multiple container arguments, such as resizing or zero-initializing multiple Armadillo matrices, vectors, or plain-old arrays — all with a single call.

## Key Goals

- **Reduce boilerplate**: No more manual `.resize()` or `.zeros()` on every variable.
- **Ensure consistency**: All related containers are resized/zeroed together.
- **Generic containers**: Works with Armadillo, `std::vector`, raw pointers, and more.
- **Inline & header-only**: No runtime overhead or linkage complications.

## Available Utilities

### `generics::resize(rows, cols, ...)`

Resizes all provided containers (e.g., matrices, vectors) to the specified shape.

```cpp
generics::resize(R, C, matrix1, matrix2, ..., matrixN);
generics::resize(R, scalar1, scalar2, ..., scalarN);  // for 1D data
```
### `generics::zeros(...)`
Zeroes all supplied containers using the most appropriate `.zeros()` or `fill(0)` logic.

```cpp
generics::zeros(matrix1, vector1, scalar1, ..., objectN);
```

## Example: Matrix Initialization

```cpp
template <typename clock_t>
struct consumer_t : public io::base::consumer_t<clock_t> {
    [...]
    void resize(size_t rows, size_t cols) {
        generics::resize(rows,cols, h5_bid,h5_ask,h5_trade,  h5_bid_volume, h5_ask_volume, h5_trade_volume);
        generics::resize(rows,
            start, stop, 
            event_count, trade_size, trade_count, fbid, fask, ftrade,
            avg_trade_count, avg_spread, day_high, day_low, day_close, day_open);
    }

    arma::fmat h5_bid, h5_ask, h5_trade;
    arma::umat h5_bid_volume, h5_ask_volume, h5_trade_volume;
    arma::uvec trade_size, trade_count, event_count;
    arma::fvec avg_trade_count, avg_spread, day_high, day_low, day_close, day_open;
    std::vector<std::string> start, stop;
    filters::ema_filter_t<clock> fbid, fask, ftrade;            
};
```


## 🔁 Example: Daily Reset

```cpp
void day_begin(time_point day) {
    generics::zeros(
        fbid, fask, ftrade,
        h5_ask, h5_trade, h5_bid,
        h5_bid_volume, h5_ask_volume, h5_trade_volume,
        trade_count, event_count, trade_size,
        avg_trade_count, avg_spread,
        day_high, day_low, day_close, slot
    );
}
```

---

## 🧠 How It Works

At its core, `generics.hpp` uses **C++17 fold expressions** and **template packs** to dispatch the appropriate logic per argument type.

```cpp
template<typename T, typename... Args>
inline void resize(size_t R, size_t C, T& first, Args&... args) {
    detail::resize_dispatch(R, C, first);
    (detail::resize_dispatch(R, C, args), ...);
}
```

Inside `detail::resize_dispatch()`, the correct method is chosen via overload resolution or `if constexpr`.

---

## 🧵 Supported Types

* `arma::mat`, `arma::vec`, `arma::rowvec`
* `std::vector<T>`
* `std::array<T,N>`
* `scalar_t` values (`T{}` zero-init)
* Custom containers with `.resize()` and `.zeros()` methods

> 📌 Extensible: You can specialize or overload dispatchers for your own types.

---

## 🧩 Source Location

* Header: [`generics.hpp`](../tree/main/src/generics.hpp)
* No external dependencies

---

## ✍️ Why It Matters

When managing 10s of matrices per contract across thousands of time slices, it’s easy to write bugs or forget to initialize a column. Using `generics`:

* Improves **readability**
* Minimizes **code duplication**
* Encourages **clean abstraction boundaries**

---

## 🧬 Related Files

* [`consumers.hpp`](../tree/main/src/consumers.hpp): Uses generics to manage contract state
* [`tick.hpp`](../tree/main/src/tick.hpp): Defines many of the Armadillo matrix types being passed into these helpers

---

```

Let me know if you want this linked into your sidebar or cross-referenced from `file-format.md` or `development.md`.
```
