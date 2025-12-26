/* *****************************************************************************
TLS 1.3 ALPN Extension Tests

Tests for Application-Layer Protocol Negotiation (RFC 7301) support.
***************************************************************************** */
#define FIO_LOG
#define FIO_CRYPT
#define FIO_TLS13
#include "fio-stl.h"

/* *****************************************************************************
Test: ALPN Extension Building (ClientHello)
***************************************************************************** */
FIO_SFUNC void fio___test_tls13_alpn_build(void) {
  FIO_LOG_DDEBUG("Testing TLS 1.3 ALPN extension building...");

  uint8_t buffer[256];
  size_t len;

  /* Test single protocol */
  len = fio___tls13_write_ext_alpn(buffer, "h2");
  FIO_ASSERT(len > 0, "ALPN extension should be built");
  FIO_ASSERT(len == 2 + 2 + 2 + 1 + 2,
             "ALPN extension length should be correct for 'h2'");

  /* Verify extension type (16 = ALPN) */
  uint16_t ext_type = ((uint16_t)buffer[0] << 8) | buffer[1];
  FIO_ASSERT(ext_type == 16, "Extension type should be ALPN (16)");

  /* Test multiple protocols */
  len = fio___tls13_write_ext_alpn(buffer, "h2,http/1.1");
  FIO_ASSERT(len > 0, "ALPN extension should be built for multiple protocols");

  /* Test empty protocols */
  len = fio___tls13_write_ext_alpn(buffer, "");
  FIO_ASSERT(len == 0, "Empty protocols should return 0");

  len = fio___tls13_write_ext_alpn(buffer, NULL);
  FIO_ASSERT(len == 0, "NULL protocols should return 0");

  FIO_LOG_DDEBUG("  PASS: ALPN extension building");
}

/* *****************************************************************************
Test: ALPN Extension Parsing (ClientHello)
***************************************************************************** */
FIO_SFUNC void fio___test_tls13_alpn_parse(void) {
  FIO_LOG_DDEBUG("Testing TLS 1.3 ALPN extension parsing...");

  /* Build a valid ALPN extension with "h2" and "http/1.1" */
  uint8_t ext_data[] = {
      /* ALPN extension list length (2 bytes) */
      0x00,
      0x0C, /* 12 bytes total */
      /* Protocol 1: "h2" */
      0x02, /* length */
      'h',
      '2',
      /* Protocol 2: "http/1.1" */
      0x08, /* length */
      'h',
      't',
      't',
      'p',
      '/',
      '1',
      '.',
      '1',
  };

  const char *protocols[8];
  size_t protocol_lens[8];

  int count = fio___tls13_parse_alpn_extension(ext_data,
                                               sizeof(ext_data),
                                               protocols,
                                               protocol_lens,
                                               8);

  FIO_ASSERT(count == 2, "Should parse 2 protocols");
  FIO_ASSERT(protocol_lens[0] == 2, "First protocol length should be 2");
  FIO_ASSERT(FIO_MEMCMP(protocols[0], "h2", 2) == 0,
             "First protocol should be 'h2'");
  FIO_ASSERT(protocol_lens[1] == 8, "Second protocol length should be 8");
  FIO_ASSERT(FIO_MEMCMP(protocols[1], "http/1.1", 8) == 0,
             "Second protocol should be 'http/1.1'");

  /* Test empty extension */
  uint8_t empty_ext[] = {0x00, 0x00};
  count = fio___tls13_parse_alpn_extension(empty_ext,
                                           sizeof(empty_ext),
                                           protocols,
                                           protocol_lens,
                                           8);
  FIO_ASSERT(count == 0, "Empty extension should return 0 protocols");

  FIO_LOG_DDEBUG("  PASS: ALPN extension parsing");
}

