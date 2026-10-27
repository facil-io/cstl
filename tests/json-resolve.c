/* Standalone tests for the public JSON conflict helpers and parser callbacks. */
#include "test-helpers.h"
#define FIO_JSON
#define FIO_LOG
#include FIO_INCLUDE_FILE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum value_kind_e { V_STRING, V_NUMBER, V_NULL, V_UPLOAD, V_MAP, V_ARRAY };
struct value_s {
  enum value_kind_e kind;
  char *text;
  struct value_s **items;
  char **keys;
  size_t count, capacity;
};
static struct value_s *value_new(enum value_kind_e kind, const char *text) {
  struct value_s *v = calloc(1, sizeof(*v));
  FIO_ASSERT(v, "allocation failed");
  v->kind = kind;
  if (text) {
    v->text = malloc(strlen(text) + 1);
    FIO_ASSERT(v->text, "allocation failed");
    strcpy(v->text, text);
  }
  return v;
}
static void value_free(struct value_s *v) {
  if (!v) return;
  for (size_t i = 0; i < v->count; ++i) {
    value_free(v->items[i]);
    if (v->keys) free(v->keys[i]);
  }
  free(v->keys);
  free(v->items);
  free(v->text);
  free(v);
}
static void value_grow(struct value_s *v) {
  if (v->count < v->capacity) return;
  size_t cap = v->capacity ? v->capacity * 2 : 4;
  struct value_s **items = realloc(v->items, cap * sizeof(*items));
  FIO_ASSERT(items, "allocation failed");
  v->items = items;
  if (v->kind == V_MAP) {
    char **keys = realloc(v->keys, cap * sizeof(*keys));
    FIO_ASSERT(keys, "allocation failed");
    v->keys = keys;
  }
  v->capacity = cap;
}
static size_t value_find(struct value_s *map, fio_buf_info_s key) {
  FIO_ASSERT(map && map->kind == V_MAP && key.buf, "invalid map lookup");
  for (size_t i = 0; i < map->count; ++i)
    if (strlen(map->keys[i]) == key.len &&
        !memcmp(map->keys[i], key.buf, key.len)) return i;
  return SIZE_MAX;
}
static int merge_mode;
static int no_reuse;
static int conflict_error_code;
/* Takes `value` even on error; returns a replaced value unless merging. */
static fio_json_obj_result_s cb_map_push(void *udata, void *ctx,
                                         fio_buf_info_s key, void *value) {
  (void)udata;
  fio_json_obj_result_s r = {0};
  if (conflict_error_code && !key.len) {
    value_free(value);
    r.err = conflict_error_code;
    return r;
  }
  struct value_s *map = ctx;
  size_t i = value_find(map, key);
  if (i != SIZE_MAX) {
    if (merge_mode) value_free(map->items[i]);
    else r.obj = map->items[i];
    map->items[i] = value;
    return r;
  }
  value_grow(map);
  char *copy = malloc(key.len + 1);
  FIO_ASSERT(copy, "allocation failed");
  memcpy(copy, key.buf, key.len);
  copy[key.len] = 0;
  map->keys[map->count] = copy;
  map->items[map->count++] = value;
  return r;
}
static int cb_array_push(void *udata, void *ctx, void *value) {
  (void)udata;
  if (conflict_error_code) {
    value_free(value);
    return conflict_error_code;
  }
  struct value_s *arr = ctx;
  FIO_ASSERT(arr && arr->kind == V_ARRAY, "invalid array push");
  value_grow(arr);
  arr->items[arr->count++] = value;
  return 0;
}
/* Reuses a same-kind child (merge / append); hands a replaced value back. */
static fio_json_push_result_s cb_container(void *udata, void *ctx,
                                           fio_buf_info_s key,
                                           enum value_kind_e kind) {
  fio_json_push_result_s r = {0};
  struct value_s *parent = ctx;
  if (parent && key.buf) {
    size_t i = value_find(parent, key);
    if (!no_reuse && i != SIZE_MAX && parent->items[i] &&
        parent->items[i]->kind == kind) {
      r.obj = parent->items[i];
      return r;
    }
  }
  struct value_s *v = value_new(kind, NULL);
  if (parent && key.buf) {
    fio_json_obj_result_s p = cb_map_push(udata, parent, key, v);
    r.err = p.err;
    r.old = p.obj;
  } else if (parent) {
    r.err = cb_array_push(udata, parent, v);
  }
  r.obj = v;
  return r;
}
static fio_json_push_result_s cb_on_map(void *u, void *c, fio_buf_info_s k) {
  return cb_container(u, c, k, V_MAP);
}
static fio_json_push_result_s cb_on_array(void *u, void *c, fio_buf_info_s k) {
  return cb_container(u, c, k, V_ARRAY);
}
#define OBJ(v) ((fio_json_obj_result_s){.obj = (v)})
static fio_json_obj_result_s cb_null(void *u) { (void)u; return OBJ(value_new(V_NULL, NULL)); }
static fio_json_obj_result_s cb_true(void *u) { (void)u; return OBJ(value_new(V_STRING, "true")); }
static fio_json_obj_result_s cb_false(void *u) { (void)u; return OBJ(value_new(V_STRING, "false")); }
static fio_json_obj_result_s cb_float(void *u, double n) {
  (void)u;
  char buf[64];
  snprintf(buf, sizeof(buf), "%.17g", n);
  return OBJ(value_new(V_NUMBER, buf));
}
static int number_error_code;
static fio_json_obj_result_s cb_number(void *u, int64_t n) {
  (void)u;
  if (number_error_code)
    return (fio_json_obj_result_s){.err = number_error_code};
  char buf[32];
  snprintf(buf, sizeof(buf), "%lld", (long long)n);
  return OBJ(value_new(V_NUMBER, buf));
}
static fio_json_obj_result_s cb_string_simple(void *u, fio_buf_info_s s) {
  (void)u;
  char *buf = malloc(s.len + 1);
  FIO_ASSERT(buf, "allocation failed");
  if (s.len) memcpy(buf, s.buf, s.len);
  buf[s.len] = 0;
  struct value_s *v = value_new(V_STRING, buf);
  free(buf);
  return OBJ(v);
}
static fio_json_obj_result_s cb_string(void *u, fio_buf_info_s s) {
  char *buf = malloc(s.len + 1);
  FIO_ASSERT(buf, "allocation failed");
  fio_str_info_s out = FIO_STR_INFO3(buf, 0, s.len + 1);
  fio_string_write_unescape(&out, NULL, s.buf, s.len);
  fio_json_obj_result_s v = cb_string_simple(u, FIO_BUF_INFO2(out.buf, out.len));
  free(buf);
  return v;
}
static void cb_free(void *u, void *v) { (void)u; value_free(v); }
static int cb_is_map(void *u, void *v) {
  (void)u; return v && ((struct value_s *)v)->kind == V_MAP;
}
static int cb_is_array(void *u, void *v) {
  (void)u; return v && ((struct value_s *)v)->kind == V_ARRAY;
}
static const fio_json_parser_callbacks_s callbacks = {
  .on_null = cb_null, .on_true = cb_true, .on_false = cb_false,
  .on_number = cb_number, .on_float = cb_float,
  .on_string = cb_string, .on_string_simple = cb_string_simple,
  .on_map = cb_on_map, .on_array = cb_on_array,
  .map_push = cb_map_push, .array_push = cb_array_push,
  .is_map = cb_is_map, .is_array = cb_is_array,
  .free_unused_object = cb_free,
};
#define KEY(s) ((fio_buf_info_s){.buf = (char *)(s), .len = sizeof(s) - 1})
#define NO_KEY ((fio_buf_info_s){0})
static struct value_s *root(void) {
  fio_json_obj_result_s r = fio_json_resolve_on_map(&callbacks, NULL, NULL, NO_KEY);
  struct value_s *v = r.obj;
  FIO_ASSERT(!r.err && v && v->kind == V_MAP, "root creation failed");
  return v;
}
static void put(struct value_s *m, fio_buf_info_s key, struct value_s *v) {
  FIO_ASSERT(!fio_json_resolve_map_push(&callbacks, NULL, m, key, v),
             "resolve_map_push failed");
}
static struct value_s *open_map(struct value_s *m, fio_buf_info_s k) {
  fio_json_obj_result_s r = fio_json_resolve_on_map(&callbacks, NULL, m, k);
  struct value_s *v = r.obj;
  FIO_ASSERT(!r.err && v && v->kind == V_MAP, "resolve_on_map failed");
  return v;
}
static struct value_s *open_array(struct value_s *m, fio_buf_info_s k) {
  fio_json_obj_result_s r = fio_json_resolve_on_array(&callbacks, NULL, m, k);
  struct value_s *v = r.obj;
  FIO_ASSERT(!r.err && v && v->kind == V_ARRAY, "resolve_on_array failed");
  return v;
}
#define STR(x) value_new(V_STRING, x)
#define NUM(x) value_new(V_NUMBER, x)
#define UP(x) value_new(V_UPLOAD, x)
#define PUSH(a,v) FIO_ASSERT(!callbacks.array_push(NULL, (a), (v)), "array push failed")

