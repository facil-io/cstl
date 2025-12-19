/* *****************************************************************************
ASN.1 DER Parser Tests
***************************************************************************** */
#include "test-helpers.h"

#define FIO_ASN1
#include FIO_INCLUDE_FILE

/* *****************************************************************************
Test Data - DER Encoded Test Vectors
***************************************************************************** */

/* Simple SEQUENCE with INTEGER(42) and NULL */
static const uint8_t test_seq_int_null[] = {
    0x30,
    0x05, /* SEQUENCE, length 5 */
    0x02,
    0x01,
    0x2A, /* INTEGER, length 1, value 42 */
    0x05,
    0x00 /* NULL, length 0 */
};

/* INTEGER test vectors */
static const uint8_t test_int_42[] = {0x02, 0x01, 0x2A};  /* 42 */
static const uint8_t test_int_127[] = {0x02, 0x01, 0x7F}; /* 127 */
static const uint8_t test_int_128[] = {0x02,
                                       0x02,
                                       0x00,
                                       0x80}; /* 128 (needs leading 0) */
static const uint8_t test_int_256[] = {0x02, 0x02, 0x01, 0x00}; /* 256 */
/* Note: We don't test negative integers since they're uncommon in X.509 */
static const uint8_t test_int_large[] = {
    0x02,
    0x08, /* INTEGER, length 8 */
    0x01,
    0x23,
    0x45,
    0x67,
    0x89,
    0xAB,
    0xCD,
    0xEF /* 0x0123456789ABCDEF */
};

/* BOOLEAN test vectors */
static const uint8_t test_bool_true[] = {0x01, 0x01, 0xFF};
static const uint8_t test_bool_false[] = {0x01, 0x01, 0x00};

/* NULL test vector */
static const uint8_t test_null[] = {0x05, 0x00};

/* BIT STRING test vectors */
static const uint8_t test_bitstring[] = {
    0x03,
    0x04, /* BIT STRING, length 4 */
    0x06, /* 6 unused bits */
    0x6E,
    0x5D,
    0xC0 /* data bytes (last byte has 6 bits unused) */
};
static const uint8_t test_bitstring_empty[] = {0x03,
                                               0x01,
                                               0x00}; /* empty bit string */

/* OCTET STRING is tested implicitly via BIT STRING and other types */

/* OID test vectors */
/* SHA-256 with RSA: 1.2.840.113549.1.1.11 */
static const uint8_t test_oid_sha256_rsa[] = {0x06,
                                              0x09, /* OID, length 9 */
                                              0x2A,
                                              0x86,
                                              0x48,
                                              0x86,
                                              0xF7,
                                              0x0D,
                                              0x01,
                                              0x01,
                                              0x0B};

/* secp256r1 (P-256): 1.2.840.10045.3.1.7 */
static const uint8_t test_oid_secp256r1[] = {0x06,
                                             0x08, /* OID, length 8 */
                                             0x2A,
                                             0x86,
                                             0x48,
                                             0xCE,
                                             0x3D,
                                             0x03,
                                             0x01,
                                             0x07};

/* Common Name: 2.5.4.3 */
static const uint8_t test_oid_cn[] = {
    0x06,
    0x03, /* OID, length 3 */
    0x55,
    0x04,
    0x03 /* 2.5.4.3 */
};

/* Ed25519: 1.3.101.112 */
static const uint8_t test_oid_ed25519[] = {
    0x06,
    0x03, /* OID, length 3 */
    0x2B,
    0x65,
    0x70 /* 1.3.101.112 */
};

/* UTF8String test vector */
static const uint8_t test_utf8_string[] = {0x0C,
                                           0x0B, /* UTF8String, length 11 */
                                           'H',
                                           'e',
                                           'l',
                                           'l',
                                           'o',
                                           ' ',
                                           'W',
                                           'o',
                                           'r',
                                           'l',
                                           'd'};

