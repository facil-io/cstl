/* *****************************************************************************
Test - Malloc Module (system allocator backends)

Runs the tests/malloc.c suite with FIO_MEMORY_DISABLE, so the aligned
allocation API routes through the system allocator backends
(`_aligned_malloc`/`_aligned_realloc`/`_aligned_free` on Windows,
`posix_memalign`/`free` on POSIX).
***************************************************************************** */
#define FIO_MEMORY_DISABLE
#include "malloc.c"
