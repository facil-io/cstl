## HTTP Params to FIOBJ (440 http fiobj.h)

```c
#define FIO_FIOBJ
#define FIO_HTTP
#include FIO_INCLUDE_FILE
```

This bridge compiles automatically once **both** the HTTP and FIOBJ modules are included. They can be included in either order, in one pass or in separate passes. Neither module depends on the other, so you can include each one independently.

```c
FIOBJ fio_http_params2FIOBJ(FIOBJ dest, fio_http_s *h);
```

Calls [`fio_http_params_parse`](./430 http.md#params-parsing-query--body) and returns a FIOBJ Hash of query and body params. `dest` must be `FIOBJ_INVALID` (a new, owned Hash is returned - free it with `fiobj_free`) or an existing `FIOBJ_T_HASH`, which is reused as the root so params merge into its current data using the same conflict rules. A caller-supplied `dest` remains owned by the caller in all cases. Query fields are stored first, then body fields. A JSON object body merges into the Hash; a non-object JSON body contributes nothing. Bodies with missing or unknown Content-Type are ignored. Query, URL-encoded and multipart text values are Strings (including empty Strings for `key` and `key=`); JSON values keep their JSON types.

Repeated scalars become Arrays in source order, Maps merge, and Arrays append. Form field names use dot and bracket paths, including terminal `[]` for explicit appends. Invalid paths stay literal; JSON object keys always stay literal.

Name paths and JSON bodies allow up to `FIO_JSON_MAX_DEPTH` levels (default 128), with the root at level 1 (N name segments open N containers, plus one for terminal `[]`). Define the macro before including the library to change both limits; excessive nesting returns `FIOBJ_INVALID` (the partial Hash is freed, unless it is a caller-supplied `dest`). This differs from NeoRack's suggested 32. These results are checked in [`tests/http-fiobj.c`](../tests/http-fiobj.c):

| Input | Result |
|---|---|
| Query `a=1&a=2&a=3` | `{"a":["1","2","3"]}` |
| Query `user=x&user.name=A` | `{"user":{"":"x","name":"A"}}` |
| Query `user[name]=A`; JSON body `{"user":{"age":3}}` | `{"user":{"name":"A","age":3}}` |
| Query `tag=x&tag[]=a` | `{"tag":["x","a"]}` |
| Query `a=1`; JSON body `[1,2]` | `{"a":"1"}` |
| JSON body `{"a.b":1,"c[d]":2}` | `{"a.b":1,"c[d]":2}` |
| Query `a=1`; JSON body `{"a":null,"b":null,"b":2}` | `{"a":["1",null],"b":[null,2]}` |

JSON `null` values are real `fiobj_null()` objects, so they take part in conflict resolution like any other scalar.

A file upload is a [`FIOBJ_T_DATA`](./250 fiobj.md) object, treated as a scalar in conflict handling. Read borrowed metadata with `fiobj_data_filename(upload)`, `fiobj_data_type(upload)` and `fiobj_data_headers(upload)`; the type has a NULL `buf` if absent. The filename excludes `/` and `\\` path components. Empty filenames and empty field names are omitted. Upload content streams in chunks and spills to a temporary file once it exceeds `FIO_DATA_RAM_LIMIT`. Read it with `fio_data_read(fiobj_data_ptr(upload), len)` or save it with `fio_data_save(fiobj_data_ptr(upload), filename)`; never use the client's filename as a path without validating it. If upload storage fails, parsing fails.

Returns `FIOBJ_INVALID` on parsing error, including a params field-limit overflow, malformed parsed bodies, a `NULL` handle, or a `dest` that isn't a Hash. On error a fresh Hash is freed; a caller-supplied `dest` is NOT freed (it stays valid and caller-owned) but may hold partially merged data. An empty request returns an empty Hash. Check the result before accessing it:

```c
FIOBJ params = fio_http_params2FIOBJ(FIOBJ_INVALID, h);
if (params == FIOBJ_INVALID) {
  /* Handle parse failure. */
} else {
  FIOBJ name = fiobj_hash_get2(params, "name", 4);
  /* Use name while params is alive. */
  fiobj_free(params);
}
```
