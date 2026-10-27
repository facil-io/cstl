/* *****************************************************************************
Test: HTTP <=> FIOBJ bridge (fio_http_params2FIOBJ)

The FIOBJ and HTTP modules are included in separate passes to verify the
bridge compiles once both exist, without either module requiring the other.
***************************************************************************** */
#include "test-helpers.h"
#define FIO_FIOBJ
#include FIO_INCLUDE_FILE
#define FIO_HTTP
#include FIO_INCLUDE_FILE

static fio_http_s *test_make(const char *query,
                             const char *content_type,
                             const char *body,
                             size_t body_len) {
  fio_http_s *h = fio_http_new();
  if (query)
    fio_http_query_set(h, FIO_STR_INFO1((char *)query));
  if (content_type)
    fio_http_request_header_set(h,
                                FIO_STR_INFO2((char *)"content-type", 12),
                                FIO_STR_INFO1((char *)content_type));
  if (body_len)
    fio_http_body_write(h, body, body_len);
  return h;
}

/* Parses params to FIOBJ and compares the JSON rendering (NULL = invalid). */
static void test_expect(const char *name,
                        const char *query,
                        const char *content_type,
                        const char *body,
                        const char *expected_json) {
  fio_http_s *h =
      test_make(query, content_type, body, body ? strlen(body) : 0);
  FIOBJ o = fio_http_params2FIOBJ(FIOBJ_INVALID, h);
  fio_http_free(h);
  if (!expected_json) {
    FIO_ASSERT(o == FIOBJ_INVALID, "%s: expected FIOBJ_INVALID", name);
    return;
  }
  FIO_ASSERT(FIOBJ_TYPE(o) == FIOBJ_T_HASH, "%s: expected a Hash", name);
  FIOBJ json = fiobj2json(FIOBJ_INVALID, o, 0);
  fio_str_info_s s = fiobj2cstr(json);
  FIO_ASSERT(s.len == strlen(expected_json) &&
                 !memcmp(s.buf, expected_json, s.len),
             "%s:\n\tgot:      %.*s\n\texpected: %s",
             name,
             (int)s.len,
             s.buf,
             expected_json);
  fiobj_free(json);
  fiobj_free(o);
}

static void test_upload(FIOBJ up,
                        const char *content,
                        const char *filename,
                        const char *type,
                        const char *headers) {
  FIO_ASSERT(FIOBJ_TYPE(up) == FIOBJ_T_DATA,
             "upload must be Data");
  fio_data_s *d = fiobj_data_ptr(up);
  FIO_ASSERT(d && fio_data_len(d) == strlen(content),
             "upload length mismatch");
  fio_str_info_s s = fio_data_read(d, strlen(content));
  FIO_ASSERT(s.len == strlen(content) &&
                 !memcmp(s.buf, content, s.len),
             "upload content mismatch");
  s = fiobj_data_filename(up);
  FIO_ASSERT(s.buf && s.len == strlen(filename) &&
                 !memcmp(s.buf, filename, s.len),
             "upload filename mismatch");
  s = fiobj_data_type(up);
  FIO_ASSERT((!type && !s.buf) ||
                 (type && s.buf && s.len == strlen(type) &&
                  !memcmp(s.buf, type, s.len)),
             "upload media type mismatch");
  s = fiobj_data_headers(up);
  FIO_ASSERT(s.buf && s.len == strlen(headers) &&
                 !memcmp(s.buf, headers, s.len),
             "upload headers mismatch");
}

static void test_json_spilled_body(void) {
  static const char *const tokens[] = {"12345678901234567", "1.25", "0x123", "0b101", "-42"};
  for (size_t i = 0; i < sizeof(tokens) / sizeof(*tokens); ++i) {
    fio_http_s *h = test_make(NULL, "application/json", NULL, 0);
    fio_data_init(&h->body, 1); /* force a file-backed, exact-length body */
    size_t len = strlen(tokens[i]);
    fio_http_body_write(h, tokens[i], len);
    FIO_ASSERT(fio_data_fd(&h->body) != -1, "body must spill");
    fio_http_params_parse_result_s r = fio___http_body_parse_json(
        h, &FIO___HTTP_FIOBJ_CALLBACKS, NULL);
    FIO_ASSERT(!r.err && r.consumed == len && r.result,
               "spilled JSON numeric root failed");
    fiobj_free((FIOBJ)r.result);
    fio_http_free(h);
  }
}

