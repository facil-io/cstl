/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_ED25519            /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                          Elliptic Curve ED25519 (WIP)




Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_ED25519) && !defined(H___FIO_ED25519___H)
#define H___FIO_ED25519___H

/* *****************************************************************************
TODO: ED 25519, ED 448

ED-25519 key generation, key exchange and signatures are crucial to complete the
minimal building blocks that would allow to secure inter-machine communication
in mostly secure environments. Of course the use of a tested cryptographic
library (where accessible) might be preferred, but some security is better than
none.
***************************************************************************** */

/* *****************************************************************************
ED25519 API
***************************************************************************** */

/* *****************************************************************************
Implementation - inlined static functions
***************************************************************************** */

/* *****************************************************************************
Implementation - possibly externed functions.
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

/* prevent ED25519 keys from having a small period (cyclic value). */
FIO_IFUNC void fio___ed25519_clamp_on_key(fio_u256 *k) {
  k->u8[0] &= 0xF8U;  /* zero out 3 least significant bits (emulate mul by 8) */
  k->u8[31] &= 0x7FU; /* unset most significant bit (constant time fix) */
  k->u8[31] |= 0x40U; /* set the 255th bit (making sure the value is big) */
}

/* *****************************************************************************
Testing
***************************************************************************** */
#ifdef FIO_TEST_CSTL
FIO_SFUNC void FIO_NAME_TEST(stl, FIO_ED25519)(void) {
  /*
   * TODO: test module here
   */
}

#endif /* FIO_TEST_CSTL */
/* *****************************************************************************
Cleanup
***************************************************************************** */

#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_ED25519
#endif /* FIO_ED25519 */
