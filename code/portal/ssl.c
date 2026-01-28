#ifdef PORTAL_SSL_C
#error "ssl.c included more than once"
#endif
#define PORTAL_SSL_C

#define SSL_HS_CLIENT_HELLO            1
#define SSL_HS_SERVER_HELLO            2
#define SSL_HS_SERVER_KEY_EXCHANGE     12
#define SSL_HS_CLIENT_KEY_EXCHANGE     16
#define SSL_HS_FINISHED                20

#define TLS_SRP_SHA_WITH_AES_256_CBC_SHA 0xC020
#define TLS_SRP_SHA_WITH_AES_128_CBC_SHA 0xC01D
#define TLS_SRP_SHA_WITH_LEGACY_PASSWORD 0xFF04

#define SSL_MSG_CHANGE_CIPHER_SPEC     20
#define SSL_MSG_ALERT                  21
#define SSL_MSG_HANDSHAKE              22
#define SSL_MSG_APPLICATION_DATA       23
#define SSL_MSG_CID                    25

#define SSL_ALERT_LEVEL_WARNING 1
#define SSL_ALERT_LEVEL_FATAL   2


#define SSL_ALERT_DESCRIPTION_CLOSE_NOTIFY               0
#define SSL_ALERT_DESCRIPTION_UNEXPECTED_MESSAGE        10
#define SSL_ALERT_DESCRIPTION_BAD_RECORD_MAC            20
#define SSL_ALERT_DESCRIPTION_DECRYPTION_FAILED         21
#define SSL_ALERT_DESCRIPTION_RECORD_OVERFLOW           22
#define SSL_ALERT_DESCRIPTION_DECOMPRESSION_FAILURE     30
#define SSL_ALERT_DESCRIPTION_HANDSHAKE_FAILURE         40
#define SSL_ALERT_DESCRIPTION_NO_CERT                   41
#define SSL_ALERT_DESCRIPTION_BAD_CERT                  42
#define SSL_ALERT_DESCRIPTION_UNSUPPORTED_CERT          43
#define SSL_ALERT_DESCRIPTION_CERT_REVOKED              44
#define SSL_ALERT_DESCRIPTION_CERT_EXPIRED              45
#define SSL_ALERT_DESCRIPTION_CERT_UNKNOWN              46
#define SSL_ALERT_DESCRIPTION_ILLEGAL_PARAMETER         47
#define SSL_ALERT_DESCRIPTION_UNKNOWN_CA                48
#define SSL_ALERT_DESCRIPTION_ACCESS_DENIED             49
#define SSL_ALERT_DESCRIPTION_DECODE_ERROR              50
#define SSL_ALERT_DESCRIPTION_DECRYPT_ERROR             51
#define SSL_ALERT_DESCRIPTION_EXPORT_RESTRICTION        60
#define SSL_ALERT_DESCRIPTION_PROTOCOL_VERSION          70
#define SSL_ALERT_DESCRIPTION_INSUFFICIENT_SECURITY     71
#define SSL_ALERT_DESCRIPTION_INTERNAL_ERROR            80
#define SSL_ALERT_DESCRIPTION_INAPROPRIATE_FALLBACK     86
#define SSL_ALERT_DESCRIPTION_USER_CANCELED             90
#define SSL_ALERT_DESCRIPTION_NO_RENEGOTIATION         100
#define SSL_ALERT_DESCRIPTION_UNSUPPORTED_EXT          110
#define SSL_ALERT_DESCRIPTION_UNRECOGNIZED_NAME        112
#define SSL_ALERT_DESCRIPTION_UNKNOWN_PSK_IDENTITY     115
#define SSL_ALERT_DESCRIPTION_NO_APPLICATION_PROTOCOL  120

#define SHA1_DIGEST_SIZE 20

const char* ssl_err_string(const int error_code)
{
    switch (error_code) {
    case ERR_SSL_UNSUCCESSFUL:
        return "Unsuccessul";
    case ERR_SSL_CONTINUE_PROCESSING:
        return "Continue Processing";
    case ERR_SSL_UNEXPECTED_MESSAGE:
        return "Unexpected Message";
    case ERR_SSL_UNSUPPORTED_PROTOCOL:
        return "Unsupported Protocol";
    case ERR_SSL_BAD_INPUT_DATA:
        return "Bad Input Data";
    case ERR_SSL_BUFFER_TOO_SMALL:
        return "Buffer Too Small";
    case ERR_SSL_IO_FAILURE:
        return "I/O Failure";
    }
    return "Unknown Error";
}

static int compute_srp_hash(
    uint8_t *digest,
    const uint8_t *part1, size_t part1_len,
    const uint8_t *part2, size_t part2_len)
{
    mbedtls_sha1_context ctx;
    mbedtls_sha1_init(&ctx);

    int ret;
    if ((ret = mbedtls_sha1_starts(&ctx)) != 0) {
        goto cleanup;
    }

    if ((ret = mbedtls_sha1_update(&ctx, part1, part1_len)) != 0) {
        goto cleanup;
    }

    uint8_t seperator = ':';
    if ((ret = mbedtls_sha1_update(&ctx, &seperator, sizeof(seperator))) != 0) {
        goto cleanup;
    }

    if ((ret = mbedtls_sha1_update(&ctx, part2, part2_len)) != 0) {
        goto cleanup;
    }

    if ((ret = mbedtls_sha1_finish(&ctx, digest)) != 0) {
        goto cleanup;
    }

cleanup:
    mbedtls_sha1_free(&ctx);
    if (ret != 0)
        ret = 1;
    return 0;
}

static int sha1_of_two_values(
    uint8_t *digest,
    const uint8_t *part1, size_t part1_len,
    const uint8_t *part2, size_t part2_len)
{
    mbedtls_sha1_context ctx;
    mbedtls_sha1_init(&ctx);

    int ret;
    if ((ret = mbedtls_sha1_starts(&ctx)) != 0) {
        goto cleanup;
    }

    if ((ret = mbedtls_sha1_update(&ctx, part1, part1_len)) != 0) {
        goto cleanup;
    }

    if ((ret = mbedtls_sha1_update(&ctx, part2, part2_len)) != 0) {
        goto cleanup;
    }

    if ((ret = mbedtls_sha1_finish(&ctx, digest)) != 0) {
        goto cleanup;
    }

cleanup:
    mbedtls_sha1_free(&ctx);
    if (ret != 0)
        ret = 1;
    return 0;
}

static int sha1_of_two_values_with_padding(
    uint8_t *digest, size_t padding,
    const uint8_t *part1, size_t part1_len,
    const uint8_t *part2, size_t part2_len)
{
    uint8_t buffer1[256];
    uint8_t buffer2[256];

    if ((sizeof(buffer1) < padding) && (sizeof(buffer2) < padding))
        return ERR_SSL_UNSUCCESSFUL;

    if ((padding < part1_len) || (padding < part2_len))
        return ERR_SSL_UNSUCCESSFUL;

    size_t number_of_zeroes = padding - part1_len;
    memset(buffer1, 0, number_of_zeroes);
    memcpy(buffer1 + number_of_zeroes, part1, part1_len);

    number_of_zeroes = padding - part2_len;
    memset(buffer2, 0, number_of_zeroes);
    memcpy(buffer2 + number_of_zeroes, part2, part2_len);

    return sha1_of_two_values(digest, buffer1, padding, buffer2, padding);
}

static int compute_legacy_hash(
    uint8_t *digest,
    const char *username, size_t username_len,
    const char *password, size_t password_len)
{
    uint16_t buffer[256];

    size_t buffer_required_size = (username_len + password_len) * 2;
    if (sizeof(buffer) < buffer_required_size) {
        fprintf(stderr, "Buffer is too short (%zu bytes) for the current username and password\n", sizeof(buffer));
        return 1;
    }

    size_t i = 0;
    for (size_t j = 0; j < password_len; j++, i++) {
        if ((password[j] & ~0x7F) != 0) {
            fprintf(stderr, "We currently don't support non-ascii character\n");
            return 1;
        }
        buffer[i] = password[i];
    }

    for (size_t j = 0; j < username_len; j++, i++) {
        if ((username[j] & ~0x7F) != 0) {
            fprintf(stderr, "We currently don't support non-ascii character\n");
            return 1;
        }

        buffer[i] = username[j];
    }

    int ret;
    uint8_t temp_digest[SHA1_DIGEST_SIZE];
    if ((ret = mbedtls_sha1((const uint8_t *)buffer, buffer_required_size, temp_digest)) != 0)
        return 1;

    // Yeah, the encoding of the words are reversed...
    le32enc(&temp_digest[0], be32dec(&temp_digest[0]));
    le32enc(&temp_digest[4], be32dec(&temp_digest[4]));
    le32enc(&temp_digest[8], be32dec(&temp_digest[8]));
    le32enc(&temp_digest[12], be32dec(&temp_digest[12]));
    le32enc(&temp_digest[16], be32dec(&temp_digest[16]));

    return compute_srp_hash(digest, (const uint8_t *)username, username_len, temp_digest, sizeof(temp_digest));
}

