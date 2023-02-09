/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_VMATH              /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                    Basic Vector Mathematical Operations
                              (TODO, maybe)



Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_VMATH) && !defined(H___FIO_VMATH___H)
#define H___FIO_VMATH___H 1

#if defined(__ARM_FEATURE_CRYPTO) &&                                           \
    (defined(__ARM_NEON) || defined(__ARM_NEON__))
#include <arm_acle.h>
#include <arm_neon.h>
#define FIO___HAS_ARM_INTRIN 1
#endif

#ifndef FIO_MATH_USE_COMPILER_VECTORS
/* Note: currently vector types appear slower than the optimizer */
#define FIO_MATH_USE_COMPILER_VECTORS 0
#endif

#if !(__has_attribute(vector_size))
#undef FIO_MATH_USE_COMPILER_VECTORS
#define FIO_MATH_USE_COMPILER_VECTORS 0
#endif

/* *****************************************************************************
Compiler supported Vector Types
***************************************************************************** */
#if FIO_MATH_USE_COMPILER_VECTORS
#define FIO___DEF_VTYPE(bt, gr)                                                \
  typedef uint##bt##_t __attribute__((vector_size(((bt / 8) * gr))))           \
  fio_u##bt##x##gr;                                                            \
  FIO_IFUNC fio_u##bt##x##gr fio_u##bt##x##gr##_load(const void *buf) {        \
    fio_u##bt##x##gr r;                                                        \
    FIO_MEMCPY(&r, buf, (bt / 8 * gr));                                        \
    return r;                                                                  \
  }                                                                            \
  FIO_IFUNC fio_u##bt##x##gr fio_u##bt##x##gr##_load_le(const void *buf) {     \
    fio_u##bt##x##gr r = fio_u##bt##x##gr##_load(buf);                         \
    for (size_t i = 0; i < gr; ++i)                                            \
      r[i] = fio_ltole##bt(r[i]);                                              \
    return r;                                                                  \
  }                                                                            \
  FIO_IFUNC fio_u##bt##x##gr fio_u##bt##x##gr##_load_be(const void *buf) {     \
    fio_u##bt##x##gr r = fio_u##bt##x##gr##_load(buf);                         \
    for (size_t i = 0; i < gr; ++i)                                            \
      r[i] = fio_lton##bt(r[i]);                                               \
    return r;                                                                  \
  }                                                                            \
  FIO_IFUNC void fio_u##bt##x##gr##_store(void *buf, fio_u##bt##x##gr a) {     \
    FIO_MEMCPY(buf, &a, (bt / 8 * gr));                                        \
  }                                                                            \
  FIO_IFUNC uint##bt##_t fio_u##bt##x##gr##_i(fio_u##bt##x##gr a, size_t i) {  \
    return a[i];                                                               \
  }                                                                            \
  FIO_IFUNC fio_u##bt##x##gr fio_u##bt##x##gr##_iset(fio_u##bt##x##gr a,       \
                                                     size_t i,                 \
                                                     uint##bt##_t val) {       \
    a[i] = val;                                                                \
    return a;                                                                  \
  }

#define FIO___DEF_OPT(bt, gr, nm, op)                                          \
  FIO_IFUNC fio_u##bt##x##gr fio_u##bt##x##gr##_##nm(fio_u##bt##x##gr a,       \
                                                     fio_u##bt##x##gr b) {     \
    return a op b;                                                             \
  }                                                                            \
  FIO_IFUNC fio_u##bt##x##gr fio_u##bt##x##gr##_c##nm(fio_u##bt##x##gr a,      \
                                                      uint##bt##_t b) {        \
    return a op b;                                                             \
  }