/* *****************************************************************************
Test: ALPN Protocol Selection
***************************************************************************** */
FIO_SFUNC void fio___test_tls13_alpn_select(void) {
  FIO_LOG_DDEBUG("Testing TLS 1.3 ALPN protocol selection...");

  /* Use static strings to ensure they're properly null-terminated */
  static const char proto_h2[] = "h2";
  static const char proto_http11[] = "http/1.1";
  static const char proto_spdy[] = "spdy/3.1";
  const char *client_protos[] = {proto_h2, proto_http11, proto_spdy};
  size_t client_lens[] = {2, 8, 8}; /* "spdy/3.1" is 8 chars */
  char selected[64];
  size_t selected_len = 0;
  int result;

  FIO_MEMSET(selected, 0, sizeof(selected));

  /* Test: Server prefers h2, client offers h2 first */
  result = fio___tls13_select_alpn(client_protos,
                                   client_lens,
                                   3,
                                   "h2,http/1.1",
                                   selected,
                                   &selected_len,
                                   sizeof(selected));
  FIO_ASSERT(result == 0, "Selection should succeed (h2)");
  FIO_ASSERT(selected_len == 2, "Selected protocol length should be 2");
  FIO_ASSERT(FIO_MEMCMP(selected, "h2", 2) == 0, "Selected should be 'h2'");

  /* Test: Server prefers http/1.1, client offers h2 first */
  FIO_MEMSET(selected, 0, sizeof(selected));
  selected_len = 0;
  result = fio___tls13_select_alpn(client_protos,
                                   client_lens,
                                   3,
                                   "http/1.1,h2",
                                   selected,
                                   &selected_len,
                                   sizeof(selected));
  FIO_ASSERT(result == 0, "Selection should succeed (http/1.1)");
  FIO_ASSERT(selected_len == 8, "Selected protocol length should be 8");
  FIO_ASSERT(FIO_MEMCMP(selected, "http/1.1", 8) == 0,
             "Selected should be 'http/1.1'");

  /* Test: No common protocol */
  result = fio___tls13_select_alpn(client_protos,
                                   client_lens,
                                   3,
                                   "grpc,mqtt",
                                   selected,
                                   &selected_len,
                                   sizeof(selected));
  FIO_ASSERT(result == -1, "Selection should fail with no common protocol");

  /* Test: Server has single protocol */
  result = fio___tls13_select_alpn(client_protos,
                                   client_lens,
                                   3,
                                   "spdy/3.1",
                                   selected,
                                   &selected_len,
                                   sizeof(selected));
  FIO_ASSERT(result == 0, "Selection should succeed (spdy/3.1)");
  FIO_ASSERT(selected_len == 8, "Selected protocol length should be 8");
  FIO_ASSERT(FIO_MEMCMP(selected, "spdy/3.1", 8) == 0,
             "Selected should be 'spdy/3.1'");

  /* Test: Empty client list */
  result = fio___tls13_select_alpn(NULL,
                                   NULL,
                                   0,
                                   "h2",
                                   selected,
                                   &selected_len,
                                   sizeof(selected));
  FIO_ASSERT(result == -1, "Selection should fail with empty client list");

  /* Test: Empty server list */
  result = fio___tls13_select_alpn(client_protos,
                                   client_lens,
                                   3,
                                   "",
                                   selected,
                                   &selected_len,
                                   sizeof(selected));
  FIO_ASSERT(result == -1, "Selection should fail with empty server list");

  FIO_LOG_DDEBUG("  PASS: ALPN protocol selection");
}

