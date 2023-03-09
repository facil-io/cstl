/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_TEST_ALL           /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                        FIO_MODULE_NAME Test Helper




Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_TEST_ALL) && !defined(FIO___TEST_REINCLUDE) &&                 \
    !defined(H___FIO_MODULE_NAME_TEST___H)
#define H___FIO_MODULE_NAME_TEST___H
// #ifndef H___FIO_MODULE_NAME___H
// #define FIO_MODULE_NAME
// #define FIO___TEST_REINCLUDE
// #include FIO_INCLUDE_FILE
// #undef FIO___TEST_REINCLUDE
// #endif

FIO_SFUNC void FIO_NAME_TEST(stl, FIO_MODULE_NAME)(void) {}

/* *****************************************************************************
Cleanup
***************************************************************************** */
#endif /* FIO_TEST_ALL */