/* Compare entire ordered tree, including type, keys, lengths and values. */
static int equal_value(const struct value_s *a, const struct value_s *b) {
  if (!a || !b) return a == b;
  if (a->kind != b->kind || a->count != b->count) return 0;
  if (!!a->text != !!b->text || (a->text && strcmp(a->text, b->text))) return 0;
  for (size_t i = 0; i < a->count; ++i) {
    if (a->kind == V_MAP && strcmp(a->keys[i], b->keys[i])) return 0;
    if (!equal_value(a->items[i], b->items[i])) return 0;
  }
  return 1;
}
/* Independent expected-tree constructors: no resolver or callback invocation. */
static struct value_s *expected_array(struct value_s *a, struct value_s *b,
                                       struct value_s *c) {
  struct value_s *v = value_new(V_ARRAY, NULL);
  if (a) PUSH(v, a);
  if (b) PUSH(v, b);
  if (c) PUSH(v, c);
  return v;
}
static struct value_s *expected_map(const char *k1, struct value_s *v1,
                                     const char *k2, struct value_s *v2) {
  struct value_s *v = value_new(V_MAP, NULL);
  if (k1) cb_map_push(NULL, v, (fio_buf_info_s){.buf=(char *)k1,.len=strlen(k1)}, v1);
  if (k2) cb_map_push(NULL, v, (fio_buf_info_s){.buf=(char *)k2,.len=strlen(k2)}, v2);
  return v;
}
static void check(const char *name, struct value_s *actual, struct value_s *want) {
  int ok = equal_value(actual, want);
  value_free(want);
  value_free(actual);
  FIO_ASSERT(ok, "conflict case failed: %s", name);
}
#define EM(k,v) expected_map(k,v,NULL,NULL)

