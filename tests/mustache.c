#define FIO_EVERYTHING
#define FIO_LOG
#include "fio-stl/include.h"

FIO_SFUNC void *mtest_get_var(void *ctx, fio_buf_info_s name) {
  if (!ctx)
    return NULL;
  if (!FIOBJ_TYPE_IS(ctx, FIOBJ_T_HASH))
    return NULL;
  return fiobj_hash_get3(ctx, name.buf, name.len);
}

FIO_SFUNC void *mtest_get_var_index(void *ctx, size_t index) {
  if (!FIOBJ_TYPE_IS(ctx, FIOBJ_T_ARRAY))
    return NULL;
  return fiobj_array_get(ctx, index);
}

FIO_SFUNC fio_buf_info_s mtest_val2str(void *var) {
  fio_buf_info_s r = {0};
  if (!var || var == fiobj_null())
    return r;
  fio_str_info_s tmp = fiobj2cstr(var);
  r = FIO_STR2BUF_INFO(tmp);
  return r;
}

FIO_SFUNC int mtest_var_is_truthful(void *v) {
  return v && v != fiobj_null() && v != fiobj_false() &&
         (!FIOBJ_TYPE_IS(v, FIOBJ_T_ARRAY) || fiobj_array_count(v));
}

static void mustache_json_run_test(FIOBJ test) {
  printf("Test name: %s\n"
         "Test desc: %s\n",
         fiobj2cstr(fiobj_hash_get3(test, "name", 4)).buf,
         fiobj2cstr(fiobj_hash_get3(test, "desc", 4)).buf);
  FIOBJ data = fiobj_hash_get3(test, "data", 4);
  fio_buf_info_s template_data =
      fiobj_str_buf(fiobj_hash_get3(test, "template", 8));
  fio_buf_info_s expected_data =
      fiobj_str_buf(fiobj_hash_get3(test, "expected", 8));
  FIO_ASSERT(template_data.buf, "template content missing");
  FIO_ASSERT(expected_data.buf, "expected result missing");
  if (!FIOBJ_IS_INVALID(fiobj_hash_get3(test, "partials", 8))) {
    FIOBJ list = fiobj_hash_get3(test, "partials", 8);
    /* write partials to temporary files */
    FIO_MAP_EACH(fiobj_hash, list, o) {
      FIO_STR_INFO_TMP_VAR(fn, 4095);
      if (fiobj2cstr(o.key).len > 4000) {
        FIO_LOG_ERROR("couldn't write partial template to disk, name too long");
        continue;
      }
      fio_string_write2(&fn,
                        NULL,
                        FIO_STRING_WRITE_STR_INFO(fiobj2cstr(o.key)),
                        FIO_STRING_WRITE_STR2(".mustache", 9));
      fio_filename_overwrite(fn.buf,
                             fiobj2cstr(o.value).buf,
                             fiobj2cstr(o.value).len);
    }
  }
  fio_mustache_s *m = fio_mustache_load(.data = template_data);
  char *result = fio_mustache_build(m,
                                    .get_var = mtest_get_var,
                                    .get_var_index = mtest_get_var_index,
                                    .var2str = mtest_val2str,
                                    .var_is_truthful = mtest_var_is_truthful,
                                    .ctx = data);

  if (!FIOBJ_IS_INVALID(fiobj_hash_get3(test, "partials", 8))) {
    FIOBJ list = fiobj_hash_get3(test, "partials", 8);
    /* delete temporary files */
    FIO_MAP_EACH(fiobj_hash, list, o) {
      FIO_STR_INFO_TMP_VAR(fn, 4095);
      if (fiobj2cstr(o.key).len > 4000) {
        continue;
      }
      fio_string_write2(&fn,
                        NULL,
                        FIO_STRING_WRITE_STR_INFO(fiobj2cstr(o.key)),
                        FIO_STRING_WRITE_STR2(".mustache", 9));
      unlink(fn.buf);
    }
  }
  FIO_ASSERT(m, "template build error!");
  if (!FIO_BUF_INFO_IS_EQ(fio_bstr_buf(result), expected_data)) {
    char *unescaped_expect = fio_bstr_write_html_unescape(NULL,
                                                          expected_data.buf,
                                                          expected_data.len);
    char *unescaped_result =
        fio_bstr_write_html_unescape(NULL,
                                     fio_bstr_buf(result).buf,
                                     fio_bstr_buf(result).len);
    FIO_ASSERT(FIO_BUF_INFO_IS_EQ(fio_bstr_buf(unescaped_expect),
                                  fio_bstr_buf(unescaped_result)),
               "template result match error!\n\n%s\n\n%s\nWith JSON data: %s",
               result,
               expected_data.buf,
               fiobj_str_ptr(fiobj2json(FIOBJ_INVALID, data, 0)));
    FIO_LOG_WARNING("HTML escape mismatch:\n%s\n%s", result, expected_data.buf);
    fio_bstr_free(unescaped_expect);
    fio_bstr_free(unescaped_result);
  }
  printf(
      "* PASSED (%zu/%zu bytes used by mustache object, %zu/%zu by output)\n",
      fio_bstr_len((char *)m),
      fio_bstr_info((char *)m).capa,
      fio_bstr_len(result),
      fio_bstr_info(result).capa);
  fio_mustache_free(m);
  fio_bstr_free(result);
}

static void mustache_json_test(const char *json_file_name) {
  char *json_txt = fio_bstr_readfile(NULL, json_file_name, 0, 0);
  FIO_ASSERT(json_txt, "file %s couldn't be read.", json_file_name);
  FIOBJ json = fiobj_json_parse(fio_bstr_info(json_txt), NULL);
  FIO_ASSERT(json, "JSON parsing failed for:\n%s", json_txt);
  fio_bstr_free(json_txt);
  FIO_LOG_INFO(
      "testing specification file %s.\n===\n%s===\n",
      json_file_name,
      fiobj2cstr(fiobj_json_find(json, FIO_STR_INFO1("overview"))).buf);
  FIOBJ tests = fiobj_hash_get3(json, "tests", 5);
  FIO_ASSERT(FIOBJ_TYPE_IS(tests, FIOBJ_T_ARRAY),
             "JSON tests array type mismatch or missing");
  for (size_t i = 0; i < fiobj_array_count(tests); ++i) {
    FIOBJ t = fiobj_array_get(tests, i);
    FIO_ASSERT(FIOBJ_TYPE_IS(t, FIOBJ_T_HASH),
               "JSON test type mismatch - should be an object (hash map)");
    mustache_json_run_test(t);
  }
  fiobj_free(json);
}

int main(int argc, char const *argv[]) {
  fio_cli_start(argc,
                argv,
                0,
                -1,
                "Mustache template testing using JSON specification file",
                FIO_CLI_PRINT_LINE("Accepts JSON specification file name(s)."));
  if (!fio_cli_unnamed_count()) {
    char *all_common_specs[] = {
        "./tests/mustache-specs/comments.json",
        "./tests/mustache-specs/delimiters.json",
        "./tests/mustache-specs/interpolation.json",
        "./tests/mustache-specs/inverted.json",
        "./tests/mustache-specs/partials.json",
        "./tests/mustache-specs/sections.json",
        NULL,
    };
    for (size_t i = 0; all_common_specs[i]; ++i) {
      fio_cli_set_unnamed(i, all_common_specs[i]);
    }
  }

  unsigned int index = 0;
  const char *fn;
  while ((fn = fio_cli_unnamed(index++)))
    mustache_json_test(fn);
}
