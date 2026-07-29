# HTTP Module — Types and Core (432 http types.h)

```c
#define FIO_HTTP
#include FIO_INCLUDE_FILE
```

Internal types plus the HTTP handle and core implementation: the
`fio_http_s` request / response object, header cache, body storage (RAM or
file), routing table, and static-file responses (including the
`compress_static` failure-memoization shift register).

Internal — the public surface is documented in the family doc,
[./439 http.md](./439 http.md).
