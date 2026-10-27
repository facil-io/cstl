/* *****************************************************************************
Test
***************************************************************************** */
#include "test-helpers.h"

#define FIO_IMAP_TESTER_IMAP_HASH(n)   ((n)[0] | ((n)[0] << 9))
#define FIO_IMAP_TESTER_IMAP_CMP(a, b) (*(a) == *(b))
#define FIO_IMAP_TESTER_IMAP_VALID(n)  ((n)[0])
FIO_TYPEDEF_IMAP_ARRAY(fio_imap_tester,
                       size_t,
                       uint32_t, /* good for up to 65K objects */
                       FIO_IMAP_TESTER_IMAP_HASH,
                       FIO_IMAP_TESTER_IMAP_CMP,
                       FIO_IMAP_TESTER_IMAP_VALID)

#undef FIO_IMAP_TESTER_IMAP_HASH
#undef FIO_IMAP_TESTER_IMAP_CMP
#undef FIO_IMAP_TESTER_IMAP_VALID

/* Regression maps: 8-bit capacity limit, constant hashes, always valid and
 * odd sized elements with a 64-bit index (alignment). */
#define FIO_IMAP_TESTER_U8_HASH(n) ((uint8_t)((n)[0] * 37))
FIO_TYPEDEF_IMAP_ARRAY(fio_imap_tester_u8,
                       uint32_t,
                       uint8_t,
                       FIO_IMAP_TESTER_U8_HASH,
                       FIO_IMAP_SIMPLE_CMP,
                       FIO_IMAP_VALID_NON_ZERO)
#define FIO_IMAP_TESTER_CONST_HASH(n) ((uint32_t)0x12345678UL)
FIO_TYPEDEF_IMAP_ARRAY(fio_imap_tester_const,
                       uint64_t,
                       uint32_t,
                       FIO_IMAP_TESTER_CONST_HASH,
                       FIO_IMAP_SIMPLE_CMP,
                       FIO_IMAP_VALID_NON_ZERO)
#define FIO_IMAP_TESTER_ID_HASH(n) ((n)[0])
FIO_TYPEDEF_IMAP_ARRAY(fio_imap_tester_always,
                       uint64_t,
                       uint64_t,
                       FIO_IMAP_TESTER_ID_HASH,
                       FIO_IMAP_SIMPLE_CMP,
                       FIO_IMAP_ALWAYS_VALID)
typedef struct {
  uint8_t b[3];
} fio_imap_odd_elem_s;
#define FIO_IMAP_TESTER_ODD_HASH(o)                                            \
  ((uint64_t)(o)->b[0] * 0x9E3779B97F4A7C15ULL)
#define FIO_IMAP_TESTER_ODD_CMP(x, y) ((x)->b[0] == (y)->b[0])
#define FIO_IMAP_TESTER_ODD_VALID(o)  ((o)->b[0])
FIO_TYPEDEF_IMAP_ARRAY(fio_imap_tester_odd,
                       fio_imap_odd_elem_s,
                       uint64_t,
                       FIO_IMAP_TESTER_ODD_HASH,
                       FIO_IMAP_TESTER_ODD_CMP,
                       FIO_IMAP_TESTER_ODD_VALID)

