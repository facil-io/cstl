/* *****************************************************************************
Test for 155 der.h

Coverage: ASN.1 DER parsing and encoding for universal types (INTEGER,
BOOLEAN, NULL, BIT STRING, OCTET STRING, OID, UTF8String, PrintableString,
UTC/Generalized Time), SEQUENCE/SET iteration, context-specific tags, long-form
lengths, and error handling. Encoding round-trips are validated by parsing the
encoded bytes back. Performance loops and external references are omitted.
***************************************************************************** */
#define FIO_TIME
#define FIO_SHA2
#define FIO_MATH
#define FIO_DER
#define FIO_RSA
#define FIO_ED25519
#define FIO_P256
#define FIO_X509
#include "test-helpers.h"

/* *****************************************************************************
Test Vectors
***************************************************************************** */

/* SEQUENCE { INTEGER(42), NULL } */
static const uint8_t test_seq_int_null[] = {
    0x30, 0x05, 0x02, 0x01, 0x2A, 0x05, 0x00};

static const uint8_t test_int_42[] = {0x02, 0x01, 0x2A};
static const uint8_t test_int_127[] = {0x02, 0x01, 0x7F};
static const uint8_t test_int_128[] = {0x02, 0x02, 0x00, 0x80};
static const uint8_t test_int_256[] = {0x02, 0x02, 0x01, 0x00};
static const uint8_t test_int_large[] = {
    0x02, 0x08, 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};

static const uint8_t test_bool_true[] = {0x01, 0x01, 0xFF};
static const uint8_t test_bool_false[] = {0x01, 0x01, 0x00};
static const uint8_t test_null[] = {0x05, 0x00};

static const uint8_t test_bitstring[] = {
    0x03, 0x04, 0x06, 0x6E, 0x5D, 0xC0};
static const uint8_t test_bitstring_empty[] = {0x03, 0x01, 0x00};

static const uint8_t test_oid_sha256_rsa[] = {
    0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x0B};
static const uint8_t test_oid_secp256r1[] = {
    0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x03, 0x01, 0x07};
static const uint8_t test_oid_cn[] = {0x06, 0x03, 0x55, 0x04, 0x03};
static const uint8_t test_oid_ed25519[] = {0x06, 0x03, 0x2B, 0x65, 0x70};

static const uint8_t test_utf8_string[] = {
    0x0C, 0x0B, 'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd'};
static const uint8_t test_printable_string[] = {
    0x13, 0x07, 'e', 'x', 'a', 'm', 'p', 'l', 'e'};

static const uint8_t test_utc_time[] = {
    0x17, 0x0D, '2', '3', '1', '2', '2', '5', '1', '2', '3', '0', '4', '5', 'Z'};
static const uint8_t test_gen_time[] = {
    0x18, 0x0F,
    '2', '0', '2', '3', '1', '2', '2', '5', '1', '2', '3', '0', '4', '5', 'Z'};

static const uint8_t test_context_0[] = {0xA0, 0x03, 0x02, 0x01, 0x03};

static const uint8_t test_nested_seq[] = {
    0x30, 0x19,
    0x30, 0x07, 0x06, 0x03, 0x55, 0x04, 0x03, 0x05, 0x00,
    0x30, 0x0E, 0x31, 0x0C, 0x30, 0x0A, 0x06, 0x03, 0x55, 0x04, 0x03,
    0x0C, 0x03, 'f',  'o',  'o'};