/* *****************************************************************************
Test: ALPN Response Building (EncryptedExtensions)
***************************************************************************** */
FIO_SFUNC void fio___test_tls13_alpn_response(void) {
  FIO_LOG_DDEBUG("Testing TLS 1.3 ALPN response building...");

  uint8_t buffer[64];
  size_t len;

  /* Build response for "h2" */
  len = fio___tls13_build_alpn_response(buffer, "h2", 2);
  FIO_ASSERT(len > 0, "ALPN response should be built");

  /* Verify extension type (16 = ALPN) */
  uint16_t ext_type = ((uint16_t)buffer[0] << 8) | buffer[1];
  FIO_ASSERT(ext_type == 16, "Extension type should be ALPN (16)");

  /* Parse the response */
  char selected[64];
  size_t selected_len;
  int result = fio___tls13_parse_alpn_response(buffer + 4,
                                               len - 4,
                                               selected,
                                               &selected_len,
                                               sizeof(selected));
  FIO_ASSERT(result == 0, "Response parsing should succeed");
  FIO_ASSERT(selected_len == 2, "Selected length should be 2");
  FIO_ASSERT(FIO_MEMCMP(selected, "h2", 2) == 0, "Selected should be 'h2'");

  /* Build response for "http/1.1" */
  len = fio___tls13_build_alpn_response(buffer, "http/1.1", 8);
  FIO_ASSERT(len > 0, "ALPN response should be built");

  result = fio___tls13_parse_alpn_response(buffer + 4,
                                           len - 4,
                                           selected,
                                           &selected_len,
                                           sizeof(selected));
  FIO_ASSERT(result == 0, "Response parsing should succeed");
  FIO_ASSERT(selected_len == 8, "Selected length should be 8");
  FIO_ASSERT(FIO_MEMCMP(selected, "http/1.1", 8) == 0,
             "Selected should be 'http/1.1'");

  FIO_LOG_DDEBUG("  PASS: ALPN response building");
}

/* *****************************************************************************
Test: Client ALPN API
***************************************************************************** */
FIO_SFUNC void fio___test_tls13_client_alpn_api(void) {
  FIO_LOG_DDEBUG("Testing TLS 1.3 client ALPN API...");

  fio_tls13_client_s client;
  FIO_MEMSET(&client, 0, sizeof(client));

  /* Test setting ALPN protocols */
  fio_tls13_client_alpn_set(&client, "h2,http/1.1");
  FIO_ASSERT(client.alpn_protocols_len == 11,
             "ALPN protocols length should be 11");
  FIO_ASSERT(FIO_MEMCMP(client.alpn_protocols, "h2,http/1.1", 11) == 0,
             "ALPN protocols should be 'h2,http/1.1'");

  /* Test getting ALPN before negotiation */
  const char *selected = fio_tls13_client_alpn_get(&client);
  FIO_ASSERT(selected == NULL, "ALPN should be NULL before negotiation");

  /* Simulate negotiation result */
  FIO_MEMCPY(client.alpn_selected, "h2", 2);
  client.alpn_selected[2] = '\0';
  client.alpn_selected_len = 2;

  selected = fio_tls13_client_alpn_get(&client);
  FIO_ASSERT(selected != NULL, "ALPN should not be NULL after negotiation");
  FIO_ASSERT(FIO_MEMCMP(selected, "h2", 2) == 0, "Selected should be 'h2'");

  /* Test clearing ALPN */
  fio_tls13_client_alpn_set(&client, NULL);
  FIO_ASSERT(client.alpn_protocols_len == 0,
             "ALPN protocols should be cleared");

  fio_tls13_client_alpn_set(&client, "");
  FIO_ASSERT(client.alpn_protocols_len == 0,
             "Empty string should clear ALPN protocols");

  FIO_LOG_DDEBUG("  PASS: Client ALPN API");
}