int main(void) {
  test_json_spilled_body();
  fprintf(stderr, "Testing HTTP params => FIOBJ bridge:\n");
  fprintf(stderr, "  * query values are Strings\n");
  test_expect("query",
              "s=hi+there&n=NULL&t=True&i=-3&d=0.5&z=007&e=&m",
              NULL,
              NULL,
              "{\"s\":\"hi there\",\"n\":\"NULL\",\"t\":\"True\","
              "\"i\":\"-3\",\"d\":\"0.5\",\"z\":\"007\",\"e\":\"\","
              "\"m\":\"\"}");
  fprintf(stderr, "  * query + JSON object body (repeated values merge)\n");
  test_expect("JSON object",
              "a=1&b=true",
              "application/json",
              "{\"a\": 2, \"n\": null, \"arr\": [1, \"x\", {\"k\": false}]}",
              "{\"b\":\"true\",\"a\":[\"1\",2],\"n\":null,\"arr\":[1,\"x\",{"
              "\"k\":false}]}");
  fprintf(stderr, "  * non-object JSON roots are ignored\n");
  test_expect("JSON array",
              "a=1",
              "application/json",
              "[1, 2]",
              "{\"a\":\"1\"}");
  test_expect("JSON number", "a=1", "application/json", "42", "{\"a\":\"1\"}");
  fprintf(stderr, "  * query + urlencoded body\n");
  test_expect("urlencoded",
              "a=1",
              "application/x-www-form-urlencoded",
              "b=2&a=x",
              "{\"b\":\"2\",\"a\":[\"1\",\"x\"]}");
  test_expect("query duplicates",
              "a=1&a=2&a=3",
              NULL,
              NULL,
              "{\"a\":[\"1\",\"2\",\"3\"]}");
  test_expect("invalid percent and flags",
              "a=%GG&flag&x=a+b",
              NULL,
              NULL,
              "{\"a\":\"%GG\",\"flag\":\"\",\"x\":\"a b\"}");
  test_expect("query and form",
              "q=1",
              "application/x-www-form-urlencoded",
              "q=2",
              "{\"q\":[\"1\",\"2\"]}");
  test_expect("query and JSON",
              "q=1",
              "application/json",
              "{\"q\":2,\"m\":{\"x\":1}}",
              "{\"q\":[\"1\",2],\"m\":{\"x\":1}}");
  test_expect("scalar to map",
              "m=v",
              "application/json",
              "{\"m\":{\"x\":1}}",
              "{\"m\":{\"\":\"v\",\"x\":1}}");
  fprintf(stderr, "  * query + urlencoded body decode exactly once\n");
  test_expect("single decode",
              "q%252B=%2526%2B+",
              "application/x-www-form-urlencoded",
              "b%2526=%252B%2B+",
              "{\"q%2B\":\"%26+ \",\"b%26\":\"%2B+ \"}");
  fprintf(stderr, "  * query + multipart body with file upload\n");
  test_expect("multipart",
              "q=1",
              "multipart/form-data; boundary=XyZ",
              "--XyZ\r\n"
              "Content-Disposition: form-data; name=\"f\"\r\n\r\n"
              "field\r\n"
              "--XyZ\r\n"
              "Content-Disposition: form-data; name=\"up\"; "
              "filename=\"a.txt\"\r\n"
              "Content-Type: text/plain\r\n\r\n"
              "hello\r\n"
              "--XyZ--\r\n",
              "{\"q\":\"1\",\"f\":\"field\",\"up\":\"hello\"}");
  test_expect("multipart duplicates",
              NULL,
              "multipart/form-data; boundary=XyZ",
              "--XyZ\r\n"
              "Content-Disposition: form-data; name=\"a\"\r\n\r\n"
              "1\r\n"
              "--XyZ\r\n"
              "Content-Disposition: form-data; name=\"a\"\r\n\r\n"
              "2\r\n"
              "--XyZ--\r\n",
              "{\"a\":[\"1\",\"2\"]}");
  fprintf(stderr, "  * decoded form names create nested paths\n");
  test_expect("bracket path",
              "user[name]=A",
              NULL,
              NULL,
              "{\"user\":{\"name\":\"A\"}}");
  test_expect("dot path",
              "user.name=A",
              NULL,
              NULL,
              "{\"user\":{\"name\":\"A\"}}");
  test_expect("append path",
              "tag[]=a&tag[]=b",
              NULL,
              NULL,
              "{\"tag\":[\"a\",\"b\"]}");
  test_expect("invalid empty segment",
              "a..b=1",
              NULL,
              NULL,
              "{\"a..b\":\"1\"}");
  test_expect("invalid nonterminal append",
              "items[][id]=1",
              NULL,
              NULL,
              "{\"items[][id]\":\"1\"}");
  test_expect("scalar then nested map",
              "user=x&user.name=A",
              NULL,
              NULL,
              "{\"user\":{\"\":\"x\",\"name\":\"A\"}}");
  test_expect("nested map then scalar",
              "user.name=A&user=x",
              NULL,
              NULL,
              "{\"user\":{\"name\":\"A\",\"\":\"x\"}}");
  test_expect("scalar then append",
              "tag=x&tag[]=a",
              NULL,
              NULL,
              "{\"tag\":[\"x\",\"a\"]}");
  test_expect("deep dot path",
              "user.address.country=USA",
              NULL,
              NULL,
              "{\"user\":{\"address\":{\"country\":\"USA\"}}}");
  test_expect("equivalent JSON tree",
              NULL,
              "application/json",
              "{\"user\":{\"address\":{\"country\":\"USA\"}}}",
              "{\"user\":{\"address\":{\"country\":\"USA\"}}}");
  test_expect("form path merges JSON",
              "user[name]=A",
              "application/json",
              "{\"user\":{\"age\":3}}",
              "{\"user\":{\"name\":\"A\",\"age\":3}}");
  test_expect("JSON keys remain literal",
              NULL,
              "application/json",
              "{\"a.b\":1,\"c[d]\":2}",
              "{\"a.b\":1,\"c[d]\":2}");
  test_expect("urlencoded paths",
              NULL,
              "application/x-www-form-urlencoded",
              "user[name]=A&user[tags][]=x",
              "{\"user\":{\"name\":\"A\",\"tags\":[\"x\"]}}");
  test_expect(
      "multipart nested and uploads",
      NULL,
      "multipart/form-data; boundary=XyZ",
      "--XyZ\r\n"
      "Content-Disposition: form-data; name=\"user[name]\"\r\n\r\n"
      "A\r\n"
      "--XyZ\r\n"
      "Content-Disposition: form-data; name=\"files[]\"; "
      "filename=\"a.txt\"\r\n"
      "Content-Type: text/plain\r\n\r\n"
      "one\r\n"
      "--XyZ\r\n"
      "Content-Disposition: form-data; name=\"files[]\"; "
      "filename=\"b.txt\"\r\n"
      "Content-Type: text/plain\r\n\r\n"
      "two\r\n"
      "--XyZ--\r\n",
      "{\"user\":{\"name\":\"A\"},\"files\":[\"one\",\"two\"]}");
  fprintf(stderr, "  * upload metadata and scalar conflict merging\n");
  {
    const char *body =
        "--XyZ\r\nContent-Disposition: form-data; name=\"up\"; "
        "filename=\"a/b/c.txt\"\r\nContent-Type: text/plain\r\n"
        "X-Upload: yes\r\n\r\none\r\n"
        "--XyZ\r\nContent-Disposition: form-data; name=\"up\"; "
        "filename=\"C:\\x\\y.png\"\r\n\r\ntwo\r\n"
        "--XyZ\r\nContent-Disposition: form-data; name=\"mixed\"; "
        "filename=\"../z\"\r\n\r\nthree\r\n"
        "--XyZ\r\nContent-Disposition: form-data; name=\"mixed\""
        "\r\n\r\ntext\r\n--XyZ--\r\n";
    fio_http_s *h = test_make(NULL, "multipart/form-data; boundary=XyZ",
                               body, strlen(body));
    FIOBJ o = fio_http_params2FIOBJ(FIOBJ_INVALID, h);
    fio_http_free(h);
    FIOBJ up = fiobj_hash_get2(o, "up", 2);
    FIO_ASSERT(FIOBJ_TYPE_CLASS(up) == FIOBJ_T_ARRAY &&
                   fiobj_array_count(up) == 2,
               "repeated uploads must form an Array");
    test_upload(fiobj_array_get(up, 0), "one", "c.txt", "text/plain",
                "Content-Disposition: form-data; name=\"up\"; "
                "filename=\"a/b/c.txt\"\r\nContent-Type: text/plain\r\n"
                "X-Upload: yes");
    test_upload(fiobj_array_get(up, 1), "two", "y.png", NULL,
                "Content-Disposition: form-data; name=\"up\"; "
                "filename=\"C:\\x\\y.png\"");
    FIOBJ mixed = fiobj_hash_get2(o, "mixed", 5);
    FIO_ASSERT(FIOBJ_TYPE_CLASS(mixed) == FIOBJ_T_ARRAY &&
                   fiobj_array_count(mixed) == 2,
               "upload + text must form an Array");
    test_upload(fiobj_array_get(mixed, 0), "three", "z", NULL,
                "Content-Disposition: form-data; name=\"mixed\"; "
                "filename=\"../z\"");
    FIO_ASSERT(FIOBJ_TYPE_CLASS(fiobj_array_get(mixed, 1)) == FIOBJ_T_STRING,
               "text field must remain a String");
    fiobj_free(o);
  }
  fprintf(stderr, "  * large upload => spilled FIOBJ_T_DATA\n");
  {
    const size_t file_len = (FIO_DATA_RAM_LIMIT * 3) + 777;
    char *file = (char *)malloc(file_len);
    FIO_ASSERT(file, "allocation failed");
    for (size_t i = 0; i < file_len; ++i)
      file[i] = (char)(i * 13 + (i >> 11));
    const char *head = "--XyZ\r\nContent-Disposition: form-data; name=\"up\"; "
                       "filename=\"big.bin\"\r\n\r\n";
    const char *tail = "\r\n--XyZ--\r\n";
    fio_http_s *h = test_make(NULL, "multipart/form-data; boundary=XyZ", NULL, 0);
    fio_http_body_write(h, head, strlen(head));
    fio_http_body_write(h, file, file_len);
    fio_http_body_write(h, tail, strlen(tail));
    FIOBJ o = fio_http_params2FIOBJ(FIOBJ_INVALID, h);
    fio_http_free(h);
    FIOBJ up = fiobj_hash_get2(o, "up", 2);
    fio_data_s *d = fiobj_data_ptr(up);
    FIO_ASSERT(d && fio_data_len(d) == file_len,
               "large upload: data length %zu / %zu",
               d ? fio_data_len(d) : 0,
               file_len);
    FIO_ASSERT(fio_data_fd(d) != -1, "large upload should spill to a file");
    size_t got = 0;
    for (fio_str_info_s s; (s = fio_data_read(d, 65536)).len; got += s.len)
      FIO_ASSERT(!memcmp(s.buf, file + got, s.len),
                 "large upload: content mismatch at %zu",
                 got);
    FIO_ASSERT(got == file_len, "large upload: read %zu bytes", got);
    fiobj_free(o);
#if FIO_OS_POSIX
    /* the upload can't be stored (no temporary folder) => parse error */
    fprintf(stderr, "  * upload storage failure => FIOBJ_INVALID\n");
    h = test_make(NULL, "multipart/form-data; boundary=XyZ", NULL, 0);
    FIO_ASSERT(!fio_http_body_write(h, head, strlen(head)) &&
                   !fio_http_body_write(h, file, file_len) &&
                   !fio_http_body_write(h, tail, strlen(tail)),
               "body writes should succeed");
    char *old = test_tmpdir_set("tmp/test-http-fiobj-no-such-folder/nope");
    o = fio_http_params2FIOBJ(FIOBJ_INVALID, h);
    test_tmpdir_restore(old);
    fio_http_free(h);
    FIO_ASSERT(o == FIOBJ_INVALID,
               "an upload that can't be stored should fail the parse");
#endif
    free(file);
  }
  fprintf(stderr, "  * unknown Content-Type body is ignored\n");
  test_expect("unknown body", "q=1", "text/plain", "a=b", "{\"q\":\"1\"}");
  fprintf(stderr, "  * empty request => empty Hash\n");
  test_expect("empty", NULL, NULL, NULL, "{}");
  fprintf(stderr, "  * params field overflow => FIOBJ_INVALID\n");
  {
    size_t cap = (FIO_HTTP_PARAMS_MAX_FIELDS + 1) * 16;
    char *query = malloc(cap);
    FIO_ASSERT(query, "overflow query allocation");
    size_t len = 0;
    for (size_t i = 0; i <= FIO_HTTP_PARAMS_MAX_FIELDS; ++i) {
      int n = snprintf(query + len, cap - len, "%sk%zu=v", i ? "&" : "", i);
      FIO_ASSERT(n > 0 && (size_t)n < cap - len, "overflow query buffer");
      len += (size_t)n;
    }
    fio_http_s *h = test_make(query, NULL, NULL, 0);
    FIO_ASSERT(fio_http_params2FIOBJ(FIOBJ_INVALID, h) == FIOBJ_INVALID,
               "overflow must not return truncated params");
    fio_http_free(h);
    free(query);
  }
  fprintf(stderr, "  * name depth boundaries => Hash or FIOBJ_INVALID\n");
  {
    char name[2 * FIO_JSON_MAX_DEPTH + 8];
    for (size_t append = 0; append <= 1; ++append) {
      size_t accepted = FIO_JSON_MAX_DEPTH - append;
      for (size_t n = accepted; n <= accepted + 1; ++n) {
        size_t len = 0;
        for (size_t i = 0; i < n; ++i) {
          if (i)
            name[len++] = '.';
          name[len++] = 'a';
        }
        if (append) {
          name[len++] = '[';
          name[len++] = ']';
        }
        memcpy(name + len, "=1", 3);
        fio_http_s *h = test_make(name, NULL, NULL, 0);
        FIOBJ result = fio_http_params2FIOBJ(FIOBJ_INVALID, h);
        FIO_ASSERT((result == FIOBJ_INVALID) == (n > accepted),
                   "name depth n=%zu append=%zu", n, append);
        if (result != FIOBJ_INVALID)
          fiobj_free(result);
        fio_http_free(h);
      }
    }
  }
  fprintf(stderr, "  * merge into an existing Hash (dest)\n");
  {
    const char json[] = "{\"a\": \"body\", \"m\": {\"x\": 1}}";
    fio_http_s *h =
        test_make("a=query&n=1", "application/json", json, sizeof(json) - 1);
    FIOBJ dest = fiobj_hash_new();
    fiobj_hash_set2(dest, "keep", 4, fiobj_str_new_cstr("old", 3));
    fiobj_hash_set2(dest, "a", 1, fiobj_str_new_cstr("seed", 4));
    fiobj_hash_set2(dest, "m", 1, fiobj_hash_new());
    fiobj_hash_set2(fiobj_hash_get2(dest, "m", 1), "y", 1, fiobj_num_new(2));
    FIOBJ r = fio_http_params2FIOBJ(dest, h);
    fio_http_free(h);
    FIO_ASSERT(r == dest, "merge must return `dest` itself");
    fio_str_info_s keep = fiobj2cstr(fiobj_hash_get2(dest, "keep", 4));
    FIO_ASSERT(keep.len == 3 && !memcmp(keep.buf, "old", 3),
               "merge must preserve untouched keys");
    FIOBJ a = fiobj_hash_get2(dest, "a", 1);
    FIO_ASSERT(FIOBJ_TYPE(a) == FIOBJ_T_ARRAY && fiobj_array_count(a) == 3,
               "scalar conflicts must merge into an Array (seed+query+body)");
    const char *want[] = {"seed", "query", "body"};
    for (size_t i = 0; i < 3; ++i) {
      fio_str_info_s s = fiobj2cstr(fiobj_array_get(a, (int64_t)i));
      FIO_ASSERT(s.len == strlen(want[i]) && !memcmp(s.buf, want[i], s.len),
                 "merged array order mismatch at %zu",
                 i);
    }
    FIOBJ m = fiobj_hash_get2(dest, "m", 1);
    FIO_ASSERT(FIOBJ_TYPE(m) == FIOBJ_T_HASH, "map conflicts must merge");
    FIO_ASSERT(fiobj2i(fiobj_hash_get2(m, "x", 1)) == 1 &&
                   fiobj2i(fiobj_hash_get2(m, "y", 1)) == 2,
               "merged map must hold old and new keys");
    fio_str_info_s n = fiobj2cstr(fiobj_hash_get2(dest, "n", 1));
    FIO_ASSERT(n.len == 1 && n.buf[0] == '1', "query key missing");
    fiobj_free(dest);
  }
  fprintf(stderr, "  * error keeps a caller-supplied dest alive\n");
  {
    const char bad[] = "{\"a\": [1, ";
    fio_http_s *h =
        test_make("q=1", "application/json", bad, sizeof(bad) - 1);
    FIOBJ dest = fiobj_hash_new();
    FIOBJ r = fio_http_params2FIOBJ(dest, h);
    fio_http_free(h);
    FIO_ASSERT(r == FIOBJ_INVALID, "error must return FIOBJ_INVALID");
    FIO_ASSERT(FIOBJ_TYPE(dest) == FIOBJ_T_HASH,
               "dest must not be freed on error");
    FIO_ASSERT(fiobj_hash_get2(dest, "q", 1) != FIOBJ_INVALID,
               "partial query data should remain in dest");
    fiobj_free(dest);
  }
  fprintf(stderr, "  * failing bodies keep a caller-supplied dest valid\n");
  {
    static const struct {
      const char *content_type;
      const char *body;
    } bad[] = {
        {"application/json", "{\"m\": {\"x\": [1, {\"y\": "},
        {"multipart/form-data; boundary=X",
         "--X\r\nContent-Disposition: form-data; name=\"f\"; "
         "filename=\"a.txt\"\r\n\r\ntruncated"},
        {"multipart/form-data; boundary=X", "--X\r\n"},
    };
    for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); ++i) {
      fio_http_s *h = test_make("q=1&m.z=2",
                                bad[i].content_type,
                                bad[i].body,
                                strlen(bad[i].body));
      FIOBJ dest = fiobj_hash_new();
      fiobj_hash_set2(dest, "keep", 4, fiobj_str_new_cstr("old", 3));
      FIOBJ r = fio_http_params2FIOBJ(dest, h);
      fio_http_free(h);
      FIO_ASSERT(r == FIOBJ_INVALID, "failing body #%zu must fail", i);
      FIO_ASSERT(FIOBJ_TYPE(dest) == FIOBJ_T_HASH &&
                     fiobj_hash_get2(dest, "keep", 4) != FIOBJ_INVALID &&
                     fiobj_hash_get2(dest, "q", 1) != FIOBJ_INVALID,
                 "failing body #%zu: dest lost its data",
                 i);
      /* dest stays fully usable: render, mutate and reuse it */
      FIOBJ json = fiobj2json(FIOBJ_INVALID, dest, 0);
      FIO_ASSERT(fiobj2cstr(json).len > 2, "failing body #%zu: render", i);
      fiobj_free(json);
      fiobj_hash_set2(dest, "after", 5, fiobj_num_new(1));
      h = test_make("again=1", NULL, NULL, 0);
      FIO_ASSERT(fio_http_params2FIOBJ(dest, h) == dest &&
                     fiobj_hash_get2(dest, "again", 5) != FIOBJ_INVALID,
                 "failing body #%zu: dest must stay reusable",
                 i);
      fio_http_free(h);
      fiobj_free(dest);
    }
  }
  fprintf(stderr, "  * null values take part in conflict resolution\n");
  test_expect("null conflict",
              "a=1",
              "application/json",
              "{\"a\": null, \"b\": null, \"b\": 2}",
              "{\"a\":[\"1\",null],\"b\":[null,2]}");
  fprintf(stderr, "  * non-Hash dest => FIOBJ_INVALID\n");
  {
    FIOBJ not_hash = fiobj_num_new(1);
    FIO_ASSERT(fio_http_params2FIOBJ(not_hash, NULL) == FIOBJ_INVALID,
               "non-Hash dest must return FIOBJ_INVALID");
    fiobj_free(not_hash);
  }
  fprintf(stderr, "  * errors => FIOBJ_INVALID\n");
  test_expect("bad JSON", "q=1", "application/json", "{\"a\": [1, ", NULL);
  test_expect("bad multipart",
              "q=1",
              "multipart/form-data",
              "--x\r\n",
              NULL);
  FIO_ASSERT(fio_http_params2FIOBJ(FIOBJ_INVALID, NULL) == FIOBJ_INVALID,
             "NULL handle: expected FIOBJ_INVALID");
  fprintf(stderr, "All HTTP params => FIOBJ tests passed!\n");
  return 0;
}