/* *****************************************************************************
Core Parsing Tests
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, der_parse_element)(void) {
  fio_der_element_s elem;
  const uint8_t *next;

  next = fio_der_parse(&elem, test_int_42, sizeof(test_int_42));
  FIO_ASSERT(next, "INTEGER(42) parse failed");
  FIO_ASSERT((elem.tag & 0x1F) == FIO_DER_INTEGER, "INTEGER tag mismatch");
  FIO_ASSERT(elem.tag_class == FIO_DER_CLASS_UNIVERSAL,
             "INTEGER class mismatch");
  FIO_ASSERT(!elem.is_constructed, "INTEGER must be primitive");
  FIO_ASSERT(elem.len == 1, "INTEGER length mismatch");
  FIO_ASSERT(elem.data[0] == 0x2A, "INTEGER value mismatch");
  FIO_ASSERT(next == test_int_42 + sizeof(test_int_42), "next pointer wrong");

  next = fio_der_parse(&elem, test_null, sizeof(test_null));
  FIO_ASSERT(next, "NULL parse failed");
  FIO_ASSERT((elem.tag & 0x1F) == FIO_DER_NULL, "NULL tag mismatch");
  FIO_ASSERT(elem.len == 0, "NULL length mismatch");

  next = fio_der_parse(&elem, test_bool_true, sizeof(test_bool_true));
  FIO_ASSERT(next, "BOOLEAN parse failed");
  FIO_ASSERT((elem.tag & 0x1F) == FIO_DER_BOOLEAN, "BOOLEAN tag mismatch");
  FIO_ASSERT(elem.len == 1 && elem.data[0] == 0xFF,
             "BOOLEAN TRUE value mismatch");

  next = fio_der_parse(&elem, test_seq_int_null, sizeof(test_seq_int_null));
  FIO_ASSERT(next, "SEQUENCE parse failed");
  FIO_ASSERT((elem.tag & 0x1F) == FIO_DER_SEQUENCE, "SEQUENCE tag mismatch");
  FIO_ASSERT(elem.is_constructed, "SEQUENCE must be constructed");
  FIO_ASSERT(elem.len == 5, "SEQUENCE length mismatch");

  next = fio_der_parse(&elem, test_context_0, sizeof(test_context_0));
  FIO_ASSERT(next, "context tag [0] parse failed");
  FIO_ASSERT(elem.tag_class == FIO_DER_CLASS_CONTEXT,
             "context tag class mismatch");
  FIO_ASSERT((elem.tag & 0x1F) == 0, "context tag number mismatch");
  FIO_ASSERT(elem.is_constructed, "context tag [0] must be constructed");
}

FIO_SFUNC void FIO_NAME_TEST(stl, der_parse_integer)(void) {
  fio_der_element_s elem;
  uint64_t value;

  FIO_ASSERT(fio_der_parse(&elem, test_int_42, sizeof(test_int_42)) &&
                 fio_der_parse_integer(&elem, &value) == 0 && value == 42,
             "INTEGER(42) failed");
  FIO_ASSERT(fio_der_parse(&elem, test_int_127, sizeof(test_int_127)) &&
                 fio_der_parse_integer(&elem, &value) == 0 && value == 127,
             "INTEGER(127) failed");
  FIO_ASSERT(fio_der_parse(&elem, test_int_128, sizeof(test_int_128)) &&
                 fio_der_parse_integer(&elem, &value) == 0 && value == 128,
             "INTEGER(128) failed");
  FIO_ASSERT(fio_der_parse(&elem, test_int_256, sizeof(test_int_256)) &&
                 fio_der_parse_integer(&elem, &value) == 0 && value == 256,
             "INTEGER(256) failed");
  FIO_ASSERT(fio_der_parse(&elem, test_int_large, sizeof(test_int_large)) &&
                 fio_der_parse_integer(&elem, &value) == 0 &&
                 value == 0x0123456789ABCDEFULL,
             "large INTEGER failed");

  FIO_ASSERT(fio_der_parse(&elem, test_null, sizeof(test_null)) &&
                 fio_der_parse_integer(&elem, &value) != 0,
             "NULL as INTEGER should fail");

  /* Lenient INTEGERs: {bytes, len, ok_with_value, ok_without_value, v} */
  static const struct {
    uint8_t b[12];
    uint8_t n;
    uint8_t ok_v, ok_nov;
    uint64_t v;
  } mi[] = {
      {{0x02, 0x00}, 2, 0, 0, 0},                         /* empty */
      {{0x02, 0x01, 0x00}, 3, 1, 1, 0},                   /* 0 */
      {{0x02, 0x02, 0x00, 0x7F}, 4, 1, 1, 127},           /* redundant 00 */
      {{0x02, 0x02, 0x00, 0x00}, 4, 1, 1, 0},             /* redundant 00 */
      {{0x02, 0x02, 0x00, 0x80}, 4, 1, 1, 128},           /* sign byte ok */
      {{0x02, 0x01, 0xFF}, 3, 0, 1, 0},                   /* -1 */
      {{0x02, 0x02, 0xFF, 0x80}, 4, 0, 1, 0},             /* redundant FF */
      {{0x02, 0x02, 0xFF, 0x7F}, 4, 0, 1, 0},             /* -129 */
      {{0x02, 0x01, 0x80}, 3, 0, 1, 0},                   /* -128 */
      {{0x02, 0x09, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
       11, 1, 1, ~(uint64_t)0},                           /* UINT64_MAX */
      {{0x02, 0x09, 0x01, 0, 0, 0, 0, 0, 0, 0, 0}, 11, 0, 1, 0}, /* 2^64 */
      {{0x02, 0x0A, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
       12, 1, 1, ~(uint64_t)0}, /* UINT64_MAX, redundant 00 (lenient) */
  };
  for (size_t i = 0; i < sizeof(mi) / sizeof(mi[0]); ++i) {
    FIO_ASSERT(fio_der_parse(&elem, mi[i].b, mi[i].n), "INTEGER TLV %zu", i);
    value = 0xDEAD;
    int r = fio_der_parse_integer(&elem, &value);
    FIO_ASSERT((r == 0) == mi[i].ok_v && (r || value == mi[i].v),
               "INTEGER minimal/sign check (value) failed at %zu",
               i);
    FIO_ASSERT((fio_der_parse_integer(&elem, NULL) == 0) == mi[i].ok_nov,
               "INTEGER minimal check (NULL) failed at %zu",
               i);
  }
}

FIO_SFUNC void FIO_NAME_TEST(stl, der_parse_boolean)(void) {
  fio_der_element_s elem;
  int value;

  FIO_ASSERT(fio_der_parse(&elem, test_bool_true, sizeof(test_bool_true)) &&
                 fio_der_parse_boolean(&elem, &value) == 0 && value,
             "BOOLEAN TRUE failed");
  FIO_ASSERT(fio_der_parse(&elem, test_bool_false, sizeof(test_bool_false)) &&
                 fio_der_parse_boolean(&elem, &value) == 0 && !value,
             "BOOLEAN FALSE failed");
}

FIO_SFUNC void FIO_NAME_TEST(stl, der_parse_bit_string)(void) {
  fio_der_element_s elem;
  const uint8_t *bits = NULL;
  size_t bit_len = 0;
  uint8_t unused = 0;

  FIO_ASSERT(fio_der_parse(&elem, test_bitstring, sizeof(test_bitstring)) &&
                 fio_der_parse_bit_string(&elem, &bits, &bit_len, &unused) ==
                     0,
             "BIT STRING parse failed");
  FIO_ASSERT(unused == 6 && bit_len == 3, "BIT STRING length/unused mismatch");
  FIO_ASSERT(bits[0] == 0x6E && bits[1] == 0x5D && bits[2] == 0xC0,
             "BIT STRING data mismatch");

  FIO_ASSERT(
      fio_der_parse(&elem, test_bitstring_empty, sizeof(test_bitstring_empty))
          && fio_der_parse_bit_string(&elem, &bits, &bit_len, &unused) == 0
          && bit_len == 0 && unused == 0,
      "empty BIT STRING failed");
}

FIO_SFUNC void FIO_NAME_TEST(stl, der_parse_oid)(void) {
  fio_der_element_s elem;
  char oid_buf[128];
  int len;

  FIO_ASSERT(fio_der_parse(&elem, test_oid_sha256_rsa,
                            sizeof(test_oid_sha256_rsa)),
             "SHA256-RSA OID parse failed");
  len = fio_der_parse_oid(&elem, oid_buf, sizeof(oid_buf));
  FIO_ASSERT(len > 0, "OID stringify failed");
  FIO_ASSERT(!FIO_MEMCMP(oid_buf, "1.2.840.113549.1.1.11", (size_t)len),
             "SHA256-RSA OID mismatch");
  FIO_ASSERT(fio___der_oid_eq(fio___der_oid_value(&elem),
                              FIO_X509_OID_SHA256_WITH_RSA),
             "SHA256-RSA oid value/eq failed");
  FIO_ASSERT(!fio___der_oid_eq(fio___der_oid_value(&elem),
                               FIO_X509_OID_ED25519),
             "different OID should not match");

  FIO_ASSERT(fio_der_parse(&elem, test_oid_secp256r1,
                            sizeof(test_oid_secp256r1)) &&
                 fio___der_oid_eq(fio___der_oid_value(&elem),
                                  FIO_X509_OID_SECP256R1),
             "secp256r1 OID failed");
  FIO_ASSERT(fio_der_parse(&elem, test_oid_cn, sizeof(test_oid_cn)) &&
                 fio___der_oid_eq(fio___der_oid_value(&elem),
                                  FIO_X509_OID_COMMON_NAME),
             "CN OID failed");
  FIO_ASSERT(fio_der_parse(&elem, test_oid_ed25519, sizeof(test_oid_ed25519))
                 && fio___der_oid_eq(fio___der_oid_value(&elem),
                                      FIO_X509_OID_ED25519),
             "Ed25519 OID failed");

  /* Length is folded into byte 15: {..0B} vs {..0B 00} must differ */
  {
    static const uint8_t oid_short[] = {0x06, 0x01, 0x2A};
    static const uint8_t oid_long[] = {0x06, 0x02, 0x2A, 0x00};
    fio_der_element_s e1, e2;
    FIO_ASSERT(fio_der_parse(&e1, oid_short, sizeof(oid_short)) &&
                   fio_der_parse(&e2, oid_long, sizeof(oid_long)),
               "padding-ambiguity OIDs parse failed");
    FIO_ASSERT(!fio___der_oid_eq(fio___der_oid_value(&e1),
                                 fio___der_oid_value(&e2)),
               "zero-padding must not make different OIDs equal");
  }

  /* Oversized OID (content > 15 bytes) must never match a valid value */
  {
    static const uint8_t oid_huge[] = {0x06, 0x10, 0x2A, 0x86, 0x48, 0x86,
                                       0xF7, 0x0D, 0x01, 0x01, 0x0B, 0x01,
                                       0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
    fio_der_element_s huge;
    FIO_ASSERT(fio_der_parse(&huge, oid_huge, sizeof(oid_huge)),
               "oversized OID parse failed");
    fio_u128 v = fio___der_oid_value(&huge);
    FIO_ASSERT(v.u8[15] == 0xFF, "oversized OID must become never-match");
    FIO_ASSERT(!fio___der_oid_eq(v, FIO_X509_OID_SHA256_WITH_RSA),
               "oversized OID must not match any constant");
    FIO_ASSERT(!fio___der_oid_eq(fio___der_oid_value(NULL),
                                 FIO_X509_OID_SHA256_WITH_RSA),
               "NULL element must not match any constant");
  }
}

FIO_SFUNC void FIO_NAME_TEST(stl, der_parse_string)(void) {
  fio_der_element_s elem;
  const char *str;
  size_t len;

  FIO_ASSERT(fio_der_parse(&elem, test_utf8_string, sizeof(test_utf8_string))
                 && (str = fio_der_parse_string(&elem, &len)) && len == 11
                 && !FIO_MEMCMP(str, "Hello World", 11),
             "UTF8String failed");
  FIO_ASSERT(fio_der_parse(&elem, test_printable_string,
                            sizeof(test_printable_string))
                 && (str = fio_der_parse_string(&elem, &len)) && len == 7
                 && !FIO_MEMCMP(str, "example", 7),
             "PrintableString failed");

  /* IA5String is also accepted as a string type */
  static const uint8_t test_ia5[] = {0x16, 0x05, 'h', 'e', 'l', 'l', 'o'};
  FIO_ASSERT(fio_der_parse(&elem, test_ia5, sizeof(test_ia5))
                 && (str = fio_der_parse_string(&elem, &len)) && len == 5
                 && !FIO_MEMCMP(str, "hello", 5),
             "IA5String failed");

  /* NumericString is accepted */
  static const uint8_t test_numeric[] = {0x12, 0x05, '1', '2', '3', '4', '5'};
  FIO_ASSERT(fio_der_parse(&elem, test_numeric, sizeof(test_numeric))
                 && (str = fio_der_parse_string(&elem, &len)) && len == 5
                 && !FIO_MEMCMP(str, "12345", 5),
             "NumericString failed");

  /* Non-string type should be rejected by parse_string */
  FIO_ASSERT(fio_der_parse(&elem, test_int_42, sizeof(test_int_42))
                 && fio_der_parse_string(&elem, &len) == NULL,
             "INTEGER should not parse as string");
}

FIO_SFUNC void FIO_NAME_TEST(stl, der_parse_time)(void) {
  fio_der_element_s elem;
  int64_t utc_ts = 0;
  int64_t gen_ts = 0;

  FIO_ASSERT(fio_der_parse(&elem, test_utc_time, sizeof(test_utc_time)) &&
                 fio_der_parse_time(&elem, &utc_ts) == 0,
             "UTCTime parse failed");
  FIO_ASSERT(utc_ts > 1703500000 && utc_ts < 1703510000,
             "UTCTime value out of range");

  FIO_ASSERT(fio_der_parse(&elem, test_gen_time, sizeof(test_gen_time)) &&
                 fio_der_parse_time(&elem, &gen_ts) == 0 && gen_ts == utc_ts,
             "GeneralizedTime != UTCTime");
  {
    /* 2106 dates must not wrap via a 32-bit time_t to appear valid today. */
    static const uint8_t future[] = {
        0x18, 0x0F, '2', '1', '0', '6', '0', '2', '0', '7',
        '0', '6', '2', '8', '1', '6', 'Z'};
    FIO_ASSERT(fio_der_parse(&elem, future, sizeof(future)) &&
                   fio_der_parse_time(&elem, &gen_ts) == 0 &&
                   gen_ts == 4294967296LL,
               "GeneralizedTime must preserve dates beyond 32-bit time_t");
  }
}

FIO_SFUNC void FIO_NAME_TEST(stl, der_iterator)(void) {
  fio_der_element_s seq, elem;
  fio_der_iterator_s it;
  uint64_t value;

  FIO_ASSERT(fio_der_parse(&seq, test_seq_int_null, sizeof(test_seq_int_null)),
             "iterator SEQUENCE parse failed");
  fio_der_iterator_init(&it, &seq);
  FIO_ASSERT(fio_der_iterator_has_next(&it),
             "iterator should have elements");
  FIO_ASSERT(!fio_der_iterator_next(&it, &elem) &&
                 fio_der_is_tag(&elem, FIO_DER_INTEGER) &&
                 fio_der_parse_integer(&elem, &value) == 0 && value == 42,
             "iterator first element failed");
  FIO_ASSERT(!fio_der_iterator_next(&it, &elem) &&
                 fio_der_is_tag(&elem, FIO_DER_NULL),
             "iterator second element failed");
  FIO_ASSERT(!fio_der_iterator_has_next(&it),
             "iterator should be exhausted");
  FIO_ASSERT(fio_der_iterator_next(&it, &elem) != 0,
             "exhausted iterator should fail");
}

FIO_SFUNC void FIO_NAME_TEST(stl, der_nested)(void) {
  fio_der_element_s outer, inner, elem;
  fio_der_iterator_s it;

  FIO_ASSERT(fio_der_parse(&outer, test_nested_seq, sizeof(test_nested_seq)) &&
                 fio_der_is_tag(&outer, FIO_DER_SEQUENCE),
             "nested SEQUENCE parse failed");
  fio_der_iterator_init(&it, &outer);

  FIO_ASSERT(!fio_der_iterator_next(&it, &inner) &&
                 fio_der_is_tag(&inner, FIO_DER_SEQUENCE),
             "AlgorithmIdentifier SEQUENCE failed");
  fio_der_iterator_init(&it, &inner);
  FIO_ASSERT(!fio_der_iterator_next(&it, &elem) &&
                 fio_der_is_tag(&elem, FIO_DER_OID) &&
                 fio___der_oid_eq(fio___der_oid_value(&elem),
                                  FIO_X509_OID_COMMON_NAME),
             "nested OID failed");
  FIO_ASSERT(!fio_der_iterator_next(&it, &elem) &&
                 fio_der_is_tag(&elem, FIO_DER_NULL),
             "nested NULL params failed");
}

FIO_SFUNC void FIO_NAME_TEST(stl, der_helpers)(void) {
  fio_der_element_s elem;

  FIO_ASSERT(fio_der_parse(&elem, test_int_42, sizeof(test_int_42)) &&
                 fio_der_is_tag(&elem, FIO_DER_INTEGER) &&
                 !fio_der_is_tag(&elem, FIO_DER_BOOLEAN),
             "fio_der_is_tag failed");
  FIO_ASSERT(fio_der_parse(&elem, test_context_0, sizeof(test_context_0)) &&
                 fio_der_is_context_tag(&elem, 0) &&
                 !fio_der_is_context_tag(&elem, 1) &&
                 fio_der_tag_number(&elem) == 0,
             "context tag helpers failed");
  FIO_ASSERT(fio_der_parse(&elem, test_int_42, sizeof(test_int_42)) &&
                 fio_der_element_total_len(&elem, test_int_42) ==
                     sizeof(test_int_42),
             "element_total_len failed");
}

FIO_SFUNC void FIO_NAME_TEST(stl, der_errors)(void) {
  fio_der_element_s elem;
  uint64_t value;
  int bool_val;

  FIO_ASSERT(!fio_der_parse(NULL, test_int_42, sizeof(test_int_42)),
             "NULL elem should fail");
  FIO_ASSERT(!fio_der_parse(&elem, NULL, 10), "NULL data should fail");
  FIO_ASSERT(!fio_der_parse(&elem, test_int_42, 0),
             "zero length should fail");
  FIO_ASSERT(!fio_der_parse(&elem, test_int_42, 1),
             "truncated data should fail");

  FIO_ASSERT(fio_der_parse(&elem, test_null, sizeof(test_null)) &&
                 fio_der_parse_integer(&elem, &value) != 0,
             "NULL as INTEGER should fail");
  FIO_ASSERT(fio_der_parse(&elem, test_null, sizeof(test_null)) &&
                 fio_der_parse_boolean(&elem, &bool_val) != 0,
             "NULL as BOOLEAN should fail");
}

FIO_SFUNC void FIO_NAME_TEST(stl, der_long_length)(void) {
  fio_der_element_s elem;
  uint8_t buf[260];

  buf[0] = 0x04;
  buf[1] = 0x82;
  buf[2] = 0x01;
  buf[3] = 0x00;
  FIO_MEMSET(buf + 4, 0xAB, 256);

  const uint8_t *next = fio_der_parse(&elem, buf, sizeof(buf));
  FIO_ASSERT(next, "long-form length parse failed");
  FIO_ASSERT((elem.tag & 0x1F) == FIO_DER_OCTET_STRING,
             "long-form tag mismatch");
  FIO_ASSERT(elem.len == 256, "long-form length mismatch");
  FIO_ASSERT(elem.data[0] == 0xAB, "long-form content mismatch");
}

/* *****************************************************************************
Encoding Round-Trip Tests
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, der_encode_length)(void) {
  uint8_t buf[8];

  FIO_ASSERT(fio_der_encode_length(buf, 50) == 1 && buf[0] == 50,
             "short-form length encode failed");
  FIO_ASSERT(fio_der_encode_length(buf, 200) == 2 && buf[0] == 0x81 &&
                 buf[1] == 200,
             "long-form (1 byte) length encode failed");
  FIO_ASSERT(fio_der_encode_length(buf, 300) == 3 && buf[0] == 0x82 &&
                 buf[1] == 0x01 && buf[2] == 0x2C,
             "long-form (2 byte) length encode failed");
}

FIO_SFUNC void FIO_NAME_TEST(stl, der_encode_integer)(void) {
  uint8_t buf[16];
  fio_der_element_s elem;
  uint64_t value;

  FIO_ASSERT(fio_der_encode_integer_small(buf, 42) == 3 && buf[0] == 0x02 &&
                 buf[1] == 0x01 && buf[2] == 42,
             "INTEGER(42) encode failed");
  FIO_ASSERT(fio_der_encode_integer_small(buf, 128) == 4 && buf[0] == 0x02 &&
                 buf[1] == 0x02 && buf[2] == 0x00 && buf[3] == 128,
             "INTEGER(128) encode failed");

  uint8_t big[8] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
  size_t len = fio_der_encode_integer(buf, big, sizeof(big));
  FIO_ASSERT(len > 0 && fio_der_parse(&elem, buf, len) &&
                 fio_der_parse_integer(&elem, &value) == 0 &&
                 value == 0x0123456789ABCDEFULL,
             "INTEGER round-trip failed");

}

FIO_SFUNC void FIO_NAME_TEST(stl, der_encode_oid)(void) {
  uint8_t buf[32];
  fio_der_element_s elem;
  char oid_buf[128];

  size_t len = fio___der_encode_oid(buf, FIO_X509_OID_SHA256_WITH_RSA);
  FIO_ASSERT(len == sizeof(test_oid_sha256_rsa) &&
                 !FIO_MEMCMP(buf, test_oid_sha256_rsa, len),
             "OID encode bytes mismatch");
  FIO_ASSERT(fio_der_parse(&elem, buf, len) &&
                 fio___der_oid_eq(fio___der_oid_value(&elem),
                                  FIO_X509_OID_SHA256_WITH_RSA),
             "OID encode round-trip failed");
  FIO_ASSERT(fio_der_parse_oid(&elem, oid_buf, sizeof(oid_buf)) > 0 &&
                 !FIO_MEMCMP(oid_buf, "1.2.840.113549.1.1.11", 21),
             "OID encode round-trip dot-string failed");

  len = fio___der_encode_oid(buf, FIO_X509_OID_ED25519);
  FIO_ASSERT(len > 0 && fio_der_parse(&elem, buf, len) &&
                 fio___der_oid_eq(fio___der_oid_value(&elem),
                                  FIO_X509_OID_ED25519),
             "Ed25519 OID encode round-trip failed");

  /* Invalid (never-match) value cannot be encoded */
  FIO_ASSERT(fio___der_encode_oid(buf, fio___der_oid_value(NULL)) == 0,
             "invalid OID value must not encode");
}

/* *****************************************************************************
OID Registry Round-Trip Test (T026)
*****************************************************************************

Every FIO_X509_OID_* constant is encoded to a TLV with fio___der_encode_oid,
parsed back with fio_der_parse_oid, and the resulting dot string must equal
the registry dot string (`bun ./ai-tools/oid-table.js table`). This catches
registry/define drift.
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, der_oid_registry_round_trip)(void) {
  const struct {
    fio_u128 oid;
    const char *dot;
  } registry[] = {
      {FIO_X509_OID_SHA256_WITH_RSA, "1.2.840.113549.1.1.11"},
      {FIO_X509_OID_SHA384_WITH_RSA, "1.2.840.113549.1.1.12"},
      {FIO_X509_OID_SHA512_WITH_RSA, "1.2.840.113549.1.1.13"},
      {FIO_X509_OID_RSA_PSS, "1.2.840.113549.1.1.10"},
      {FIO_X509_OID_MGF1, "1.2.840.113549.1.1.8"},
      {FIO_X509_OID_SHA256, "2.16.840.1.101.3.4.2.1"},
      {FIO_X509_OID_SHA384, "2.16.840.1.101.3.4.2.2"},
      {FIO_X509_OID_SHA512, "2.16.840.1.101.3.4.2.3"},
      {FIO_X509_OID_ECDSA_WITH_SHA256, "1.2.840.10045.4.3.2"},
      {FIO_X509_OID_ECDSA_WITH_SHA384, "1.2.840.10045.4.3.3"},
      {FIO_X509_OID_ECDSA_WITH_SHA512, "1.2.840.10045.4.3.4"},
      {FIO_X509_OID_ED25519, "1.3.101.112"},
      {FIO_X509_OID_ED448, "1.3.101.113"},
      {FIO_X509_OID_RSA_ENCRYPTION, "1.2.840.113549.1.1.1"},
      {FIO_X509_OID_EC_PUBLIC_KEY, "1.2.840.10045.2.1"},
      {FIO_X509_OID_SECP256R1, "1.2.840.10045.3.1.7"},
      {FIO_X509_OID_SECP384R1, "1.3.132.0.34"},
      {FIO_X509_OID_SECP521R1, "1.3.132.0.35"},
      {FIO_X509_OID_X25519, "1.3.101.110"},
      {FIO_X509_OID_X448, "1.3.101.111"},
      {FIO_X509_OID_SUBJECT_KEY_ID, "2.5.29.14"},
      {FIO_X509_OID_KEY_USAGE, "2.5.29.15"},
      {FIO_X509_OID_SUBJECT_ALT_NAME, "2.5.29.17"},
      {FIO_X509_OID_BASIC_CONSTRAINTS, "2.5.29.19"},
      {FIO_X509_OID_CRL_DIST_POINTS, "2.5.29.31"},
      {FIO_X509_OID_CERT_POLICIES, "2.5.29.32"},
      {FIO_X509_OID_AUTH_KEY_ID, "2.5.29.35"},
      {FIO_X509_OID_EXT_KEY_USAGE, "2.5.29.37"},
      {FIO_X509_OID_NAME_CONSTRAINTS, "2.5.29.30"},
      {FIO_X509_OID_EKU_ANY, "2.5.29.37.0"},
      {FIO_X509_OID_EKU_SERVER_AUTH, "1.3.6.1.5.5.7.3.1"},
      {FIO_X509_OID_EKU_CLIENT_AUTH, "1.3.6.1.5.5.7.3.2"},
      {FIO_X509_OID_COMMON_NAME, "2.5.4.3"},
      {FIO_X509_OID_COUNTRY, "2.5.4.6"},
      {FIO_X509_OID_LOCALITY, "2.5.4.7"},
      {FIO_X509_OID_STATE, "2.5.4.8"},
      {FIO_X509_OID_ORGANIZATION, "2.5.4.10"},
      {FIO_X509_OID_ORG_UNIT, "2.5.4.11"},
  };

  uint8_t buf[32];
  char dot_buf[128];

  for (size_t i = 0; i < sizeof(registry) / sizeof(registry[0]); ++i) {
    size_t len = fio___der_encode_oid(buf, registry[i].oid);
    FIO_ASSERT(len > 0, "registry OID encode failed");

    fio_der_element_s elem;
    FIO_ASSERT(fio_der_parse(&elem, buf, len), "registry OID reparse failed");

    /* Binary value round-trips exactly (u64-lane ==, no memcmp) */
    FIO_ASSERT(fio___der_oid_eq(fio___der_oid_value(&elem), registry[i].oid),
               "registry OID value round-trip mismatch");

    /* Dot string matches the registry entry */
    int dot_len = fio_der_parse_oid(&elem, dot_buf, sizeof(dot_buf));
    FIO_ASSERT(dot_len > 0, "registry OID stringify failed");
    FIO_ASSERT((size_t)dot_len == FIO_STRLEN(registry[i].dot) &&
                   !FIO_MEMCMP(dot_buf, registry[i].dot, (size_t)dot_len),
               "registry OID dot-string mismatch (registry drift?)");
  }
}

