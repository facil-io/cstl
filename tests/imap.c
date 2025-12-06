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
}
