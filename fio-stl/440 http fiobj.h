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
 * Parses the query and body params (see `fio_http_params_parse`) into a FIOBJ
 * Hash.
 *
 * `dest` must be `FIOBJ_INVALID` (a new Hash is allocated) or an existing
 * `FIOBJ_T_HASH`, which is reused as the root so params merge into its current
 * data. A caller-supplied `dest` remains owned by the caller in all cases.
 *
 * Repeated scalar keys become Arrays in source order; Maps merge and Arrays
 * append across query and body sources using NeoRack conflict rules. Query and
 * form field names use dot and bracket paths (`a.b`, `a[b]`, terminal `[]`);
 * invalid paths and JSON object keys stay literal. Name paths and JSON bodies
 * allow up to FIO_JSON_MAX_DEPTH levels (default 128), with the root at
 * level 1; excessive nesting returns FIOBJ_INVALID.
 * File uploads are `FIOBJ_T_DATA` objects (scalars in conflicts). Read their
 * metadata using `fiobj_data_filename`, `fiobj_data_type`, and
 * `fiobj_data_headers`; read content through `fiobj_data_ptr`. Content moves
 * to a temporary file once it exceeds `FIO_DATA_RAM_LIMIT`.
 *
 * Returns the Hash (new or `dest`) on success; a fresh Hash's result must be
 * freed with `fiobj_free`. Returns `FIOBJ_INVALID` on error: a fresh Hash is
 * freed, while a caller-supplied `dest` is NOT freed but may hold partially
 * merged data.
 */
SFUNC FIOBJ fio_http_params2FIOBJ(FIOBJ dest, fio_http_s *h);

/* *****************************************************************************
HTTP Params to FIOBJ - Implementation
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

#define FIO___HTTP_FIOBJ_OBJ(o) ((fio_json_obj_result_s){.obj = (void *)(o)})
FIO_SFUNC fio_json_obj_result_s fio___http_fiobj_on_null(void *udata) {
  (void)udata;
  return FIO___HTTP_FIOBJ_OBJ(fiobj_null()); /* a real object: conflicts */
}
FIO_SFUNC fio_json_obj_result_s fio___http_fiobj_on_true(void *udata) {
  (void)udata;
  return FIO___HTTP_FIOBJ_OBJ(fiobj_true());
}
FIO_SFUNC fio_json_obj_result_s fio___http_fiobj_on_false(void *udata) {
  (void)udata;
  return FIO___HTTP_FIOBJ_OBJ(fiobj_false());
}
FIO_SFUNC fio_json_obj_result_s fio___http_fiobj_on_number(void *udata,
                                                           int64_t num) {
  (void)udata;
  return FIO___HTTP_FIOBJ_OBJ(fiobj_num_new((intptr_t)num));
}
FIO_SFUNC fio_json_obj_result_s fio___http_fiobj_on_float(void *udata,
                                                          double num) {
  (void)udata;
  return FIO___HTTP_FIOBJ_OBJ(fiobj_float_new(num));
}
FIO_SFUNC fio_json_obj_result_s fio___http_fiobj_on_string(void *udata,
                                                           fio_buf_info_s s) {
  (void)udata;
  FIOBJ str = fiobj_str_new();
  fiobj_str_write_unescape(str, s.buf, s.len);
  return FIO___HTTP_FIOBJ_OBJ(str);
}
FIO_SFUNC fio_json_obj_result_s
fio___http_fiobj_on_string_simple(void *udata, fio_buf_info_s s) {
  (void)udata;
  return FIO___HTTP_FIOBJ_OBJ(fiobj_str_new_cstr(s.buf, s.len));
}
/* Opens a container at `parent[key]`. The root reuses `udata` (the caller's
 * Hash) when its type matches. A matching member is reused (merge / append);
 * any other member is replaced and handed back in `old` for the HTTP form
 * conflict rules. */
