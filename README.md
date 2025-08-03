# intfp: Integer-based Fixed-Point and Pseudo-Logarithmic Library

`intfp` is a C99 header-only library for high-performance fixed-point and pseudo-logarithmic arithmetic, optimized for embedded systems and other resource-constrained environments.

It is designed for platforms without a Floating-Point Unit (FPU) or where FPU usage is prohibitively expensive, such as within a kernel's context. The library excels at transforming expensive multiplication and division into highly efficient addition and subtraction, and at dramatically reducing the memory footprint of large numerical datasets.

## Core Concepts: Numerical Formats

The library provides three specialized numerical formats:

### 1. Linear Fixed-Point (`<type><bits>fp`)
A standard integer-based representation for fractional numbers. This serves as the foundation for converting to and from the other formats.

### 2. Unsigned Logarithmic Compressed (`loc`)
A memory-efficient, unsigned, pseudo-logarithmic format designed for storage and transmission.
- **Primary Use**: Compressing large integer values (e.g., `uint64_t`) into a much smaller bit-width (`loc16` or `loc8`).
- **Key Characteristic**: This format is **not calculable**. It must be converted back to a linear format for arithmetic.
- **Special Encoding**: `0` is encoded as `1`, and `1` is encoded as `0` to distinguish them.

### 3. Signed Pseudo-Logarithmic (`log`)
A calculable, signed, pseudo-logarithmic format optimized for high-speed computation.
- **Primary Use**: Replacing expensive multiplication/division with cheap addition/subtraction. `a * b` becomes `log(a) + log(b)`.
- **Key Characteristic**: Ideal for applications like digital signal processing (DSP), real-time filtering (EWMA), and control systems.
- **Special Encoding**: `0` is represented by the most negative value (`intfp_log_0(bits)`).

## Key Features

- **Blazing-Fast Computation**: Utilizes compiler intrinsics (`__builtin_clz`) for extremely fast conversions, avoiding costly standard math library calls.
- **Extreme Memory Efficiency**: The `loc` format significantly reduces the data footprint of large integer datasets (e.g., compressing 64-bit integers into 16 bits).
- **High Flexibility**: Employs extensive preprocessor macros to generate a wide range of conversion functions (e.g., `u64` to `loc16`, `log8` to `log32`), allowing you to fine-tune for specific needs.
- **Practical Utilities**: Includes ready-to-use functions for Exponentially Weighted Moving Average (EWMA) and radix conversion (e.g., to decibels).

## Why `intfp`? Performance & Memory Efficiency

In environments like the Linux kernel or bare-metal firmware, using floating-point hardware is not free. `intfp` is designed to outperform it by orders of magnitude.

### 🚀 Blazing-Fast Performance

An `intfp` conversion is fundamentally faster than a kernel-space FPU operation.

| Operation | `intfp` (e.g., `u64_to_log32_fpmax`) | FPU in Kernel (e.g., `log2()`) |
| :--- | :--- | :--- |
| **Method** | Integer `CLZ` instruction + shifts | FPU context save, `int->double`, `log2`, context restore |
| **Est. Cycles** | **~10-20 cycles** | **~150-500+ cycles** |

The dramatic difference comes from avoiding the **FPU context switch** (`kernel_fpu_begin()` / `kernel_fpu_end()`), which is a massive performance bottleneck. `intfp` uses only general-purpose integer registers.

### 💾 Extreme Memory Efficiency

`intfp` provides two layers of memory savings:

1.  **Storage Compression**: The `loc` format is a powerful compression tool.
    - **Example**: Storing one million 64-bit sensor readings.
        - **Standard `uint64_t`**: 1,000,000 * 8 bytes = **8 MB**
        - **`intfp` `loc16`**: 1,000,000 * 2 bytes = **2 MB** (A **75%** saving!)

2.  **Runtime Memory Footprint**: `intfp` avoids the runtime memory cost of FPU context saves.
    - FPU context save areas can range from ~256 bytes (SSE) to over **2 KB** (AVX-512). This memory is consumed on the stack for every calculation block.
    - `intfp`'s runtime memory cost is **zero**.

## Quick Start

### Installation

This is a header-only library. Simply include `intfp.h` in your project.

```c
#include "intfp.h"
#include <stdio.h>
#include <stdint.h>

// Define standard integer types if not already available
typedef uint8_t  u8;
typedef int8_t   s8;
typedef uint16_t u16;
typedef int16_t  s16;
typedef uint32_t u32;
typedef int32_t  s32;
typedef uint64_t u64;
typedef int64_t  s64;
typedef _Bool    bool;
```

### Basic Usage

```c
#include "intfp.h" // With type definitions as above

void main() {
    // ---- Multiplication via Logarithmic Addition ----
    u64 val_a = 1000;
    u64 val_b = 2000;

    // Convert values to the 'log' domain
    s32 log_a = u64_to_log32_fpmax(val_a);
    s32 log_b = u64_to_log32_fpmax(val_b);

    // Perform multiplication by simple addition
    s32 log_product = log_a + log_b;

    // Convert back to the linear domain
    u64 product = log32fpmax_to_u64(log_product);

    // `product` will be approximately 2,000,000
    printf("%llu * %llu ≈ %llu\n", val_a, val_b, product);


    // ---- Data Compression ----
    u64 large_value = 0x123456789ABCDEF0;
    
    // Compress an 8-byte integer into 2 bytes
    u16 compressed_value = u64_to_loc16_fpmax(large_value);

    // Decompress it back
    u64 decompressed_value = loc16fpmax_to_u64(compressed_value);

    printf("Original:     0x%016llX\n", large_value);
    printf("Compressed:   0x%04X\n", compressed_value);
    printf("Decompressed: 0x%016llX\n", decompressed_value); // Note the precision loss
}
```

## API Naming Convention

The function names are systematic and predictable:
`<input_type>_to_<output_type>`

- **Types**: `u8`, `s16`, `u32fp`, `loc16`, `log32`, etc.
- **Precision**:
    - Suffix `fp` indicates a function that requires an explicit fractional/exponent bit count parameter (e.g., `ofp`).
    - Suffix `fpmax` indicates a variant that automatically uses the optimal precision for the given bit-widths.

**Examples:**
- `u64_to_loc16_fpmax(value)`: Convert a `u64` to a `loc16` with maximum precision.
- `u32fp_to_log16_fp(value, ifp, ofp)`: Convert a `u32fp` (with `ifp` fractional bits) to a `log16` (with `ofp` mantissa bits).

## ⚠️ Important: The `log` Format is a Linear Approximation

The extreme speed of the `log` format is achieved through a trade-off. It does **not** represent a true mathematical logarithm. It uses a fast, linear approximation.

- **True Base-2 Logarithm**: `log2(v) = e + log2(1 + m)` (where `v = (1+m) * 2^e`)
- **`intfp`'s `log` Value**: `e + m`

This `e + m` approximation is extremely fast to compute but introduces a predictable, non-linear error. It is smallest when the input value `v` is close to a power of two. Be aware of this inherent trade-off when designing your system.

## Platform Dependencies

This library relies on GCC/Clang-specific built-in functions for high performance:
- `__builtin_clz()` (for 32-bit integers)
- `__builtin_clzll()` (for 64-bit integers)

Porting to other compilers (like MSVC) would require providing equivalent implementations for these functions (e.g., using `_BitScanReverse`).

## License

Copyright (C) 2025 Masahito Suzuki. All rights reserved. Please see the header file for full details.