static void test_helpers(void) {
  struct value_s *r, *a, *h;
  merge_mode = 0;
  /* Rule 1: empty slot, each kind. */
  r = root(); put(r, KEY("a"), NUM("1"));
  check("rule 1 scalar", r, EM("a", NUM("1")));
  r = root(); open_map(r, KEY("a"));
  check("rule 1 map", r, EM("a", expected_map(NULL,NULL,NULL,NULL)));
  r = root(); open_array(r, KEY("a"));
  check("rule 1 array", r, EM("a", expected_array(NULL,NULL,NULL)));
  /* Rule 2, merge existing map. */
  r = root(); h = open_map(r, KEY("a")); put(h, KEY("x"), NUM("1"));
  FIO_ASSERT(open_map(r, KEY("a")) == h, "rule 2 did not reuse map");
  put(h, KEY("y"), NUM("2"));
  check("rule 2 map + map", r, EM("a", expected_map("x",NUM("1"),"y",NUM("2"))));
  /* Rule 5, reuse and append existing array. */
  r = root(); a = open_array(r, KEY("a")); PUSH(a, NUM("1"));
  FIO_ASSERT(open_array(r, KEY("a")) == a, "rule 5 did not reuse array");
  PUSH(a, NUM("2"));
  check("rule 5 array + array", r, EM("a", expected_array(NUM("1"),NUM("2"),NULL)));
  /* Rule 3: map over scalar, array, upload. */
  r = root(); put(r, KEY("a"), STR("x")); h = open_map(r, KEY("a"));
  put(h, KEY("name"), STR("A"));
  check("rule 3 scalar -> map", r, EM("a",expected_map("",STR("x"),"name",STR("A"))));
  r = root(); a = open_array(r, KEY("a")); PUSH(a, STR("x"));
  h = open_map(r, KEY("a")); put(h, KEY("name"), STR("A"));
  check("rule 3 array -> map", r, EM("a",expected_map("",expected_array(STR("x"),NULL,NULL),"name",STR("A"))));
  r = root(); put(r, KEY("a"), UP("file")); open_map(r, KEY("a"));
  check("rule 3 upload -> map", r, EM("a",EM("",UP("file"))));
  /* Rule 7: scalar/upload -> array, preserving incoming append order. */
  r = root(); put(r, KEY("a"), STR("x")); a = open_array(r, KEY("a")); PUSH(a, STR("y"));
  check("rule 7 scalar -> array", r, EM("a",expected_array(STR("x"),STR("y"),NULL)));
  r = root(); put(r, KEY("a"), UP("file")); a = open_array(r, KEY("a")); PUSH(a, STR("y"));
  check("rule 7 upload -> array", r, EM("a",expected_array(UP("file"),STR("y"),NULL)));
  /* Rule 4: maps retain named keys; conflict is retried at empty key. */
  r = root(); h = open_map(r, KEY("a")); put(h, KEY("name"), STR("A"));
  a = open_array(r, KEY("a")); PUSH(a, STR("x"));
  check("rule 4 map -> array", r, EM("a",expected_map("name",STR("A"),"",expected_array(STR("x"),NULL,NULL))));
  r = root(); h = open_map(r, KEY("a")); put(h, KEY("name"), STR("A"));
  put(r, KEY("a"), STR("x"));
  check("rule 4 map -> scalar", r, EM("a",expected_map("name",STR("A"),"",STR("x"))));
  r = root(); h = open_map(r, KEY("a")); put(h, KEY(""), STR("first"));
  put(h, KEY("name"), STR("A")); put(r, KEY("a"), STR("second"));
  check("rule 4 existing empty + scalar", r, EM("a",expected_map("",expected_array(STR("first"),STR("second"),NULL),"name",STR("A"))));
  r = root(); h = open_map(r, KEY("a")); put(h, KEY(""), STR("first"));
  a = open_array(r, KEY("a")); PUSH(a, STR("second"));
  check("rule 4 existing empty + array", r, EM("a",EM("",expected_array(STR("first"),STR("second"),NULL))));
  /* Rule 6 and rule 8 (including the third scalar). */
  r = root(); a = open_array(r, KEY("a")); PUSH(a, STR("first"));
  put(r, KEY("a"), STR("second"));
  check("rule 6 array + scalar", r, EM("a",expected_array(STR("first"),STR("second"),NULL)));
  r = root(); put(r, KEY("a"), NUM("1")); put(r, KEY("a"), NUM("2"));
  check("rule 8 scalar + scalar", r, EM("a",expected_array(NUM("1"),NUM("2"),NULL)));
  r = root(); put(r, KEY("a"), UP("file")); put(r, KEY("a"), STR("x"));
  check("rule 8 upload + scalar", r, EM("a",expected_array(UP("file"),STR("x"),NULL)));
  r = root(); put(r, KEY("a"), NUM("1")); put(r, KEY("a"), NUM("2")); put(r, KEY("a"), NUM("3"));
  check("rule 8 third scalar", r, EM("a",expected_array(NUM("1"),NUM("2"),NUM("3"))));
  /* Section 5 path examples, using explicit segments rather than JSON keys. */
  r = root(); put(r, KEY("a"), STR("1")); put(r, KEY("a"), STR("2"));
  check("a=1&a=2", r, EM("a",expected_array(STR("1"),STR("2"),NULL)));
  r = root(); put(r, KEY("user"), STR("x")); h = open_map(r, KEY("user"));
  put(h, KEY("name"), STR("A"));
  check("user=x&user.name=A", r, EM("user",expected_map("",STR("x"),"name",STR("A"))));
  r = root(); h = open_map(r, KEY("user")); put(h, KEY("name"), STR("A"));
  put(r, KEY("user"), STR("x"));
  check("user.name=A&user=x", r, EM("user",expected_map("name",STR("A"),"",STR("x"))));
  r = root(); put(r, KEY("tag"), STR("x")); a = open_array(r, KEY("tag")); PUSH(a, STR("a"));
  check("tag=x&tag[]=a", r, EM("tag",expected_array(STR("x"),STR("a"),NULL)));
  /* Explicit no-key container append and null are also ownership checks. */
  r = root(); a = open_array(r, KEY("a")); h = open_map(a, NO_KEY);
  put(h, KEY("x"), value_new(V_NULL,NULL));
  check("array element map with null", r, EM("a",expected_array(EM("x",value_new(V_NULL,NULL)),NULL,NULL)));
}
static void test_parse(void) {
  const char *input[] = {
    "{\"a\":1,\"a\":2}",
    "{\"a\":{\"x\":1},\"a\":2}",
    "{\"a\":[1],\"a\":[2]}",
    "{\"a\":1,\"a\":{\"x\":2}}",
    "{\"a\":[{\"x\":1},{\"y\":[{\"z\":2}]}]}",
  };
  for (int mode = 0; mode < 2; ++mode) {
    merge_mode = mode;
    for (size_t i = 0; i < sizeof(input)/sizeof(*input); ++i) {
      fio_json_result_s result = fio_json_parse(&callbacks, NULL, NULL, input[i], strlen(input[i]));
      FIO_ASSERT(!result.err && result.ctx, "JSON parse failed: mode %d: %s", mode, input[i]);
      struct value_s *expected = NULL;
      switch (i) {
      case 0: expected = EM("a", mode ? NUM("2") : expected_array(NUM("1"),NUM("2"),NULL)); break;
      case 1: expected = EM("a", mode ? NUM("2") : expected_map("x",NUM("1"),"",NUM("2"))); break;
      case 2: expected = EM("a", expected_array(NUM("1"),NUM("2"),NULL)); break;
      case 3: expected = EM("a", mode ? EM("x",NUM("2")) : expected_map("",NUM("1"),"x",NUM("2"))); break;
      case 4: expected = EM("a",expected_array(EM("x",NUM("1")),EM("y",expected_array(EM("z",NUM("2")),NULL,NULL)),NULL)); break;
      }
      check(input[i], result.ctx, expected);
    }
  }
  merge_mode = 0;
}
static void test_root_reuse(void) {
  merge_mode = 0;
  conflict_error_code = 0;
  struct value_s *map =
      expected_map("items", expected_array(NUM("1"), NULL, NULL),
                   "keep", STR("yes"));
  const char *json = "{\"added\":2,\"items\":[3]}";
  fio_json_result_s r =
      fio_json_parse(&callbacks, NULL, map, json, strlen(json));
  FIO_ASSERT(!r.err && r.ctx == map && map->count == 3,
             "an explicit map root must be updated in place");
  size_t at = value_find(map, KEY("items"));
  FIO_ASSERT(at != SIZE_MAX && map->items[at]->count == 2 &&
                 !strcmp(map->items[at]->items[1]->text, "3"),
             "root update must append into an existing array");
  at = value_find(map, KEY("added"));
  FIO_ASSERT(at != SIZE_MAX && !strcmp(map->items[at]->text, "2"),
             "root update must merge new map values");

  /* Type mismatch (including a scalar root) changes nothing. */
  const char *mismatch[] = {"[1]", "42", "\"s\"", "null"};
  for (size_t i = 0; i < sizeof(mismatch) / sizeof(*mismatch); ++i) {
    r = fio_json_parse(&callbacks, NULL, map, mismatch[i], strlen(mismatch[i]));
    FIO_ASSERT(r.err == FIO_JSON_ERROR_GENERIC && r.ctx == map &&
                   map->count == 3,
               "root type mismatch must fail without changes: %s",
               mismatch[i]);
  }
  /* Without is_map / is_array, an explicit root never matches. */
  fio_json_parser_callbacks_s no_types = callbacks;
  no_types.is_map = NULL;
  no_types.is_array = NULL;
  r = fio_json_parse(&no_types, NULL, map, "{\"z\":1}", 7);
  FIO_ASSERT(r.err == FIO_JSON_ERROR_GENERIC && r.ctx == map &&
                 map->count == 3,
             "explicit root requires is_map");
  value_free(map);

  struct value_s *array = expected_array(NUM("1"), NULL, NULL);
  r = fio_json_parse(&callbacks, NULL, array, "[2]", 3);
  FIO_ASSERT(!r.err && r.ctx == array && array->count == 2 &&
                 !strcmp(array->items[1]->text, "2"),
             "an explicit array root must be appended in place");
  r = fio_json_parse(&callbacks, NULL, array, "{}", 2);
  FIO_ASSERT(r.err == FIO_JSON_ERROR_GENERIC && r.ctx == array &&
                 array->count == 2,
             "array root mismatch must fail without changes");
  value_free(array);
}

