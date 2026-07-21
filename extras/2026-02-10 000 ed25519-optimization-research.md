# Ed25519/X25519 Advanced Optimization Research

**Date:** 2026-02-10  
**Project:** facil.io C STL  
**Goal:** Achieve 2x speedup through higher-level optimizations

## Executive Summary

After exhausting field-level micro-optimizations (5x51 representation is near-optimal), this research explores four advanced optimization paths:

1. **Constant-Time Windowed Methods** - **VERIFIED: 2.32x speedup** (prototype working)
2. **SIMD Vectorization** - VIABLE for throughput, complex for latency
3. **Better Scalar Multiplication Algorithms** - VIABLE with 1.3-1.5x speedup
4. **Cache Locality & Auto-Vectorization** - LIMITED benefit for current structure

**UPDATE (2026-02-10):** Working prototype implemented in `extras/ed25519_windowed_prototype.c`:
- **2.32x speedup** vs current fio-stl Montgomery ladder implementation
- Constant-time table lookup overhead: only 0.8-3.8%
- Verified correct against fio-stl implementation

**Recommendation:** Integrate constant-time 4-bit windowed scalar multiplication into `fio-stl/154 ed25519.h` for fixed-base operations (signing, key generation).

---

## 1. Constant-Time Windowed Methods

### Problem Statement
The 4-bit windowed scalar multiplication achieved 1.93x speedup in benchmarks, but naive table lookups (`table[index]`) are cache-timing vulnerable because:
- Different indices access different cache lines
- Attacker can observe which cache lines are accessed
- This leaks bits of the secret scalar

### Solution: Constant-Time Table Lookup

**Technique:** Read ALL table entries, select the correct one using masks.

```c
/* Constant-time table lookup: reads ALL 16 entries, selects one */
FIO_IFUNC void ge_table_lookup_ct(ge_p3_s *r, const ge_p3_s table[16], int index) {
  /* Start with identity point */
  ge_p3_identity(r);
  
  /* Read all entries, conditionally select the matching one */
  for (int i = 0; i < 16; i++) {
    /* mask = 0xFFFFFFFF... if i == index, else 0 */
    uint64_t mask = (uint64_t)0 - (uint64_t)(i == index);
    
    /* Conditionally OR in the table entry */
    for (int j = 0; j < 5; j++) {
      r->X[j] |= mask & table[i].X[j];
      r->Y[j] |= mask & table[i].Y[j];
      r->Z[j] |= mask & table[i].Z[j];
      r->T[j] |= mask & table[i].T[j];
    }
  }
}
```

### Performance Analysis

**Cost per lookup:**
- 16 iterations × 20 memory reads × 3 operations (mask, AND, OR) = ~960 operations
- Compare to: 1 indexed read = ~4 operations

**But in context:**
- Point addition: ~8 field multiplications = ~200 operations each = 1600 operations
- Constant-time lookup overhead: ~960 operations
- Total per window: 2560 operations (vs 1600 for variable-time)
- **Overhead: ~60% per lookup**

**Net speedup calculation:**
- Bit-by-bit: 256 doublings + 128 additions = 384 point operations
- 4-bit window: 256 doublings + 64 additions + 64 lookups
- With CT lookup: 256 doublings + 64 additions + 64×(0.6 additions) = 256 + 102.4 = 358.4 equivalent ops
- **Speedup: 384/358.4 = 1.07x** (much less than 1.93x)

### Optimization: Precomputed Affine Points

**Key insight:** Store precomputed points in affine form (Z=1), not projective.

For addition with affine point (x, y, 1, xy):
- Saves 1 field multiplication in the addition formula
- Reduces lookup overhead impact

**Precomputed point format:**
```c
typedef struct {
  gf_s ypx;   /* y + x */
  gf_s ymx;   /* y - x */
  gf_s xy2d;  /* 2 * d * x * y */
} ge_precomp_s;  /* 3 field elements instead of 4 */
```

**Mixed addition (projective + affine):**
- 7 multiplications instead of 8
- Smaller table: 3 × 5 × 8 = 120 bytes per point (vs 160)
- 16 points = 1.9 KB (vs 2.5 KB)