/* PrintableString test vector */
static const uint8_t test_printable_string[] = {
    0x13,
    0x07, /* PrintableString, length 7 */
    'e',
    'x',
    'a',
    'm',
    'p',
    'l',
    'e'};

/* UTC Time test vectors */
/* 2023-12-25 12:30:45Z */
static const uint8_t test_utc_time[] = {0x17,
                                        0x0D, /* UTCTime, length 13 */
                                        '2',
                                        '3',
                                        '1',
                                        '2',
                                        '2',
                                        '5',
                                        '1',
                                        '2',
                                        '3',
                                        '0',
                                        '4',
                                        '5',
                                        'Z'};

/* Generalized Time test vector */
/* 2023-12-25 12:30:45Z */
static const uint8_t test_gen_time[] = {0x18,
                                        0x0F, /* GeneralizedTime, length 15 */
                                        '2',
                                        '0',
                                        '2',
                                        '3',
                                        '1',
                                        '2',
                                        '2',
                                        '5',
                                        '1',
                                        '2',
                                        '3',
                                        '0',
                                        '4',
                                        '5',
                                        'Z'};

/* Context-specific tags */
static const uint8_t test_context_0[] = {
    0xA0,
    0x03, /* [0] CONSTRUCTED, length 3 */
    0x02,
    0x01,
    0x03 /* INTEGER 3 (version) */
};

/* Nested SEQUENCE for X.509-like structure
 * Structure:
 *   SEQUENCE (outer)
 *     SEQUENCE (AlgorithmIdentifier) = OID(5) + NULL(2) = 7 bytes
 *     SEQUENCE (Name) = SET(14) = 14 bytes
 *       SET = RDN SEQUENCE(12) = 12 bytes
 *         SEQUENCE (RDN) = OID(5) + UTF8(5) = 10 bytes
 * Total outer content: (2+7) + (2+14) = 25 bytes
 */
static const uint8_t test_nested_seq[] = {
    0x30, 0x19,                   /* SEQUENCE, length 25 */
    0x30, 0x07,                   /* SEQUENCE (AlgorithmIdentifier), length 7 */
    0x06, 0x03, 0x55, 0x04, 0x03, /* OID 2.5.4.3 (5 bytes) */
    0x05, 0x00,                   /* NULL params (2 bytes) */
    0x30, 0x0E,                   /* SEQUENCE (Name), length 14 */
    0x31, 0x0C,                   /* SET, length 12 */
    0x30, 0x0A,                   /* SEQUENCE (RDN), length 10 */
    0x06, 0x03, 0x55, 0x04, 0x03, /* OID 2.5.4.3 */
    0x0C, 0x03, 'f',  'o',  'o'   /* UTF8String "foo" */
};

/* Note: Long-form length is tested in fio___test_asn1_long_length */

/* *****************************************************************************
Basic Element Parsing Tests
***************************************************************************** */

