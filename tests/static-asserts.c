/* *****************************************************************************
Test - layout assertions shared across modules

Struct layout requirements that can't be compile-time checks without
`offsetof` (`FIO_PTR_FIELD_OFFSET` isn't an integer constant expression).
***************************************************************************** */
#include "test-helpers.h"

#define FIO_GFM
#define FIO_HTTP
#define FIO_IPC
#include FIO_INCLUDE_FILE

#define FIO___TEST_OFFSET(T_type, field)                                       \
  ((size_t)FIO_PTR_FIELD_OFFSET(T_type, field))

/* HTTP / JSON callback tables: the HTTP table is passed to the JSON parser
 * through a pointer cast; its head MUST keep the JSON table layout. */
#define FIO___TEST_HTTP_JSON_CB(field)                                         \
  FIO_ASSERT(FIO___TEST_OFFSET(fio_http_params_parse_callbacks_s, field) ==    \
                 FIO___TEST_OFFSET(fio_json_parser_callbacks_s, field),        \
             "HTTP / JSON callback offset mismatch: " #field)

FIO_SFUNC void fio___test_http_layout(void) {
  /* 430 accessors rely on the fio_http_s pointer prefix. */
  FIO_ASSERT(FIO___TEST_OFFSET(fio_http_s, udata) == 0 &&
                 FIO___TEST_OFFSET(fio_http_s, udata2) == sizeof(void *) &&
                 FIO___TEST_OFFSET(fio_http_s, cdata) == 2 * sizeof(void *) &&
                 FIO___TEST_OFFSET(fio_http_s, controller) ==
                     3 * sizeof(void *) &&
                 sizeof(fio_http_controller_s *) == sizeof(void *),
             "fio_http_s pointer prefix layout changed (430 accessors)");
  FIO___TEST_HTTP_JSON_CB(on_null);
  FIO___TEST_HTTP_JSON_CB(on_true);
  FIO___TEST_HTTP_JSON_CB(on_false);
  FIO___TEST_HTTP_JSON_CB(on_number);
  FIO___TEST_HTTP_JSON_CB(on_float);
  FIO___TEST_HTTP_JSON_CB(on_string);
  FIO___TEST_HTTP_JSON_CB(on_string_simple);
  FIO___TEST_HTTP_JSON_CB(on_map);
  FIO___TEST_HTTP_JSON_CB(on_array);
  FIO___TEST_HTTP_JSON_CB(map_push);
  FIO___TEST_HTTP_JSON_CB(array_push);
  FIO___TEST_HTTP_JSON_CB(is_array);
  FIO___TEST_HTTP_JSON_CB(is_map);
  FIO___TEST_HTTP_JSON_CB(free_unused_object);
  FIO_ASSERT(
      FIO___TEST_OFFSET(fio_http_params_parse_callbacks_s, on_file) ==
          FIO___TEST_OFFSET(fio_json_parser_callbacks_s, free_unused_object) +
              sizeof(void *),
      "HTTP / JSON callback head size mismatch");
}

FIO_SFUNC void fio___test_ipc_layout(void) {
  /* the per-message nonce reads `timestamp || id || nonce` as one block */
  FIO_ASSERT(FIO___TEST_OFFSET(fio_ipc_s, id) ==
                     FIO___TEST_OFFSET(fio_ipc_s, timestamp) + 8 &&
                 FIO___TEST_OFFSET(fio_ipc_s, nonce) ==
                     FIO___TEST_OFFSET(fio_ipc_s, id) + 8,
             "IPC nonce fields must be contiguous");
}

FIO_SFUNC void fio___test_gfm_layout(void) {
  /* cache line 0 = 8 pointers; cache line 1 starts with the control fields */
  FIO_ASSERT(FIO___TEST_OFFSET(fio___gfm_parser_s, err) == 8 * sizeof(void *),
             "fio___gfm_parser_s: mutable control must start at byte 64 "
             "(64 bit)");
}

int main(void) {
  fio___test_http_layout();
  fio___test_ipc_layout();
  fio___test_gfm_layout();
  fprintf(stderr, "* static (layout) assertions passed\n");
  return 0;
}
