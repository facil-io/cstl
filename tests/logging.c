/* *****************************************************************************
Test
***************************************************************************** */
#include "test-helpers.h"

int main(void) {
  FIO_LOG_FATAL("this is a fatal error message!");
  FIO_LOG_ERROR("this is an error message!");
  FIO_LOG_WARNING("this is a warning message!");
  FIO_LOG_INFO("this is an informative message.");
  return 0;
}
