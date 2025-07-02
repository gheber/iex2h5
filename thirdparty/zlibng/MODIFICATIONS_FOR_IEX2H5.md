
# Modifications for IEX2H5 Compatibility with zlib-ng

To enable safe coexistence between zlib-ng and system libraries such as `libhdf5` (which depend on the classic zlib ABI and headers), the following symbol renaming has been applied to the zlib-ng source:

### Changes Applied

The internal function pointer typedef `in_func` has been renamed to `iex_in_func` in both `zlib.h.in` and `zlib-ng.h.in` to prevent symbol and type conflicts with the system-installed `zlib.h` headers used by `libhdf5`.

**Example of the renamed typedef:**

```c
typedef uint32_t (*iex_in_func)(void *, z_const unsigned char **);
````

This change ensures ABI consistency when both zlib-ng and the system zlib are present in the build or link process.

### Modified Source Locations

```text
./zlib-ng.h.in:1079:typedef uint32_t (*iex_in_func)(void *, const uint8_t **);
./zlib-ng.h.in:1083:int32_t zng_inflateBack(zng_stream *strm, iex_in_func in, void *in_desc, out_func out, void *out_desc);
./zlib.h.in:1082:typedef uint32_t (*iex_in_func)(void *, z_const unsigned char **);
./infback.c:152:int32_t Z_EXPORT PREFIX(inflateBack)(PREFIX3(stream) *strm, iex_in_func in, void *in_desc, out_func out, void *out_desc);
```

### Rationale

The original `in_func` symbol name matches a typedef found in `zlib.h` (from system zlib), which causes header conflicts and build failures when both headers are seen by the compiler during translation unit preprocessing.

By renaming `in_func` → `iex_in_func`, these collisions are avoided while preserving compatibility with the zlib-ng implementation.

Steven Varga, Varga Consulting, Toronto, ON, Canada 🇨🇦 2025 June 