FIO_SFUNC void fio___test_asn1_parse_element(void) {
  fio_asn1_element_s elem;
  const uint8_t *next;

  FIO_LOG_DDEBUG("Testing ASN.1 element parsing...");

  /* Test INTEGER parsing */
  next = fio_asn1_parse(&elem, test_int_42, sizeof(test_int_42));
  FIO_ASSERT(next != NULL, "Failed to parse INTEGER(42)");
  FIO_ASSERT(elem.tag == 0x02, "Wrong tag for INTEGER: 0x%02X", elem.tag);
  FIO_ASSERT(elem.tag_class == FIO_ASN1_CLASS_UNIVERSAL,
             "Wrong tag class for INTEGER");
  FIO_ASSERT(elem.is_constructed == 0, "INTEGER should not be constructed");
  FIO_ASSERT(elem.len == 1, "Wrong length for INTEGER(42): %zu", elem.len);
  FIO_ASSERT(elem.data[0] == 0x2A, "Wrong data for INTEGER(42)");
  FIO_ASSERT(next == test_int_42 + sizeof(test_int_42),
             "Wrong next pointer for INTEGER(42)");

  /* Test NULL parsing */
  next = fio_asn1_parse(&elem, test_null, sizeof(test_null));
  FIO_ASSERT(next != NULL, "Failed to parse NULL");
  FIO_ASSERT((elem.tag & 0x1F) == FIO_ASN1_NULL, "Wrong tag for NULL");
  FIO_ASSERT(elem.len == 0, "Wrong length for NULL");

  /* Test BOOLEAN parsing */
  next = fio_asn1_parse(&elem, test_bool_true, sizeof(test_bool_true));
  FIO_ASSERT(next != NULL, "Failed to parse BOOLEAN TRUE");
  FIO_ASSERT((elem.tag & 0x1F) == FIO_ASN1_BOOLEAN, "Wrong tag for BOOLEAN");
  FIO_ASSERT(elem.len == 1, "Wrong length for BOOLEAN");
  FIO_ASSERT(elem.data[0] == 0xFF, "Wrong value for BOOLEAN TRUE");

  /* Test SEQUENCE parsing */
  next = fio_asn1_parse(&elem, test_seq_int_null, sizeof(test_seq_int_null));
  FIO_ASSERT(next != NULL, "Failed to parse SEQUENCE");
  FIO_ASSERT((elem.tag & 0x1F) == FIO_ASN1_SEQUENCE, "Wrong tag for SEQUENCE");
  FIO_ASSERT(elem.is_constructed == 1, "SEQUENCE should be constructed");
  FIO_ASSERT(elem.len == 5, "Wrong length for SEQUENCE");

  /* Test context-specific tag */
  next = fio_asn1_parse(&elem, test_context_0, sizeof(test_context_0));
  FIO_ASSERT(next != NULL, "Failed to parse [0] context tag");
  FIO_ASSERT(elem.tag_class == FIO_ASN1_CLASS_CONTEXT,
             "Wrong tag class for context tag");
  FIO_ASSERT((elem.tag & 0x1F) == 0, "Wrong tag number for [0]");
  FIO_ASSERT(elem.is_constructed == 1, "[0] should be constructed");

  FIO_LOG_DDEBUG("ASN.1 element parsing tests passed");
}

/* *****************************************************************************
Integer Parsing Tests
***************************************************************************** */

FIO_SFUNC void fio___test_asn1_parse_integer(void) {
  fio_asn1_element_s elem;
  uint64_t value;

  FIO_LOG_DDEBUG("Testing ASN.1 INTEGER parsing...");

  /* Test small positive integer */
  FIO_ASSERT(fio_asn1_parse(&elem, test_int_42, sizeof(test_int_42)),
             "Failed to parse INTEGER(42)");
  FIO_ASSERT(fio_asn1_parse_integer(&elem, &value) == 0,
             "Failed to extract INTEGER(42) value");
  FIO_ASSERT(value == 42, "Wrong value for INTEGER(42): %" PRIu64, value);

  /* Test 127 (max single byte positive) */
  FIO_ASSERT(fio_asn1_parse(&elem, test_int_127, sizeof(test_int_127)),
             "Failed to parse INTEGER(127)");
  FIO_ASSERT(fio_asn1_parse_integer(&elem, &value) == 0,
             "Failed to extract INTEGER(127) value");
  FIO_ASSERT(value == 127, "Wrong value for INTEGER(127)");

  /* Test 128 (requires leading zero byte) */
  FIO_ASSERT(fio_asn1_parse(&elem, test_int_128, sizeof(test_int_128)),
             "Failed to parse INTEGER(128)");
  FIO_ASSERT(fio_asn1_parse_integer(&elem, &value) == 0,
             "Failed to extract INTEGER(128) value");
  FIO_ASSERT(value == 128, "Wrong value for INTEGER(128): %" PRIu64, value);

  /* Test 256 */
  FIO_ASSERT(fio_asn1_parse(&elem, test_int_256, sizeof(test_int_256)),
             "Failed to parse INTEGER(256)");
  FIO_ASSERT(fio_asn1_parse_integer(&elem, &value) == 0,
             "Failed to extract INTEGER(256) value");
  FIO_ASSERT(value == 256, "Wrong value for INTEGER(256)");

  /* Test large integer (8 bytes) */
  FIO_ASSERT(fio_asn1_parse(&elem, test_int_large, sizeof(test_int_large)),
             "Failed to parse large INTEGER");
  FIO_ASSERT(fio_asn1_parse_integer(&elem, &value) == 0,
             "Failed to extract large INTEGER value");
  FIO_ASSERT(value == 0x0123456789ABCDEFULL,
             "Wrong value for large INTEGER: 0x%" PRIX64,
             value);

  FIO_LOG_DDEBUG("ASN.1 INTEGER parsing tests passed");
}

