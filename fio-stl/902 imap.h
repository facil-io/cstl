/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_TEST_ALL           /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                        FIO_IMAP_CORE Test Helper




Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_TEST_ALL) && !defined(FIO___TEST_REINCLUDE) &&                 \
    !defined(H___FIO_IMAP_CORE_TEST___H)
#define H___FIO_IMAP_CORE_TEST___H

#ifndef H___FIO_IMAP_CORE___H
#define FIO_IMAP_CORE
#define FIO___TEST_REINCLUDE
#include FIO_INCLUDE_FILE
#undef FIO___TEST_REINCLUDE
#endif

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

FIO_SFUNC void FIO_NAME_TEST(stl, imap_core)(void) {
  fprintf(stderr, "* Testing core indexed array type (imap).\n");
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
/* *****************************************************************************
Cleanup
***************************************************************************** */
#endif /* FIO_TEST_ALL */
