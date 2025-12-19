/**
 * X.509 Extensions Debug Test
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
    0x30, 0x82, 0x01, 0x2B, /* Certificate SEQUENCE (299 bytes) */
    0x30, 0x81, 0xD6,       /* TBS Certificate SEQUENCE (214 bytes) */
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
    0x30, 0x5D,             /* SubjectPublicKeyInfo SEQUENCE (93 bytes) */
    0x30, 0x0D, 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01,
    0x01, 0x05, 0x00,       /* Algorithm = rsaEncryption */
    0x03, 0x4B, 0x00,       /* BIT STRING (75 bytes, 0 unused) */
    0x30, 0x48,             /* RSAPublicKey SEQUENCE (72 bytes) */
    0x02, 0x41, 0x00,       /* n INTEGER (65 bytes with leading 0) */
    0xC3, 0x52, 0x85, 0x3C, 0xF4, 0x68, 0x2E, 0x08,
    0x40, 0x47, 0x97, 0x2F, 0x9B, 0x5A, 0xC8, 0x3F,
    0x52, 0x47, 0x85, 0x3F, 0xE4, 0x6C, 0x2F, 0x0B,
    0x5C, 0x4A, 0x87, 0x3F, 0x5A, 0x4C, 0x8F, 0x38,
    0x5A, 0x4E, 0x8F, 0x3C, 0x5E, 0x4A, 0x8B, 0x34,
    0x56, 0x42, 0x87, 0x30, 0x52, 0x46, 0x83, 0x2C,
    0x4E, 0x42, 0x8F, 0x28, 0x4A, 0x3E, 0x8B, 0x24,
    0x46, 0x3A, 0x87, 0x20, 0x42, 0x36, 0x83, 0xAB,
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
  fio_x509_cert_s cert;
  int ret = fio_x509_parse(&cert, test_minimal_cert, sizeof(test_minimal_cert));

  if (ret != 0) {
    FIO_LOG_ERROR("Failed to parse cert: %d", ret);
    return 1;
  }

  FIO_LOG_DDEBUG("Certificate parsed successfully");
  FIO_LOG_DDEBUG("  Version: %d", cert.version);
  FIO_LOG_DDEBUG("  is_ca: %d", cert.is_ca);
  FIO_LOG_DDEBUG("  has_key_usage: %d", cert.has_key_usage);
  FIO_LOG_DDEBUG("  key_usage: 0x%04X", cert.key_usage);

  if (cert.san_dns) {
    FIO_LOG_DDEBUG("  SAN DNS: '%.*s' (len=%zu)",
                   (int)cert.san_dns_len,
                   cert.san_dns,
                   cert.san_dns_len);
  } else {
    FIO_LOG_DDEBUG("  SAN DNS: NULL");
  }

  if (cert.subject_cn) {
    FIO_LOG_DDEBUG("  CN: '%.*s' (len=%zu)",
                   (int)cert.subject_cn_len,
                   cert.subject_cn,
                   cert.subject_cn_len);
  } else {
    FIO_LOG_DDEBUG("  CN: NULL");
  }

  // Now manually parse extensions to debug
  FIO_LOG_DDEBUG("\nManually parsing extensions...");

  // Parse the TBS and get extensions
  fio_asn1_element_s cert_seq;
  fio_asn1_parse(&cert_seq, test_minimal_cert, sizeof(test_minimal_cert));

  fio_asn1_iterator_s cert_it;
  fio_asn1_element_s tbs;

  fio_asn1_iterator_init(&cert_it, &cert_seq);
  fio_asn1_iterator_next(&cert_it, &tbs);

  // Iterate through TBS to find extensions [3]
  fio_asn1_iterator_s tbs_it;
  fio_asn1_element_s elem;
  int idx = 0;
  (void)idx; /* Used only in debug logging */

  fio_asn1_iterator_init(&tbs_it, &tbs);

  while (fio_asn1_iterator_next(&tbs_it, &elem) == 0) {
    FIO_LOG_DDEBUG("TBS[%d]: tag=0x%02X len=%zu is_context_3=%d",
                   idx,
                   elem.tag,
                   elem.len,
                   fio_asn1_is_context_tag(&elem, 3));

    if (fio_asn1_is_context_tag(&elem, 3)) {
      FIO_LOG_DDEBUG("  Found extensions context tag [3]!");
      FIO_LOG_DDEBUG("  elem.data[0..4]: 0x%02X 0x%02X 0x%02X 0x%02X",
                     elem.data[0],
                     elem.data[1],
                     elem.data[2],
                     elem.data[3]);

      // Parse the inner SEQUENCE
      fio_asn1_element_s exts;
      if (fio_asn1_parse(&exts, elem.data, elem.len)) {
        FIO_LOG_DDEBUG("  Extensions SEQUENCE: tag=0x%02X len=%zu",
                       exts.tag,
                       exts.len);

        // Iterate extensions
        fio_asn1_iterator_s exts_it;
        fio_asn1_element_s ext;
        int ext_idx = 0;
        (void)ext_idx; /* Used only in debug logging */

        fio_asn1_iterator_init(&exts_it, &exts);

        while (fio_asn1_iterator_next(&exts_it, &ext) == 0) {
          FIO_LOG_DDEBUG("  Extension[%d]: tag=0x%02X len=%zu",
                         ext_idx,
                         ext.tag,
                         ext.len);

          // Parse extension components
          fio_asn1_iterator_s ext_it;
          fio_asn1_element_s oid, value;

          fio_asn1_iterator_init(&ext_it, &ext);

          if (fio_asn1_iterator_next(&ext_it, &oid) == 0) {
            FIO_LOG_DDEBUG("    OID tag=0x%02X len=%zu", oid.tag, oid.len);

            // Print OID bytes
            char oid_str[64] = {0};
            int len = 0;
            for (size_t i = 0; i < oid.len && len < 60; i++) {
              len += snprintf(oid_str + len,
                              sizeof(oid_str) - len,
                              "%02X ",
                              oid.data[i]);
            }
            FIO_LOG_DDEBUG("    OID bytes: %s", oid_str);

            // Check if it's SAN
            if (fio_asn1_oid_eq(&oid, FIO_OID_SUBJECT_ALT_NAME)) {
              FIO_LOG_DDEBUG("    -> This is SubjectAltName!");
            }
          }

          if (fio_asn1_iterator_next(&ext_it, &value) == 0) {
            FIO_LOG_DDEBUG("    Value tag=0x%02X len=%zu",
                           value.tag,
                           value.len);

            // If it's OCTET STRING, look inside
            if (fio_asn1_is_tag(&value, FIO_ASN1_OCTET_STRING)) {
              fio_asn1_element_s san_seq;
              if (fio_asn1_parse(&san_seq, value.data, value.len)) {
                FIO_LOG_DDEBUG("    Inner: tag=0x%02X len=%zu",
                               san_seq.tag,
                               san_seq.len);

                // Iterate GeneralNames
                fio_asn1_iterator_s san_it;
                fio_asn1_element_s gn;

                fio_asn1_iterator_init(&san_it, &san_seq);

                while (fio_asn1_iterator_next(&san_it, &gn) == 0) {
                  FIO_LOG_DDEBUG("      GeneralName: tag=0x%02X len=%zu",
                                 gn.tag,
                                 gn.len);
                  FIO_LOG_DDEBUG("      is_context_2=%d",
                                 fio_asn1_is_context_tag(&gn, 2));
                  if (gn.len > 0 && gn.len < 64) {
                    FIO_LOG_DDEBUG("      data: '%.*s'", (int)gn.len, gn.data);
                  }
                }
              }
            }
          }

          ext_idx++;
        }
      }
    }

    idx++;
  }

  return 0;
}