static void fio_imap_test_limits(void) {
  { /* full 8-bit map + tombstone: must never write past `capa` */
    fio_imap_tester_u8_s m = {0};
    uint32_t v = 1;
    while (fio_imap_tester_u8_set(&m, v, 0))
      ++v;
    size_t capa = fio_imap_tester_u8_capa(&m);
    FIO_ASSERT(capa == 64 && m.w <= capa && m.count == v - 1,
               "8-bit imap should stop at its capacity limit");
    FIO_ASSERT(!fio_imap_tester_u8_set(&m, v, 0),
               "8-bit imap insert past capacity should fail");
    FIO_ASSERT(!fio_imap_tester_u8_remove(&m, 2), "8-bit imap remove failed");
    uint32_t *p = fio_imap_tester_u8_set(&m, v, 0);
    FIO_ASSERT(m.w <= fio_imap_tester_u8_capa(&m) &&
                   (!p || (p >= m.ary && p < m.ary + m.w && *p == v)),
               "8-bit imap insert after removal exceeded capacity");
    for (uint32_t i = 1; i < v; ++i)
      FIO_ASSERT((i == 2) == !fio_imap_tester_u8_get(&m, i),
                 "8-bit imap lost item %u",
                 (unsigned)i);
    fio_imap_tester_u8_destroy(&m);
  }
  { /* identical hashes: bounded failure, no runaway growth */
    fio_imap_tester_const_s m = {0};
    size_t accepted = 0;
    for (uint64_t i = 1; i <= 64; ++i)
      accepted += !!fio_imap_tester_const_set(&m, i, 0);
    FIO_ASSERT(accepted && accepted < 64 && m.count == accepted,
               "constant hash imap should reject saturated inserts");
    FIO_ASSERT(fio_imap_tester_const_capa(&m) <= 512,
               "constant hash imap grew without bound (%zu)",
               fio_imap_tester_const_capa(&m));
    for (uint64_t i = 1; i <= accepted; ++i)
      FIO_ASSERT(fio_imap_tester_const_get(&m, i),
                 "constant hash imap lost an accepted item");
    FIO_ASSERT(!fio_imap_tester_const_get(&m, 64),
               "constant hash imap found a rejected item");
    fio_imap_tester_const_destroy(&m);
  }
  { /* always valid: removal unsupported, rehash keeps state */
    fio_imap_tester_always_s m = {0};
    for (uint64_t i = 1; i < 4; ++i)
      FIO_ASSERT(fio_imap_tester_always_set(&m, i, 0), "always-valid set");
    FIO_ASSERT(fio_imap_tester_always_remove(&m, 2) == -1 && m.count == 3,
               "always-valid imap removal should be refused");
    FIO_ASSERT(!fio_imap_tester_always_rehash(&m) && m.count == 3 &&
                   m.w == 3 && fio_imap_tester_always_get(&m, 2),
               "always-valid imap rehash failed");
    /* capacity arithmetic must not wrap */
    FIO_ASSERT(fio_imap_tester_always_reserve(&m, (uint64_t)1 << 60) == -1 &&
                   fio_imap_tester_always_reserve(&m, ~(uint64_t)0) == -1 &&
                   m.count == 3 && fio_imap_tester_always_get(&m, 3),
               "imap reserve overflow should fail without side effects");
    fio_imap_tester_always_destroy(&m);
  }
  { /* odd sized elements: the 64-bit index must stay aligned */
    fio_imap_tester_odd_s m = {0};
    for (uint8_t i = 1; i < 200; ++i)
      FIO_ASSERT(fio_imap_tester_odd_set(&m, (fio_imap_odd_elem_s){{i}}, 0),
                 "odd imap set failed");
    FIO_ASSERT(!((uintptr_t)fio_imap_tester_odd_imap(&m) & 7),
               "odd imap index misaligned");
    for (uint8_t i = 1; i < 200; ++i)
      FIO_ASSERT(fio_imap_tester_odd_get(&m, (fio_imap_odd_elem_s){{i}}),
                 "odd imap get failed");
    fio_imap_tester_odd_destroy(&m);
  }
}

int main(void) {
  fio_imap_tester_s a = {0};
  fio_imap_tester_reserve(&a, 1024);
  FIO_ASSERT(fio_imap_tester_capa(&a) >= 1024 &&
                 fio_imap_tester_capa(&a) < 4096,
             "fio_imap_tester_reserve failed");
  for (size_t val = 1; val < 4096; ++val) {
    size_t *pobj = fio_imap_tester_set(&a, val, 1);
    FIO_ASSERT(a.count == val, "imap array count failed at set %zu!", val);
    size_t *ptmp = fio_imap_tester_set(&a, val, 0);
    FIO_ASSERT(ptmp == pobj,
               "fio_imap_tester_set should return pointer to existing item");
    ptmp = fio_imap_tester_set(&a, val, 0);
    FIO_ASSERT(ptmp == pobj,
               "fio_imap_tester_set should return pointer to existing item");
    ptmp = fio_imap_tester_set(&a, val, 0);
    FIO_ASSERT(ptmp == pobj,
               "fio_imap_tester_set should return pointer to existing item");
    FIO_ASSERT(a.count == val, "imap array double-set error %zu!", val);
    FIO_ASSERT(fio_imap_tester_get(&a, val) == pobj &&
                   fio_imap_tester_get(&a, val)[0] == val,
               "imap array get failed for %zu!",
               val);
  }
  for (size_t val = 1; val < 4096; ++val) {
    FIO_ASSERT(fio_imap_tester_get(&a, val) &&
                   fio_imap_tester_get(&a, val)[0] == val,
               "imap array get failed for %zu (2)!",
               val);
  }
  for (size_t val = 4096; --val;) {
    FIO_ASSERT(fio_imap_tester_get(&a, val) &&
                   fio_imap_tester_get(&a, val)[0] == val,
               "imap array get failed for %zu (2)!",
               val);
    fio_imap_tester_remove(&a, val);
    FIO_ASSERT((size_t)(a.count + 1) == val,
               "imap array count failed at remove %zu!",
               val);
    FIO_ASSERT(!fio_imap_tester_get(&a, val),
               "imap array get should fail after remove for %zu!",
               val);
  }
  fio_imap_tester_destroy(&a);
  fio_imap_test_limits();
}
