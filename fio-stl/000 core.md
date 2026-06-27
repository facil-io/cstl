# facil.io STL — Core

Welcome to the heart of the facil.io C STL. This is where the boring-but-essential stuff lives: version numbers, compiler incantations, OS patches, atomics, memory primitives, and the low-level helpers that everything else builds on. It is not glamorous, but without it the rest of the library would be a pile of undefined behavior and segfaults.

The core group covers the first two slices of the library — `000 core.h` and `001 header.h` — plus their specialized sidekicks. Each file below is a focused guide to one corner of that foundation.

## Core documentation index

- [001 version and settings](./001%20version%20and%20settings.md) — Version macros and the compile-time knobs that tune the library's behavior.
- [001 compiler attributes](./001%20compiler%20attributes.md) — Compiler detection, C/C++ attribute helpers, and pointer-math macros.
- [001 os detection](./001%20os%20detection.md) — POSIX/Windows detection, process helpers, and portable type patches.
- [001 atomics and locks](./001%20atomics%20and%20locks.md) — Atomic operations, spinlocks, atomic bitmap access, and thread-yield hints.
- [001 static alloc](./001%20static%20alloc.md) — The round-robin scratch allocator for short-lived temporary buffers.
- [001 logging](./001%20logging.md) — Log levels, assertion macros, and the leak counter.
- [001 endian](./001%20endian.md) — Byte-order detection, swaps, and conversions between host and network/little-endian layouts.
- [001 memory primitives](./001%20memory%20primitives.md) — Low-level copy/move/compare helpers and unaligned buffer accessors.
- [001 memalt](./001%20memalt.md) — Portable fallback memory routines and the environment helpers they lean on.
- [001 vector math](./001%20vector%20math.md) — Wide integer unions from 128 to 4096 bits and their lane-wise arithmetic.
- [001 simd](./001%20simd.md) — Fake/portable x86 intrinsics and SIMD helpers built on the wide vector types.
- [001 linked lists](./001%20linked%20lists.md) — Pointer-based and indexed linked-list macros.
- [001 constant time](./001%20constant%20time.md) — Branchless selection, bitwise comparisons, and other security-sensitive primitives.
- [001 bit operations](./001%20bit%20operations.md) — Rotations, popcount, bitmap access, and byte-value bit tricks.
- [001 math](./001%20math.md) — Hash primes, modular arithmetic, and multi-precision add/sub/mul helpers.
- [001 random core](./001%20random%20core.md) — The deterministic 128-bit PRNG and its reseeding/cycle-counter hooks.
- [001 string info](./001%20string%20info.md) — Lightweight string and buffer descriptors plus equality helpers.
- [001 utf8](./001%20utf8.md) — Encoding, decoding, and validation helpers for UTF-8 text.

Everything after the core builds on these pages. Pick a topic and dig in.