static int compute_static_hash(
    struct ssl_sts_connection *ssl,
    const char *username, size_t username_len,
    const char *password, size_t password_len)
{
    STATIC_ASSERT(SHA1_DIGEST_SIZE == sizeof(ssl->static_legacy_hash));
    STATIC_ASSERT(SHA1_DIGEST_SIZE == sizeof(ssl->static_verifier_hash));

    int ret = compute_srp_hash(
        ssl->static_verifier_hash,
        (const uint8_t *)username, username_len,
        (const uint8_t *)password, password_len);
    if (ret != 0)
        return ret;

    ret = compute_legacy_hash(
        ssl->static_legacy_hash,
        username, username_len,
        password, password_len);
	return ret;
}

void ssl_sts_connection_init(struct ssl_sts_connection *ssl)
{
    memset(ssl, 0, sizeof(*ssl));

    ssl->fd = INVALID_SOCKET;
    ssl->state = AWAIT_CLIENT_HELLO;

    mbedtls_sha256_init(&ssl->checksum);
    mbedtls_sha256_starts(&ssl->checksum, 0);

    mbedtls_aes_init(&ssl->cipher_enc);
    mbedtls_aes_init(&ssl->cipher_dec);
    mbedtls_md_init(&ssl->mac_enc);
    mbedtls_md_init(&ssl->mac_dec);

    array_init(&ssl->read);
    array_init(&ssl->write);
}

void ssl_sts_connection_free(struct ssl_sts_connection *ssl)
{
    if (ssl->fd != INVALID_SOCKET) {
        closesocket(ssl->fd);
        ssl->fd = INVALID_SOCKET;
    }

    mbedtls_sha256_free(&ssl->checksum);

    mbedtls_aes_free(&ssl->cipher_enc);
    mbedtls_aes_free(&ssl->cipher_dec);
    mbedtls_md_free(&ssl->mac_enc);
    mbedtls_md_free(&ssl->mac_dec);

    array_reset(&ssl->read);
    array_reset(&ssl->write);
}

int ssl_sts_connection_init_srp(struct ssl_sts_connection *ssl, const char *username, const char *password)
{
    int ret;

    size_t username_len = strlen(username);
    size_t password_len = strlen(password);
    if ((ret = compute_static_hash(ssl, username, username_len, password, password_len)) != 0) {
        return ret;
    }

    if (sizeof(ssl->srp_username) < username_len) {
        return ERR_SSL_UNSUCCESSFUL;
    }

    memcpy(ssl->srp_username, username, username_len);
    ssl->srp_username_len = username_len;

    return 0;
}

int ssl_sts_connection_seed(struct ssl_sts_connection *ssl, mbedtls_entropy_context *entropy)
{
    const uint8_t custom[] = "Master Togo";

    mbedtls_ctr_drbg_context prng;
    mbedtls_ctr_drbg_init(&prng);

    int ret = mbedtls_ctr_drbg_seed(
        &prng,
        mbedtls_entropy_func,
        entropy,
        custom,
        sizeof(custom) - 1);

    if (ret != 0) {
        fprintf(stderr, "Failed to see the prng\n");
        goto cleanup;
    }

    ret = mbedtls_ctr_drbg_random(
        &prng,
        (uint8_t *)&ssl->client_random,
        sizeof(ssl->client_random));

    if (ret != 0) {
        goto cleanup;
    }

    ret = mbedtls_ctr_drbg_random(
        &prng,
        ssl->client_key.private,
        sizeof(ssl->client_key.private));

    if (ret != 0) {
        goto cleanup;
    }

    ret = mbedtls_ctr_drbg_random(
        &prng,
        ssl->iv_enc,
        sizeof(ssl->iv_enc));

    if (ret != 0) {
        goto cleanup;
    }

cleanup:
    mbedtls_ctr_drbg_free(&prng);
    if (ret != 0)
        ret = 1;
    return ret;
}

int ssl_sts_connection_seed_test(
    struct ssl_sts_connection *ssl,
    const uint8_t *client_private, size_t client_private_len,
    const uint8_t *client_random, size_t client_random_len,
    const uint8_t *iv, size_t iv_len)
{
    if (sizeof(ssl->client_key.private) < client_private_len) {
        fprintf(stderr, "Couldn't initialize client private, too many bytes\n");
        return ERR_SSL_UNSUCCESSFUL;
    }

    size_t zero_count = sizeof(ssl->client_key.private) - client_private_len;
    memcpy(ssl->client_key.private + zero_count, client_private, client_private_len);

    if (sizeof(ssl->client_random) != client_random_len) {
        fprintf(stderr, "'client_random_len' must be %zu bytes\n", sizeof(ssl->client_random));
        return ERR_SSL_UNSUCCESSFUL;
    }

    ssl->client_random.time = be32dec(client_random);
    memcpy(&ssl->client_random.bytes, client_random + 4, client_random_len - 4);

    if (sizeof(ssl->iv_enc) != iv_len) {
        fprintf(stderr, "'iv' must be %zu bytes\n", sizeof(ssl->iv_enc));
        return ERR_SSL_UNSUCCESSFUL;
    }

    memcpy(&ssl->iv_enc, iv, iv_len);
    return 0;
}

static int ssl_srp_write_protocol_header(struct ssl_sts_connection *ssl, uint8_t msg_type, size_t msg_size)
{
    if ((size_t)UINT16_MAX < msg_size)
        return ERR_SSL_UNSUCCESSFUL;

    array_add(&ssl->write, msg_type);

    // The version is always "\x03\x03". (i.e., TLS v1.2)
    array_add(&ssl->write, 0x03);
    array_add(&ssl->write, 0x03);

    // Reserve two bytes to later write the length.
    void *p = array_push(&ssl->write, 2);
    be16enc(p, (uint16_t)msg_size);
    return 0;
}

static void ssl_srp_start_protocol_msg(struct ssl_sts_connection *ssl, uint8_t msg_type, size_t *header_pos)
{
    *header_pos = array_size(&ssl->write);
    // *header_pos = (&ssl->write)->size;

    array_add(&ssl->write, msg_type);

    // The version is always "\x03\x03". (i.e., TLS v1.2)
    array_add(&ssl->write, 0x03);
    array_add(&ssl->write, 0x03);

    // Reserve two bytes to later write the length.
    (void)array_push(&ssl->write, 2);
}

static int ssl_srp_finish_protocol_msg(struct ssl_sts_connection *ssl, uint8_t msg_type, size_t header_pos)
{
    int ret;

    const size_t MSG_HDR_LEN = 5;
    assert((header_pos + MSG_HDR_LEN) <= array_size(&ssl->write));
    assert(array_at(&ssl->write, header_pos) == msg_type);

    size_t msg_offset = header_pos + MSG_HDR_LEN;
    size_t msg_size = array_size(&ssl->write) - msg_offset;
    if ((size_t)UINT16_MAX < msg_size)
        return ERR_SSL_UNSUCCESSFUL;

    // The HMAC is computed on everything following the initial header.
    const uint8_t *msg_start = &array_at(&ssl->write, msg_offset);
    if ((ret = mbedtls_sha256_update(&ssl->checksum, msg_start, msg_size)) != 0) {
        return ERR_SSL_UNSUCCESSFUL;
    }

    uint8_t *p = &array_at(&ssl->write, header_pos + 3);
    be16enc(p, (uint16_t)msg_size);
    return 0;
}

static void ssl_srp_start_handshake_msg(struct ssl_sts_connection *ssl, size_t *pos, uint8_t hs_type)
{
    size_t header_pos;
    ssl_srp_start_protocol_msg(ssl, SSL_MSG_HANDSHAKE, &header_pos);

    // We assume this offset when we finish the handhsake message.
    // There should be more elegant solution, but that's what we
    // have for now.
    assert((array_size(&ssl->write) - header_pos) == 5);

    //
    // Reserve 4 bytes for hanshake header.
    //    ...
    //    HandshakeType msg_type;
    //    uint24 length;
    //    ...
    //

    // Save the position of the header to later write the appropriate size.
    *pos = array_size(&ssl->write);

    array_add(&ssl->write, hs_type);
    (void)array_push(&ssl->write, 3);
}

static int ssl_srp_finish_handshake_msg(struct ssl_sts_connection *ssl, size_t header_pos, uint8_t hs_type)
{
    int ret;
    const size_t HANDSHAKE_HDR_LEN = 4;
    assert((header_pos + HANDSHAKE_HDR_LEN) <= array_size(&ssl->write));
    assert(array_at(&ssl->write, header_pos) == hs_type);

    uint8_t *content_begin = array_begin(&ssl->write) + header_pos + HANDSHAKE_HDR_LEN;
    size_t content_length = array_end(&ssl->write) - content_begin;

    const size_t UINT24_MAX = 0xFFFFFF;
    if (UINT24_MAX <= content_length)
        return ERR_SSL_UNSUCCESSFUL;

    uint8_t *hs_len_ptr = array_begin(&ssl->write) + 1;
    hs_len_ptr[header_pos + 0] = (content_length >> 16) & 0xFF;
    hs_len_ptr[header_pos + 1] = (content_length >> 8) & 0xFF;
    hs_len_ptr[header_pos + 2] = content_length & 0xFF;

    if ((ret = ssl_srp_finish_protocol_msg(ssl, SSL_MSG_HANDSHAKE, (header_pos - 5))) != 0) {
        return ret;
    }

    return 0;
}

