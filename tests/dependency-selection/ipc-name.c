/* An unrelated build macro must not select library crypto modules. */
#define IPC
#include "fio-stl/include.h"
#if defined(FIO_CHACHA) || defined(FIO_SECRET) || defined(H___FIO_SECRET___H)
#error Unrelated IPC macro selected crypto modules
#endif
int main(void) { return 0; }