/* *****************************************************************************
Boolean Parsing Tests
***************************************************************************** */

FIO_SFUNC void fio___test_asn1_parse_boolean(void) {
  fio_asn1_element_s elem;
  int value;

  FIO_LOG_DDEBUG("Testing ASN.1 BOOLEAN parsing...");

  /* Test TRUE */
  FIO_ASSERT(fio_asn1_parse(&elem, test_bool_true, sizeof(test_bool_true)),
             "Failed to parse BOOLEAN TRUE");
  FIO_ASSERT(fio_asn1_parse_boolean(&elem, &value) == 0,
             "Failed to extract BOOLEAN TRUE");
  FIO_ASSERT(value != 0, "BOOLEAN TRUE should be non-zero");

  /* Test FALSE */
  FIO_ASSERT(fio_asn1_parse(&elem, test_bool_false, sizeof(test_bool_false)),
             "Failed to parse BOOLEAN FALSE");
  FIO_ASSERT(fio_asn1_parse_boolean(&elem, &value) == 0,
             "Failed to extract BOOLEAN FALSE");
  FIO_ASSERT(value == 0, "BOOLEAN FALSE should be zero");

  FIO_LOG_DDEBUG("ASN.1 BOOLEAN parsing tests passed");
}

/* *****************************************************************************
Bit String Parsing Tests
***************************************************************************** */

FIO_SFUNC void fio___test_asn1_parse_bit_string(void) {
  fio_asn1_element_s elem;
  const uint8_t *bits;
  size_t bit_len;
  uint8_t unused_bits;

  FIO_LOG_DDEBUG("Testing ASN.1 BIT STRING parsing...");

  /* Test non-empty bit string */
  FIO_ASSERT(fio_asn1_parse(&elem, test_bitstring, sizeof(test_bitstring)),
             "Failed to parse BIT STRING");
  FIO_ASSERT(fio_asn1_parse_bit_string(&elem, &bits, &bit_len, &unused_bits) ==
                 0,
             "Failed to extract BIT STRING");
  FIO_ASSERT(unused_bits == 6, "Wrong unused bits: %u", unused_bits);
  FIO_ASSERT(bit_len == 3, "Wrong bit length: %zu", bit_len);
  FIO_ASSERT(bits[0] == 0x6E && bits[1] == 0x5D && bits[2] == 0xC0,
             "Wrong bit data");

  /* Test empty bit string */
  FIO_ASSERT(
      fio_asn1_parse(&elem, test_bitstring_empty, sizeof(test_bitstring_empty)),
      "Failed to parse empty BIT STRING");
  FIO_ASSERT(fio_asn1_parse_bit_string(&elem, &bits, &bit_len, &unused_bits) ==
                 0,
             "Failed to extract empty BIT STRING");
  FIO_ASSERT(bit_len == 0, "Empty BIT STRING should have zero length");
  FIO_ASSERT(unused_bits == 0, "Empty BIT STRING should have zero unused bits");

  FIO_LOG_DDEBUG("ASN.1 BIT STRING parsing tests passed");
}

/* *****************************************************************************
OID Parsing Tests
***************************************************************************** */

