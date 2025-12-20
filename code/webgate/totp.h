#pragma once

static void hmac_sha1(
    const uint8_t *secret,
    size_t n_secret,
    const uint8_t *data,
    size_t n_data,
    uint8_t *result);

static bool totp(const char *secret, size_t digits, uint32_t *result);
