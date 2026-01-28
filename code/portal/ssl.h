#ifdef PORTAL_SSL_H
#error "ssl.h included more than once"
#endif
#define PORTAL_SSL_H

#define ERR_SSL_UNSUCCESSFUL           1
#define ERR_SSL_CONTINUE_PROCESSING    2
#define ERR_SSL_UNEXPECTED_MESSAGE     3
#define ERR_SSL_UNSUPPORTED_PROTOCOL   4
#define ERR_SSL_BAD_INPUT_DATA         5
#define ERR_SSL_BUFFER_TOO_SMALL       6
#define ERR_SSL_IO_FAILURE             7

enum ssl_sts_state {
    AWAIT_CLIENT_HELLO,
    AWAIT_SERVER_HELLO,
    AWAIT_SERVER_KEY_EXCHANGE,
    AWAIT_SERVER_HELLO_DONE,
    AWAIT_CLIENT_KEY_EXCHANGE,
    AWAIT_CLIENT_CHANGE_CIPHER_SPEC,
    AWAIT_CLIENT_HANDSHAKE,
    AWAIT_SERVER_CHANGE_CIPHER_SPEC,
    AWAIT_SERVER_ENC_HANDSHAKE,
    AWAIT_CLIENT_FINISHED,
};

const char* ssl_err_string(const int error_code);

// The hardcoded size here do not reflect the size(s) that are supported by
// the protocol, but it's currently what is used by portal.  We could be more
// flexible, trading some simplicity.
struct server_key {
    uint8_t prime[128];
    uint8_t generator[1];
    uint8_t salt[8];
    uint8_t server_public[128];
};

struct client_key {
    uint8_t private[128];
    uint8_t public[128];
};

struct tls12_random {
    uint32_t time;
    uint8_t  bytes[28];
};

struct ssl_sts_connection {
    SOCKET fd;
    enum ssl_sts_state state;

    mbedtls_sha256_context checksum;

    // Those ids are values incrementing on every messages, used in the
    // calculation of the HMAC. They don't seem to have any other usage.
    // In addition, they increment, but always from BE endian point of view.
    uint8_t next_read_id[8];
    uint8_t next_write_id[8];

    // We don't strictly need the IV for the encryption, because the IV is
    // always sent with the packet. In fact, this is why we don't store the
    // iv for decrypting. We keep it, to reproduce the official implementation
    // behavior.
    uint8_t iv_enc[16];

    // AES256-CBC
    mbedtls_aes_context cipher_enc;
    mbedtls_aes_context cipher_dec;

    // Actually sha1, but mbedtls hmac implementation only works
    // on `mbedtls_md_context_t`.
    mbedtls_md_context_t mac_enc;
    mbedtls_md_context_t mac_dec;

    array_uint8_t read;
    array_uint8_t write;

    struct tls12_random client_random;
    struct tls12_random server_random;
    struct server_key   server_key;
    struct client_key   client_key;

    char srp_username[256];
    size_t srp_username_len;

    uint8_t which_hash_verifier[20];
    uint8_t static_legacy_hash[20];
    uint8_t static_verifier_hash[20];
    uint8_t verifier_hash[20];

    uint8_t premaster_secret[128];
    uint8_t master_secret[48];
    uint8_t client_finished[12];
    uint8_t server_finished[12];
};

void ssl_sts_connection_init(struct ssl_sts_connection *ssl);
void ssl_sts_connection_free(struct ssl_sts_connection *ssl);

int ssl_sts_connection_init_srp(struct ssl_sts_connection *ssl, const char *username, const char *password);
int ssl_sts_connection_seed(struct ssl_sts_connection *ssl, mbedtls_entropy_context *entropy);
int ssl_sts_connection_seed_test(
    struct ssl_sts_connection *ssl,
    const uint8_t *client_private, size_t client_private_len,
    const uint8_t *client_random, size_t client_random_len,
    const uint8_t *iv, size_t iv_len);

int ssl_srp_compute_premaster_secret(struct ssl_sts_connection *ssl);

int ssl_sts_connection_handshake(struct ssl_sts_connection *ssl);
int ssl_sts_connection_send(struct ssl_sts_connection *ssl, const uint8_t *data, size_t data_len);
int ssl_sts_connection_recv(struct ssl_sts_connection *ssl, uint8_t *buffer, size_t buffer_len, size_t *retlen);