FIO_SFUNC void FIO_NAME_TEST(stl, der_encode_strings)(void) {
  uint8_t buf[64];
  fio_der_element_s elem;
  const char *str;
  size_t len;

  size_t enclen = fio_der_encode_utf8_string(buf, "localhost", 9);
  FIO_ASSERT(enclen > 0 && fio_der_parse(&elem, buf, enclen) &&
                 (str = fio_der_parse_string(&elem, &len)) && len == 9 &&
                 !FIO_MEMCMP(str, "localhost", 9),
             "UTF8String encode round-trip failed");

  enclen = fio_der_encode_printable_string(buf, "example", 7);
  FIO_ASSERT(enclen > 0 && fio_der_parse(&elem, buf, enclen) &&
                 (elem.tag & 0x1F) == FIO_DER_PRINTABLE_STRING &&
                 (str = fio_der_parse_string(&elem, &len)) && len == 7 &&
                 !FIO_MEMCMP(str, "example", 7),
             "PrintableString encode round-trip failed");

  uint8_t bits[4] = {0xDE, 0xAD, 0xBE, 0xEF};
  enclen = fio_der_encode_bit_string(buf, bits, 4, 0);
  FIO_ASSERT(enclen > 0 && fio_der_parse(&elem, buf, enclen) &&
                 (elem.tag & 0x1F) == FIO_DER_BIT_STRING,
             "BIT STRING encode round-trip failed");

  enclen = fio_der_encode_octet_string(buf, bits, 4);
  FIO_ASSERT(enclen > 0 && fio_der_parse(&elem, buf, enclen) &&
                 (elem.tag & 0x1F) == FIO_DER_OCTET_STRING &&
                 elem.len == 4 && !FIO_MEMCMP(elem.data, bits, 4),
             "OCTET STRING encode round-trip failed");
}

