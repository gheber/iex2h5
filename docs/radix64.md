# :material-exponent-box:{.icon} Order-Preserving Radix-64 Encoding [^1]

**Terminology.** Here “Radix-64” means a positional numeral system in base $64$ over a chosen, ordered alphabet $\Sigma$. This is **not** the [RFC 4648][100] “Base64” byte codec.

## :material-alphabetical-variant:{.icon} Alphabet and code map

Let \( (A,\le_A) \) be a totally ordered finite alphabet of size \( |A|\le 64\).
Choose a **monotone** (order-embedding) code map \( \sigma:A\to\{0,\dots,63\},\qquad
a\le_A b \;\Rightarrow\; \sigma(a)\le \sigma(b). \) (Injective and order-preserving is ideal; ties collapse symbols.)

## :material-format-size:{.icon} Encoding fixed-length strings

For $N$-character strings $s=s_0s_1\ldots s_{N-1}\in A^N$, define the **big-endian** radix-64 integer \(
E(s)\;=\;\sum_{i=0}^{N-1}\sigma(s_i)\,64^{\,N-1-i}. \tag{1} \) This treats the first character as the **most-significant 6-bit digit**.

### :fontawesome-solid-equals:{.icon} Order equivalence

Let $x,y\in A^N$ and let $k$ be the first index where they differ. Then

$$
\operatorname{sign}\!\big(E(x)-E(y)\big)
=\operatorname{sign}\!\big(\sigma(x_k)-\sigma(y_k)\big),
$$

because the residual tail $\sum_{i>k}\big(\cdot\big)64^{\,N-1-i}$ is $<64^{\,N-1-k}$. Hence

$$
x<_{\text{lex}} y \quad\Longleftrightarrow\quad E(x)<E(y).
\tag{2}
$$

So **unsigned integer comparison** of $E(\cdot)$ is exactly lexicographic comparison on $A^N$.

## :fontawesome-solid-down-left-and-up-right-to-center:{.icon} Variable length via padding

For variable-length strings, pick a **sentinel** $p\in A$ with the **smallest code** $\sigma(p)=0$ and pad right to a common length $N$:

$$
\widetilde{s} \;=\; s_0\ldots s_{m-1}\, p\,\ldots\,p \quad (m\le N).
$$

Then (2) continues to hold **and** prefixes sort before their extensions (e.g., “A” < “AA”).

## :fontawesome-solid-sack-dollar:{.icon}  Bit budget (48-bit symbol + 16-bit index)

An $N$-char code occupies exactly $6N$ bits. For $N=8$,

$$
0\le E(s) < 64^8 \;=\; 2^{48},
$$

so $E(s)$ fits in **48 bits**. You can pack a 16-bit auxiliary field (e.g., an integer index) into the low bits:

$$
K \;=\; \big(E(s)\ll 16\big)\; \lor\; \text{idx}_{16}.
$$

Comparing 64-bit keys $K$ remains order-preserving on the symbol part because the **symbol lives in the upper 48 bits** (MSBs).

## :material-check-all:{.icon} Practical checklist

1. **Monotone $\sigma$.** Respect the chosen alphabet order.
2. **Fixed length or pad with min-code sentinel.** Ensures prefix ordering.
3. **Big-endian digits.** First character in the MSBs; then unsigned integer compare $\equiv$ lex compare.

---

## :material-file-code:{.icon} Reference Algorithm for Order-Preserving Radix-64 Encoding
```
# ===============================
# Radix-64 (order-preserving) spec
# ===============================

CONST BASE := 64
CONST WIDTH := 8              # number of 6-bit digits (chars)
# REQUIRE: ALPHABET[0] is the padding sentinel (min code), e.g. ' ' or '\0'
INPUT  ALPHABET[0..63] : array of char, total order matches desired lex order

# -------------------------------
# Build lookup: char -> code in [0..63]
# -------------------------------
PROC build_lookup(ALPHABET):
    TABLE[0..127] := -1                # ASCII only; extend if needed
    FOR i IN 0..63:
        TABLE[ ord(ALPHABET[i]) ] := i
    RETURN TABLE

TABLE := build_lookup(ALPHABET)
PAD_CODE := 0                          # since ALPHABET[0] is the smallest

# -------------------------------
# Map a single character to code
# -------------------------------
FUNC char_to_code(c: char) -> int:
    IF ord(c) < 0 OR ord(c) > 127 THEN RETURN -1
    RETURN TABLE[ord(c)]               # -1 if not in alphabet

# -------------------------------
# Encode symbol s (length ≤ WIDTH) to 48-bit integer E(s)
# Big-endian 6-bit digits: first character is most significant digit
# -------------------------------
FUNC encode_symbol(s: string) -> uint64:
    IF len(s) > WIDTH: RAISE "symbol too long"
    E := 0
    FOR i IN 0..(WIDTH-1):
        IF i < len(s) THEN
            code := char_to_code(s[i])
            IF code = -1: RAISE "invalid character"
        ELSE
            code := PAD_CODE           # right-pad with smallest code
        E := (E << 6) OR code          # append 6-bit digit
    RETURN E                           # 0 ≤ E < 2^(6*WIDTH) = 2^48

# -------------------------------
# Pack 48-bit symbol + 16-bit index into a 64-bit key
# (symbol in MSBs preserves lex order under unsigned <)
# -------------------------------
FUNC make_key(s: string, idx16: uint16) -> uint64:
    E := encode_symbol(s)
    RETURN (E << 16) OR idx16

# -------------------------------
# Decode 48-bit integer back to WIDTH characters (padded form)
# To recover the unpadded string, strip trailing PAD_CHARs on the right
# -------------------------------
FUNC decode_symbol(E: uint64) -> string:
    out[0..(WIDTH-1)] : array of char
    FOR i IN (WIDTH-1) DOWNTO 0:
        code := E AND 0x3F             # extract least-significant 6 bits
        out[i] := ALPHABET[code]
        E := E >> 6
    RETURN concat(out)                 # caller may rstrip PAD_CHAR if desired

# -------------------------------
# Invariants / Preconditions
# -------------------------------
# 1) Monotone map: the order of ALPHABET defines the character order AND
#    its indices 0..63 (via build_lookup) respect that order.
# 2) Padding uses the smallest code: PAD_CODE = 0 => prefixes sort before extensions.
# 3) Big-endian digits: shifting left then OR code means s[0] occupies the top 6 bits.
# RESULT: For fixed WIDTH, unsigned integer compare on encode_symbol(s) equals lex order.

```

That’s it: monotone $\sigma$, min-code padding, big-endian digits ⇒ lex order ↔ unsigned integer order, perfectly within a 64-bit key.

[^1]: **Author:** Steven Varga, 2025 Toronto, ON
[100]: https://datatracker.ietf.org/doc/html/rfc4648