static void increment_be(uint8_t *bytes, size_t len)
{
    for (size_t i = len - 1; i < len; ++i) {
        if (++bytes[i] != 0)
            break;
    }
}

static int ssl_sts_issue_next_iv(struct ssl_sts_connection *ssl, uint8_t *iv, size_t iv_len)
{
    if (iv_len != sizeof(ssl->iv_enc))
        return ERR_SSL_UNSUCCESSFUL;

    // We increment the `uint128_t` by 1.
    for (size_t i = 0; i < sizeof(ssl->iv_enc); ++i) {
        if (++ssl->iv_enc[i] != 0)
            break;
    }

    if (mbedtls_internal_aes_encrypt(&ssl->cipher_enc, ssl->iv_enc, iv) != 0)
        return ERR_SSL_UNSUCCESSFUL;

    memcpy(ssl->iv_enc, iv, iv_len);
    return 0;
}

static int ssl_srp_build_message_to_encrypt(
    array_uint8_t *buffer,
    mbedtls_md_context_t *hmac,
    const uint8_t *msg,
    size_t msg_size)
{
    int ret;

    assert(array_size(buffer) == 0);

    // Write the message in the output.
    array_insert(buffer, msg_size, msg);

    // Write the hmac in the output.
    uint8_t *p = array_push(buffer, SHA1_DIGEST_SIZE);
    if ((ret = mbedtls_md_hmac_finish(hmac, p)) != 0)
        return ERR_SSL_UNSUCCESSFUL;

    // Reset the hmac structure to allow re-using for the next packet.
    if ((ret = mbedtls_md_hmac_reset(hmac)) != 0)
        return ERR_SSL_UNSUCCESSFUL;

    // Write the padding with the modified PKCS7.
    // We use AES, so the block size is 16, we pad to this value.
    const size_t AES_BLOCK_SIZE = 16;

    // We compute the next alignment size. If the buffer is already
    // aligned, the aligned size will be the same as the size. This
    // is incorrect, because it could be ambiguous whether there is
    // a padding or not.
    size_t size = array_size(buffer);
    size_t alligned = (size + (AES_BLOCK_SIZE - 1)) & ~(AES_BLOCK_SIZE - 1);

    // When the buffer was already aligned, we simply pad it with
    // an extra complete block.
    if (size == alligned)
        alligned = size + AES_BLOCK_SIZE;

    assert(size < alligned);
    assert((alligned - size) <= AES_BLOCK_SIZE);

    // PKCS7 padding value should be the number of bytes padded, but
    // portal use a custom? one similar, but the padded value is the
    // number of bytes - 1.
    uint8_t padval = (uint8_t)(alligned - size) - 1;

    array_reserve(buffer, padval + 1);
    for (uint8_t i = 0; i < (padval + 1); ++i)
        array_add(buffer, padval);

    return 0;
}

static int ssl_sts_connection_send_internal(
    struct ssl_sts_connection *ssl,
    uint8_t message_type,
    const uint8_t *data,
    size_t data_len)
{
    int ret;

    // if (ssl->state != AWAIT_CLIENT_FINISHED) {
    //     fprintf(stderr, "Handshake wasn't completed successfully\n");
    //     return ERR_SSL_UNSUCCESSFUL;
    // }

    size_t size_at_start = array_size(&ssl->write);

    // We write the header with an length of the specified message, and we need
    // that to compute the HMAC, but the HMAC is then append to the message and
    // we need to update this length.  We will update the actual length, just
    // before encrypting the message.
    if ((ret = ssl_srp_write_protocol_header(ssl, message_type, data_len)) != 0)
        return 0;

    if ((ret = mbedtls_md_hmac_update(&ssl->mac_enc, ssl->next_write_id, sizeof(ssl->next_write_id))) != 0) {
        return ERR_SSL_UNSUCCESSFUL;
    }

    // Increment the packet number for next update.
    increment_be(ssl->next_write_id, sizeof(ssl->next_write_id));

    const size_t HEADER_SIZE = 5;
    assert((size_at_start + HEADER_SIZE) <= array_size(&ssl->write));
    const uint8_t *header_bytes = &array_at(&ssl->write, size_at_start);
    if ((ret = mbedtls_md_hmac_update(&ssl->mac_enc, header_bytes, HEADER_SIZE)) != 0) {
        return ERR_SSL_UNSUCCESSFUL;
    }

    if ((ret = mbedtls_md_hmac_update(&ssl->mac_enc, data, data_len)) != 0) {
        return ERR_SSL_UNSUCCESSFUL;
    }

    uint8_t *iv_buffer = array_push(&ssl->write, sizeof(ssl->iv_enc));
    if ((ret = ssl_sts_issue_next_iv(ssl, iv_buffer, sizeof(ssl->iv_enc))) != 0)
        return ret;

    // Invalidate pointer to avoid re-using it after pushing in the array.
    iv_buffer = NULL;

    array_uint8_t buffer;
    array_init(&buffer);
    if ((ret = ssl_srp_build_message_to_encrypt(&buffer, &ssl->mac_enc, data, data_len)) != 0) {
        array_reset(&buffer);
        return ERR_SSL_UNSUCCESSFUL;
    }

    uint8_t *output = array_push(&ssl->write, buffer.size);
    ret = mbedtls_aes_crypt_cbc(
        &ssl->cipher_enc,
        MBEDTLS_AES_ENCRYPT,
        buffer.size,
        ssl->iv_enc,
        buffer.data,
        output);

    array_reset(&buffer);
    if (ret != 0) {
        return ERR_SSL_UNSUCCESSFUL;
    }

    uint8_t *send_data = ssl->write.data + size_at_start;
    size_t send_size = array_size(&ssl->write) - size_at_start;

    // Update the header size containing the HMAC
    size_t content_size = send_size - HEADER_SIZE;
    assert(content_size <= (size_t)UINT16_MAX);
    be16enc(send_data + 3, (uint16_t)content_size);

    if ((ret = send_full(ssl->fd, send_data, send_size)) != 0) {
        return ERR_SSL_IO_FAILURE;
    }

    // Reset the vector to the size at start
    ssl->write.size = size_at_start;
    // array_resize(ssl->write, size_at_start);

    return 0;
}

static const char *alert_level_to_string(uint8_t level)
{
    switch (level)
    {
        case SSL_ALERT_LEVEL_WARNING:
            return "Warning";
		case SSL_ALERT_LEVEL_FATAL:
            return "Fatal";
        default:
            return "Unknown";
    }
}

static const char *alert_description_to_string(uint8_t description)
{
    switch (description)
    {
    case SSL_ALERT_DESCRIPTION_CLOSE_NOTIFY:
        return "Close Notify";
    case SSL_ALERT_DESCRIPTION_UNEXPECTED_MESSAGE:
        return "Unexpected Message";
    case SSL_ALERT_DESCRIPTION_BAD_RECORD_MAC:
        return "Bad Record MAC";
    case SSL_ALERT_DESCRIPTION_DECRYPTION_FAILED:
        return "Decryption Failed";
    case SSL_ALERT_DESCRIPTION_RECORD_OVERFLOW:
        return "Record Overflow";
    case SSL_ALERT_DESCRIPTION_DECOMPRESSION_FAILURE:
        return "Decompression Failure";
    case SSL_ALERT_DESCRIPTION_HANDSHAKE_FAILURE:
        return "Handshake Failure";
    case SSL_ALERT_DESCRIPTION_NO_CERT:
        return "No Cert";
    case SSL_ALERT_DESCRIPTION_BAD_CERT:
        return "Bad Cert";
    case SSL_ALERT_DESCRIPTION_UNSUPPORTED_CERT:
        return "Unsupported Cert";
    case SSL_ALERT_DESCRIPTION_CERT_REVOKED:
        return "Cert Revoked";
    case SSL_ALERT_DESCRIPTION_CERT_EXPIRED:
        return "Cert Expired";
    case SSL_ALERT_DESCRIPTION_CERT_UNKNOWN:
        return "Cert Unknown";
    case SSL_ALERT_DESCRIPTION_ILLEGAL_PARAMETER:
        return "Illegal Parameter";
    case SSL_ALERT_DESCRIPTION_UNKNOWN_CA:
        return "Unknown CA";
    case SSL_ALERT_DESCRIPTION_ACCESS_DENIED:
        return "Access Denied";
    case SSL_ALERT_DESCRIPTION_DECODE_ERROR:
        return "Decode Error";
    case SSL_ALERT_DESCRIPTION_DECRYPT_ERROR:
        return "Decrypt Error";
    case SSL_ALERT_DESCRIPTION_EXPORT_RESTRICTION:
        return "Export Restriction";
    case SSL_ALERT_DESCRIPTION_PROTOCOL_VERSION:
        return "Protocol Version";
    case SSL_ALERT_DESCRIPTION_INSUFFICIENT_SECURITY:
        return "Insufficient Security";
    case SSL_ALERT_DESCRIPTION_INTERNAL_ERROR:
        return "Internal Error";
    case SSL_ALERT_DESCRIPTION_INAPROPRIATE_FALLBACK:
        return "Inapropriate Fallback";
    case SSL_ALERT_DESCRIPTION_USER_CANCELED:
        return "User Canceled";
    case SSL_ALERT_DESCRIPTION_NO_RENEGOTIATION:
        return "No Renegotiation";
    case SSL_ALERT_DESCRIPTION_UNSUPPORTED_EXT:
        return "Unsupported Ext";
    case SSL_ALERT_DESCRIPTION_UNRECOGNIZED_NAME:
        return "Unrecognized Name";
    case SSL_ALERT_DESCRIPTION_UNKNOWN_PSK_IDENTITY:
        return "Unknown Psk Identity";
    case SSL_ALERT_DESCRIPTION_NO_APPLICATION_PROTOCOL:
        return "No Application Protocol";
    default:
        return "Unknown";
    }
}

