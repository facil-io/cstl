# HTTP Module — Public API (430 http api.h)

```c
#define FIO_HTTP
#include FIO_INCLUDE_FILE
```

Public declarations for the HTTP module: `fio_http_settings_s`, the
listener / route / client APIs (`fio_http_listen`, `fio_http_route`,
`fio_http_connect`, `fio_http_websocket_connect`), and the full HTTP handle
API.

Everything declared here is documented in the family doc,
[./439 http.md](./439 http.md).
