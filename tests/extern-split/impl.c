/* *****************************************************************************
Split FIO_EXTERN build - implementation translation unit.

Compiled separately from `main.c` (see `make tests/extern-split`): this unit
holds the single copy of every non-static implementation, while `main.c` only
sees the declarations.
***************************************************************************** */
#define FIO_LOG
#define FIO_EVERYTHING
#define FIO_EXTERN
#define FIO_EXTERN_COMPLETE
#include "fio-stl/include.h"
