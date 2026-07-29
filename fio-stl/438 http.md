# HTTP Module — Listen / Connect Glue (438 http.h)

```c
#define FIO_HTTP
#include FIO_INCLUDE_FILE
```

Listen / connect glue, protocol wiring (ALPN, attach, upgrade routing), and
shared helpers for the HTTP module, including the `fio_http_connect` /
`fio_http_websocket_connect` client implementations.

Internal — the public surface is documented in the family doc,
[./439 http.md](./439 http.md).