FIO_SFUNC void fio___test_asn1_parse_oid(void) {
  fio_asn1_element_s elem;
  char oid_buf[128];
  int len;

  FIO_LOG_DDEBUG("Testing ASN.1 OID parsing...");

  /* Test SHA-256 with RSA OID: 1.2.840.113549.1.1.11 */
  FIO_ASSERT(
      fio_asn1_parse(&elem, test_oid_sha256_rsa, sizeof(test_oid_sha256_rsa)),
      "Failed to parse SHA256-RSA OID");
  len = fio_asn1_parse_oid(&elem, oid_buf, sizeof(oid_buf));
  FIO_ASSERT(len > 0, "Failed to convert OID to string");
  FIO_ASSERT(FIO_MEMCMP(oid_buf, FIO_OID_SHA256_WITH_RSA, len) == 0,
             "Wrong OID string: %s (expected %s)",
             oid_buf,
             FIO_OID_SHA256_WITH_RSA);

  /* Test with fio_asn1_oid_eq */
  FIO_ASSERT(fio_asn1_oid_eq(&elem, FIO_OID_SHA256_WITH_RSA),
             "OID comparison failed for SHA256-RSA");
  FIO_ASSERT(!fio_asn1_oid_eq(&elem, FIO_OID_ED25519),
             "OID comparison should fail for different OID");

  /* Test secp256r1 OID: 1.2.840.10045.3.1.7 */
  FIO_ASSERT(
      fio_asn1_parse(&elem, test_oid_secp256r1, sizeof(test_oid_secp256r1)),
      "Failed to parse secp256r1 OID");
  FIO_ASSERT(fio_asn1_oid_eq(&elem, FIO_OID_SECP256R1),
             "OID comparison failed for secp256r1");

  /* Test Common Name OID: 2.5.4.3 */
  FIO_ASSERT(fio_asn1_parse(&elem, test_oid_cn, sizeof(test_oid_cn)),
             "Failed to parse CN OID");
  len = fio_asn1_parse_oid(&elem, oid_buf, sizeof(oid_buf));
  FIO_ASSERT(len > 0, "Failed to convert CN OID to string");
  FIO_ASSERT(FIO_MEMCMP(oid_buf, FIO_OID_COMMON_NAME, len) == 0,
             "Wrong CN OID string: %s",
             oid_buf);

  /* Test Ed25519 OID: 1.3.101.112 */
  FIO_ASSERT(fio_asn1_parse(&elem, test_oid_ed25519, sizeof(test_oid_ed25519)),
             "Failed to parse Ed25519 OID");
  FIO_ASSERT(fio_asn1_oid_eq(&elem, FIO_OID_ED25519),
             "OID comparison failed for Ed25519");

  FIO_LOG_DDEBUG("ASN.1 OID parsing tests passed");
}

/* *****************************************************************************
String Parsing Tests
***************************************************************************** */

FIO_SFUNC void fio___test_asn1_parse_string(void) {
  fio_asn1_element_s elem;
  const char *str;
  size_t len;

  FIO_LOG_DDEBUG("Testing ASN.1 string parsing...");

  /* Test UTF8String */
  FIO_ASSERT(fio_asn1_parse(&elem, test_utf8_string, sizeof(test_utf8_string)),
             "Failed to parse UTF8String");
  str = fio_asn1_parse_string(&elem, &len);
  FIO_ASSERT(str != NULL, "Failed to extract UTF8String");
  FIO_ASSERT(len == 11, "Wrong UTF8String length: %zu", len);
  FIO_ASSERT(FIO_MEMCMP(str, "Hello World", 11) == 0,
             "Wrong UTF8String content");

  /* Test PrintableString */
  FIO_ASSERT(fio_asn1_parse(&elem,
                            test_printable_string,
                            sizeof(test_printable_string)),
             "Failed to parse PrintableString");
  str = fio_asn1_parse_string(&elem, &len);
  FIO_ASSERT(str != NULL, "Failed to extract PrintableString");
  FIO_ASSERT(len == 7, "Wrong PrintableString length");
  FIO_ASSERT(FIO_MEMCMP(str, "example", 7) == 0,
             "Wrong PrintableString content");

  FIO_LOG_DDEBUG("ASN.1 string parsing tests passed");
}

