/**
 * Detailed X.509 debug test
 */
#define FIO_LOG
#define FIO_TIME
#define FIO_SHA2
#define FIO_MATH
#define FIO_ASN1
#define FIO_RSA
#define FIO_ED25519
#define FIO_X509
#include "fio-stl/include.h"

static const uint8_t
    test_minimal_cert[] =
        {
            /* clang-format off */
    0x30, 0x82, 0x01, 0x2D, /* Certificate SEQUENCE (301 bytes) */
    0x30, 0x81, 0xD8,       /* TBS Certificate SEQUENCE (216 bytes) */
    0xA0, 0x03, 0x02, 0x01, 0x02, /* Version [0] = v3 (2) */
    0x02, 0x01, 0x01,       /* Serial Number = 1 */
    0x30, 0x0D, 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01,
    0x0B, 0x05, 0x00,       /* Signature Algorithm = sha256WithRSAEncryption */
    0x30, 0x0F, 0x31, 0x0D, 0x30, 0x0B, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0C,
    0x04, 0x74, 0x65, 0x73, 0x74, /* Issuer = CN=test */
    0x30, 0x1E,             /* Validity SEQUENCE */
    0x17, 0x0D, 0x32, 0x30, 0x30, 0x31, 0x30, 0x31, 0x30, 0x30, 0x30, 0x30,
    0x30, 0x30, 0x5A,       /* notBefore = 200101000000Z */
    0x17, 0x0D, 0x33, 0x30, 0x30, 0x31, 0x30, 0x31, 0x30, 0x30, 0x30, 0x30,
    0x30, 0x30, 0x5A,       /* notAfter = 300101000000Z */
    0x30, 0x0F, 0x31, 0x0D, 0x30, 0x0B, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0C,
    0x04, 0x74, 0x65, 0x73, 0x74, /* Subject = CN=test */
    0x30, 0x5E,             /* SubjectPublicKeyInfo SEQUENCE (94 bytes) */
    0x30, 0x0D, 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01,
    0x01, 0x05, 0x00,       /* Algorithm = rsaEncryption */
    0x03, 0x4D, 0x00,       /* BIT STRING (76 bytes, 0 unused) */
    0x30, 0x4A,             /* RSAPublicKey SEQUENCE (74 bytes) */
    0x02, 0x43, 0x00,       /* n INTEGER (67 bytes with leading 0) */
    0xC3, 0x52, 0x85, 0x3C, 0xF4, 0x68, 0x2E, 0x08,
    0x40, 0x47, 0x97, 0x2F, 0x9B, 0x5A, 0xC8, 0x3F,
    0x52, 0x47, 0x85, 0x3F, 0xE4, 0x6C, 0x2F, 0x0B,
    0x5C, 0x4A, 0x87, 0x3F, 0x5A, 0x4C, 0x8F, 0x38,
    0x5A, 0x4E, 0x8F, 0x3C, 0x5E, 0x4A, 0x8B, 0x34,
    0x56, 0x42, 0x87, 0x30, 0x52, 0x46, 0x83, 0x2C,
    0x4E, 0x42, 0x8F, 0x28, 0x4A, 0x3E, 0x8B, 0x24,
    0x46, 0x3A, 0x87, 0x20, 0x42, 0x36, 0x83, 0x1C,
    0x3E, 0x32,             /* (added 2 bytes to complete 64 bytes + leading 0) */
    0x02, 0x03, 0x01, 0x00, 0x01, /* e INTEGER = 65537 */
    0xA3, 0x1D, 0x30, 0x1B, /* Extensions [3] SEQUENCE (27 bytes) */
    0x30, 0x09, 0x06, 0x03, 0x55, 0x1D, 0x13, 0x04, 0x02, 0x30,
    0x00,                   /* BasicConstraints (empty = end entity) */
    0x30, 0x0E, 0x06, 0x03, 0x55, 0x1D, 0x11, 0x04, 0x07, 0x30, 0x05, 0x82,
    0x03, 0x77, 0x77, 0x77, /* SubjectAltName = DNS:www */
    0x30, 0x0D, 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01,
    0x0B, 0x05, 0x00,       /* Signature Algorithm = sha256WithRSAEncryption */
    0x03, 0x41, 0x00,       /* Signature BIT STRING (64 bytes, 0 unused) */
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
    0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
    0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
            /* clang-format on */
};