#define FIO___DEF_OPT_RLL(bt, gr)                                              \
  FIO_IFUNC fio_u##bt##x##gr fio_u##bt##x##gr##_lrot(fio_u##bt##x##gr a,       \
                                                     fio_u##bt##x##gr b) {     \
    return (a << b) | (a >> b);                                                \
  }                                                                            \
  FIO_IFUNC fio_u##bt##x##gr fio_u##bt##x##gr##_rrot(fio_u##bt##x##gr a,       \
                                                     fio_u##bt##x##gr b) {     \
    return (a >> b) | (a << b);                                                \
  }                                                                            \
  FIO_IFUNC fio_u##bt##x##gr fio_u##bt##x##gr##_clrot(fio_u##bt##x##gr a,      \
                                                      uint##bt##_t b) {        \
    for (size_t i = 0; i < gr; ++i)                                            \
      a[i] = (a[i] << b) | (a[i] >> (bt - b));                                 \
    return a;                                                                  \
  }                                                                            \
  FIO_IFUNC fio_u##bt##x##gr fio_u##bt##x##gr##_crrot(fio_u##bt##x##gr a,      \
                                                      uint##bt##_t b) {        \
    for (size_t i = 0; i < gr; ++i)                                            \
      a[i] = (a[i] >> b) | (a[i] << (bt - b));                                 \
    return a;                                                                  \
  }                                                                            \
  FIO___DEF_OPT(bt, gr, lshift, <<)                                            \
  FIO___DEF_OPT(bt, gr, rshift, >>)

#define FIO___DEF_OPT_SIG(bt, gr, nm, op)                                      \
  FIO_IFUNC fio_u##bt##x##gr fio_u##bt##x##gr##_##nm(fio_u##bt##x##gr a) {     \
    return (op a);                                                             \
  }

#if __has_builtin(__builtin_reduce_add) &&                                     \
    __has_builtin(__builtin_reduce_mul) &&                                     \
    __has_builtin(__builtin_reduce_and) &&                                     \
    __has_builtin(__builtin_reduce_or) && __has_builtin(__builtin_reduce_xor)
#define FIO___DEF_OPT_REDUCE(bt, gr, nm, op)                                   \
  FIO_IFUNC uint##bt##_t fio_u##bt##x##gr##_reduce_##nm(fio_u##bt##x##gr a) {  \
    return __builtin_reduce_##nm(a);                                           \
  }
#endif /* __builtin_reduce */

#else /* FIO_MATH_USE_COMPILER_VECTORS */

#define FIO___DEF_VTYPE(bt, gr)                                                \
  typedef union {                                                              \
    uint##bt##_t v[gr];                                                        \
  } fio_u##bt##x##gr;                                                          \
  FIO_IFUNC fio_u##bt##x##gr fio_u##bt##x##gr##_load(const void *buf) {        \
    fio_u##bt##x##gr r;                                                        \
    FIO_MEMCPY(&r, buf, (bt / 8 * gr));                                        \
    return r;                                                                  \
  }                                                                            \
  FIO_IFUNC fio_u##bt##x##gr fio_u##bt##x##gr##_load_le(const void *buf) {     \
    fio_u##bt##x##gr r = fio_u##bt##x##gr##_load(buf);                         \
    for (size_t i = 0; i < gr; ++i)                                            \
      r.v[i] = fio_ltole##bt(r.v[i]);                                          \
    return r;                                                                  \
  }                                                                            \
  FIO_IFUNC fio_u##bt##x##gr fio_u##bt##x##gr##_load_be(const void *buf) {     \
    fio_u##bt##x##gr r = fio_u##bt##x##gr##_load(buf);                         \
    for (size_t i = 0; i < gr; ++i)                                            \
      r.v[i] = fio_lton##bt(r.v[i]);                                           \
    return r;                                                                  \
  }                                                                            \
  FIO_IFUNC void fio_u##bt##x##gr##_store(void *buf, fio_u##bt##x##gr a) {     \
    FIO_MEMCPY(buf, &a, (bt / 8 * gr));                                        \
  }                                                                            \
  FIO_IFUNC uint##bt##_t fio_u##bt##x##gr##_i(fio_u##bt##x##gr a, size_t i) {  \
    return a.v[i];                                                             \
  }                                                                            \
  FIO_IFUNC fio_u##bt##x##gr fio_u##bt##x##gr##_iset(fio_u##bt##x##gr a,       \
                                                     size_t i,                 \
                                                     uint##bt##_t val) {       \
    a.v[i] = val;                                                              \
    return a;                                                                  \
  }                                                                            \
  FIO_IFUNC fio_u##bt##x##gr fio_u##bt##x##gr##_shuffle_up(                    \
      fio_u##bt##x##gr a) {                                                    \
    uint##bt##_t tmp = a.v[gr - 1];                                            \
    for (size_t i = gr - 1; i--;)                                              \
      a.v[i + 1] = a.v[i];                                                     \
    a.v[0] = tmp;                                                              \
    return a;                                                                  \
  }                                                                            \
  FIO_IFUNC fio_u##bt##x##gr fio_u##bt##x##gr##_shuffle_down(                  \
      fio_u##bt##x##gr a) {                                                    \
    uint##bt##_t tmp = a.v[0];                                                 \
    for (size_t i = 0; i < (gr - 1); ++i)                                      \
      a.v[i] = a.v[i + 1];                                                     \
    a.v[gr - 1] = tmp;                                                         \
    return a;                                                                  \
  }

