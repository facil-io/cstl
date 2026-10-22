/* Standalone depth-1 JSON test: a per-file macro needs its own translation
 * unit. */
#define FIO_JSON_MAX_DEPTH 1
#include "test-helpers.h"
#define FIO_JSON
#include FIO_INCLUDE_FILE

static int depth_value;

static fio_json_obj_result_s depth_scalar(void *udata) {
  (void)udata;
  return (fio_json_obj_result_s){.obj = &depth_value};
}
static fio_json_obj_result_s depth_number(void *udata, int64_t value) {
  (void)value;
  return depth_scalar(udata);
}
static fio_json_obj_result_s depth_float(void *udata, double value) {
  (void)value;
  return depth_scalar(udata);
}
static fio_json_obj_result_s depth_string(void *udata, fio_buf_info_s str) {
  (void)str;
  return depth_scalar(udata);
}
static fio_json_push_result_s depth_container(void *udata,
                                              void *parent,
                                              fio_buf_info_s key) {
  (void)udata;
  (void)parent;
  (void)key;
  return (fio_json_push_result_s){.obj = &depth_value};
}
static fio_json_obj_result_s depth_push(void *udata,
                                        void *parent,
                                        fio_buf_info_s key,
                                        void *value) {
  (void)udata;
  (void)parent;
  (void)key;
  (void)value;
  return (fio_json_obj_result_s){0};
}
static int depth_array_push(void *udata, void *parent, void *value) {
  (void)udata;
  (void)parent;
  (void)value;
  return 0;
}
static void depth_free(void *udata, void *value) {
  (void)udata;
  (void)value;
}

int main(void) {
  static const fio_json_parser_callbacks_s cb = {
      .on_null = depth_scalar,
      .on_true = depth_scalar,
      .on_false = depth_scalar,
      .on_number = depth_number,
      .on_float = depth_float,
      .on_string = depth_string,
      .on_map = depth_container,
      .on_array = depth_container,
      .map_push = depth_push,
      .array_push = depth_array_push,
      .free_unused_object = depth_free,
  };
  static const struct {
    const char *text;
    int error;
  } cases[] = {{"{}", 0},
               {"[]", 0},
               {"{\"a\":1}", 0},
               {"42", 0},
               {"{\"a\":{}}", FIO_JSON_ERROR_NESTING},
               {"[[]]", FIO_JSON_ERROR_NESTING}};
  for (size_t i = 0; i < sizeof(cases) / sizeof(*cases); ++i) {
    fio_json_result_s result =
        fio_json_parse(&cb, NULL, NULL, cases[i].text, strlen(cases[i].text));
    FIO_ASSERT(result.err == cases[i].error,
               "depth-1 parse mismatch: %s (%d)",
               cases[i].text,
               result.err);
    if (!cases[i].error)
      FIO_ASSERT(result.ctx == &depth_value &&
                     result.stop_pos == strlen(cases[i].text),
                 "depth-1 parse incomplete: %s",
                 cases[i].text);
  }
  return 0;
}
