/* Standalone test: zero selects the default before JSON state is declared. */
#define FIO_JSON_MAX_DEPTH 0
#include "test-helpers.h"
#define FIO_JSON
#include FIO_INCLUDE_FILE

FIO_ASSERT_STATIC(FIO_JSON_MAX_DEPTH == 128,
                  "zero JSON depth should select the default of 128");

int main(void) { return FIO_JSON_MAX_DEPTH != 128; }