/* Callback errors pass through unchanged; callees own values on failure
 * (ASan / leak checks verify nothing is freed twice or leaked). */
static void test_callback_error_propagation(void) {
  const int errors[] = {-81, -1, -2, 39};
  const char *map_json = "{\"a\":1,\"a\":{\"x\":2}}";
  const char *array_json = "{\"a\":1,\"a\":[2]}";
  merge_mode = 0;
  for (size_t i = 0; i < sizeof(errors) / sizeof(*errors); ++i) {
    conflict_error_code = errors[i];
    fio_json_result_s r =
        fio_json_parse(&callbacks, NULL, NULL, map_json, strlen(map_json));
    FIO_ASSERT(r.err == errors[i] && r.ctx,
               "map resolver changed callback error: %d != %d",
               r.err,
               errors[i]);
    value_free(r.ctx); /* the caller owns the partial root */
    r = fio_json_parse(&callbacks, NULL, NULL, array_json, strlen(array_json));
    FIO_ASSERT(r.err == errors[i] && r.ctx,
               "array resolver changed callback error: %d != %d",
               r.err,
               errors[i]);
    value_free(r.ctx);
    r = fio_json_parse(&callbacks, NULL, NULL, "[1,2]", 5);
    FIO_ASSERT(r.err == errors[i] && r.ctx,
               "array_push error changed: %d != %d",
               r.err,
               errors[i]);
    value_free(r.ctx);
    struct value_s *map = root();
    int err = fio_json_resolve_map_push(&callbacks,
                                        NULL,
                                        map,
                                        NO_KEY,
                                        NUM("9"));
    FIO_ASSERT(err == errors[i],
               "public map resolver changed callback error: %d != %d",
               err,
               errors[i]);
    value_free(map);
  }
  conflict_error_code = 0;
  /* value callback errors */
  for (size_t i = 0; i < sizeof(errors) / sizeof(*errors); ++i) {
    number_error_code = errors[i];
    fio_json_result_s r =
        fio_json_parse(&callbacks, NULL, NULL, "{\"a\":\"s\",\"b\":1}", 15);
    FIO_ASSERT(r.err == errors[i] && r.ctx &&
                   ((struct value_s *)r.ctx)->count == 1,
               "value callback error changed: %d != %d",
               r.err,
               errors[i]);
    value_free(r.ctx);
    r = fio_json_parse(&callbacks, NULL, NULL, "7", 1);
    FIO_ASSERT(r.err == errors[i] && !r.ctx,
               "scalar root callback error changed: %d != %d",
               r.err,
               errors[i]);
  }
  number_error_code = 0;
}

