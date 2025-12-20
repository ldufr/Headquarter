#define SHA1_BLOCK_SIZE  64
#define SHA1_DIGEST_SIZE 20 

static void hmac_sha1(const uint8_t *secret, size_t n_secret, const uint8_t *data, size_t n_data, uint8_t *result)
{
    mbedtls_sha1_context ctx;

    uint8_t key[SHA1_BLOCK_SIZE];
    memset(key, 0, sizeof(key));

    if (n_secret <= SHA1_BLOCK_SIZE) {
        memcpy(key, secret, n_secret);
    } else {
        mbedtls_sha1_init(&ctx);
        mbedtls_sha1_starts(&ctx);
        mbedtls_sha1_update(&ctx, secret, n_secret);
        mbedtls_sha1_finish(&ctx, key);
    }

    uint8_t ipad[SHA1_BLOCK_SIZE];
    memset(ipad, 0x36, sizeof(ipad));

    uint8_t opad[SHA1_BLOCK_SIZE];
    memset(opad, 0x5C, sizeof(opad));

    for (size_t idx = 0; idx < SHA1_BLOCK_SIZE; ++idx) {
        ipad[idx] ^= key[idx];
        opad[idx] ^= key[idx];
    }

    uint8_t stage1[SHA1_DIGEST_SIZE];

    mbedtls_sha1_init(&ctx);
    mbedtls_sha1_starts(&ctx);
    mbedtls_sha1_update(&ctx, ipad, sizeof(ipad));
    mbedtls_sha1_update(&ctx, data, n_data);
    mbedtls_sha1_finish(&ctx, stage1);

    mbedtls_sha1_init(&ctx);
    mbedtls_sha1_starts(&ctx);
    mbedtls_sha1_update(&ctx, opad, sizeof(opad));
    mbedtls_sha1_update(&ctx, stage1, sizeof(stage1));
    mbedtls_sha1_finish(&ctx, result);
}

static bool b32decode(const char *input, size_t len, array_uint8_t *result)
{
    uint32_t tmp = 0;
    uint32_t left = 0;

    array_reserve(result, ((len + 4) / 5) * 8);
    for (size_t idx = 0; idx < len; ++idx) {
        char ch = input[idx];
        if (ch == '=') {
            for (idx = idx + 1; idx < len; ++idx) {
                if (input[idx] != '=') {
                    return false;
                }
            }
            break;
        }

        tmp = (tmp << 5);
        left += 5;

        if ('A' <= ch && ch <= 'Z') {
            tmp += (uint32_t)(ch - 'A');
        } else if ('a' <= ch && ch <= 'z') {
            tmp += (uint32_t)(ch - 'a');
        } else if ('2' <= ch && ch <= '7') {
            tmp += (uint32_t)((ch - '2') + 26);
        } else {
            return false;
        }

        if (8 <= left) {
            array_add(result, (uint8_t)(tmp >> (left - 8)));
            tmp &= 0xFF;
            left -= 8;
        }
    }

    return true;
}

static const uint32_t MOD_TABLE[] = {
    1,
    10,
    100,
    1000,
    10000,
    100000,
    1000000,
    10000000,
    100000000,
    1000000000,
    UINT32_MAX,
};

static bool totp(const char *secret, size_t digits, uint32_t *result)
{
    array_uint8_t bytes = {0};
    if (!b32decode(secret, strlen(secret), &bytes)) {
        array_reset(&bytes);
        return false;
    }

    uint64_t time_since_epoch_secs = time_get_epoch_sec();
    uint64_t moving_factor = time_since_epoch_secs / 30;
    uint8_t moving_factor_bytes[sizeof(moving_factor)];
    be64enc(moving_factor_bytes, moving_factor);

    uint8_t mac[SHA1_DIGEST_SIZE];
    hmac_sha1(bytes.data, bytes.size, moving_factor_bytes, sizeof(moving_factor_bytes), mac);
    array_reset(&bytes);

    uint8_t offset = mac[19] & 0xf;
    uint32_t number = ((uint32_t)(mac[offset + 0] & 0x7F) << 24) |
                      ((uint32_t)(mac[offset + 1] & 0xFF) << 16) |
                      ((uint32_t)(mac[offset + 2] & 0xFF) << 8) |
                      ((uint32_t)(mac[offset + 3] & 0xFF));

    if (digits < ARRAY_SIZE(MOD_TABLE)) {
        *result = number % MOD_TABLE[digits];
    } else {
        *result = number;
    }

    return true;
}