static void show_alert_message(const uint8_t *data, size_t length)
{
    if (length < 2) {
        fprintf(stderr, "Not enough bytes (%zu instead of 2) for the alert message\n", length);
        return;
    }

    uint8_t level = data[0];
    uint8_t description = data[1];

    fprintf(
        stderr, "Alert message, level: %s (%hhu), Description: %s (%hhu)\n",
        alert_level_to_string(level), level,
        alert_description_to_string(description), description);
}

static int parse_tls12_handshake(
    struct ssl_sts_connection *ssl,
    uint8_t content_type,
    const uint8_t **subdata, size_t *sublen,
    int include_msg_in_checksum)
{
    const size_t HEADER_LEN = 5;

    const uint8_t *data = ssl->read.data;
    size_t length = array_size(&ssl->read);

    // The packet starts with 1 byte defining the content type, 2 bytes defining
    // the version and 2 bytes defining the length of the payload.
    if (length < HEADER_LEN)
        return ERR_SSL_CONTINUE_PROCESSING;

    // Parse that before the content type to avoid having to do it again in
    // case of an alert message.
    if (data[1] != 3 && data[2] != 3)
        return ERR_SSL_UNSUPPORTED_PROTOCOL;

    uint16_t client_sublen = be16dec(&data[3]);

    // This can't overflow, we checked it before.
    // We ensure we have all the data in the header to ensure we will not
    // try to parse the same packet because of incomplete data. This is done
    // to ensure we don't calculate the checksum on the packet twice.
    if ((length - HEADER_LEN) < client_sublen)
        return ERR_SSL_CONTINUE_PROCESSING;

    *subdata = &data[HEADER_LEN];
    *sublen = client_sublen;

    if (data[0] != content_type) {
        if (data[0] == SSL_MSG_ALERT)
            show_alert_message(*subdata, *sublen);
        return ERR_SSL_UNEXPECTED_MESSAGE;
    }

    if (include_msg_in_checksum) {
        int ret;
        if ((ret = mbedtls_sha256_update(&ssl->checksum, *subdata, *sublen)) != 0) {
            return ERR_SSL_UNSUCCESSFUL;
        }
    }

    return 0;
}

int ssl_sts_connection_recv_internal(
    struct ssl_sts_connection *ssl,
    uint8_t message_type,
    uint8_t *buffer, size_t buffer_len,
    size_t *retlen)
{
    int ret;

    const uint8_t *inner_data;
    size_t inner_len;

    for (;;) {
        ret = parse_tls12_handshake(ssl, message_type, &inner_data, &inner_len, 0);

        if (ret == 0)
            break;

        if (ret != ERR_SSL_CONTINUE_PROCESSING) {
            return ret;
        }

        if ((ret = recv_to_buffer(ssl->fd, &ssl->read)) != 0) {
            fprintf(stderr, "Failed to recv data from network, errno: %d\n", ret);
            return ERR_SSL_IO_FAILURE;
        }
    }

    uint8_t iv[16];
    if (inner_len < sizeof(iv)) {
        fprintf(stderr, "Not enough bytes to contain the IV\n");
        return ERR_SSL_BAD_INPUT_DATA;
    }

    memcpy(iv, inner_data, sizeof(iv));

    const uint8_t *enc_data = inner_data + sizeof(iv);
    size_t enc_len = inner_len - sizeof(iv);

    const size_t AES_BLOCK_SIZE = 16;
    if (((enc_len / AES_BLOCK_SIZE) * AES_BLOCK_SIZE) != enc_len) {
        fprintf(stderr, "Encrypted length isn't a multiple of 'AES_BLOCK_SIZE'\n");
        return ERR_SSL_BAD_INPUT_DATA;
    }

    // The message will always be padded with the slight modification of PKCS7,
    // so we ensure the the `enc_len` is not 0 which would means there is no
    // paddding which isn't possible.
    if (enc_len == 0) {
        fprintf(stderr, "Encrypted message is too small to contain a padding");
        return ERR_SSL_BAD_INPUT_DATA;
    }

    // @Cleanup:
    // Not enough bytes in the output buffer to read the complete output
    // records. This isn't a great solution, we should simply keep an internal
    // buffer with the decrypted content and return as much as the user wants.
    if (buffer_len < enc_len) {
        return ERR_SSL_BUFFER_TOO_SMALL;
    }

    // We decrypt the data with the given IV.
    ret = mbedtls_aes_crypt_cbc(
        &ssl->cipher_dec,
        MBEDTLS_AES_DECRYPT,
        enc_len,
        iv,
        enc_data,
        buffer);

    if (ret != 0) {
        return ERR_SSL_BAD_INPUT_DATA;
    }

    uint8_t pad_val = buffer[enc_len - 1];
    uint8_t pad_len = pad_val + 1;

    if (enc_len < pad_len) {
        fprintf(stderr, "The padding length is larger than the actual message\n");
        return ERR_SSL_BAD_INPUT_DATA;
    }

    for (size_t i = (enc_len - pad_len); i < enc_len; ++i) {
        if (buffer[i] != pad_val) {
            fprintf(stderr, "Invalid padding value at %zu\n", i);
            return ERR_SSL_BAD_INPUT_DATA;
        }
    }

    // The padding is valid, we substract it from the acutal message len.
    enc_len -= pad_len;

    // Ensure the authenticity of the message by computing the HMAC.
    if ((ret = mbedtls_md_hmac_update(&ssl->mac_dec, ssl->next_read_id, sizeof(ssl->next_read_id))) != 0) {
        return ERR_SSL_UNSUCCESSFUL;
    }

    // Increment the packet number for next update.
    increment_be(ssl->next_read_id, sizeof(ssl->next_read_id));

    uint8_t ssl_header[5];
    assert(sizeof(ssl_header) <= ssl->read.size);
    memcpy(ssl_header, ssl->read.data, sizeof(ssl_header));

    if (enc_len < SHA1_DIGEST_SIZE) {
        fprintf(stderr, "Message is too small to contain an HMAC\n");
        return ERR_SSL_BAD_INPUT_DATA;
    }

    size_t msg_len = enc_len - SHA1_DIGEST_SIZE;
    if ((size_t)UINT16_MAX < msg_len) {
        fprintf(stderr, "msg_len too large to fit a uint16_t");
        return ERR_SSL_UNSUCCESSFUL;
    }

    // Not sure if it's on purpose, but the size of the IV and the HMAC are
    // substracted from the header before computing the HMAC. This is kind of
    // odd, seems unnecessary complexity and could be a bug in the implem of
    // ANet. Could also be part of the standard, to be confirmed.
    be16enc(ssl_header + 3, (uint16_t)msg_len);

    if ((ret = mbedtls_md_hmac_update(&ssl->mac_dec, ssl_header, sizeof(ssl_header))) != 0) {
        return ERR_SSL_UNSUCCESSFUL;
    }

    if ((ret = mbedtls_md_hmac_update(&ssl->mac_dec, buffer, msg_len)) != 0) {
        return ERR_SSL_UNSUCCESSFUL;
    }

    // Write the hmac in the output.
    uint8_t hmac[SHA1_DIGEST_SIZE];
    if ((ret = mbedtls_md_hmac_finish(&ssl->mac_dec, hmac)) != 0)
        return ERR_SSL_UNSUCCESSFUL;

    // Reset the hmac structure to allow re-using for the next packet.
    if ((ret = mbedtls_md_hmac_reset(&ssl->mac_dec)) != 0)
        return ERR_SSL_UNSUCCESSFUL;

    if (memcmp(buffer + msg_len, hmac, SHA1_DIGEST_SIZE) != 0) {
        fprintf(stderr, "The HMAC doesn't match\n");
        return ERR_SSL_BAD_INPUT_DATA;
    }

    size_t processed = (inner_data - ssl->read.data) + inner_len;
    array_remove_range_ordered(&ssl->read, 0, processed);

    *retlen = msg_len;
    return 0;
}