static fio_json_obj_result_s cb_null_ptr(void *u) {
  (void)u;
  return (fio_json_obj_result_s){0};
}
/* A native NULL is "nothing": stored as-is, never resolved as a conflict. */
static void test_null_values(void) {
  fio_json_parser_callbacks_s cb = callbacks;
  cb.on_null = cb_null_ptr;
  const char *arr = "[null,1,null,]";
  fio_json_result_s r = fio_json_parse(&cb, NULL, NULL, arr, strlen(arr));
  struct value_s *v = r.ctx;
  FIO_ASSERT(!r.err && v && v->kind == V_ARRAY && v->count == 3 &&
                 !v->items[0] && v->items[1] && !v->items[2],
             "NULL array elements must be kept (trailing comma skipped)");
  value_free(v);
  const char *map = "{\"a\":null,\"b\":}";
  r = fio_json_parse(&cb, NULL, NULL, map, strlen(map));
  v = r.ctx;
  FIO_ASSERT(!r.err && v && v->kind == V_MAP && v->count == 2 &&
                 !v->items[0] && !v->items[1],
             "NULL map values must be stored");
  value_free(v);
  /* replacing a NULL value is not a conflict; a dedicated null object is */
  const char *dup = "{\"a\":null,\"a\":1}";
  r = fio_json_parse(&cb, NULL, NULL, dup, strlen(dup));
  v = r.ctx;
  FIO_ASSERT(!r.err && v && v->count == 1 && v->items[0] &&
                 v->items[0]->kind == V_NUMBER,
             "a replaced native NULL must not trigger conflict resolution");
  value_free(v);
  r = fio_json_parse(&callbacks, NULL, NULL, dup, strlen(dup));
  v = r.ctx;
  FIO_ASSERT(!r.err && v && v->count == 1 && v->items[0] &&
                 v->items[0]->kind == V_ARRAY && v->items[0]->count == 2 &&
                 v->items[0]->items[0]->kind == V_NULL,
             "a dedicated null object must be resolved as a conflict");
  value_free(v);
}

