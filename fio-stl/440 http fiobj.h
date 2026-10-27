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
#if !defined(FIO___RECURSIVE_INCLUDE) && defined(H___FIO_HTTP_API___H) &&      \
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

/* Fail closed: a native NULL `obj` means "nothing" to the parsers, so a
 * failed constructor must report an error rather than silently drop data. */
#define FIO___HTTP_FIOBJ_ERR    FIO_HTTP_PARAMS_PARSE_ERROR
#define FIO___HTTP_FIOBJ_OBJ(o) ((fio_json_obj_result_s){.obj = (void *)(o)})
#define FIO___HTTP_FIOBJ_NEW(o)                                                \
  ((o) ? FIO___HTTP_FIOBJ_OBJ(o)                                               \
       : (fio_json_obj_result_s){.err = FIO___HTTP_FIOBJ_ERR})
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
FIO_SFUNC fio_json_obj_result_s fio___http_fiobj_on_float(void *udata,
                                                          double num) {
  (void)udata;
  FIOBJ o = fiobj_float_new(num);
  return FIO___HTTP_FIOBJ_NEW(o);
}
FIO_SFUNC fio_json_obj_result_s fio___http_fiobj_on_number(void *udata,
                                                           int64_t num) {
#if INTPTR_MAX < INT64_MAX
  /* FIOBJ Numbers hold an `intptr_t`: never narrow a peer's integer. Wider
   * values become Floats, as integers past the int64_t range already do. */
  if (num > (int64_t)INTPTR_MAX || num < (int64_t)INTPTR_MIN)
    return fio___http_fiobj_on_float(udata, (double)num);
#endif
  (void)udata;
  FIOBJ o = fiobj_num_new((intptr_t)num);
  return FIO___HTTP_FIOBJ_NEW(o);
}
FIO_SFUNC fio_json_obj_result_s fio___http_fiobj_on_string(void *udata,
                                                           fio_buf_info_s s) {
  (void)udata;
  FIOBJ str = fiobj_str_new();
  if (!str)
    return (fio_json_obj_result_s){.err = FIO___HTTP_FIOBJ_ERR};
  /* Unescaping never grows the text: one up-front reservation means the
   * write can't fail midway and publish a truncated value. */
  if (s.len && (fiobj_str_reserve(str, s.len).capa <= s.len ||
                !fiobj_str_write_unescape(str, s.buf, s.len).len)) {
    fiobj_free(str);
    return (fio_json_obj_result_s){.err = FIO___HTTP_FIOBJ_ERR};
  }
  return FIO___HTTP_FIOBJ_OBJ(str);
}
FIO_SFUNC fio_json_obj_result_s
fio___http_fiobj_on_string_simple(void *udata, fio_buf_info_s s) {
  (void)udata;
  FIOBJ str = fiobj_str_new_cstr(s.buf, s.len); /* NULL on any failure */
  return FIO___HTTP_FIOBJ_NEW(str);
}
/* Stores `value` at `map[key]`, consuming it on every path; a replaced value
 * is handed back through `old`. Returns 0 on success. */
FIO_SFUNC int fio___http_fiobj_hash_set(FIOBJ map,
                                        fio_buf_info_s key,
                                        FIOBJ value,
                                        FIOBJ *old) {
  FIOBJ name = fiobj_str_new_cstr(key.buf, key.len);
  if (!name) {
    fiobj_free(value);
    return FIO___HTTP_FIOBJ_ERR;
  }
  /* the map duplicates the key and discards `value` if insertion fails */
  FIOBJ stored = fiobj_hash_set(map, name, value, old);
  fiobj_free(name);
  return stored == value ? 0 : FIO___HTTP_FIOBJ_ERR;
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
  FIOBJ replaced = FIOBJ_INVALID;
  if (!o)
    goto failed;
  /* Insertion consumes `o` even on failure: never publish it unless stored. */
  if (parent) {
    if (key.buf) {
      if (fio___http_fiobj_hash_set((FIOBJ)parent, key, o, &replaced))
        goto failed;
    } else if (!fiobj_array_push((FIOBJ)parent, o)) {
      goto failed;
    }
  }
  r.obj = (void *)o; /* a new, owned root when `parent` is NULL */
  r.old = (void *)replaced;
  return r;
failed:
  r.err = FIO___HTTP_FIOBJ_ERR;
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
  (void)udata; /* push destroys `value` on failure */
  return fiobj_array_push((FIOBJ)a, (FIOBJ)value) ? 0 : FIO___HTTP_FIOBJ_ERR;
}
/* Stores `value`, handing a replaced value back for conflict resolution. */
FIO_SFUNC fio_json_obj_result_s fio___http_fiobj_map_push(void *udata,
                                                          void *map,
                                                          fio_buf_info_s key,
                                                          void *value) {
  (void)udata;
  FIOBJ replaced = FIOBJ_INVALID;
  if (fio___http_fiobj_hash_set((FIOBJ)map, key, (FIOBJ)value, &replaced))
    return (fio_json_obj_result_s){.err = FIO___HTTP_FIOBJ_ERR};
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
#undef FIO___HTTP_FIOBJ_NEW
#undef FIO___HTTP_FIOBJ_OBJ
/* Copies a complete upload into a data object (spilled to a temporary file
 * past its RAM limit). */
FIO_SFUNC fio_json_obj_result_s
fio___http_fiobj_on_file(void *udata,
                         fio_str_info_s name,
                         fio_str_info_s filename,
                         fio_str_info_s type,
                         fio_str_info_s headers,
                         fio_buf_info_s data) {
  (void)udata, (void)name;
  FIOBJ file = fiobj_data_new(0);
  fio_data_s *d = fiobj_data_ptr(file);
  if (!d)
    return (fio_json_obj_result_s){.err = FIO___HTTP_FIOBJ_ERR};
  /* A setter leaves its property unset on OOM (a NULL `buf` sets nothing). */
  fiobj_data_filename_set(file, filename);
  fiobj_data_type_set(file, type);
  fiobj_data_headers_set(file, headers);
  if ((filename.buf && !fiobj_data_filename(file).buf) ||
      (type.buf && !fiobj_data_type(file).buf) ||
      (headers.buf && !fiobj_data_headers(file).buf))
    goto failed;
  fio_data_expect(d, data.len);
  if (data.len && fio_data_write(d, data.buf, data.len))
    goto failed;
  return (fio_json_obj_result_s){.obj = (void *)file};
failed:
  fiobj_free(file);
  return (fio_json_obj_result_s){.err = FIO___HTTP_FIOBJ_ERR};
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

#undef FIO___HTTP_FIOBJ_ERR
#endif /* FIO_EXTERN_COMPLETE */
#endif /* H___FIO_HTTP_FIOBJ___H */