FIO_IFUNC fio_json_push_result_s
fio___http_fiobj_on_container(void *udata,
                              void *parent,
                              fio_buf_info_s key,
                              fiobj_class_en type) {
  fio_json_push_result_s r = {0};
  if (!parent && udata && FIOBJ_TYPE_CLASS((FIOBJ)udata) == type) {
    r.obj = udata;
    return r;
  }
  if (parent && key.buf) {
    FIOBJ current = fiobj_hash_get2((FIOBJ)parent, key.buf, key.len);
    if (FIOBJ_TYPE_CLASS(current) == type) {
      r.obj = (void *)current;
      return r;
    }
  }
  FIOBJ o = (type == FIOBJ_T_HASH) ? fiobj_hash_new() : fiobj_array_new();
  r.obj = (void *)o;
  if (!parent)
    return r; /* a new, owned root */
  if (!key.buf) {
    fiobj_array_push((FIOBJ)parent, o);
    return r;
  }
  FIOBJ name = fiobj_str_new_cstr(key.buf, key.len);
  FIOBJ replaced = FIOBJ_INVALID;
  fiobj_hash_set((FIOBJ)parent, name, o, &replaced);
  fiobj_free(name); /* fiobj_hash_set duplicates the key */
  r.old = (void *)replaced;
  return r;
}
FIO_SFUNC fio_json_push_result_s fio___http_fiobj_on_map(void *udata,
                                                         void *parent,
                                                         fio_buf_info_s key) {
  return fio___http_fiobj_on_container(udata, parent, key, FIOBJ_T_HASH);
}
FIO_SFUNC fio_json_push_result_s fio___http_fiobj_on_array(void *udata,
                                                           void *parent,
                                                           fio_buf_info_s key) {
  return fio___http_fiobj_on_container(udata, parent, key, FIOBJ_T_ARRAY);
}
FIO_SFUNC int fio___http_fiobj_array_push(void *udata, void *a, void *value) {
  (void)udata;
  fiobj_array_push((FIOBJ)a, (FIOBJ)value);
  return 0;
}
/* Stores `value`, handing a replaced value back for conflict resolution. */
FIO_SFUNC fio_json_obj_result_s fio___http_fiobj_map_push(void *udata,
                                                          void *map,
                                                          fio_buf_info_s key,
                                                          void *value) {
  (void)udata;
  FIOBJ name = fiobj_str_new_cstr(key.buf, key.len);
  FIOBJ replaced = FIOBJ_INVALID;
  fiobj_hash_set((FIOBJ)map, name, (FIOBJ)value, &replaced);
  fiobj_free(name); /* fiobj_hash_set duplicates the key */
  return FIO___HTTP_FIOBJ_OBJ(replaced);
}
FIO_SFUNC int fio___http_fiobj_is_array(void *udata, void *obj) {
  (void)udata;
  return FIOBJ_TYPE_CLASS((FIOBJ)obj) == FIOBJ_T_ARRAY;
}
FIO_SFUNC int fio___http_fiobj_is_map(void *udata, void *obj) {
  (void)udata;
  return FIOBJ_TYPE_CLASS((FIOBJ)obj) == FIOBJ_T_HASH;
}
FIO_SFUNC void fio___http_fiobj_free_unused(void *udata, void *obj) {
  (void)udata;
  fiobj_free((FIOBJ)obj);
}
#undef FIO___HTTP_FIOBJ_OBJ
FIO_SFUNC void *fio___http_fiobj_on_file(void *udata,
                                         fio_str_info_s name,
                                         fio_str_info_s filename,
                                         fio_str_info_s content_type,
                                         fio_str_info_s headers) {
  (void)udata, (void)name;
  FIOBJ file = fiobj_data_new(0);
  if (file == FIOBJ_INVALID)
    return NULL;
  fiobj_data_filename_set(file, filename);
  if (content_type.buf)
    fiobj_data_type_set(file, content_type);
  fiobj_data_headers_set(file, headers);
  return (void *)file;
}
FIO_SFUNC int fio___http_fiobj_on_file_data(void *udata,
                                            void *file,
                                            fio_buf_info_s data) {
  (void)udata;
  fio_data_s *d = fiobj_data_ptr((FIOBJ)file);
  if (!d)
    return -1;
  return fio_data_write(d, data.buf, data.len);
}

static const fio_http_params_parse_callbacks_s FIO___HTTP_FIOBJ_CALLBACKS = {
    .on_null = fio___http_fiobj_on_null,
    .on_true = fio___http_fiobj_on_true,
    .on_false = fio___http_fiobj_on_false,
    .on_number = fio___http_fiobj_on_number,
    .on_float = fio___http_fiobj_on_float,
    .on_string = fio___http_fiobj_on_string,
    .on_string_simple = fio___http_fiobj_on_string_simple,
    .on_map = fio___http_fiobj_on_map,
    .on_array = fio___http_fiobj_on_array,
    .map_push = fio___http_fiobj_map_push,
    .array_push = fio___http_fiobj_array_push,
    .is_array = fio___http_fiobj_is_array,
    .is_map = fio___http_fiobj_is_map,
    .free_unused_object = fio___http_fiobj_free_unused,
    .on_file = fio___http_fiobj_on_file,
    .on_file_data = fio___http_fiobj_on_file_data,
};

SFUNC FIOBJ fio_http_params2FIOBJ(FIOBJ dest, fio_http_s *h) {
  const int fresh = (dest == FIOBJ_INVALID);
  if (!h || (!fresh && FIOBJ_TYPE_CLASS(dest) != FIOBJ_T_HASH))
    return FIOBJ_INVALID;
  if (fresh) {
    dest = fiobj_hash_new();
    if (dest == FIOBJ_INVALID)
      return FIOBJ_INVALID;
  }
  /* The root `on_map` reuses `dest` (passed as udata) as the params map. */
  fio_http_params_parse_result_s r =
      fio_http_params_parse(h,
                            .callbacks = &FIO___HTTP_FIOBJ_CALLBACKS,
                            .udata = (void *)dest);
  if (r.err) {
    /* The partial result is `dest` (or NULL): only a fresh Hash is ours. */
    if (fresh)
      fiobj_free(dest);
    return FIOBJ_INVALID;
  }
  return dest;
}

#endif /* FIO_EXTERN_COMPLETE */
#endif /* H___FIO_HTTP_FIOBJ___H */