/* validate is a pure check; the parser fills string defaults in its copy. */
static void test_validate(void) {
  fio_json_parser_callbacks_s cb = callbacks;
  FIO_ASSERT(!fio_json_callbacks_validate(&cb), "full table must validate");
  cb.on_string = NULL;
  FIO_ASSERT(!fio_json_callbacks_validate(&cb), "one string callback is enough");
  fio_json_result_s r = fio_json_parse(&cb, NULL, NULL, "[\"a\",1]", 7);
  FIO_ASSERT(!r.err && r.ctx && !cb.on_string && !cb.is_map == !callbacks.is_map,
             "parse must not write to the callback table");
  value_free(r.ctx);
  cb.on_string = cb_string;
  cb.on_string_simple = NULL;
  r = fio_json_parse(&cb, NULL, NULL, "\"a\"", 3);
  FIO_ASSERT(!r.err && r.ctx && !cb.on_string_simple,
             "on_string must back a missing on_string_simple");
  value_free(r.ctx);
  cb.on_string = NULL;
  FIO_ASSERT(fio_json_callbacks_validate(&cb) == -1,
             "a missing string callback must fail validation");
  FIO_ASSERT(fio_json_callbacks_validate(NULL) == -1, "NULL must fail");
  struct value_s *map = root();
  r = fio_json_parse(&cb, NULL, map, "{}", 2);
  FIO_ASSERT(r.err == FIO_JSON_ERROR_GENERIC && r.ctx == map,
             "invalid callbacks must fail and return the root");
  value_free(map);
}