/* *****************************************************************************
Time Parsing Tests
***************************************************************************** */

FIO_SFUNC void fio___test_asn1_parse_time(void) {
  fio_asn1_element_s elem;
  int64_t unix_time;

  FIO_LOG_DDEBUG("Testing ASN.1 time parsing...");

  /* Test UTC Time: 231225123045Z = 2023-12-25 12:30:45 UTC */
  FIO_ASSERT(fio_asn1_parse(&elem, test_utc_time, sizeof(test_utc_time)),
             "Failed to parse UTCTime");
  FIO_ASSERT(fio_asn1_parse_time(&elem, &unix_time) == 0,
             "Failed to extract UTCTime");
  /* 2023-12-25 12:30:45 UTC should be around 1703507445 */
  FIO_ASSERT(unix_time > 1703500000 && unix_time < 1703510000,
             "UTCTime out of expected range: %" PRId64,
             unix_time);

  /* Test Generalized Time: 20231225123045Z */
  FIO_ASSERT(fio_asn1_parse(&elem, test_gen_time, sizeof(test_gen_time)),
             "Failed to parse GeneralizedTime");
  int64_t gen_unix_time;
  FIO_ASSERT(fio_asn1_parse_time(&elem, &gen_unix_time) == 0,
             "Failed to extract GeneralizedTime");
  /* Should be same as UTC time */
  FIO_ASSERT(gen_unix_time == unix_time,
             "GeneralizedTime != UTCTime: %" PRId64 " vs %" PRId64,
             gen_unix_time,
             unix_time);

  FIO_LOG_DDEBUG("ASN.1 time parsing tests passed");
}

/* *****************************************************************************
Iterator Tests
***************************************************************************** */

FIO_SFUNC void fio___test_asn1_iterator(void) {
  fio_asn1_element_s seq_elem, elem;
  fio_asn1_iterator_s it;
  uint64_t int_value;

  FIO_LOG_DDEBUG("Testing ASN.1 iterator...");

  /* Parse SEQUENCE containing INTEGER(42) and NULL */
  FIO_ASSERT(
      fio_asn1_parse(&seq_elem, test_seq_int_null, sizeof(test_seq_int_null)),
      "Failed to parse SEQUENCE");

  /* Initialize iterator */
  fio_asn1_iterator_init(&it, &seq_elem);
  FIO_ASSERT(fio_asn1_iterator_has_next(&it), "Iterator should have elements");

  /* Get first element (INTEGER) */
  FIO_ASSERT(fio_asn1_iterator_next(&it, &elem) == 0,
             "Failed to get first element");
  FIO_ASSERT(fio_asn1_is_tag(&elem, FIO_ASN1_INTEGER),
             "First element should be INTEGER");
  FIO_ASSERT(fio_asn1_parse_integer(&elem, &int_value) == 0,
             "Failed to parse INTEGER");
  FIO_ASSERT(int_value == 42, "Wrong INTEGER value: %" PRIu64, int_value);

  /* Get second element (NULL) */
  FIO_ASSERT(fio_asn1_iterator_has_next(&it),
             "Iterator should have more elements");
  FIO_ASSERT(fio_asn1_iterator_next(&it, &elem) == 0,
             "Failed to get second element");
  FIO_ASSERT(fio_asn1_is_tag(&elem, FIO_ASN1_NULL),
             "Second element should be NULL");

  /* No more elements */
  FIO_ASSERT(!fio_asn1_iterator_has_next(&it), "Iterator should be exhausted");
  FIO_ASSERT(fio_asn1_iterator_next(&it, &elem) != 0,
             "Should fail to get element from exhausted iterator");

  FIO_LOG_DDEBUG("ASN.1 iterator tests passed");
}