int main(void) {
  FIO_LOG_DDEBUG("Certificate size: %zu bytes", sizeof(test_minimal_cert));

  /* Manual step-by-step parsing to match fio_x509_parse */
  const uint8_t *der_data = test_minimal_cert;
  size_t der_len = sizeof(test_minimal_cert);

  /* Step 1: Parse outer SEQUENCE */
  fio_asn1_element_s cert_seq;
  if (!fio_asn1_parse(&cert_seq, der_data, der_len)) {
    FIO_LOG_ERROR("Step 1: Failed to parse certificate SEQUENCE");
    return 1;
  }
  FIO_LOG_DDEBUG("Step 1: cert_seq tag=0x%02X len=%zu (is_tag=0x30: %d)",
                 cert_seq.tag,
                 cert_seq.len,
                 fio_asn1_is_tag(&cert_seq, FIO_ASN1_SEQUENCE));

  /* Step 2: Iterate to get TBS, sig_alg, sig_value */
  fio_asn1_iterator_s cert_it;
  fio_asn1_element_s tbs, sig_alg, sig_value;

  fio_asn1_iterator_init(&cert_it, &cert_seq);

  if (fio_asn1_iterator_next(&cert_it, &tbs) != 0) {
    FIO_LOG_ERROR("Step 2a: Failed to get TBS");
    return 1;
  }
  FIO_LOG_DDEBUG("Step 2a: TBS tag=0x%02X len=%zu", tbs.tag, tbs.len);

  if (!fio_asn1_is_tag(&tbs, FIO_ASN1_SEQUENCE)) {
    FIO_LOG_ERROR("Step 2b: TBS is not SEQUENCE");
    return 1;
  }

  if (fio_asn1_iterator_next(&cert_it, &sig_alg) != 0) {
    FIO_LOG_ERROR("Step 2c: Failed to get sig_alg");
    return 1;
  }
  FIO_LOG_DDEBUG("Step 2c: sig_alg tag=0x%02X len=%zu",
                 sig_alg.tag,
                 sig_alg.len);

  if (fio_asn1_iterator_next(&cert_it, &sig_value) != 0) {
    FIO_LOG_ERROR("Step 2d: Failed to get sig_value");
    return 1;
  }
  FIO_LOG_DDEBUG("Step 2d: sig_value tag=0x%02X len=%zu",
                 sig_value.tag,
                 sig_value.len);

  /* Step 3: Parse signature value */
  const uint8_t *bits;
  size_t bit_len;
  uint8_t unused;
  if (fio_asn1_parse_bit_string(&sig_value, &bits, &bit_len, &unused) != 0) {
    FIO_LOG_ERROR("Step 3: Failed to parse signature BIT STRING");
    return 1;
  }
  FIO_LOG_DDEBUG("Step 3: Signature len=%zu unused=%d", bit_len, unused);

  /* Step 4: Parse TBS contents */
  fio_asn1_iterator_s tbs_it;
  fio_asn1_element_s elem;

  fio_asn1_iterator_init(&tbs_it, &tbs);

  /* Get version or serial */
  if (fio_asn1_iterator_next(&tbs_it, &elem) != 0) {
    FIO_LOG_ERROR("Step 4a: Failed to get first TBS element");
    return 1;
  }
  FIO_LOG_DDEBUG("Step 4a: First elem tag=0x%02X len=%zu", elem.tag, elem.len);

  int version = 0;
  if (fio_asn1_is_context_tag(&elem, 0)) {
    FIO_LOG_DDEBUG("Step 4b: Found version tag [0]");
    fio_asn1_element_s version_int;
    if (fio_asn1_parse(&version_int, elem.data, elem.len)) {
      uint64_t ver;
      if (fio_asn1_parse_integer(&version_int, &ver) == 0) {
        version = (int)ver;
        FIO_LOG_DDEBUG("Step 4b: Version = %d", version);
        (void)version;
      }
    }
    /* Get serial */
    if (fio_asn1_iterator_next(&tbs_it, &elem) != 0) {
      FIO_LOG_ERROR("Step 4c: Failed to get serial after version");
      return 1;
    }
    FIO_LOG_DDEBUG("Step 4c: Serial tag=0x%02X len=%zu", elem.tag, elem.len);
  }

  /* Serial should be INTEGER */
  if (!fio_asn1_is_tag(&elem, FIO_ASN1_INTEGER)) {
    FIO_LOG_ERROR("Step 4d: Serial is not INTEGER (tag=0x%02X)", elem.tag);
    return 1;
  }
  FIO_LOG_DDEBUG("Step 4d: Serial verified as INTEGER");

  /* Get signature alg */
  if (fio_asn1_iterator_next(&tbs_it, &elem) != 0) {
    FIO_LOG_ERROR("Step 5a: Failed to get TBS sig alg");
    return 1;
  }
  FIO_LOG_DDEBUG("Step 5a: TBS sig_alg tag=0x%02X len=%zu", elem.tag, elem.len);

  if (!fio_asn1_is_tag(&elem, FIO_ASN1_SEQUENCE)) {
    FIO_LOG_ERROR("Step 5b: TBS sig_alg is not SEQUENCE");
    return 1;
  }

  /* Get issuer */
  if (fio_asn1_iterator_next(&tbs_it, &elem) != 0) {
    FIO_LOG_ERROR("Step 6a: Failed to get issuer");
    return 1;
  }
  FIO_LOG_DDEBUG("Step 6a: Issuer tag=0x%02X len=%zu", elem.tag, elem.len);

  if (!fio_asn1_is_tag(&elem, FIO_ASN1_SEQUENCE)) {
    FIO_LOG_ERROR("Step 6b: Issuer is not SEQUENCE");
    return 1;
  }

  /* Get validity */
  if (fio_asn1_iterator_next(&tbs_it, &elem) != 0) {
    FIO_LOG_ERROR("Step 7a: Failed to get validity");
    return 1;
  }
  FIO_LOG_DDEBUG("Step 7a: Validity tag=0x%02X len=%zu", elem.tag, elem.len);

  if (!fio_asn1_is_tag(&elem, FIO_ASN1_SEQUENCE)) {
    FIO_LOG_ERROR("Step 7b: Validity is not SEQUENCE");
    return 1;
  }

  /* Get subject */
  if (fio_asn1_iterator_next(&tbs_it, &elem) != 0) {
    FIO_LOG_ERROR("Step 8a: Failed to get subject");
    return 1;
  }
  FIO_LOG_DDEBUG("Step 8a: Subject tag=0x%02X len=%zu", elem.tag, elem.len);

  if (!fio_asn1_is_tag(&elem, FIO_ASN1_SEQUENCE)) {
    FIO_LOG_ERROR("Step 8b: Subject is not SEQUENCE");
    return 1;
  }

  /* Get SubjectPublicKeyInfo */
  if (fio_asn1_iterator_next(&tbs_it, &elem) != 0) {
    FIO_LOG_ERROR("Step 9a: Failed to get SPKI");
    return 1;
  }
  FIO_LOG_DDEBUG("Step 9a: SPKI tag=0x%02X len=%zu", elem.tag, elem.len);

  if (!fio_asn1_is_tag(&elem, FIO_ASN1_SEQUENCE)) {
    FIO_LOG_ERROR("Step 9b: SPKI is not SEQUENCE");
    return 1;
  }

  /* Parse public key - test the key parsing */
  FIO_LOG_DDEBUG("Step 10: Parsing public key...");

  fio_asn1_iterator_s spki_it;
  fio_asn1_element_s alg_id, pubkey_bits;

  fio_asn1_iterator_init(&spki_it, &elem);

  if (fio_asn1_iterator_next(&spki_it, &alg_id) != 0) {
    FIO_LOG_ERROR("Step 10a: Failed to get alg_id");
    return 1;
  }
  FIO_LOG_DDEBUG("Step 10a: alg_id tag=0x%02X len=%zu", alg_id.tag, alg_id.len);

  if (fio_asn1_iterator_next(&spki_it, &pubkey_bits) != 0) {
    FIO_LOG_ERROR("Step 10b: Failed to get pubkey bits");
    return 1;
  }
  FIO_LOG_DDEBUG("Step 10b: pubkey_bits tag=0x%02X len=%zu",
                 pubkey_bits.tag,
                 pubkey_bits.len);

  if (fio_asn1_parse_bit_string(&pubkey_bits, &bits, &bit_len, &unused) != 0) {
    FIO_LOG_ERROR("Step 10c: Failed to parse pubkey BIT STRING");
    return 1;
  }
  FIO_LOG_DDEBUG("Step 10c: pubkey bit_len=%zu unused=%d", bit_len, unused);

  /* Parse RSA key */
  fio_asn1_element_s rsa_seq;
  if (!fio_asn1_parse(&rsa_seq, bits, bit_len)) {
    FIO_LOG_ERROR("Step 10d: Failed to parse RSA SEQUENCE");
    return 1;
  }
  FIO_LOG_DDEBUG("Step 10d: RSA seq tag=0x%02X len=%zu",
                 rsa_seq.tag,
                 rsa_seq.len);

  if (!fio_asn1_is_tag(&rsa_seq, FIO_ASN1_SEQUENCE)) {
    FIO_LOG_ERROR("Step 10e: RSA key is not SEQUENCE");
    return 1;
  }

  fio_asn1_iterator_s rsa_it;
  fio_asn1_element_s n_elem, e_elem;

  fio_asn1_iterator_init(&rsa_it, &rsa_seq);

  if (fio_asn1_iterator_next(&rsa_it, &n_elem) != 0) {
    FIO_LOG_ERROR("Step 10f: Failed to get n");
    return 1;
  }
  FIO_LOG_DDEBUG("Step 10f: n tag=0x%02X len=%zu", n_elem.tag, n_elem.len);

  if (!fio_asn1_is_tag(&n_elem, FIO_ASN1_INTEGER)) {
    FIO_LOG_ERROR("Step 10g: n is not INTEGER");
    return 1;
  }

  if (fio_asn1_iterator_next(&rsa_it, &e_elem) != 0) {
    FIO_LOG_ERROR("Step 10h: Failed to get e");
    return 1;
  }
  FIO_LOG_DDEBUG("Step 10h: e tag=0x%02X len=%zu", e_elem.tag, e_elem.len);

  if (!fio_asn1_is_tag(&e_elem, FIO_ASN1_INTEGER)) {
    FIO_LOG_ERROR("Step 10i: e is not INTEGER");
    return 1;
  }

  FIO_LOG_DDEBUG("All manual steps passed! Now testing full parser...");

  /* Now test the full X.509 parser */
  fio_x509_cert_s cert;
  int ret = fio_x509_parse(&cert, test_minimal_cert, sizeof(test_minimal_cert));

  if (ret != 0) {
    FIO_LOG_ERROR("fio_x509_parse failed: %d", ret);
    return 1;
  }

  FIO_LOG_DDEBUG("X.509 parse succeeded!");
  return 0;
}