static void ssl_srp_write_srp(
    array_uint8_t *buffer,
    const char *username, size_t length)
{
    assert(length < (UINT8_MAX - 1));

    static const uint16_t EXTENSION_SRP = 12;
    array_add_be_uint16(buffer, EXTENSION_SRP);

    // The length of the extension which effectively encode the length of the
    // username on 1 byte and then the actual username. So, it's length + 1.
    array_add_be_uint16(buffer, (uint16_t)(length + 1));
    array_add(buffer, (uint8_t)length);

    // Add the username.
    array_insert(buffer, length, (uint8_t *)username);
}

static int ssl_srp_write_extension(struct ssl_sts_connection *ssl)
{
    // We first need to write the length on a `uint16be_t`, but we don't know
    // it yet. Instead, we reserve those bytes and we will write it later.
    size_t extensions_len_pos = array_size(&ssl->write);
    (void)array_push(&ssl->write, 2);

    // The first extension is undocumented and doesn't contain any data.
    // So we simply hardcode it.
    array_add_be_uint16(&ssl->write, 0xADAE);
    array_add_be_uint16(&ssl->write, 0); // Length is 0

    // The second extension is SRP specifying the username. (i.e., email)
    ssl_srp_write_srp(&ssl->write, ssl->srp_username, ssl->srp_username_len);

    size_t extension_len = array_size(&ssl->write) - (extensions_len_pos + 2);
    if ((size_t)UINT16_MAX < extension_len)
        return ERR_SSL_UNSUCCESSFUL;

    be16enc(&array_at(&ssl->write, extensions_len_pos), (uint16_t)extension_len);
    return 0;
}

static int ssl_srp_write_client_hello_body(struct ssl_sts_connection *ssl)
{
    // The version is always "\x03\x03". (i.e., TLS v1.2)
    array_add(&ssl->write, 0x03);
    array_add(&ssl->write, 0x03);

    // Write random bytes.
    {
        // This pointer gets invalidated whenever we do anything with the
        // array.
        uint8_t *random_time_ptr = array_push(&ssl->write, 4);
        be32enc(random_time_ptr, ssl->client_random.time);
    }

    array_insert(&ssl->write, sizeof(ssl->client_random.bytes), ssl->client_random.bytes);

    // The session id is always null
    array_add(&ssl->write, 0);

    // Write cipher suites
    static const uint16_t ciphers[] = {
        TLS_SRP_SHA_WITH_AES_256_CBC_SHA,
        TLS_SRP_SHA_WITH_AES_128_CBC_SHA,
        0xFF02, // No ideas what this is...
        0xFF01, // No ideas what this is...
        TLS_SRP_SHA_WITH_LEGACY_PASSWORD,
        0xFF03, // No ideas what this is...
    };

    assert(sizeof(ciphers) <= UINT16_MAX);
    array_add_be_uint16(&ssl->write, (uint16_t)sizeof(ciphers));
    for (size_t i = 0; i < ARRAY_SIZE(ciphers); ++i) {
        array_add_be_uint16(&ssl->write, ciphers[i]);
    }

    // Write compression methods.
    // This is hardcoded and means 1 compression methods named "null".
    // Not sure if required, but simply reproducing the behavior of the
    // official DLL.
    array_add(&ssl->write, 1);
    array_add(&ssl->write, 0);

    if (ssl_srp_write_extension(ssl) != 0) {
        return ERR_SSL_UNSUCCESSFUL;
    }

    return 0;
}

static int ssl_srp_write_client_hello(struct ssl_sts_connection *ssl)
{
    assert(ssl->state == AWAIT_CLIENT_HELLO);

    size_t header_pos;
    ssl_srp_start_handshake_msg(ssl, &header_pos, SSL_HS_CLIENT_HELLO);

    if (ssl_srp_write_client_hello_body(ssl) != 0) {
        return ERR_SSL_UNSUCCESSFUL;
    }

    if (ssl_srp_finish_handshake_msg(ssl, header_pos, SSL_HS_CLIENT_HELLO) != 0) {
        return ERR_SSL_UNSUCCESSFUL;
    }

    // @Cleanup: We need to calculate the checksum for the hashmac.

    ssl->state = AWAIT_SERVER_HELLO;
    return 0;
}

static int compute_client_public(struct ssl_sts_connection *ssl)
{
    int ret;
    mbedtls_mpi generator;
    mbedtls_mpi prime_modulus;
    mbedtls_mpi client_private;
    mbedtls_mpi client_public;

    mbedtls_mpi_init(&generator);
    mbedtls_mpi_init(&prime_modulus);
    mbedtls_mpi_init(&client_private);
    mbedtls_mpi_init(&client_public);

    if ((ret = mbedtls_mpi_read_binary(&prime_modulus, ssl->server_key.prime, sizeof(ssl->server_key.prime))) != 0) {
        goto cleanup;
    }
    if ((ret = mbedtls_mpi_read_binary(&generator, ssl->server_key.generator, sizeof(ssl->server_key.generator))) != 0) {
        goto cleanup;
    }
    if ((ret = mbedtls_mpi_read_binary(&client_private, ssl->client_key.private, sizeof(ssl->client_key.private))) != 0) {
        goto cleanup;
    }
    if ((ret = mbedtls_mpi_exp_mod(&client_public, &generator, &client_private, &prime_modulus, NULL)) != 0) {
        goto cleanup;
    }
    if ((ret = mbedtls_mpi_write_binary(&client_public, ssl->client_key.public, sizeof(ssl->client_key.public))) != 0) {
        goto cleanup;
    }

cleanup:
    mbedtls_mpi_free(&generator);
    mbedtls_mpi_free(&prime_modulus);
    mbedtls_mpi_free(&client_private);
    mbedtls_mpi_free(&client_public);
    if (ret != 0)
        ret = 1;
    return ret;
}

static int ssl_srp_write_client_key_exchange(struct ssl_sts_connection *ssl)
{
    assert(ssl->state == AWAIT_CLIENT_KEY_EXCHANGE);
    STATIC_ASSERT(sizeof(ssl->client_key.public) <= UINT16_MAX);

    int ret;
    if ((ret = compute_client_public(ssl)) != 0) {
        return ret;
    }

    size_t header_pos;
    ssl_srp_start_handshake_msg(ssl, &header_pos, SSL_HS_CLIENT_KEY_EXCHANGE);

    const uint16_t keylen = sizeof(ssl->client_key.public);
    array_add_be_uint16(&ssl->write, keylen);
    array_insert(&ssl->write, keylen, ssl->client_key.public);

    if (ssl_srp_finish_handshake_msg(ssl, header_pos, SSL_HS_CLIENT_KEY_EXCHANGE) != 0) {
        return ERR_SSL_UNSUCCESSFUL;
    }

    ssl->state = AWAIT_CLIENT_CHANGE_CIPHER_SPEC;
    return 0;
}

static int ssl_srp_write_change_cipher_spec(struct ssl_sts_connection *ssl)
{
    array_add(&ssl->write, SSL_MSG_CHANGE_CIPHER_SPEC);

    // The version is always "\x03\x03". (i.e., TLS v1.2)
    array_add(&ssl->write, 0x03);
    array_add(&ssl->write, 0x03);

    // We hardcode the length of the message, because we only send one.
    array_add_be_uint16(&ssl->write, 1);

    array_add(&ssl->write, 1);

    ssl->state = AWAIT_CLIENT_HANDSHAKE;
    return 0;
}

static int ssl_srp_write_finished(struct ssl_sts_connection *ssl)
{
    assert(ssl->state == AWAIT_CLIENT_HANDSHAKE);

    size_t size_at_start = ssl->write.size;

    size_t len = sizeof(ssl->client_finished);
    array_add(&ssl->write, SSL_HS_FINISHED);
    uint8_t *p = array_push(&ssl->write, 3);
    p[0] = (len >> 16) & 0xFF;
    p[1] = (len >> 8) & 0xFF;
    p[2] = len & 0xFF;

    array_insert(&ssl->write, sizeof(ssl->client_finished), ssl->client_finished);

    int ret;
    const uint8_t *pack = ssl->write.data + size_at_start;
    size_t pack_size = ssl->write.size - size_at_start;
    if ((ret = mbedtls_sha256_update(&ssl->checksum, pack, pack_size)) != 0)
        return ERR_SSL_UNSUCCESSFUL;

    ssl->state = AWAIT_SERVER_CHANGE_CIPHER_SPEC;
    return 0;
}

