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
  const uint8_t *bits;
  size_t bit_len;
  uint8_t unused;

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
  int64_t utc_ts;
  int64_t gen_ts;

  FIO_ASSERT(fio_der_parse(&elem, test_utc_time, sizeof(test_utc_time)) &&
                 fio_der_parse_time(&elem, &utc_ts) == 0,
             "UTCTime parse failed");
  FIO_ASSERT(utc_ts > 1703500000 && utc_ts < 1703510000,
             "UTCTime value out of range");

  FIO_ASSERT(fio_der_parse(&elem, test_gen_time, sizeof(test_gen_time)) &&
                 fio_der_parse_time(&elem, &gen_ts) == 0 && gen_ts == utc_ts,
             "GeneralizedTime != UTCTime");
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
  static const struct {
    fio_u128 oid;
    const char *dot;
  } registry[] = {
      {FIO_X509_OID_SHA256_WITH_RSA, "1.2.840.113549.1.1.11"},
      {FIO_X509_OID_SHA384_WITH_RSA, "1.2.840.113549.1.1.12"},
      {FIO_X509_OID_SHA512_WITH_RSA, "1.2.840.113549.1.1.13"},
      {FIO_X509_OID_RSA_PSS, "1.2.840.113549.1.1.10"},
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
  return 0;
}
