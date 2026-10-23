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
  FIOBJ o = fio_http_params2FIOBJ(h);
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

int main(void) {
  fprintf(stderr, "Testing HTTP params => FIOBJ bridge:\n");
  fprintf(stderr, "  * query values are Strings\n");
  test_expect("query",
              "s=hi+there&n=NULL&t=True&i=-3&d=0.5&z=007&e=&m",
              NULL,
              NULL,
              "{\"s\":\"hi there\",\"n\":\"NULL\",\"t\":\"True\","
              "\"i\":\"-3\",\"d\":\"0.5\",\"z\":\"007\",\"e\":\"\","
              "\"m\":\"\"}");
  fprintf(stderr, "  * query + JSON object body (last value wins, moves to end)\n");
  test_expect("JSON object",
              "a=1&b=true",
              "application/json",
              "{\"a\": 2, \"n\": null, \"arr\": [1, \"x\", {\"k\": false}]}",
              "{\"b\":\"true\",\"a\":2,\"n\":null,\"arr\":[1,\"x\",{\"k\":false}]}");
  fprintf(stderr, "  * query + JSON array body (\"_json\")\n");
  test_expect("JSON array",
              "a=1",
              "application/json",
              "[1, 2]",
              "{\"a\":\"1\",\"_json\":[1,2]}");
  fprintf(stderr, "  * query + urlencoded body\n");
  test_expect("urlencoded",
              "a=1",
              "application/x-www-form-urlencoded",
              "b=2&a=x",
              "{\"b\":\"2\",\"a\":\"x\"}");
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
              "{\"q\":\"1\",\"f\":\"field\",\"up\":{\"filename\":\"a.txt\","
              "\"type\":\"text/plain\",\"data\":\"hello\"}}");
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
    FIOBJ o = fio_http_params2FIOBJ(h);
    fio_http_free(h);
    FIOBJ up = fiobj_hash_get2(o, "up", 2);
    fio_data_s *d = fiobj_data_ptr(fiobj_hash_get2(up, "data", 4));
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
    o = fio_http_params2FIOBJ(h);
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
  fprintf(stderr, "  * errors => FIOBJ_INVALID\n");
  test_expect("bad JSON", "q=1", "application/json", "{\"a\": [1, ", NULL);
  test_expect("bad multipart",
              "q=1",
              "multipart/form-data",
              "--x\r\n",
              NULL);
  FIO_ASSERT(fio_http_params2FIOBJ(NULL) == FIOBJ_INVALID,
             "NULL handle: expected FIOBJ_INVALID");
  fprintf(stderr, "All HTTP params => FIOBJ tests passed!\n");
  return 0;
}