/* Incomplete escapes must never advance the reported cursor past input. */
static void test_truncated_escapes(void) {
  static const char *const inputs[] = {"\"\\", "{\"a\\", "{a\\"};
  const int saved_level = FIO_LOG_LEVEL;
  FIO_LOG_LEVEL = FIO_LOG_LEVEL_DEBUG;
  for (size_t i = 0; i < sizeof(inputs) / sizeof(*inputs); ++i) {
    size_t len = strlen(inputs[i]);
    char *buf = malloc(len);
    FIO_ASSERT(buf, "allocation failed");
    /* These nonnumeric scanners don't require the numeric guard byte. */
    memcpy(buf, inputs[i], len);
    fio_json_result_s r = fio_json_parse(&callbacks, NULL, NULL, buf, len);
    FIO_ASSERT(r.err && r.stop_pos <= len,
               "terminal escape cursor %zu exceeds %zu", r.stop_pos, len);
    value_free(r.ctx);
    free(buf);
  }
  FIO_LOG_LEVEL = saved_level;
  static const char *const infinity[] = {"i", "in", "inf", "infi", "infin", "infini", "infinit", "infinity"};
  for (size_t i = 0; i < sizeof(infinity) / sizeof(*infinity); ++i) {
    size_t len = strlen(infinity[i]);
    char *buf = malloc(len);
    FIO_ASSERT(buf, "allocation failed");
    memcpy(buf, infinity[i], len);
    fio_json_result_s r = fio_json_parse(&callbacks, NULL, NULL, buf, len);
    FIO_ASSERT(r.stop_pos <= len && (!!r.err == (len < 3)),
               "short infinity parsing failed");
    value_free(r.ctx);
    free(buf);
  }
  fio_json_result_s r = fio_json_parse(&callbacks, NULL, NULL, "\xEF\xBB\xBF", 3);
  FIO_ASSERT(!r.err && !r.ctx && r.stop_pos == 3, "BOM must be consumed");
}

