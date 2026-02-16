# intfp: Integer-based Fixed-Point and Pseudo-Logarithmic Library

`intfp` is a C99 header-only library for high-performance fixed-point and pseudo-logarithmic arithmetic, optimized for embedded systems and other resource-constrained environments.

It is designed for platforms without a Floating-Point Unit (FPU) or where FPU usage is prohibitively expensive, such as within a kernel's context. The library excels at transforming expensive multiplication and division into highly efficient addition and subtraction, and at dramatically reducing the memory footprint of large numerical datasets.

## Core Concepts: Numerical Formats

The library provides three specialized numerical formats:

### 1. Linear Fixed-Point (`<type><bits>fp`)
A standard integer-based representation for fractional numbers. This serves as the foundation for converting to and from the other formats.

### 2. Packed Unsigned Log (`pul`)
A memory-efficient, unsigned, pseudo-logarithmic format designed for storage and transmission.
- **Primary Use**: Compressing large integer values (e.g., `uint64_t`) into a much smaller bit-width (`pul16` or `pul8`).
- **Key Characteristic**: This format is **not calculable**. It must be converted back to a linear format for arithmetic.
- **Special Encoding**: `0` is encoded as `1`, and `1` is encoded as `0` to distinguish them.

### 3. Signed Pseudo-Logarithmic (`log`)
A calculable, signed, pseudo-logarithmic format optimized for high-speed computation.
- **Primary Use**: Replacing expensive multiplication/division with cheap addition/subtraction. `a * b` becomes `log(a) + log(b)`.
- **Key Characteristic**: Ideal for applications like digital signal processing (DSP), real-time filtering (EWMA), and control systems.
- **Special Encoding**: `0` is represented by the most negative value (`intfp_log_0(bits)`).
- **Corrected Variant** (`_corr` suffix): LUT-based correction reduces end-to-end arithmetic error from ~11% to ~1.3% with minimal overhead (see [Corrected Log Conversion](#corrected-log-conversion-_corr)).

## Key Features

- **Blazing-Fast Computation**: Utilizes compiler intrinsics (`__builtin_clz`) for extremely fast conversions, avoiding costly standard math library calls.
- **Extreme Memory Efficiency**: The `pul` format significantly reduces the data footprint of large integer datasets (e.g., compressing 64-bit integers into 16 bits).
- **LUT-Corrected Precision**: Optional `_corr` variants apply a 256-entry lookup table correction to both encode and decode, reducing multiplication/division error from ~11% to ~1.3% with only 1 KB of additional memory.
- **High Flexibility**: Employs extensive preprocessor macros to generate a wide range of conversion functions (e.g., `u64` to `pul16`, `log8` to `log32`), allowing you to fine-tune for specific needs.
- **Practical Utilities**: Includes ready-to-use functions for Exponentially Weighted Moving Average (EWMA) and radix conversion (e.g., to decibels).

## Why `intfp`? Performance Comparison

In environments like the Linux kernel or bare-metal firmware, using floating-point hardware is not free. `intfp` is designed to outperform it by orders of magnitude.

### CPU Cycle Comparison: Division (`a / b`)

Measured from `gcc -O2` generated assembly (x86-64):

| Method | Cycles | Max Error | Extra Memory |
| :--- | ---: | ---: | ---: |
| Kernel FPU (`divsd` + context switch) | ~800 | 0% | 0 |
| Integer `divq` (u64) | 35-90 | 0% | 0 |
| Integer `divl` (u32) | ~26 | 0% | 0 |
| **intfp `log`** (uncorrected) | **~14** | ~11% | 0 |
| **intfp `log` + `_corr`** (LUT-corrected) | **~25** | ~1.3% | 1024 B |

The dramatic difference vs kernel FPU comes from avoiding the FPU context switch (`kernel_fpu_begin()` / `kernel_fpu_end()`), which requires XSAVE/XRSTOR of FPU state. `intfp` uses only general-purpose integer registers.

When values are kept in `log` form across multiple operations, the per-operation cost drops to just **1 cycle** (a single integer `add`/`sub`), making the gap even wider:

| Method | Per-Operation | vs `divq` |
| :--- | ---: | ---: |
| `divq` (u64) | 35-90 cycles | 1x |
| intfp `log` operation | 1 cycle | **35-90x faster** |

### Memory Efficiency

`intfp` provides two layers of memory savings:

1.  **Storage Compression**: The `pul` format is a powerful compression tool.
    - **Example**: Storing one million 64-bit sensor readings.
        - **Standard `uint64_t`**: 1,000,000 * 8 bytes = **8 MB**
        - **`intfp` `pul16`**: 1,000,000 * 2 bytes = **2 MB** (A **75%** saving!)

2.  **Runtime Memory Footprint**: `intfp` avoids the runtime memory cost of FPU context saves.
    - FPU context save areas can range from ~256 bytes (SSE) to over **2 KB** (AVX-512). This memory is consumed on the stack for every calculation block.
    - `intfp`'s runtime memory cost is **zero** (uncorrected) or **1 KB** (corrected, for the LUT tables).

## Quick Start

### Installation

This is a header-only library. Simply include `intfp.h` in your project.

```c
#include <stdint.h>
#include <stdbool.h>

// Define standard integer types required by intfp.h
typedef uint8_t  u8;
typedef int8_t   s8;
typedef uint16_t u16;
typedef int16_t  s16;
typedef uint32_t u32;
typedef int32_t  s32;
typedef uint64_t u64;
typedef int64_t  s64;

#include "intfp.h"
```

### Basic Usage

```c
// ---- Multiplication via Logarithmic Addition ----
u64 val_a = 1000;
u64 val_b = 2000;

// Convert values to the 'log' domain (uncorrected, fastest)
s32 log_a = u64_to_log32fpmax(val_a);
s32 log_b = u64_to_log32fpmax(val_b);

// Perform multiplication by simple addition
s32 log_product = log_a + log_b;

// Convert back to the linear domain
u64 product = log32fpmax_to_u64(log_product);
// product ≈ 2,000,000 (max error ~11%)

// ---- With LUT Correction (higher precision) ----
s32 log_a_c = u64_to_log32fpmax_corr(val_a);
s32 log_b_c = u64_to_log32fpmax_corr(val_b);
s32 log_product_c = log_a_c + log_b_c;
u64 product_c = log32fpmax_to_u64_corr(log_product_c);
// product_c ≈ 2,000,000 (max error ~1.3%)

// ---- Division via Logarithmic Subtraction ----
s32 log_quotient = log_a_c - log_b_c;
u64 quotient = log32fpmax_to_u64_corr(log_quotient);
// quotient ≈ 0 (1000/2000 < 1, represented as 0 in integer)

// ---- Data Compression with 'pul' ----
u64 large_value = 0x123456789ABCDEF0ULL;

// Compress an 8-byte integer into 2 bytes
u16 compressed = u64_to_pul16fpmax(large_value);

// Decompress it back
u64 decompressed = pul16fpmax_to_u64(compressed);
```

## Corrected Log Conversion (`_corr`)

The standard `log` format uses a linear approximation (`e + m` instead of `e + log2(1+m)`), which introduces up to ~8.6% error per conversion. When two values are multiplied (encode + encode + decode), the error compounds to ~11%.

The `_corr` variants apply a quadratic correction based on Mitchell's improvement:

- **Encode**: `log2(1+m) ≈ m + c_enc · m · (1-m)` with `c_enc = 89/256`
- **Decode**: `2^m ≈ (1+m) - c_dec · m · (1-m)` with `c_dec = 88/256`

The correction values are pre-computed in two 256-entry lookup tables (512 bytes each, 1024 bytes total), indexed by the top 8 bits of the fractional mantissa. This replaces two integer multiplications with a single table lookup.

| Metric | Uncorrected `log` | Corrected `_corr` | Improvement |
| :--- | ---: | ---: | ---: |
| Log-domain max error | 0.0861 | 0.0085 | 10x |
| E2E multiply max error | ~11.1% | ~1.3% | 8.5x |
| E2E division max error | ~11.1% | ~0.8% | 14x |
| Encode latency | ~7 cycles | ~13 cycles | +6 cycles |
| Decode latency | ~6 cycles | ~11 cycles | +5 cycles |
| Extra memory | 0 | 1024 bytes | — |

Corrected and uncorrected values share the same bit-level format and can be freely mixed in arithmetic (add/subtract). However, mixing corrected and uncorrected encode/decode will degrade the precision benefit.

## API Naming Convention

The function names are systematic and predictable:
`<input_type>_to_<output_type>[_corr]`

- **Types**: `u8`, `s16`, `u32fp`, `pul16`, `log32`, etc.
- **Precision**:
    - Suffix `fp` indicates a function that requires an explicit fractional/exponent bit count parameter (e.g., `ofp`).
    - Suffix `fpmax` indicates a variant that automatically uses the optimal precision for the given bit-widths.
- **Correction**:
    - Suffix `_corr` indicates LUT-corrected encode/decode for improved precision.

**Examples:**
| Function | Description |
| :--- | :--- |
| `u64_to_pul16fpmax(value)` | `u64` to `pul16` with max precision |
| `pul16fpmax_to_u64(value)` | `pul16` to `u64` with max precision |
| `u64_to_log32fpmax(value)` | `u64` to `log32` (uncorrected, fastest) |
| `u64_to_log32fpmax_corr(value)` | `u64` to `log32` (LUT-corrected) |
| `log32fpmax_to_u64(value)` | `log32` to `u64` (uncorrected) |
| `log32fpmax_to_u64_corr(value)` | `log32` to `u64` (LUT-corrected) |
| `u32fp_to_log16fp(value, ifp, ofp)` | `u32fp` (with `ifp` fractional bits) to `log16` (with `ofp` mantissa bits) |

## The `log` Format: A Linear Approximation

The extreme speed of the `log` format is achieved through a trade-off. It does **not** represent a true mathematical logarithm. It uses a fast, linear approximation.

- **True Base-2 Logarithm**: `log2(v) = e + log2(1 + m)` (where `v = (1+m) * 2^e`)
- **`intfp`'s `log` Value**: `e + m`
- **`intfp`'s `log` with `_corr`**: `e + m + c·m·(1-m)` (quadratic correction via LUT)

This `e + m` approximation is extremely fast to compute but introduces a predictable, non-linear error. It is smallest when the input value `v` is close to a power of two. The `_corr` variants reduce this error by an order of magnitude at a modest cost of ~6 extra cycles per conversion and 1 KB of lookup tables.

## Platform Dependencies

This library relies on GCC/Clang-specific built-in functions for high performance:
- `__builtin_clz()` (for 32-bit integers)
- `__builtin_clzll()` (for 64-bit integers)

Porting to other compilers (like MSVC) would require providing equivalent implementations for these functions (e.g., using `_BitScanReverse`).

## License

This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version. See [intfp.h](intfp.h) for full details.

Copyright (C) 2025 Masahito Suzuki.