static int parse_server_hello(struct ssl_sts_connection *ssl, const uint8_t *data, size_t length)
{
    int ret;
    // The type is encoded on 1 byte and the length on 3 bytes.
    const size_t MIN_LEN = 4;

    if (length < MIN_LEN)
        return ERR_SSL_BAD_INPUT_DATA;

    if (data[0] != SSL_HS_SERVER_HELLO)
        return ERR_SSL_UNEXPECTED_MESSAGE;

    size_t content_len = be24dec(&data[1]);
    if ((length - MIN_LEN) != content_len) {
        // At this point, we should already know we have the complete data.
        return ERR_SSL_BAD_INPUT_DATA;
    }

    const uint8_t *content = &data[MIN_LEN];

    uint16_t version;
    if ((ret = chk_stream_read16(&content, &content_len, &version)) != 0)
        return ret;
    // @Cleanup: Maybe not the best error code to return...
    if (version != 0x0303)
        return ERR_SSL_BAD_INPUT_DATA;

    if ((ret = chk_stream_read32(&content, &content_len, &ssl->server_random.time)) != 0)
        return ret;
    if ((ret = chk_stream_read(&content, &content_len, ssl->server_random.bytes,
                               sizeof(ssl->server_random.bytes))) != 0) {
        return ret;
    }

    uint8_t session_id_len;
    if ((ret = chk_stream_read8(&content, &content_len, &session_id_len)) != 0)
        return ret;

    // @Cleanup: Should we distinguish with unsupported input data?!?
    if (session_id_len != 0)
        return ERR_SSL_BAD_INPUT_DATA;

    uint16_t cipher_suite;
    if ((ret = chk_stream_read16(&content, &content_len, &cipher_suite)) != 0)
        return ret;

    STATIC_ASSERT(sizeof(ssl->which_hash_verifier) == sizeof(ssl->static_legacy_hash));
    STATIC_ASSERT(sizeof(ssl->which_hash_verifier) == sizeof(ssl->static_verifier_hash));
    switch (cipher_suite) {
        case TLS_SRP_SHA_WITH_AES_128_CBC_SHA:
            // @Cleanup:
            // We should support this one, since we told the server we did.
            fprintf(stderr, "Received TLS_SRP_SHA_WITH_AES_128_CBC_SHA (0x%X) which we didn't implement\n", TLS_SRP_SHA_WITH_AES_128_CBC_SHA);
            return ERR_SSL_BAD_INPUT_DATA;
        case TLS_SRP_SHA_WITH_AES_256_CBC_SHA:
            memcpy(ssl->which_hash_verifier, ssl->static_verifier_hash, sizeof(ssl->which_hash_verifier));
            break;
        case TLS_SRP_SHA_WITH_LEGACY_PASSWORD:
            memcpy(ssl->which_hash_verifier, ssl->static_legacy_hash, sizeof(ssl->which_hash_verifier));
            break;
        default:
            return ERR_SSL_BAD_INPUT_DATA;
    }

    uint8_t compression_method;
    if ((ret = chk_stream_read8(&content, &content_len, &compression_method)) != 0)
        return ret;
    if (compression_method != 0)
        return ERR_SSL_BAD_INPUT_DATA;

    uint16_t extension_len;
    if ((ret = chk_stream_read16(&content, &content_len, &extension_len)) != 0)
        return ret;
    if (content_len != extension_len)
        return ERR_SSL_BAD_INPUT_DATA;

    // @Cleanup: Should we read the extension and ensure we get what we expect?
    return 0;
}

static int ssl_srp_process_server_hello(struct ssl_sts_connection *ssl)
{
    assert(ssl->state == AWAIT_SERVER_HELLO);

    int ret;
    const uint8_t *inner_data;
    size_t inner_len;

    ret = parse_tls12_handshake(ssl, SSL_MSG_HANDSHAKE, &inner_data, &inner_len, 1);
    if (ret != 0) {
        return ret;
    }

    if ((ret = parse_server_hello(ssl, inner_data, inner_len)) != 0)
        return ret;

    size_t processed = (inner_data - ssl->read.data) + inner_len;
    array_remove_range_ordered(&ssl->read, 0, processed);
    ssl->state = AWAIT_SERVER_KEY_EXCHANGE;
    return 0;
}

static int parse_server_key_exchange(struct server_key *key, const uint8_t *data, size_t length)
{
    int ret;
    // The type is encoded on 1 byte and the length on 3 bytes.
    const size_t MIN_LEN = 4;

    if (length < MIN_LEN)
        return ERR_SSL_BAD_INPUT_DATA;

    if (data[0] != SSL_HS_SERVER_KEY_EXCHANGE)
        return ERR_SSL_UNEXPECTED_MESSAGE;

    size_t content_len = be24dec(&data[1]);
    if ((length - MIN_LEN) != content_len) {
        // At this point, we should already know we have the complete data.
        return ERR_SSL_BAD_INPUT_DATA;
    }

    const uint8_t *content = &data[MIN_LEN];

    // Currently, we support only an hardcoded format, with specified length
    // for every integers. This isn't a requirement of the protocol, but it's
    // simpler for us.

    uint16_t prime_len;
    if ((ret = chk_stream_read16(&content, &content_len, &prime_len)) != 0)
        return ret;
    if (prime_len != sizeof(key->prime)) {
        // @Cleanup: Add trace prints.
        return ERR_SSL_UNSUPPORTED_PROTOCOL;
    }
    if ((ret = chk_stream_read(&content, &content_len, key->prime, sizeof(key->prime))) != 0)
        return ret;

    uint16_t generator_len;
    if ((ret = chk_stream_read16(&content, &content_len, &generator_len)) != 0)
        return ret;
    if (generator_len != sizeof(key->generator)) {
        // @Cleanup: Add trace prints.
        return ERR_SSL_UNSUPPORTED_PROTOCOL;
    }
    if ((ret = chk_stream_read(&content, &content_len, key->generator, sizeof(key->generator))) != 0)
        return ret;

    uint8_t salt_len;
    if ((ret = chk_stream_read8(&content, &content_len, &salt_len)) != 0)
        return ret;
    if (salt_len != sizeof(key->salt)) {
        // @Cleanup: Add trace prints.
        return ERR_SSL_UNSUPPORTED_PROTOCOL;
    }
    if ((ret = chk_stream_read(&content, &content_len, key->salt, sizeof(key->salt))) != 0)
        return ret;

    uint16_t server_public_len;
    if ((ret = chk_stream_read16(&content, &content_len, &server_public_len)) != 0)
        return ret;
    if (server_public_len != sizeof(key->server_public)) {
        // @Cleanup: Add trace prints.
        return ERR_SSL_UNSUPPORTED_PROTOCOL;
    }
    if ((ret = chk_stream_read(&content, &content_len, key->server_public, sizeof(key->server_public))) != 0)
        return ret;

    if (content_len != 0)
        return ERR_SSL_BAD_INPUT_DATA;

    return 0;
}

static int ssl_srp_process_server_key_exchange(struct ssl_sts_connection *ssl)
{
    assert(ssl->state == AWAIT_SERVER_KEY_EXCHANGE);

    int ret;
    const uint8_t *inner_data;
    size_t inner_len;

    if ((ret = parse_tls12_handshake(ssl, SSL_MSG_HANDSHAKE, &inner_data, &inner_len, 1)) != 0) {
        return ret;
    }

    if ((ret = parse_server_key_exchange(&ssl->server_key, inner_data, inner_len)) != 0)
        return ret;

    size_t processed = (inner_data - ssl->read.data) + inner_len;
    array_remove_range_ordered(&ssl->read, 0, processed);
    ssl->state = AWAIT_SERVER_HELLO_DONE;
    return 0;
}

static int ssl_srp_process_server_done(struct ssl_sts_connection *ssl)
{
    assert(ssl->state == AWAIT_SERVER_HELLO_DONE);

    int ret;
    const uint8_t *inner_data;
    size_t inner_len;

    if ((ret = parse_tls12_handshake(ssl, SSL_MSG_HANDSHAKE, &inner_data, &inner_len, 1)) != 0) {
        return ret;
    }

    uint8_t expected[] = "\x0e\x00\x00\x00";
    if (inner_len != (sizeof(expected) - 1))
        return ERR_SSL_BAD_INPUT_DATA;

    if (memcmp(inner_data, expected, inner_len) != 0)
        return ERR_SSL_BAD_INPUT_DATA;

    size_t processed = (inner_data - ssl->read.data) + inner_len;
    array_remove_range_ordered(&ssl->read, 0, processed);
    ssl->state = AWAIT_CLIENT_KEY_EXCHANGE;
    return 0;
}

static int ssl_srp_process_change_cipher_spec(struct ssl_sts_connection *ssl)
{
    assert(ssl->state == AWAIT_SERVER_CHANGE_CIPHER_SPEC);

    int ret;
    const uint8_t *inner_data;
    size_t inner_len;

    if ((ret = parse_tls12_handshake(ssl, SSL_MSG_CHANGE_CIPHER_SPEC, &inner_data, &inner_len, 0)) != 0) {
        return ret;
    }

    uint8_t expected[] = "\x01";
    if (inner_len != (sizeof(expected) - 1))
        return ERR_SSL_BAD_INPUT_DATA;

    if (memcmp(inner_data, expected, inner_len) != 0)
        return ERR_SSL_BAD_INPUT_DATA;

    size_t processed = (inner_data - ssl->read.data) + inner_len;
    array_remove_range_ordered(&ssl->read, 0, processed);
    ssl->state = AWAIT_SERVER_ENC_HANDSHAKE;

    return 0;
}