#define FIO___DEF_OPT(bt, gr, nm, op)                                          \
  FIO_IFUNC fio_u##bt##x##gr fio_u##bt##x##gr##_##nm(fio_u##bt##x##gr a,       \
                                                     fio_u##bt##x##gr b) {     \
    fio_u##bt##x##gr r;                                                        \
    for (size_t i = 0; i < gr; ++i)                                            \
      r.v[i] = a.v[i] op b.v[i];                                               \
    return r;                                                                  \
  }                                                                            \
  FIO_IFUNC fio_u##bt##x##gr fio_u##bt##x##gr##_c##nm(fio_u##bt##x##gr a,      \
                                                      uint##bt##_t b) {        \
    fio_u##bt##x##gr r;                                                        \
    for (size_t i = 0; i < gr; ++i)                                            \
      r.v[i] = a.v[i] op b;                                                    \
    return r;                                                                  \
  }

#define FIO___DEF_OPT_RLL(bt, gr)                                              \
  FIO_IFUNC fio_u##bt##x##gr fio_u##bt##x##gr##_lrot(fio_u##bt##x##gr a,       \
                                                     fio_u##bt##x##gr b) {     \
    for (size_t i = 0; i < gr; ++i)                                            \
      a.v[i] = (a.v[i] << (b.v[i] & (bt - 1))) |                               \
               (a.v[i] >> ((bt - b.v[i]) & (bt - 1)));                         \
    return a;                                                                  \
  }                                                                            \
  FIO_IFUNC fio_u##bt##x##gr fio_u##bt##x##gr##_rrot(fio_u##bt##x##gr a,       \
                                                     fio_u##bt##x##gr b) {     \
    for (size_t i = 0; i < gr; ++i)                                            \
      a.v[i] = (a.v[i] >> (b.v[i] & (bt - 1))) |                               \
               (a.v[i] << ((bt - b.v[i]) & (bt - 1)));                         \
    return a;                                                                  \
  }                                                                            \
  FIO_IFUNC fio_u##bt##x##gr fio_u##bt##x##gr##_clrot(fio_u##bt##x##gr a,      \
                                                      uint##bt##_t b) {        \
    for (size_t i = 0; i < gr; ++i)                                            \
      a.v[i] = (a.v[i] << (b & (bt - 1))) | (a.v[i] >> ((bt - b) & (bt - 1))); \
    return a;                                                                  \
  }                                                                            \
  FIO_IFUNC fio_u##bt##x##gr fio_u##bt##x##gr##_crrot(fio_u##bt##x##gr a,      \
                                                      uint##bt##_t b) {        \
    for (size_t i = 0; i < gr; ++i)                                            \
      a.v[i] = (a.v[i] >> (b & (bt - 1))) | (a.v[i] << ((bt - b) & (bt - 1))); \
    return a;                                                                  \
  }                                                                            \
  FIO___DEF_OPT(bt, gr, lshift, <<)                                            \
  FIO___DEF_OPT(bt, gr, rshift, >>)

