/* Exercise PCLMUL through portable fx86, not native passthrough, on every CPU. */
#ifndef NO_INTRIN
#define NO_INTRIN
#endif
#ifndef FIO_FAKE_X86_SHADOW
#define FIO_FAKE_X86_SHADOW
#endif
#include "crc32.c"

#if !defined(FIO___HAS_X86_INTRIN) || !defined(__SSE4_2__) ||                  \
    !defined(__PCLMUL__) || defined(__ARM_FEATURE_CRC32)
#error "CRC32 shadow regression must select the x86 PCLMUL path"
#endif