static int ssl_srp_process_encrypted_handshake(struct ssl_sts_connection *ssl)
{
    assert(ssl->state == AWAIT_SERVER_ENC_HANDSHAKE);

    uint8_t buffer[1024];
    size_t msg_len;
    int ret = ssl_sts_connection_recv_internal(
        ssl,
        SSL_MSG_HANDSHAKE,
        buffer, sizeof(buffer),
        &msg_len);

    if (ret != 0) {
        return ret;
    }

    const size_t EXPECTED_LEN = 16;
    if (msg_len != EXPECTED_LEN) {
        return ERR_SSL_BAD_INPUT_DATA;
    }

    uint8_t checksum[32];
    if ((ret = mbedtls_sha256_finish(&ssl->checksum, checksum)) != 0)
        return ERR_SSL_UNSUCCESSFUL;

    const char server_finished_lbl[] = "server finished";
    ret = tls_prf_sha256(
        ssl->master_secret, sizeof(ssl->master_secret),
        server_finished_lbl, sizeof(server_finished_lbl) - 1,
        checksum, sizeof(checksum),
        ssl->server_finished, sizeof(ssl->server_finished));

    if (ret != 0)
        return ret;

    // Not the best check, but it effecetively means:
    // "SSL_MSG_CHANGE_CIPHER_SPEC", "length of 24 bits".
    const uint8_t expected_header[] = "\x14\x00\x00\x0C";
    if (memcmp(buffer, expected_header, sizeof(expected_header) - 1) != 0)
        return ERR_SSL_BAD_INPUT_DATA;

    if (memcmp(buffer + 4, ssl->server_finished, sizeof(ssl->server_finished)) != 0)
        return ERR_SSL_BAD_INPUT_DATA;

    ssl->state = AWAIT_CLIENT_FINISHED;
    return 0;
}

int ssl_srp_compute_premaster_secret(struct ssl_sts_connection *ssl)
{
    int ret;

    uint8_t verifier_digest[SHA1_DIGEST_SIZE];
    ret = sha1_of_two_values(
        verifier_digest,
        ssl->server_key.salt, sizeof(ssl->server_key.salt),
        ssl->which_hash_verifier, sizeof(ssl->which_hash_verifier));
    if (ret != 0)
        return ret;

    const size_t padding = sizeof(ssl->server_key.prime);

    uint8_t u_buffer[SHA1_DIGEST_SIZE];
    ret = sha1_of_two_values_with_padding(
        u_buffer, padding,
        ssl->client_key.public, sizeof(ssl->client_key.public),
        ssl->server_key.server_public, sizeof(ssl->server_key.server_public));
    if (ret != 0)
        return ret;

    uint8_t k_buffer[SHA1_DIGEST_SIZE];
    ret = sha1_of_two_values_with_padding(
        k_buffer, padding,
        ssl->server_key.prime, sizeof(ssl->server_key.prime),
        ssl->server_key.generator, sizeof(ssl->server_key.generator));
    if (ret != 0)
        return ret;

    // Calculating the pre-master secret.
    mbedtls_mpi generator;
    mbedtls_mpi prime;
    mbedtls_mpi server_public;
    mbedtls_mpi client_public;
    mbedtls_mpi client_private;
    mbedtls_mpi verifier;
    mbedtls_mpi u;
    mbedtls_mpi k;

    mbedtls_mpi_init(&generator); // Also "g"
    mbedtls_mpi_init(&prime); // Also "N"
    mbedtls_mpi_init(&server_public); // Also "B"
    mbedtls_mpi_init(&client_public); // Also "A"
    mbedtls_mpi_init(&client_private); // Also "a"
    mbedtls_mpi_init(&verifier); // Also "x"
    mbedtls_mpi_init(&u);
    mbedtls_mpi_init(&k);

    // Those values are the outputs or the temporary values.
    mbedtls_mpi premaster_secret;
    mbedtls_mpi base;
    mbedtls_mpi base1;
    mbedtls_mpi base2;
    mbedtls_mpi exponent;
    mbedtls_mpi exponent1;

    mbedtls_mpi_init(&premaster_secret);
    mbedtls_mpi_init(&base);
    mbedtls_mpi_init(&base1);
    mbedtls_mpi_init(&base2);
    mbedtls_mpi_init(&exponent);
    mbedtls_mpi_init(&exponent1);

    if ((ret = mbedtls_mpi_read_binary(&generator, ssl->server_key.generator, sizeof(ssl->server_key.generator))) != 0) {
        goto cleanup;
    }
    if ((ret = mbedtls_mpi_read_binary(&prime, ssl->server_key.prime, sizeof(ssl->server_key.prime))) != 0) {
        goto cleanup;
    }
    if ((ret = mbedtls_mpi_read_binary(&server_public, ssl->server_key.server_public, sizeof(ssl->server_key.server_public))) != 0) {
        goto cleanup;
    }
    if ((ret = mbedtls_mpi_read_binary(&client_public, ssl->client_key.public, sizeof(ssl->client_key.public))) != 0) {
        goto cleanup;
    }
    if ((ret = mbedtls_mpi_read_binary(&client_private, ssl->client_key.private, sizeof(ssl->client_key.private))) != 0) {
        goto cleanup;
    }
    if ((ret = mbedtls_mpi_read_binary(&verifier, verifier_digest, sizeof(verifier_digest))) != 0) {
        goto cleanup;
    }
    if ((ret = mbedtls_mpi_read_binary(&u, u_buffer, sizeof(u_buffer))) != 0) {
        goto cleanup;
    }
    if ((ret = mbedtls_mpi_read_binary(&k, k_buffer, sizeof(k_buffer))) != 0) {
        goto cleanup;
    }

    // The formula is:
    // <premaster secret> = (B - (k * g^x)) ^ (a + (u * x)) % N
    // base = (B - (k * g^x))
    //  base1 = g^x
    //  base2 = k * g^x
    if ((ret = mbedtls_mpi_exp_mod(&base1, &generator, &verifier, &prime, NULL)) != 0)
        goto cleanup;
    if ((ret = mbedtls_mpi_mul_mpi(&base2, &k, &base1)) != 0)
        goto cleanup;
    if ((ret = mbedtls_mpi_sub_mpi(&base, &server_public, &base2)) != 0)
        goto cleanup;

    // exponent = (a + (u * x))
    //  exponent1 = (u * x)
    if ((ret = mbedtls_mpi_mul_mpi(&exponent1, &u, &verifier)) != 0)
        goto cleanup;
    if ((ret = mbedtls_mpi_add_mpi(&exponent, &client_private, &exponent1)) != 0)
        goto cleanup;

    // Calculate the premastered secret
    if ((ret = mbedtls_mpi_exp_mod(&premaster_secret, &base, &exponent, &prime, NULL)) != 0)
        goto cleanup;

    // Save the pre-master secret as bytes. It will later be used to
    // derive keys used for encryption ciphers.
    STATIC_ASSERT(sizeof(ssl->premaster_secret) == sizeof(ssl->server_key.prime));
    if ((ret = mbedtls_mpi_write_binary(&premaster_secret, ssl->premaster_secret, sizeof(ssl->premaster_secret))) != 0)
        goto cleanup;

    return 0;

cleanup:;
    mbedtls_mpi_free(&generator);
    mbedtls_mpi_free(&prime);
    mbedtls_mpi_free(&server_public);
    mbedtls_mpi_free(&client_public);
    mbedtls_mpi_free(&client_private);
    mbedtls_mpi_free(&verifier);
    mbedtls_mpi_free(&u);
    mbedtls_mpi_free(&k);

    mbedtls_mpi_free(&premaster_secret);
    mbedtls_mpi_free(&base);
    mbedtls_mpi_free(&base1);
    mbedtls_mpi_free(&base2);
    mbedtls_mpi_free(&exponent);
    mbedtls_mpi_free(&exponent1);

    if (ret != 0)
        ret = 1;
    return ret;
}

static void serialize_tls12_random(uint8_t *buffer, struct tls12_random *random)
{
    be32enc(buffer, random->time);
    memcpy(buffer + 4, random->bytes, sizeof(random->bytes));
}