FIO_SFUNC void FIO_NAME_TEST(stl, der_encode_time)(void) {
  uint8_t buf[32];
  fio_der_element_s elem;
  int64_t ts;

  int64_t test_ts = 1705321845; /* 2024-01-15 12:30:45 UTC */
  size_t enclen = fio_der_encode_utc_time(buf, test_ts);
  FIO_ASSERT(enclen > 0 && fio_der_parse(&elem, buf, enclen) &&
                 fio_der_parse_time(&elem, &ts) == 0 && ts == test_ts,
             "UTCTime encode round-trip failed");

  enclen = fio_der_encode_generalized_time(buf, test_ts);
  FIO_ASSERT(enclen > 0 && fio_der_parse(&elem, buf, enclen) &&
                 fio_der_parse_time(&elem, &ts) == 0 && ts == test_ts,
             "GeneralizedTime encode round-trip failed");
}

FIO_SFUNC void FIO_NAME_TEST(stl, der_encode_headers)(void) {
  uint8_t buf[16];

  FIO_ASSERT(fio_der_encode_sequence_header(buf, 100) == 2 &&
                 buf[0] == 0x30 && buf[1] == 100,
             "SEQUENCE header encode failed");
  FIO_ASSERT(fio_der_encode_set_header(buf, 50) == 2 && buf[0] == 0x31 &&
                 buf[1] == 50,
             "SET header encode failed");
  FIO_ASSERT(fio_der_encode_context_header(buf, 0, 10, 1) == 2 &&
                 buf[0] == 0xA0 && buf[1] == 10,
             "context header encode failed");
  FIO_ASSERT(fio_der_encode_null(buf) == 2 && buf[0] == 0x05 && buf[1] == 0,
             "NULL encode failed");
  FIO_ASSERT(fio_der_encode_boolean(buf, 1) == 3 && buf[0] == 0x01 &&
                 buf[1] == 0x01 && buf[2] == 0xFF,
             "BOOLEAN encode failed");
}