/* *****************************************************************************
Nested Structure Tests
***************************************************************************** */

FIO_SFUNC void fio___test_asn1_nested(void) {
  fio_asn1_element_s outer, inner, elem;
  fio_asn1_iterator_s it_outer, it_inner;

  FIO_LOG_DDEBUG("Testing ASN.1 nested structures...");

  /* Parse outer SEQUENCE */
  FIO_ASSERT(fio_asn1_parse(&outer, test_nested_seq, sizeof(test_nested_seq)),
             "Failed to parse outer SEQUENCE");
  FIO_ASSERT(fio_asn1_is_tag(&outer, FIO_ASN1_SEQUENCE),
             "Outer element should be SEQUENCE");

  fio_asn1_iterator_init(&it_outer, &outer);

  /* First inner element: AlgorithmIdentifier SEQUENCE */
  FIO_ASSERT(fio_asn1_iterator_next(&it_outer, &inner) == 0,
             "Failed to get AlgorithmIdentifier");
  FIO_ASSERT(fio_asn1_is_tag(&inner, FIO_ASN1_SEQUENCE),
             "AlgorithmIdentifier should be SEQUENCE");

  fio_asn1_iterator_init(&it_inner, &inner);

  /* Get OID from AlgorithmIdentifier */
  FIO_ASSERT(fio_asn1_iterator_next(&it_inner, &elem) == 0,
             "Failed to get OID from AlgorithmIdentifier");
  FIO_ASSERT(fio_asn1_is_tag(&elem, FIO_ASN1_OID), "Should be OID");
  FIO_ASSERT(fio_asn1_oid_eq(&elem, FIO_OID_COMMON_NAME),
             "OID should be 2.5.4.3");

  /* Get NULL params */
  FIO_ASSERT(fio_asn1_iterator_next(&it_inner, &elem) == 0,
             "Failed to get NULL params");
  FIO_ASSERT(fio_asn1_is_tag(&elem, FIO_ASN1_NULL), "Should be NULL");

  /* Second inner element: Name SEQUENCE */
  FIO_ASSERT(fio_asn1_iterator_next(&it_outer, &inner) == 0,
             "Failed to get Name SEQUENCE");
  FIO_ASSERT(fio_asn1_is_tag(&inner, FIO_ASN1_SEQUENCE),
             "Name should be SEQUENCE");

  FIO_LOG_DDEBUG("ASN.1 nested structure tests passed");
}

/* *****************************************************************************
Helper Function Tests
***************************************************************************** */

FIO_SFUNC void fio___test_asn1_helpers(void) {
  fio_asn1_element_s elem;

  FIO_LOG_DDEBUG("Testing ASN.1 helper functions...");

  /* Test fio_asn1_is_tag */
  FIO_ASSERT(fio_asn1_parse(&elem, test_int_42, sizeof(test_int_42)),
             "Failed to parse INTEGER");
  FIO_ASSERT(fio_asn1_is_tag(&elem, FIO_ASN1_INTEGER),
             "fio_asn1_is_tag failed for INTEGER");
  FIO_ASSERT(!fio_asn1_is_tag(&elem, FIO_ASN1_BOOLEAN),
             "fio_asn1_is_tag should fail for wrong tag");

  /* Test fio_asn1_is_context_tag */
  FIO_ASSERT(fio_asn1_parse(&elem, test_context_0, sizeof(test_context_0)),
             "Failed to parse context tag");
  FIO_ASSERT(fio_asn1_is_context_tag(&elem, 0),
             "fio_asn1_is_context_tag failed for [0]");
  FIO_ASSERT(!fio_asn1_is_context_tag(&elem, 1),
             "fio_asn1_is_context_tag should fail for [1]");

  /* Test fio_asn1_tag_number */
  FIO_ASSERT(fio_asn1_tag_number(&elem) == 0,
             "fio_asn1_tag_number wrong for [0]");

  /* Test fio_asn1_element_total_len */
  FIO_ASSERT(fio_asn1_parse(&elem, test_int_42, sizeof(test_int_42)),
             "Failed to parse INTEGER");
  size_t total = fio_asn1_element_total_len(&elem, test_int_42);
  FIO_ASSERT(total == sizeof(test_int_42),
             "Wrong total length: %zu vs %zu",
             total,
             sizeof(test_int_42));

  FIO_LOG_DDEBUG("ASN.1 helper function tests passed");
}