#define FIO___DEF_OPT_SIG(bt, gr, nm, op)                                      \
  FIO_IFUNC fio_u##bt##x##gr fio_u##bt##x##gr##_##nm(fio_u##bt##x##gr a) {     \
    fio_u##bt##x##gr r;                                                        \
    for (size_t i = 0; i < gr; ++i)                                            \
      r.v[i] = op a.v[i];                                                      \
    return r;                                                                  \
  }

#endif /* FIO_MATH_USE_COMPILER_VECTORS */

#ifndef FIO___DEF_OPT_REDUCE
#define FIO___DEF_OPT_REDUCE(bt, gr, nm, op)                                   \
  FIO_IFUNC uint##bt##_t fio_u##bt##x##gr##_reduce_##nm(fio_u##bt##x##gr a) {  \
    uint##bt##_t r = fio_u##bt##x##gr##_i(a, 0);                               \
    for (size_t i = 1; i < gr; ++i)                                            \
      r = r op fio_u##bt##x##gr##_i(a, i);                                     \
    return r;                                                                  \
  }
#endif

#define FIO__DEF_VGROUP(bits, groups)                                          \
  FIO___DEF_VTYPE(bits, groups)                                                \
  FIO___DEF_OPT(bits, groups, mul, *)                                          \
  FIO___DEF_OPT(bits, groups, add, +)                                          \
  FIO___DEF_OPT(bits, groups, sub, -)                                          \
  FIO___DEF_OPT(bits, groups, div, /)                                          \
  FIO___DEF_OPT(bits, groups, reminder, %)                                     \
  FIO___DEF_OPT(bits, groups, and, &)                                          \
  FIO___DEF_OPT(bits, groups, or, |)                                           \
  FIO___DEF_OPT(bits, groups, xor, ^)                                          \
  FIO___DEF_OPT_RLL(bits, groups)                                              \
  FIO___DEF_OPT_SIG(bits, groups, flip, ~)                                     \
  FIO___DEF_OPT_REDUCE(bits, groups, mul, *)                                   \
  FIO___DEF_OPT_REDUCE(bits, groups, add, +)                                   \
  FIO___DEF_OPT_REDUCE(bits, groups, and, &)                                   \
  FIO___DEF_OPT_REDUCE(bits, groups, or, |)                                    \
  FIO___DEF_OPT_REDUCE(bits, groups, xor, ^)

FIO__DEF_VGROUP(8, 16)
FIO__DEF_VGROUP(8, 32)
FIO__DEF_VGROUP(8, 64)

FIO__DEF_VGROUP(16, 8)
FIO__DEF_VGROUP(16, 16)
FIO__DEF_VGROUP(16, 32)

FIO__DEF_VGROUP(32, 4)
FIO__DEF_VGROUP(32, 8)
FIO__DEF_VGROUP(32, 16)

FIO__DEF_VGROUP(64, 2)
FIO__DEF_VGROUP(64, 4)
FIO__DEF_VGROUP(64, 8)

#undef FIO__DEF_VGROUP
/* *****************************************************************************
Common Math operations - test
***************************************************************************** */
#if defined(FIO_TEST_CSTL)

FIO_SFUNC void FIO_NAME_TEST(stl, vmath)(void) {
  fprintf(stderr, "* Testing vector math operations (missing).\n");
  fio_u32x4 a = {1, 1, 1, 1}, b = {2, 2, 2, 2};
  a = fio_u32x4_mul(a, b);
  FIO_ASSERT(fio_u32x4_i(a, 2) == 2, "FIO_VOP routing failed.");
}

#endif /* FIO_TEST_CSTL */
/* *****************************************************************************
Math - cleanup
***************************************************************************** */
#endif /* FIO_MATH */
#undef FIO_MATH