static void test_conflict_depth(void) {
  for (size_t array = 0; array < 2; ++array) {
    size_t count = FIO_JSON_MAX_DEPTH + 32;
    char *json = calloc(count * 7 + 16, 1);
    FIO_ASSERT(json, "allocation failed");
    char *p = json;
    *p++ = '{';
    for (size_t i = 0; i < count; ++i) {
      memcpy(p, "\"a\":{},", 7);
      p += 7;
    }
    const char *last = array ? "\"a\":[]}" : "\"a\":0}";
    strcpy(p, last);
    no_reuse = 1;
    fio_json_result_s r = fio_json_parse(&callbacks, NULL, NULL, json, strlen(json));
    no_reuse = 0;
    FIO_ASSERT(r.err == FIO_JSON_ERROR_NESTING,
               "flat conflict chain must be bounded, got %d", r.err);
    value_free(r.ctx);
    free(json);
  }
}

static void run_all(void) {
  test_conflict_depth();
  test_truncated_escapes();
  test_helpers();
  test_parse();
  test_root_reuse();
  test_callback_error_propagation();
  test_null_values();
  test_validate();
}
#ifndef FIO_TEST_ALL
int main(void) {
  run_all();
  return 0;
}
#endif
#ifdef FIO_TEST_ALL
void fio_test_json_resolve(void) { run_all(); }
#endif
