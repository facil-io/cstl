## HTTP Params to FIOBJ (440 http fiobj.h)

```c
#define FIO_FIOBJ
#define FIO_HTTP
#include FIO_INCLUDE_FILE
```

This bridge compiles automatically once **both** the HTTP and FIOBJ modules are included. They can be included in either order, in one pass or in separate passes. Neither module depends on the other, so you can include each one independently.

```c
FIOBJ fio_http_params2FIOBJ(fio_http_s *h);
```

Calls `fio_http_params_parse` (see [430 http.md](./430 http.md)) and returns the query and body params as a new FIOBJ Hash. The caller must `fiobj_free` it.

- Query pairs are added first, followed by the body fields. A JSON object body is merged into the Hash. Any other JSON body (array or primitive) is stored under `"_json"`.
- Query and URL-encoded values are Strings (JSON bodies keep their JSON types). Missing values (`key` or `key=`) become empty Strings.
- When a key repeats, the last value wins. The Hash is ordered, so the replaced key moves to the end.
- A file upload is stored as a Hash with the keys `"filename"` and `"type"` (Strings) and `"data"`. `"data"` is a [`FIOBJ_T_DATA`](./250 fiobj.md) object holding the content. Uploads are streamed in chunks, and the content moves to a temporary file once it grows past `FIO_DATA_RAM_LIMIT`, so large files never sit fully in memory. Read it in chunks with `fio_data_read(fiobj_data_ptr(data), len)`.
- A body with a missing or unknown Content-Type is ignored.
- Returns `FIOBJ_INVALID` on error (malformed body, `NULL` handle).

```c
FIOBJ params = fio_http_params2FIOBJ(h);
FIOBJ name = fiobj_hash_get2(params, "name", 4);
/* ... */
fiobj_free(params);
```