/* Security regressions: bounded time parsing, OID arcs, canonical DER. */
FIO_SFUNC void FIO_NAME_TEST(stl, der_security)(void) {
  fio_der_element_s elem;
  int64_t ts;
  char oid[64];

  /* Time content ending in a digit (no 'Z') must not read past the element:
   * copy into an exact-size heap allocation so ASan catches any over-read. */
  {
    static const char *bad[] = {"2401151230",      "240115123045",
                                "2401151230Z1",    "2413151230Z",
                                "2402301230Z",     "2401152460Z",
                                "24011512 0Z",     "240115123045z",
                                "2401151230+01",   "2401151230+2400",
                                "2401151230.5Z",   "24011512Z "};
    for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); ++i) {
      size_t n = FIO_STRLEN(bad[i]);
      uint8_t *m = (uint8_t *)malloc(n + 2);
      FIO_ASSERT_ALLOC(m);
      m[0] = FIO_DER_UTC_TIME;
      m[1] = (uint8_t)n;
      FIO_MEMCPY(m + 2, bad[i], n);
      FIO_ASSERT(fio_der_parse(&elem, m, n + 2), "time TLV parse failed");
      FIO_ASSERT(fio_der_parse_time(&elem, &ts) == -1,
                 "malformed UTCTime accepted: %s",
                 bad[i]);
      free(m);
    }
    /* every truncation of a valid GeneralizedTime is rejected */
    static const char good[] = "20240229123045.123Z";
    for (size_t n = 0; n < sizeof(good) - 1; ++n) {
      uint8_t *m = (uint8_t *)malloc(n + 2);
      FIO_ASSERT_ALLOC(m);
      m[0] = FIO_DER_GENERALIZED_TIME;
      m[1] = (uint8_t)n;
      FIO_MEMCPY(m + 2, good, n);
      if (fio_der_parse(&elem, m, n + 2))
        FIO_ASSERT(fio_der_parse_time(&elem, &ts) == -1,
                   "truncated GeneralizedTime accepted (%zu)",
                   n);
      free(m);
    }
    /* lenient (BER, X.680) forms that decode unambiguously */
    static const struct {
      uint8_t tag;
      const char *s;
      int64_t t;
    } lenient[] = {
        {FIO_DER_UTC_TIME, "240115123045.5Z", 1705321845},
        {FIO_DER_UTC_TIME, "2401151230+0100", 1705318200},
        {FIO_DER_GENERALIZED_TIME, "20240115123045,5-0230", 1705330845},
        {FIO_DER_GENERALIZED_TIME, "2024011512Z", 1705320000},
        {FIO_DER_UTC_TIME, "24011512Z", 1705320000},
    };
    for (size_t i = 0; i < sizeof(lenient) / sizeof(lenient[0]); ++i) {
      size_t n = FIO_STRLEN(lenient[i].s);
      uint8_t *m = (uint8_t *)malloc(n + 2);
      FIO_ASSERT_ALLOC(m);
      m[0] = lenient[i].tag;
      m[1] = (uint8_t)n;
      FIO_MEMCPY(m + 2, lenient[i].s, n);
      FIO_ASSERT(fio_der_parse(&elem, m, n + 2) &&
                     !fio_der_parse_time(&elem, &ts) && ts == lenient[i].t,
                 "lenient time rejected or wrong: %s",
                 lenient[i].s);
      free(m);
    }
    uint8_t ok[2 + sizeof(good) - 1] = {FIO_DER_GENERALIZED_TIME,
                                        (uint8_t)(sizeof(good) - 1)};
    FIO_MEMCPY(ok + 2, good, sizeof(good) - 1);
    FIO_ASSERT(fio_der_parse(&elem, ok, sizeof(ok)) &&
                   !fio_der_parse_time(&elem, &ts) && ts == 1709209845,
               "valid leap-day GeneralizedTime rejected / wrong");
  }

  /* OID 2.999.3 has a multi-byte first subidentifier */
  {
    static const uint8_t o[] = {0x06, 0x03, 0x88, 0x37, 0x03};
    FIO_ASSERT(fio_der_parse(&elem, o, sizeof(o)) &&
                   fio_der_parse_oid(&elem, oid, sizeof(oid)) == 7 &&
                   !FIO_MEMCMP(oid, "2.999.3", 8),
               "OID 2.999.3 misdecoded: %s",
               oid);
    static const uint8_t big[] =
        {0x06, 0x0B, 0x2A, 0x82, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
         0x80, 0x00}; /* 1.2.(2^64) */
    FIO_ASSERT(fio_der_parse(&elem, big, sizeof(big)) &&
                   fio_der_parse_oid(&elem, oid, sizeof(oid)) == -1,
               "OID arc overflow accepted");
    static const uint8_t nonmin[] = {0x06, 0x03, 0x2A, 0x80, 0x01};
    FIO_ASSERT(fio_der_parse(&elem, nonmin, sizeof(nonmin)) &&
                   fio_der_parse_oid(&elem, oid, sizeof(oid)) == -1,
               "non-minimal OID arc accepted");
  }

  /* Lengths and tags: non-minimal forms decode (lenient), ambiguous fail */
  {
    static const uint8_t l1[] = {0x04, 0x81, 0x01, 0x00};
    static const uint8_t l2[] = {0x04, 0x82, 0x00, 0x01, 0x00};
    static const uint8_t l3[] = {0x04, 0x80, 0x00, 0x00}; /* indefinite */
    static const uint8_t t1[] = {0x9F, 0x82, 0x00, 0x00}; /* tag 256 */
    static const uint8_t t2[] = {0x1F, 0x02, 0x01, 0x05}; /* long INTEGER tag */
    static const uint8_t t3[] = {0x9F, 0x20, 0x00};       /* tag 32 */
    static const uint8_t t4[] = {0x9F, 0x80, 0x21, 0x00}; /* [33], zero pad */
    static const uint8_t t5[] = {0x9F, 0x81};             /* unterminated */
    uint64_t v = 0;
    FIO_ASSERT(fio_der_parse(&elem, l1, sizeof(l1)) == l1 + 4 && elem.len == 1,
               "non-minimal length should decode");
    FIO_ASSERT(fio_der_parse(&elem, l2, sizeof(l2)) == l2 + 5 && elem.len == 1,
               "leading-zero length should decode");
    FIO_ASSERT(!fio_der_parse(&elem, l3, sizeof(l3)), "indefinite length");
    FIO_ASSERT(!fio_der_parse(&elem, t1, sizeof(t1)), "tag > 255 truncated");
    FIO_ASSERT(fio_der_parse(&elem, t2, sizeof(t2)) &&
                   fio_der_is_tag(&elem, FIO_DER_INTEGER) &&
                   !fio_der_parse_integer(&elem, &v) && v == 5,
               "long-form INTEGER tag should normalize");
    FIO_ASSERT(fio_der_parse(&elem, t3, sizeof(t3)) && elem.tag_number == 32,
               "high-tag 32 rejected");
    FIO_ASSERT(fio_der_parse(&elem, t4, sizeof(t4)) &&
                   fio_der_is_context_tag(&elem, 33),
               "zero-padded high tag should decode");
    FIO_ASSERT(!fio_der_parse(&elem, t5, sizeof(t5)), "unterminated tag");
  }

  /* Full identifier matching: class + constructed bit + number */
  {
    static const uint8_t prim_seq[] = {0x10, 0x00};       /* primitive 0x10 */
    static const uint8_t cons_int[] = {0x22, 0x03, 0x02, 0x01, 0x05};
    static const uint8_t ctx_0[] = {0xA0, 0x00};          /* [0] */
    static const uint8_t seq[] = {0x30, 0x00};
    static const uint8_t set[] = {0x31, 0x00};
    static const uint8_t cons_bits[] = {0x23, 0x03, 0x03, 0x01, 0x00};
    static const uint8_t cons_oid[] = {0x26, 0x01, 0x2A};
    static const uint8_t cons_bool[] = {0x21, 0x01, 0xFF};
    static const uint8_t cons_utf8[] = {0x2C, 0x01, 'a'};
    static const uint8_t app_int[] = {0x42, 0x01, 0x05}; /* [APPLICATION 2] */
    static const uint8_t cons_time[] = {0x37, 0x0B, '2', '4', '0', '1', '1',
                                        '5', '1', '2', '3', '0', 'Z'};
    uint64_t u;
    int b;
    size_t sl;
    const uint8_t *bits;
    size_t bl;
    uint8_t ub;

    FIO_ASSERT(fio_der_parse(&elem, prim_seq, sizeof(prim_seq)) &&
                   !fio_der_is_tag(&elem, FIO_DER_SEQUENCE),
               "primitive 0x10 matched SEQUENCE");
    FIO_ASSERT(fio_der_parse(&elem, seq, sizeof(seq)) &&
                   fio_der_is_tag(&elem, FIO_DER_SEQUENCE) &&
                   fio_der_is_tag(&elem, 0x30) &&
                   !fio_der_is_tag(&elem, FIO_DER_SET),
               "SEQUENCE match failed");
    FIO_ASSERT(fio_der_parse(&elem, set, sizeof(set)) &&
                   fio_der_is_tag(&elem, FIO_DER_SET),
               "SET match failed");
    FIO_ASSERT(fio_der_parse(&elem, cons_int, sizeof(cons_int)) &&
                   !fio_der_is_tag(&elem, FIO_DER_INTEGER) &&
                   fio_der_parse_integer(&elem, &u) == -1,
               "constructed 0x22 accepted as INTEGER");
    FIO_ASSERT(fio_der_parse(&elem, app_int, sizeof(app_int)) &&
                   !fio_der_is_tag(&elem, FIO_DER_INTEGER) &&
                   fio_der_parse_integer(&elem, &u) == -1,
               "APPLICATION 2 accepted as INTEGER");
    FIO_ASSERT(fio_der_parse(&elem, ctx_0, sizeof(ctx_0)) &&
                   !fio_der_is_tag(&elem, FIO_DER_EOC) &&
                   fio_der_is_tag(&elem, FIO_DER_CONTEXT_0),
               "[0] matched universal EOC / [0] mismatch");
    FIO_ASSERT(fio_der_parse(&elem, cons_bits, sizeof(cons_bits)) &&
                   fio_der_parse_bit_string(&elem, &bits, &bl, &ub) == -1,
               "constructed BIT STRING accepted");
    FIO_ASSERT(fio_der_parse(&elem, cons_oid, sizeof(cons_oid)) &&
                   fio_der_parse_oid(&elem, oid, sizeof(oid)) == -1,
               "constructed OID accepted");
    FIO_ASSERT(fio_der_parse(&elem, cons_bool, sizeof(cons_bool)) &&
                   fio_der_parse_boolean(&elem, &b) == -1,
               "constructed BOOLEAN accepted");
    FIO_ASSERT(fio_der_parse(&elem, cons_utf8, sizeof(cons_utf8)) &&
                   !fio_der_parse_string(&elem, &sl),
               "constructed UTF8String accepted");
    FIO_ASSERT(fio_der_parse(&elem, cons_time, sizeof(cons_time)) &&
                   fio_der_parse_time(&elem, &ts) == -1,
               "constructed UTCTime accepted");

    /* context tags: is_context_tag is form-agnostic; is_tag is exact */
    static const uint8_t ctx_p2[] = {0x82, 0x01, 'a'}; /* [2] IMPLICIT prim */
    static const uint8_t ctx_hi[] = {0x9F, 0x21, 0x00}; /* [33] primitive */
    FIO_ASSERT(fio_der_parse(&elem, ctx_p2, sizeof(ctx_p2)) &&
                   fio_der_is_context_tag(&elem, 2) &&
                   fio_der_is_tag(&elem, 0x82) &&
                   !fio_der_is_tag(&elem, FIO_DER_CONTEXT_2) &&
                   fio_der_tag_number(&elem) == 2,
               "primitive [2] match failed");
    FIO_ASSERT(fio_der_parse(&elem, ctx_0, sizeof(ctx_0)) &&
                   fio_der_is_context_tag(&elem, 0) &&
                   !fio_der_is_tag(&elem, 0x80),
               "constructed [0] matched primitive [0]");
    FIO_ASSERT(fio_der_parse(&elem, ctx_hi, sizeof(ctx_hi)) &&
                   fio_der_is_context_tag(&elem, 33) &&
                   !fio_der_is_context_tag(&elem, 31) &&
                   !fio_der_is_context_tag(&elem, 1) &&
                   fio_der_tag_number(&elem) == 33,
               "high-tag context [33] match failed");
  }

  /* Time encoders: UTCTime range and GeneralizedTime past 2049 */
  {
    uint8_t buf[32];
    const int64_t y2050 = 2524608000LL; /* 2050-01-01T00:00:00Z */
    FIO_ASSERT(!fio_der_encode_utc_time(buf, y2050), "UTCTime 2050 encoded");
    FIO_ASSERT(fio_der_encode_utc_time(buf, y2050 - 1) == 15 &&
                   !FIO_MEMCMP(buf + 2, "491231235959Z", 13),
               "UTCTime 2049 boundary");
    size_t n = fio_der_encode_generalized_time(buf, y2050);
    FIO_ASSERT(n == 17 && !FIO_MEMCMP(buf + 2, "20500101000000Z", 15) &&
                   fio_der_parse(&elem, buf, n) &&
                   !fio_der_parse_time(&elem, &ts) && ts == y2050,
               "GeneralizedTime 2050 round-trip");
    n = fio_der_encode_generalized_time(buf, -1);
    FIO_ASSERT(n == 17 && !FIO_MEMCMP(buf + 2, "19691231235959Z", 15),
               "GeneralizedTime negative time");
    FIO_ASSERT(!fio_der_encode_generalized_time(buf, INT64_MAX) &&
                   !fio_der_encode_generalized_time(buf, INT64_MIN),
               "GeneralizedTime extreme range accepted");
  }
}