static int ssl_sts_connection_setup_ciphers(struct ssl_sts_connection *ssl)
{
    int ret;

    if ((ret = ssl_srp_compute_premaster_secret(ssl)) != 0) {
        return ret;
    }

    const char master_secret_lbl[] = "master secret";

    // For the master secret, the "random buffer" is the concatenation
    // of the client random followed by the server random.
    uint8_t random[sizeof(struct tls12_random) * 2];
    serialize_tls12_random(random, &ssl->client_random);
    serialize_tls12_random(random + sizeof(struct tls12_random), &ssl->server_random);

    ret = tls_prf_sha256(
        ssl->premaster_secret, sizeof(ssl->premaster_secret),
        master_secret_lbl, sizeof(master_secret_lbl) - 1,
        random, sizeof(random),
        ssl->master_secret, sizeof(ssl->master_secret));

    if (ret != 0)
        return ret;

    const char key_expansion_lbl[] = "key expansion";

    // For the master secret, the "random" buffer is the concatenation
    // of the server random followed by the client random.
    serialize_tls12_random(random, &ssl->server_random);
    serialize_tls12_random(random + sizeof(struct tls12_random), &ssl->client_random);

    uint8_t key_expansion[104];
    ret = tls_prf_sha256(
        ssl->master_secret, sizeof(ssl->master_secret),
        key_expansion_lbl, sizeof(key_expansion_lbl) - 1,
        random, sizeof(random),
        key_expansion, sizeof(key_expansion));

    if (ret != 0)
        return ret;

    // Those keys are used to initialize the following and in order:
    // 1. SHA1-HMAC for outgoing traffic.
    // 2. SHA1-HMAC for incoming traffic.
    // 3. AES256-CBC for outgoing traffic.
    // 4. AES256-CBC for incoming traffic.
    const uint8_t *mac_enc_key = key_expansion;
    const uint8_t *mac_dec_key = key_expansion + 20;
    const uint8_t *cipher_enc_key = key_expansion + 40;
    const uint8_t *cipher_dec_key = key_expansion + 72;

    const uint16_t CIPHER_KEY_BITS = 32 * 8;
    if ((ret = mbedtls_aes_setkey_enc(&ssl->cipher_enc, cipher_enc_key, CIPHER_KEY_BITS)) != 0)
        return ERR_SSL_UNSUCCESSFUL;
    if ((ret = mbedtls_aes_setkey_dec(&ssl->cipher_dec, cipher_dec_key, CIPHER_KEY_BITS)) != 0)
        return ERR_SSL_UNSUCCESSFUL;

    const mbedtls_md_info_t *md_info;
    if ((md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA1)) == NULL)
        return ERR_SSL_UNSUCCESSFUL;

    const int is_hmac = 1;
    const size_t HMAC_KEY_LEN = 20;

    if ((ret = mbedtls_md_setup(&ssl->mac_enc, md_info, is_hmac)) != 0)
        return ERR_SSL_UNSUCCESSFUL;
    if ((ret = mbedtls_md_hmac_starts(&ssl->mac_enc, mac_enc_key, HMAC_KEY_LEN)) != 0)
        return ERR_SSL_UNSUCCESSFUL;

    if ((ret = mbedtls_md_setup(&ssl->mac_dec, md_info, is_hmac)) != 0)
        return ERR_SSL_UNSUCCESSFUL;
    if ((ret = mbedtls_md_hmac_starts(&ssl->mac_dec, mac_dec_key, HMAC_KEY_LEN)) != 0)
        return ERR_SSL_UNSUCCESSFUL;

    const char client_finished_lbl[] = "client finished";

    mbedtls_sha256_context checksum_ctx;
    mbedtls_sha256_init(&checksum_ctx);
    mbedtls_sha256_clone(&checksum_ctx, &ssl->checksum);

    uint8_t checksum[32];
    ret = mbedtls_sha256_finish(&checksum_ctx, checksum);
    mbedtls_sha256_free(&checksum_ctx);

    if (ret != 0) {
        return ERR_SSL_UNSUCCESSFUL;
    }

    ret = tls_prf_sha256(
        ssl->master_secret, sizeof(ssl->master_secret),
        client_finished_lbl, sizeof(client_finished_lbl) - 1,
        checksum, sizeof(checksum),
        ssl->client_finished, sizeof(ssl->client_finished));

    if (ret != 0)
        return ret;

    return 0;
}

static int wait_for_server_step(int (*stepf)(struct ssl_sts_connection *ssl), struct ssl_sts_connection *ssl)
{
    int ret;
    for (;;) {
        ret = stepf(ssl);

        // `ERR_SSL_CONTINUE_PROCESSING` means we don't have enough data,
        // so we read more from the network.
        if (ret != ERR_SSL_CONTINUE_PROCESSING)
            break;

        if ((ret = recv_to_buffer(ssl->fd, &ssl->read)) != 0) {
            fprintf(stderr, "Failed to recv data from network, errno: %d\n", ret);
            return ERR_SSL_IO_FAILURE;
        }
    }

    return ret;
}

static int send_client_step(int (*stepf)(struct ssl_sts_connection *ssl), struct ssl_sts_connection *ssl)
{
    assert(array_size(&ssl->write) == 0);

    int ret;
    if ((ret = stepf(ssl)) != 0)
        return ret;
    if ((ret = send_full(ssl->fd, ssl->write.data, ssl->write.size)) != 0)
        return ERR_SSL_IO_FAILURE;
    array_clear(&ssl->write);
    return 0;
}

int ssl_sts_connection_handshake(struct ssl_sts_connection *ssl)
{
    if (ssl->state != AWAIT_CLIENT_HELLO) {
        fprintf(stderr, "Invalid state (%d) the proceed to the handshake\n", (int)ssl->state);
        return ERR_SSL_UNSUCCESSFUL;
    }

    int ret;
    if ((ret = send_client_step(ssl_srp_write_client_hello, ssl)) != 0) {
        fprintf(stderr, "'ssl_srp_write_client_hello' failed: %d %s\n", ret, ssl_err_string(ret));
        return ERR_SSL_UNSUCCESSFUL;
    }

    if ((ret = wait_for_server_step(ssl_srp_process_server_hello, ssl)) != 0) {
        fprintf(stderr, "'ssl_srp_process_server_hello' failed: %d %s\n", ret, ssl_err_string(ret));
        return ERR_SSL_UNSUCCESSFUL;
    }

    if ((ret = wait_for_server_step(ssl_srp_process_server_key_exchange, ssl)) != 0) {
        fprintf(stderr, "'ssl_srp_process_server_key_exchange' failed: %d %s\n", ret, ssl_err_string(ret));
        return ERR_SSL_UNSUCCESSFUL;
    }

    if ((ret = wait_for_server_step(ssl_srp_process_server_done, ssl)) != 0) {
        fprintf(stderr, "'ssl_srp_process_server_done' failed: %d %s\n", ret, ssl_err_string(ret));
        return ERR_SSL_UNSUCCESSFUL;
    }

    if ((ret = send_client_step(ssl_srp_write_client_key_exchange, ssl)) != 0) {
        fprintf(stderr, "'ssl_srp_write_client_key_exchange' failed: %d %s\n", ret, ssl_err_string(ret));
        return ERR_SSL_UNSUCCESSFUL;
    }

    if ((ret = send_client_step(ssl_srp_write_change_cipher_spec, ssl)) != 0) {
        fprintf(stderr, "'ssl_srp_write_change_cipher_spec' failed: %d %s\n", ret, ssl_err_string(ret));
        return ERR_SSL_UNSUCCESSFUL;
    }

    if ((ret = ssl_sts_connection_setup_ciphers(ssl)) != 0) {
        fprintf(stderr, "ssl_sts_connection_setup_ciphers failed: %d %s\n", ret, ssl_err_string(ret));
        return ERR_SSL_UNSUCCESSFUL;
    }

    if ((ret = ssl_srp_write_finished(ssl)) != 0) {
        fprintf(stderr, "ssl_srp_write_finished failed: %d %s\n", ret, ssl_err_string(ret));
        return ERR_SSL_UNSUCCESSFUL;
    }

    if ((ret = ssl_sts_connection_send_internal(ssl, SSL_MSG_HANDSHAKE, ssl->write.data, ssl->write.size)) != 0) {
        fprintf(stderr, "ssl_sts_connection_send_internal failed: %d %s\n", ret, ssl_err_string(ret));
        return ERR_SSL_UNSUCCESSFUL;
    }

    if ((ret = wait_for_server_step(ssl_srp_process_change_cipher_spec, ssl)) != 0) {
        fprintf(stderr, "'ssl_srp_process_change_cipher_spec' failed: %d %s\n", ret, ssl_err_string(ret));
        return ERR_SSL_UNSUCCESSFUL;
    }

    if ((ret = wait_for_server_step(ssl_srp_process_encrypted_handshake, ssl)) != 0) {
        fprintf(stderr, "'ssl_srp_process_encrypted_handshake' failed: %d %s\n", ret, ssl_err_string(ret));
        return ERR_SSL_UNSUCCESSFUL;
    }

    array_clear(&ssl->read);
    array_clear(&ssl->write);
    ssl->state = AWAIT_CLIENT_FINISHED;
    return 0;
}

int ssl_sts_connection_send(struct ssl_sts_connection *ssl, const uint8_t *data, size_t data_len)
{
    return ssl_sts_connection_send_internal(
        ssl,
        SSL_MSG_APPLICATION_DATA,
        data,
        data_len);
}

int ssl_sts_connection_recv(
    struct ssl_sts_connection *ssl,
    uint8_t *buffer, size_t buffer_len,
    size_t *retlen)
{
    return ssl_sts_connection_recv_internal(
        ssl,
        SSL_MSG_APPLICATION_DATA,
        buffer, buffer_len,
        retlen);
}

