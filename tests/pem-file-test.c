/**
 * Test PEM file loading with real certificate and key files
 * This creates test P-256 PEM files and verifies they can be loaded
 */
#include "test-helpers.h"

#define FIO_CRYPT /* Includes all crypto */
#include FIO_INCLUDE_FILE

#include <stdio.h>

int main(void) {
  const char *cert_path = "test-cert.pem";
  const char *key_path = "test-key.pem";

  FIO_LOG_INFO("=== PEM File Loading Test ===");

  /* Step 1: Generate keypair */
  FIO_LOG_INFO("Step 1: Generating P-256 keypair...");
  fio_x509_keypair_s keypair;
  if (fio_x509_keypair_p256(&keypair) != 0) {
    FIO_LOG_ERROR("Failed to generate P-256 keypair");
    return 1;
  }
  FIO_LOG_INFO("  Done");

  /* Step 2: Generate self-signed certificate */
  FIO_LOG_INFO("Step 2: Generating self-signed certificate...");
  uint8_t cert_der[4096];
  fio_x509_cert_options_s opts = {
      .subject_cn = "localhost",
      .subject_cn_len = 9,
  };
  size_t cert_len =
      fio_x509_self_signed_cert(cert_der, sizeof(cert_der), &keypair, &opts);
  if (cert_len == 0) {
    FIO_LOG_ERROR("Failed to generate self-signed certificate");
    return 1;
  }
  FIO_LOG_INFO("  Done (cert_len=%zu)", cert_len);

  /* Step 3: Write certificate PEM using fio_bstr */
  FIO_LOG_INFO("Step 3: Writing certificate PEM to %s...", cert_path);
  
  char *pem_str = fio_bstr_write(NULL, "-----BEGIN CERTIFICATE-----\n", 28);
  pem_str = fio_bstr_write_base64enc(pem_str, (const char *)cert_der, cert_len, 0);
  pem_str = fio_bstr_write(pem_str, "\n-----END CERTIFICATE-----\n", 27);
  
  FILE *f = fopen(cert_path, "w");
  if (!f || fwrite(pem_str, 1, fio_bstr_len(pem_str), f) != fio_bstr_len(pem_str)) {
    FIO_LOG_ERROR("Failed to write certificate file");
    if (f) fclose(f);
    fio_bstr_free(pem_str);
    return 1;
  }
  fclose(f);
  fio_bstr_free(pem_str);
  FIO_LOG_INFO("  Done");

  /* Step 4: Write private key PEM (SEC1 EC format) */
  FIO_LOG_INFO("Step 4: Writing private key PEM to %s...", key_path);
  
  /* Build SEC1 ECPrivateKey structure */
  uint8_t ec_key_der[256];
  size_t pos = 0;

  uint8_t inner[200];
  size_t inner_pos = 0;

  /* version = 1 */
  inner[inner_pos++] = 0x02; /* INTEGER */
  inner[inner_pos++] = 0x01; /* length 1 */
  inner[inner_pos++] = 0x01; /* value 1 */

  /* privateKey OCTET STRING (32 bytes) */
  inner[inner_pos++] = 0x04; /* OCTET STRING */
  inner[inner_pos++] = 0x20; /* length 32 */
  FIO_MEMCPY(inner + inner_pos, keypair.secret_key, 32);
  inner_pos += 32;

  /* parameters [0] OID for secp256r1 (1.2.840.10045.3.1.7) */
  inner[inner_pos++] = 0xA0; /* context [0] */
  inner[inner_pos++] = 0x0A; /* length */
  inner[inner_pos++] = 0x06; /* OID */
  inner[inner_pos++] = 0x08; /* length */
  inner[inner_pos++] = 0x2A; /* 1.2 */
  inner[inner_pos++] = 0x86;
  inner[inner_pos++] = 0x48; /* 840 */
  inner[inner_pos++] = 0xCE;
  inner[inner_pos++] = 0x3D; /* 10045 */
  inner[inner_pos++] = 0x03; /* 3 */
  inner[inner_pos++] = 0x01; /* 1 */
  inner[inner_pos++] = 0x07; /* 7 */

  /* publicKey [1] BIT STRING (65 bytes uncompressed point) */
  inner[inner_pos++] = 0xA1; /* context [1] */
  inner[inner_pos++] = 0x44; /* length 68 */
  inner[inner_pos++] = 0x03; /* BIT STRING */
  inner[inner_pos++] = 0x42; /* length 66 */
  inner[inner_pos++] = 0x00; /* unused bits = 0 */
  FIO_MEMCPY(inner + inner_pos, keypair.public_key, 65);
  inner_pos += 65;

  /* Wrap in SEQUENCE */
  ec_key_der[pos++] = 0x30; /* SEQUENCE */
  if (inner_pos < 128) {
    ec_key_der[pos++] = (uint8_t)inner_pos;
  } else {
    ec_key_der[pos++] = 0x81;
    ec_key_der[pos++] = (uint8_t)inner_pos;
  }
  FIO_MEMCPY(ec_key_der + pos, inner, inner_pos);
  pos += inner_pos;

  pem_str = fio_bstr_write(NULL, "-----BEGIN EC PRIVATE KEY-----\n", 31);
  pem_str = fio_bstr_write_base64enc(pem_str, (const char *)ec_key_der, pos, 0);
  pem_str = fio_bstr_write(pem_str, "\n-----END EC PRIVATE KEY-----\n", 30);
  
  f = fopen(key_path, "w");
  if (!f || fwrite(pem_str, 1, fio_bstr_len(pem_str), f) != fio_bstr_len(pem_str)) {
    FIO_LOG_ERROR("Failed to write private key file");
    if (f) fclose(f);
    fio_bstr_free(pem_str);
    return 1;
  }
  fclose(f);
  fio_bstr_free(pem_str);
  FIO_LOG_INFO("  Done");

  /* Step 5: Test loading the PEM files */
  FIO_LOG_INFO("Step 5: Testing PEM file loading...");

  char *cert_pem = fio_bstr_readfile(NULL, cert_path, 0, 0);
  FIO_ASSERT(cert_pem, "Failed to read certificate file");

  char *key_pem = fio_bstr_readfile(NULL, key_path, 0, 0);
  FIO_ASSERT(key_pem, "Failed to read private key file");

  /* Parse certificate */
  uint8_t der_buf[4096];
  size_t der_len = fio_pem_get_certificate_der(
      der_buf, sizeof(der_buf), cert_pem, fio_bstr_len(cert_pem));
  FIO_ASSERT(der_len > 0, "Failed to extract certificate DER");
  FIO_LOG_INFO("  Certificate DER: %zu bytes", der_len);

  /* Parse private key */
  fio_pem_private_key_s pkey;
  int result = fio_pem_parse_private_key(&pkey, key_pem, fio_bstr_len(key_pem));
  FIO_ASSERT(result == 0, "Failed to parse private key");
  FIO_ASSERT(pkey.type == FIO_PEM_KEY_ECDSA_P256, "Expected P-256 key type");
  FIO_LOG_INFO("  Private key type: P-256 ECDSA");
  FIO_LOG_INFO("  Has public key: %s",
               pkey.ecdsa_p256.has_public_key ? "yes" : "no");

  /* Clean up */
  fio_pem_private_key_clear(&pkey);
  fio_bstr_free(cert_pem);
  fio_bstr_free(key_pem);

  FIO_LOG_INFO("=== All tests passed! ===");
  FIO_LOG_INFO("Test PEM files created: %s, %s", cert_path, key_path);
  FIO_LOG_INFO("You can use these with the TLS 1.3 server.");

  return 0;
}