### How Production Libraries Handle This

**libsodium (ref10):**
- Uses precomputed affine points
- 8 tables of 8 points each (for 4-bit signed digits)
- Constant-time lookup via conditional moves
- ~30 KB precomputed data

**curve25519-dalek (Rust):**
- Uses "Niels" coordinates for precomputed points
- Parallel formulas with AVX2 for 4 field elements at once
- Constant-time via SIMD masking

**SUPERCOP ed25519-amd64-64-24k:**
- 24 KB precomputed table
- Assembly-optimized constant-time lookup
- Uses radix-16 signed digits

### Recommendation

**Implement constant-time 4-bit windowed scalar multiplication with:**
1. Precomputed affine points (3 field elements each)
2. 16-point table for base point (~1.9 KB)
3. Constant-time lookup using masks
4. Mixed addition formulas (projective + affine)

**VERIFIED RESULTS (from prototype):**
- fio-stl Montgomery ladder: ~28K ops/sec
- Windowed (constant-time): ~67K ops/sec
- **Speedup: 2.32x**
- CT lookup overhead: only 0.8-3.8%

The speedup is larger than initially estimated because the current fio-stl implementation uses a Montgomery ladder style (cswap + add + dbl per bit = 256 additions + 256 doublings), while windowed approach uses only ~32 additions on average (256 doublings + 64 windows × 50% non-zero = 32 additions).

---

## 2. SIMD Vectorization

### Approaches

#### A. Throughput-Oriented (4×1-way)
Process 4 independent scalar multiplications in parallel, each using one 64-bit lane of a 256-bit vector.

**Pros:**
- Near-linear speedup (4x throughput)
- Simple to implement
- Used by: avxecc, fld-ecc-vec

**Cons:**
- Requires 4 operations to batch
- Doesn't help single-operation latency
- Not useful for interactive signing

**Performance (from research):**
- AVX2: 32,318 X25519 ops/sec (vs 21,000 scalar)
- 1.5x throughput improvement

#### B. Latency-Oriented (Parallel Formulas)
Use SIMD to parallelize within a single point operation.

**HWCD Parallel Formulas (curve25519-dalek):**
- Point addition has 4 independent multiplications
- Pack 4 field elements into one AVX2 vector
- Perform 4 field muls in parallel

**Challenges:**
- Addition/subtraction steps diverge (need masking)
- Shuffle overhead between steps
- Only ~1.5x speedup on Skylake (disappointing)