/* *****************************************************************************
Main
***************************************************************************** */

int main(void) {
  FIO_NAME_TEST(stl, der_parse_element)();
  FIO_NAME_TEST(stl, der_parse_integer)();
  FIO_NAME_TEST(stl, der_parse_boolean)();
  FIO_NAME_TEST(stl, der_parse_bit_string)();
  FIO_NAME_TEST(stl, der_parse_oid)();
  FIO_NAME_TEST(stl, der_parse_string)();
  FIO_NAME_TEST(stl, der_parse_time)();
  FIO_NAME_TEST(stl, der_iterator)();
  FIO_NAME_TEST(stl, der_nested)();
  FIO_NAME_TEST(stl, der_helpers)();
  FIO_NAME_TEST(stl, der_errors)();
  FIO_NAME_TEST(stl, der_long_length)();
  FIO_NAME_TEST(stl, der_encode_length)();
  FIO_NAME_TEST(stl, der_encode_integer)();
  FIO_NAME_TEST(stl, der_encode_oid)();
  FIO_NAME_TEST(stl, der_oid_registry_round_trip)();
  FIO_NAME_TEST(stl, der_encode_strings)();
  FIO_NAME_TEST(stl, der_encode_time)();
  FIO_NAME_TEST(stl, der_encode_headers)();
  FIO_NAME_TEST(stl, der_security)();
  return 0;
}
