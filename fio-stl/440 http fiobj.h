/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_FIOBJ              /* Development inclusion - ignore line */
#define FIO_HTTP               /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************

                    HTTP <=> FIOBJ Bridge (params to FIOBJ)

This bridge is compiled automatically once BOTH the HTTP and FIOBJ modules were
included (in any order, in the same or separate inclusion passes). Neither
module depends on the other.

Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if !defined(FIO___RECURSIVE_INCLUDE) && defined(H___FIO_HTTP_API___H) &&       \
    defined(H___FIO_FIOBJ___H) && !defined(H___FIO_HTTP_FIOBJ___H)
#define H___FIO_HTTP_FIOBJ___H

/* *****************************************************************************
HTTP Params to FIOBJ - API
***************************************************************************** */

/**
 * Parses the query and body params (see `fio_http_params_parse`) into a new
 * FIOBJ Hash.
 *
 * Duplicate keys: the last value wins (body fields override query pairs).
 * File uploads are stored as a Hash with the keys `"filename"` and `"type"`
 * (Strings) and `"data"` (a `FIOBJ_T_DATA` object holding the content, which
 * moves to a temporary file once it grows beyond `FIO_DATA_RAM_LIMIT`).
 *
 * Returns `FIOBJ_INVALID` on error. The caller must `fiobj_free` the result.
 */
SFUNC FIOBJ fio_http_params2FIOBJ(fio_http_s *h);

/* *****************************************************************************
HTTP Params to FIOBJ - Implementation
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

FIO_SFUNC void *fio___http_fiobj_on_null(void *udata) {
  (void)udata;
  return (void *)fiobj_null();
}
FIO_SFUNC void *fio___http_fiobj_on_true(void *udata) {
  (void)udata;
  return (void *)fiobj_true();
}
FIO_SFUNC void *fio___http_fiobj_on_false(void *udata) {
  (void)udata;
  return (void *)fiobj_false();
}
FIO_SFUNC void *fio___http_fiobj_on_number(void *udata, int64_t num) {
  (void)udata;
  return (void *)fiobj_num_new((intptr_t)num);
}
FIO_SFUNC void *fio___http_fiobj_on_float(void *udata, double num) {
  (void)udata;
  return (void *)fiobj_float_new(num);
}
FIO_SFUNC void *fio___http_fiobj_on_string(void *udata,
                                           const void *data,
                                           size_t len) {
  (void)udata;
  return (void *)fiobj_str_new_cstr((const char *)data, len);
}
FIO_SFUNC void *fio___http_fiobj_on_array(void *udata, void *parent) {
  (void)udata, (void)parent;
  return (void *)fiobj_array_new();
}
FIO_SFUNC void *fio___http_fiobj_on_map(void *udata, void *parent) {
  (void)udata, (void)parent;
  return (void *)fiobj_hash_new();
}
FIO_SFUNC int fio___http_fiobj_array_push(void *udata, void *a, void *value) {
  (void)udata;
  fiobj_array_push((FIOBJ)a, (FIOBJ)value);
  return 0;
}
FIO_SFUNC int fio___http_fiobj_map_set(void *udata,
                                       void *map,
                                       void *key,
                                       void *value) {
  (void)udata;
  fiobj_hash_set((FIOBJ)map, (FIOBJ)key, (FIOBJ)value, NULL); /* last wins */
  fiobj_free((FIOBJ)key); /* fiobj_hash_set duplicates the key */
  return 0;
}
FIO_SFUNC void *fio___http_fiobj_on_file(void *udata,
                                         fio_str_info_s name,
                                         fio_str_info_s filename,
                                         fio_str_info_s content_type) {
  (void)udata, (void)name;
  FIOBJ file = fiobj_hash_new();
  fiobj_hash_set2(file,
                  "filename",
                  8,
                  fiobj_str_new_cstr(filename.buf, filename.len));
  fiobj_hash_set2(file,
                  "type",
                  4,
                  fiobj_str_new_cstr(content_type.buf, content_type.len));
  fiobj_hash_set2(file, "data", 4, fiobj_data_new(0));
  return (void *)file;
}
FIO_SFUNC int fio___http_fiobj_on_file_data(void *udata,
                                            void *file,
                                            fio_buf_info_s data) {
  (void)udata;
  fio_data_s *d = fiobj_data_ptr(fiobj_hash_get2((FIOBJ)file, "data", 4));
  if (!d)
    return -1;
  return fio_data_write(d, data.buf, data.len);
}
FIO_SFUNC void *fio___http_fiobj_on_error(void *udata, void *partial) {
  (void)udata;
  fiobj_free((FIOBJ)partial);
  return NULL;
}
FIO_SFUNC void fio___http_fiobj_free_unused(void *udata, void *obj) {
  (void)udata;
  fiobj_free((FIOBJ)obj);
}

static const fio_http_params_parse_callbacks_s FIO___HTTP_FIOBJ_CALLBACKS = {
    .on_null = fio___http_fiobj_on_null,
    .on_true = fio___http_fiobj_on_true,
    .on_false = fio___http_fiobj_on_false,
    .on_number = fio___http_fiobj_on_number,
    .on_float = fio___http_fiobj_on_float,
    .on_string = fio___http_fiobj_on_string,
    .on_array = fio___http_fiobj_on_array,
    .on_map = fio___http_fiobj_on_map,
    .array_push = fio___http_fiobj_array_push,
    .map_set = fio___http_fiobj_map_set,
    .on_file = fio___http_fiobj_on_file,
    .on_file_data = fio___http_fiobj_on_file_data,
    .on_error = fio___http_fiobj_on_error,
    .free_unused = fio___http_fiobj_free_unused,
};

SFUNC FIOBJ fio_http_params2FIOBJ(fio_http_s *h) {
  fio_http_params_parse_result_s r =
      fio_http_params_parse(h, &FIO___HTTP_FIOBJ_CALLBACKS, NULL);
  if (r.err) { /* `on_error` already freed the partial result */
    fiobj_free((FIOBJ)r.result);
    return FIOBJ_INVALID;
  }
  return (FIOBJ)r.result;
}

#endif /* FIO_EXTERN_COMPLETE */
#endif /* H___FIO_HTTP_FIOBJ___H */