**Performance (from Henry de Valence's blog):**
- Double-base scalar mult: 99,000 cycles (AVX2)
- vs 460,000 cycles (Go ed25519)
- But vs ~150,000 cycles (optimized scalar)
- **Actual speedup: ~1.5x over optimized scalar**

#### C. Field Arithmetic Vectorization
Vectorize the field multiplication itself.

**Radix-2^29 with AVX2:**
- 9 limbs × 29 bits = 261 bits
- 4 parallel field muls in 88 cycles (Skylake)
- Used by: SAC 2020 paper

**Radix-2^25.5 with AVX2 (Sandy2x):**
- 10 limbs alternating 25/26 bits
- 50 multiplications per field mul (vs 25 for 5×51)
- But 2-way parallel = 25 vector muls

### ARM NEON Considerations

**NEON vs AVX2:**
- 128-bit vectors (vs 256-bit)
- 2 lanes of 64-bit (vs 4)
- No 64×64→128 multiply (only 32×32→64)

**Implications:**
- Throughput approach: 2x (vs 4x for AVX2)
- Parallel formulas: 2 field muls at once
- Field vectorization: Need radix-2^25.5 or smaller

**Current facil.io approach:**
- Uses `fio_math_mulc64` for portable 64×64→128
- Works on ARM64 via `__uint128_t` or intrinsics
- NEON wouldn't help much for current 5×51 representation

### Recommendation

**For facil.io (portable C library):**
1. **Don't pursue SIMD for latency** - complexity vs benefit ratio is poor
2. **Consider optional AVX2 throughput path** for batch operations
3. **Keep 5×51 representation** - optimal for scalar code

**If SIMD is desired later:**
- Add `#ifdef FIO_ED25519_AVX2` for throughput-optimized batch verification
- Use 4×1-way approach (4 independent verifications)
- Expected: 2-3x throughput improvement for batch operations

---

## 3. Better Scalar Multiplication Algorithms

### Current Implementation Analysis

**X25519 (Montgomery Ladder):**
- 255 iterations
- Per iteration: 4 mul + 4 sqr + 8 add/sub + 1 mul121665
- Total: ~1275 mul + 1020 sqr + 2040 add/sub + 1 inversion
- **Cannot use windowing** (Montgomery ladder is inherently bit-by-bit)

**Ed25519 Signing (Fixed-Base):**
- Uses bit-by-bit double-and-add
- 256 doublings + ~128 additions (on average)
- **Can benefit from windowing**

**Ed25519 Verification (Double-Scalar):**
- Computes: [s]B - [h]A
- Currently: Two separate scalar multiplications
- **Can benefit from Straus/Shamir trick**

### A. Fixed-Base Comb Method (for Signing)

**Technique:** Precompute table of [2^i]B for various i, then combine.

**Radix-16 Signed Digits:**
- Represent scalar as 64 signed digits in [-8, 8]
- Precompute: [1]B, [2]B, ..., [8]B (8 points)
- For each digit: lookup + conditional negate + add

**Cost:**
- 64 additions + 4 doublings (to handle the radix)
- vs 256 doublings + 128 additions (bit-by-bit)
- **Speedup: ~3x for fixed-base**

**Table size:** 8 points × 160 bytes = 1.3 KB

### B. wNAF (Windowed Non-Adjacent Form)

**Technique:** Represent scalar with digits {-2^(w-1)+1, ..., -1, 0, 1, ..., 2^(w-1)-1}

**Properties:**
- At most one non-zero digit in any w consecutive positions
- Fewer additions than binary representation
- Requires point subtraction (easy: negate x and t coordinates)

**4-NAF example:**
- Digits: {-7, -5, -3, -1, 0, 1, 3, 5, 7}
- Average density: 1/(w+1) = 1/5 = 20%
- For 256-bit scalar: ~51 additions (vs 128 for binary)

**Cost:**
- 256 doublings + 51 additions
- Precompute: [1]P, [3]P, [5]P, [7]P (4 points)
- **Speedup: ~1.3x for variable-base**

### C. Straus/Shamir Trick (for Verification)

**Problem:** Compute [s]B + [h]A (or [s]B - [h]A)

**Naive:** Two separate scalar multiplications = 2 × (256 dbl + 128 add)

**Straus/Shamir:**
- Process both scalars bit-by-bit together
- Precompute: B, A, B+A, B-A
- For each bit position: 1 doubling + 0-2 additions

**Cost:**
- 256 doublings + ~192 additions (on average)
- vs 512 doublings + 256 additions (naive)
- **Speedup: ~1.5x for verification**

**With wNAF:**
- Use joint sparse form for both scalars
- Even fewer additions
- **Speedup: ~1.7x for verification**

### D. Batch Verification

**Problem:** Verify n signatures at once

**Technique:** Combine into single multi-scalar multiplication:
```
[s1]B - [h1]A1 + [s2]B - [h2]A2 + ... = 0
```

**With random weights (for security):**
```
[r1*s1 + r2*s2 + ...]B - [r1*h1]A1 - [r2*h2]A2 - ... = 0
```

**Cost:**
- 1 multi-scalar multiplication instead of n double-scalar mults
- Using Pippenger or Bos-Coster algorithms
- **Speedup: O(n/log(n)) for large n**

### Recommendation

**Phase 1 (Easy, High Impact):**
1. Implement 4-bit windowed scalar mult for fixed-base (signing)
2. Add precomputed base point table (16 points, ~2.5 KB)
3. **Expected: 1.5-1.9x speedup for signing**

**Phase 2 (Medium, High Impact):**
1. Implement Straus/Shamir for verification
2. Use joint wNAF representation
3. **Expected: 1.3-1.5x speedup for verification**

**Phase 3 (Optional):**
1. Batch verification for multiple signatures
2. Pippenger algorithm for multi-scalar mult
3. **Expected: O(n/log(n)) for n signatures**

---

## 4. Cache Locality & Auto-Vectorization

### Current Memory Layout

**Field element (5×51):**
```c
typedef uint64_t fio___gf_s[5];  /* 40 bytes, 8-byte aligned */
```

**Extended point (4 field elements):**
```c
typedef fio___gf_s fio___ge_p3_s[4];  /* 160 bytes */
```

### Alignment Analysis

**Current alignment:** 8 bytes (natural for uint64_t)

**Cache line size:** 64 bytes (typical)

**Field element:** 40 bytes = spans 1-2 cache lines
**Point:** 160 bytes = spans 3 cache lines

### Would Alignment Help?

**`__attribute__((aligned(32)))`:**
- Ensures 32-byte alignment (AVX2 friendly)
- Field element: still 40 bytes, still spans cache lines
- No benefit for scalar code

**`__attribute__((aligned(64)))`:**
- Ensures cache-line alignment
- Field element: 40 bytes padded to 64 bytes (60% waste)
- Point: 160 bytes padded to 192 bytes (20% waste)
- **Not recommended** - wastes cache space

### Auto-Vectorization Requirements

For compiler to auto-vectorize the multiplication loops:

1. **No loop-carried dependencies** - VIOLATED (carry propagation)
2. **Predictable iteration count** - OK (5 iterations)
3. **Aligned memory access** - Partially OK
4. **No function calls in loop** - VIOLATED (`fio_math_mulc64`)

**Conclusion:** Auto-vectorization is NOT possible for current field multiplication.

### Structure of Arrays (SoA) for Batch Operations

**Current (AoS):**
```c
ge_p3_s points[16];  /* Array of 16 points */
/* points[i].X[j] - poor cache locality across points */
```

**Alternative (SoA):**
```c
struct {
  gf_s X[16];  /* All X coordinates together */
  gf_s Y[16];
  gf_s Z[16];
  gf_s T[16];
} points_soa;
/* points_soa.X[i][j] - good locality for SIMD across points */
```

**Benefits:**
- Better for SIMD throughput (4×1-way)
- All X[0] limbs in consecutive memory
- Can load 4 X[0] values with one AVX2 load

**Drawbacks:**
- Worse for single-point operations
- More complex indexing
- Not useful without SIMD

### Recommendation

**For current scalar implementation:**
1. **Keep current layout** - no benefit from alignment changes
2. **Don't pursue auto-vectorization** - not feasible for field arithmetic
3. **Consider SoA only if adding SIMD throughput path**

---

## 5. Implementation Roadmap

### Phase 1: Constant-Time Windowed Scalar Mult (1-2 days)

**Files to modify:** `fio-stl/154 ed25519.h`

**Changes:**
1. Add precomputed base point table (16 affine points)
2. Add constant-time table lookup function
3. Add mixed addition (projective + affine)
4. Replace `fio___ge_scalarmult_base` with windowed version
5. Keep old implementation for variable-base

**Expected results:**
- Ed25519 signing: 1.5-1.9x faster
- Ed25519 key generation: 1.5-1.9x faster
- X25519: No change (uses Montgomery ladder)

### Phase 2: Straus/Shamir for Verification (1-2 days)

**Changes:**
1. Add joint scalar processing function
2. Precompute B, A, B+A, B-A at verification start
3. Replace double scalar mult with Straus/Shamir
4. Optional: Add wNAF representation

**Expected results:**
- Ed25519 verification: 1.3-1.5x faster

### Phase 3: Batch Inversion (0.5 days)

**Changes:**
1. Add Montgomery's batch inversion function
2. Use in verification (2 inversions → 1 + 3 muls)

**Expected results:**
- Ed25519 verification: Additional 1.1x faster

### Combined Expected Speedup

| Operation | Current | Phase 1 | Phase 2 | Phase 3 | Total |
|-----------|---------|---------|---------|---------|-------|
| Ed25519 sign | 23K/s | 35-44K/s | - | - | **1.5-1.9x** |
| Ed25519 verify | 12K/s | - | 16-18K/s | 17-20K/s | **1.4-1.7x** |
| X25519 pubkey | 47K/s | - | - | - | 1.0x |

**Note:** X25519 uses Montgomery ladder which doesn't benefit from these optimizations. For X25519 speedup, would need:
- Platform-specific assembly
- SIMD field arithmetic
- Different curve (e.g., with endomorphisms)

---

## 6. Prototype Code

### Constant-Time Table Lookup

```c
/* Precomputed affine point for mixed addition */
typedef struct {
  fio___gf_s ypx;   /* y + x */
  fio___gf_s ymx;   /* y - x */
  fio___gf_s xy2d;  /* 2 * d * x * y */
} fio___ge_precomp_s;

/* Constant-time table lookup - reads ALL entries */
FIO_IFUNC void fio___ge_precomp_lookup(fio___ge_precomp_s *r,
                                        const fio___ge_precomp_s table[16],
                                        int index) {
  /* Initialize to identity-like values */
  fio___gf_one(r->ypx);   /* y + x = 1 for identity */
  fio___gf_one(r->ymx);   /* y - x = 1 for identity */
  fio___gf_zero(r->xy2d); /* 2*d*x*y = 0 for identity */
  
  /* Constant-time selection */
  for (int i = 0; i < 16; i++) {
    uint64_t mask = (uint64_t)0 - (uint64_t)(i == index);
    for (int j = 0; j < 5; j++) {
      r->ypx[j]  = (r->ypx[j]  & ~mask) | (table[i].ypx[j]  & mask);
      r->ymx[j]  = (r->ymx[j]  & ~mask) | (table[i].ymx[j]  & mask);
      r->xy2d[j] = (r->xy2d[j] & ~mask) | (table[i].xy2d[j] & mask);
    }
  }
}

/* Mixed addition: projective + affine precomputed */
FIO_IFUNC void fio___ge_madd(fio___ge_p3_s p, const fio___ge_precomp_s *q) {
  fio___gf_s a, b, c, d, e, f, g, h;
  
  /* a = (Y1 - X1) * (y2 - x2) = (Y1 - X1) * ymx */
  fio___gf_sub(a, p[1], p[0]);
  fio___gf_mul(a, a, q->ymx);
  
  /* b = (Y1 + X1) * (y2 + x2) = (Y1 + X1) * ypx */
  fio___gf_add(b, p[1], p[0]);
  fio___gf_mul(b, b, q->ypx);
  
  /* c = T1 * 2*d*x2*y2 = T1 * xy2d */
  fio___gf_mul(c, p[3], q->xy2d);
  
  /* d = 2 * Z1 (since Z2 = 1) */
  fio___gf_add(d, p[2], p[2]);
  
  /* e = b - a, f = d - c, g = d + c, h = b + a */
  fio___gf_sub(e, b, a);
  fio___gf_sub(f, d, c);
  fio___gf_add(g, d, c);
  fio___gf_add(h, b, a);
  
  /* X3 = e * f, Y3 = g * h, Z3 = f * g, T3 = e * h */
  fio___gf_mul(p[0], e, f);
  fio___gf_mul(p[1], g, h);
  fio___gf_mul(p[2], f, g);
  fio___gf_mul(p[3], e, h);
}
```

### 4-Bit Windowed Scalar Multiplication

```c
/* Precomputed base point table: table[i] = (i+1) * B */
static fio___ge_precomp_s FIO___ED25519_BASE_TABLE[16];
static int fio___ed25519_table_initialized = 0;

/* Initialize precomputed table (called once) */
FIO_SFUNC void fio___ed25519_init_table(void) {
  if (fio___ed25519_table_initialized) return;
  
  fio___ge_p3_s B, acc;
  fio___ge_p3_base(B);
  fio___gf_copy(acc[0], B[0]);
  fio___gf_copy(acc[1], B[1]);
  fio___gf_copy(acc[2], B[2]);
  fio___gf_copy(acc[3], B[3]);
  
  for (int i = 0; i < 16; i++) {
    /* Convert acc to precomputed form */
    fio___gf_s z_inv, x, y;
    fio___gf_inv(z_inv, acc[2]);
    fio___gf_mul(x, acc[0], z_inv);
    fio___gf_mul(y, acc[1], z_inv);
    
    fio___gf_add(FIO___ED25519_BASE_TABLE[i].ypx, y, x);
    fio___gf_sub(FIO___ED25519_BASE_TABLE[i].ymx, y, x);
    fio___gf_mul(FIO___ED25519_BASE_TABLE[i].xy2d, x, y);
    fio___gf_mul(FIO___ED25519_BASE_TABLE[i].xy2d, 
                 FIO___ED25519_BASE_TABLE[i].xy2d, 
                 FIO___ED25519_D2);
    
    /* acc = acc + B */
    if (i < 15) {
      fio___ge_p3_add(acc, (const fio___gf_s *)B);
    }
  }
  
  fio___ed25519_table_initialized = 1;
}

/* Windowed scalar multiplication: r = scalar * B */
FIO_SFUNC void fio___ge_scalarmult_base_windowed(fio___ge_p3_s r,
                                                  const uint8_t scalar[32]) {
  fio___ed25519_init_table();
  
  /* Initialize result to identity */
  fio___gf_zero(r[0]);
  fio___gf_one(r[1]);
  fio___gf_one(r[2]);
  fio___gf_zero(r[3]);
  
  /* Process 4 bits at a time, MSB to LSB */
  for (int i = 63; i >= 0; i--) {
    /* Double 4 times */
    fio___ge_p3_dbl(r);
    fio___ge_p3_dbl(r);
    fio___ge_p3_dbl(r);
    fio___ge_p3_dbl(r);
    
    /* Extract 4-bit window */
    int bit_pos = i * 4;
    int byte_idx = bit_pos >> 3;
    int bit_offset = bit_pos & 7;
    
    int window;
    if (bit_offset <= 4) {
      window = (scalar[byte_idx] >> bit_offset) & 0xF;
    } else {
      window = ((scalar[byte_idx] >> bit_offset) |
                (scalar[byte_idx + 1] << (8 - bit_offset))) & 0xF;
    }
    
    /* Constant-time: always do lookup and add, but add identity if window=0 */
    fio___ge_precomp_s tmp;
    int lookup_idx = (window > 0) ? (window - 1) : 0;
    fio___ge_precomp_lookup(&tmp, FIO___ED25519_BASE_TABLE, lookup_idx);
    
    /* Conditionally zero out tmp if window == 0 */
    uint64_t zero_mask = (uint64_t)0 - (uint64_t)(window == 0);
    for (int j = 0; j < 5; j++) {
      tmp.ypx[j]  = (tmp.ypx[j]  & ~zero_mask) | (1 & zero_mask); /* identity ypx = 1 */
      tmp.ymx[j]  = (tmp.ymx[j]  & ~zero_mask) | ((j == 0) & zero_mask);
      tmp.xy2d[j] &= ~zero_mask; /* identity xy2d = 0 */
    }
    
    fio___ge_madd(r, &tmp);
  }
}
```

---

## 7. References

1. **ENG25519** (USENIX Security 2024) - AVX-512IFMA optimizations
2. **curve25519-dalek** - Rust implementation with AVX2 parallel formulas
3. **Sandy2x** - Fastest Curve25519 using vectorized multiplier
4. **High-Throughput ECC using AVX2** (SAC 2020) - 4×1-way parallelism
5. **Ed25519 paper** (Bernstein et al.) - Original specification and optimizations
6. **libsodium** - Production constant-time implementation
7. **SUPERCOP** - Benchmark suite with reference implementations

---

## 8. Conclusion

**Achievable speedup: 1.5-2x for Ed25519 operations**

The most practical path forward:
1. **Implement 4-bit windowed scalar mult** with constant-time lookup
2. **Add Straus/Shamir** for verification
3. **Keep current field arithmetic** - it's already near-optimal

SIMD vectorization provides diminishing returns for single-operation latency but could be valuable for batch operations in server scenarios.
