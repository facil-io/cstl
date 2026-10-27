/* Capture real IPC listener diagnostics for secret-bearing URL options. */
#include <stdio.h>
static FILE *ipc_log_stream;
#define FIO_STDERR_FILE (ipc_log_stream ? ipc_log_stream : stderr)
#define FIO_LOG
#define FIO_IPC
#include "fio-stl/include.h"

int main(int argc, char **argv) {
  char logs[16384] = {0};
  ipc_log_stream = tmpfile();
  FIO_ASSERT(ipc_log_stream, "couldn't capture IPC diagnostics");
  FIO_LOG_LEVEL_SET(FIO_LOG_LEVEL_DEBUG);
  if (argc == 1 || !strcmp(argv[1], "sock")) {
    char text[2200];
    const char *suffix = "?pass=ipc_passphrase_marker";
    /* All URL length-error branches must avoid dumping the original URL. */
    for (size_t i = 0; i < 3; ++i) {
#ifndef AF_UNIX
      if (i == 0)
        continue;
#endif
      size_t prefix = 0;
      if (i == 0) {
        FIO_MEMCPY(text, "unix:///", 8);
        prefix = 8;
      } else if (i == 1) {
        FIO_MEMCPY(text, "tcp://h:", 8);
        prefix = 8;
      }
      size_t count = i == 1 ? 64 : 2048;
      FIO_MEMSET(text + prefix, 'x', count);
      FIO_MEMCPY(text + prefix + count, suffix, strlen(suffix) + 1);
      FIO_ASSERT(!FIO_SOCK_FD_ISVALID(fio_sock_open2(text, FIO_SOCK_SERVER)),
                 "overlong socket URL should be rejected");
    }
  }
  if (argc == 1 || !strcmp(argv[1], "ipc")) {
    FIO_ASSERT(
        !fio_ipc_url_set("tcp://127.0.0.1:0/?tls&key=ipc_private_marker&"
                         "cert=ipc_cert_marker&pass=ipc_passphrase_marker"),
        "couldn't set IPC URL");
    fio___ipc_listen(NULL);
    fio_queue_perform_all(fio_io_queue());
  }
  FIO_ASSERT(!fflush(ipc_log_stream) && !fseek(ipc_log_stream, 0, SEEK_SET),
             "couldn't read IPC diagnostics");
  size_t len = fread(logs, 1, sizeof(logs) - 1, ipc_log_stream);
  FIO_ASSERT(!ferror(ipc_log_stream) && len < sizeof(logs) - 1,
             "IPC diagnostic capture truncated or failed");
  FIO_ASSERT(!fclose(ipc_log_stream), "couldn't close IPC diagnostic stream");
  ipc_log_stream = NULL;
  if (argc == 1 || !strcmp(argv[1], "ipc"))
    FIO_ASSERT(strstr(logs, "ignoring TLS") && strstr(logs, "IPC listening"),
               "IPC diagnostics were not captured");
  else
    FIO_ASSERT(strstr(logs, "too long"), "socket diagnostics not captured");
  FIO_ASSERT(!strstr(logs, "ipc_passphrase_marker") &&
                 !strstr(logs, "ipc_private_marker") &&
                 !strstr(logs, "ipc_cert_marker"),
             "IPC diagnostics disclosed URL material: %s",
             logs);
  FIO_LOG_LEVEL_SET(FIO_LOG_LEVEL_NONE);
  return 0;
}