/* *****************************************************************************
Error Handling Tests
***************************************************************************** */

FIO_SFUNC void fio___test_asn1_errors(void) {
  fio_asn1_element_s elem;
  uint64_t value;
  int bool_val;

  FIO_LOG_DDEBUG("Testing ASN.1 error handling...");

  /* Test NULL inputs */
  FIO_ASSERT(fio_asn1_parse(NULL, test_int_42, sizeof(test_int_42)) == NULL,
             "Should fail with NULL elem");
  FIO_ASSERT(fio_asn1_parse(&elem, NULL, 10) == NULL,
             "Should fail with NULL data");
  FIO_ASSERT(fio_asn1_parse(&elem, test_int_42, 0) == NULL,
             "Should fail with zero length");

  /* Test type mismatches */
  FIO_ASSERT(fio_asn1_parse(&elem, test_null, sizeof(test_null)),
             "Failed to parse NULL");
  FIO_ASSERT(fio_asn1_parse_integer(&elem, &value) != 0,
             "Should fail to parse NULL as INTEGER");
  FIO_ASSERT(fio_asn1_parse_boolean(&elem, &bool_val) != 0,
             "Should fail to parse NULL as BOOLEAN");

  /* Test truncated data */
  FIO_ASSERT(fio_asn1_parse(&elem, test_int_42, 1) == NULL,
             "Should fail on truncated data");

  FIO_LOG_DDEBUG("ASN.1 error handling tests passed");
}

/* *****************************************************************************
Long Form Length Tests
***************************************************************************** */

FIO_SFUNC void fio___test_asn1_long_length(void) {
  fio_asn1_element_s elem;

  FIO_LOG_DDEBUG("Testing ASN.1 long-form length encoding...");

  /* Create a buffer with long-form length and enough content */
  uint8_t buf[260];
  buf[0] = 0x04;                  /* OCTET STRING */
  buf[1] = 0x82;                  /* Long form: 2 bytes of length */
  buf[2] = 0x01;                  /* Length high byte */
  buf[3] = 0x00;                  /* Length low byte = 256 */
  FIO_MEMSET(buf + 4, 0xAB, 256); /* 256 bytes of content */

  const uint8_t *next = fio_asn1_parse(&elem, buf, sizeof(buf));
  FIO_ASSERT(next != NULL, "Failed to parse long-form length element");
  FIO_ASSERT((elem.tag & 0x1F) == FIO_ASN1_OCTET_STRING,
             "Wrong tag for OCTET STRING");
  FIO_ASSERT(elem.len == 256, "Wrong length for long-form: %zu", elem.len);
  FIO_ASSERT(elem.data[0] == 0xAB, "Wrong content byte");

  FIO_LOG_DDEBUG("ASN.1 long-form length tests passed");
}

/* *****************************************************************************
Main Test Runner
***************************************************************************** */

int main(void) {
  FIO_LOG_DDEBUG("Testing ASN.1 DER Parser");

  fio___test_asn1_parse_element();
  fio___test_asn1_parse_integer();
  fio___test_asn1_parse_boolean();
  fio___test_asn1_parse_bit_string();
  fio___test_asn1_parse_oid();
  fio___test_asn1_parse_string();
  fio___test_asn1_parse_time();
  fio___test_asn1_iterator();
  fio___test_asn1_nested();
  fio___test_asn1_helpers();
  fio___test_asn1_errors();
  fio___test_asn1_long_length();

  FIO_LOG_DDEBUG("All ASN.1 tests passed!");
  return 0;
}