/* *****************************************************************************
Test: Server ALPN API
***************************************************************************** */
FIO_SFUNC void fio___test_tls13_server_alpn_api(void) {
  FIO_LOG_DDEBUG("Testing TLS 1.3 server ALPN API...");

  fio_tls13_server_s server;
  FIO_MEMSET(&server, 0, sizeof(server));

  /* Test setting ALPN protocols */
  fio_tls13_server_alpn_set(&server, "h2,http/1.1");
  FIO_ASSERT(server.alpn_supported_len == 11,
             "ALPN supported length should be 11");
  FIO_ASSERT(FIO_MEMCMP(server.alpn_supported, "h2,http/1.1", 11) == 0,
             "ALPN supported should be 'h2,http/1.1'");

  /* Test getting ALPN before negotiation */
  const char *selected = fio_tls13_server_alpn_get(&server);
  FIO_ASSERT(selected == NULL, "ALPN should be NULL before negotiation");

  /* Simulate negotiation result */
  FIO_MEMCPY(server.selected_alpn, "h2", 2);
  server.selected_alpn[2] = '\0';
  server.selected_alpn_len = 2;

  selected = fio_tls13_server_alpn_get(&server);
  FIO_ASSERT(selected != NULL, "ALPN should not be NULL after negotiation");
  FIO_ASSERT(FIO_MEMCMP(selected, "h2", 2) == 0, "Selected should be 'h2'");

  /* Test clearing ALPN */
  fio_tls13_server_alpn_set(&server, NULL);
  FIO_ASSERT(server.alpn_supported_len == 0,
             "ALPN supported should be cleared");

  fio_tls13_server_alpn_set(&server, "");
  FIO_ASSERT(server.alpn_supported_len == 0,
             "Empty string should clear ALPN supported");

  FIO_LOG_DDEBUG("  PASS: Server ALPN API");
}

/* *****************************************************************************
Test: ALPN in ClientHello Round-Trip
***************************************************************************** */
FIO_SFUNC void fio___test_tls13_alpn_client_hello_roundtrip(void) {
  FIO_LOG_DDEBUG("Testing TLS 1.3 ALPN in ClientHello round-trip...");

  fio_tls13_client_s client;
  FIO_MEMSET(&client, 0, sizeof(client));

  /* Initialize client first (this clears the struct) */
  fio_tls13_client_init(&client, "example.com");

  /* Set ALPN protocols AFTER init (init clears the struct) */
  fio_tls13_client_alpn_set(&client, "h2,http/1.1");

  uint8_t ch_buf[1024];
  int ch_len = fio_tls13_client_start(&client, ch_buf, sizeof(ch_buf));
  FIO_ASSERT(ch_len > 0, "ClientHello should be generated");

  /* Parse the ClientHello on server side */
  fio_tls13_client_hello_s ch;
  FIO_MEMSET(&ch, 0, sizeof(ch));

  /* Skip record header (5 bytes) and handshake header (4 bytes) */
  const uint8_t *ch_body = ch_buf + 5 + 4;
  size_t ch_body_len = (size_t)ch_len - 5 - 4;

  int parse_result = fio___tls13_parse_client_hello(&ch, ch_body, ch_body_len);
  FIO_ASSERT(parse_result == 0, "ClientHello parsing should succeed");

  /* Verify ALPN was included */
  FIO_ASSERT(ch.alpn_protocol_count == 2, "Should have 2 ALPN protocols");
  FIO_ASSERT(ch.alpn_protocol_lens[0] == 2,
             "First protocol length should be 2");
  FIO_ASSERT(FIO_MEMCMP(ch.alpn_protocols[0], "h2", 2) == 0,
             "First protocol should be 'h2'");
  FIO_ASSERT(ch.alpn_protocol_lens[1] == 8,
             "Second protocol length should be 8");
  FIO_ASSERT(FIO_MEMCMP(ch.alpn_protocols[1], "http/1.1", 8) == 0,
             "Second protocol should be 'http/1.1'");

  FIO_LOG_DDEBUG("  PASS: ALPN in ClientHello round-trip");
}

/* *****************************************************************************
Main
***************************************************************************** */
int main(void) {
  FIO_LOG_INFO("Testing TLS 1.3 ALPN Extension (RFC 7301)...");

  fio___test_tls13_alpn_build();
  fio___test_tls13_alpn_parse();
  fio___test_tls13_alpn_select();
  fio___test_tls13_alpn_response();
  fio___test_tls13_client_alpn_api();
  fio___test_tls13_server_alpn_api();
  fio___test_tls13_alpn_client_hello_roundtrip();

  FIO_LOG_INFO("All TLS 1.3 ALPN tests passed!");
  return 0;
}
