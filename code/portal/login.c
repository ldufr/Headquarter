#ifdef PORTAL_LOGIN_C
#error "login.c included more than once"
#endif
#define PORTAL_LOGIN_C

#define PORTAL_ERR_UKNOWN_ERROR      1
#define PORTAL_ERR_2FA_REQUIRE_TOTP  2
#define PORTAL_ERR_2FA_REQUIRE_EMAIL 3
#define PORTAL_ERR_2FA_REQUIRE_SMS   4

struct str {
    const char *ptr;
    size_t      len;
};

static const uint32_t TIMEOUT_MS = 29 * 1000;

static struct str s_from_unsigned_utf8(const uint8_t *ptr, size_t len)
{
    return (struct str){(const char *)ptr, len};
}

static struct str s_from_signed_utf8(const char *ptr, size_t len)
{
    return (struct str){ptr, len};
}

static struct str s_from_c_str(const char *ptr)
{
    size_t len = strlen(ptr);
    return s_from_signed_utf8(ptr, len);
}

static int s_cmp(struct str *str1, struct str *str2)
{
    if (str1->len < str2->len)
        return -1;
    else if (str2->len < str1->len)
        return 1;
    else
        return memcmp(str1->ptr, str2->ptr, str1->len);
}

// Find str1 in str2
static size_t s_find(struct str *str1, struct str *str2)
{
    if (str1->len < str2->len)
        return (size_t)-1;

    size_t count = str1->len - str2->len;
    for (size_t i = 0; i < count; ++i) {
        struct str temp = {
            .ptr = str1->ptr + i,
            .len = str2->len,
        };

        if (s_cmp(&temp, str2) == 0)
            return i;
    }

    return (size_t)-1;
}

static struct str s_substr(struct str *str, size_t pos, size_t len)
{
    if (str->len < pos)
        pos = str->len;
    if ((str->len - pos) < len)
        len = str->len - pos;
    return (struct str){ .ptr = str->ptr + pos, .len = len };
}

static bool s_parse_uuid(struct str *str, struct uuid *u)
{
    // This check means that the `sscanf` is safe.
    if (str->len != sizeof("AABBCCDD-AABB-AABB-AABB-AABBCCDDEEFF") - 1) {
        return false;
    }

    int ret = sscanf(
        str->ptr,
        "%08" SCNx32 "-%04" SCNx16 "-%04" SCNx16 "-%02hhx%02hhx-%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx",
        &u->time_low, &u->time_mid, &u->time_hi_and_version,
        &u->clock_seq_hi_and_reserved, &u->clock_seq_low, &u->node[0],
        &u->node[1], &u->node[2], &u->node[3], &u->node[4], &u->node[5]);

    if (ret != 11)
        return false;

    return true;
}

static int find_between(struct str *dest, struct str *src, const char *left, const char *right)
{
    struct str l = s_from_c_str(left);
    struct str r = s_from_c_str(right);

    size_t left_pos = s_find(src, &l);
    if (left_pos == (size_t)-1) {
        return 1;
    }

    struct str rem = s_substr(src, (left_pos + l.len), SIZE_MAX);
    size_t right_pos = s_find(&rem, &r);
    if (right_pos == (size_t)-1) {
        return 1;
    }

    *dest = s_substr(&rem, 0, right_pos);
    return 0;
}

static int recv_sts_response(
    struct ssl_sts_connection *ssl,
    array_uint8_t *response,
    struct sts_reply *reply,
    size_t expected_seq_number)
{
    int ret;

    for (;;) {
        const size_t BUFFER_SIZE = 1024;
        uint8_t *ptr = array_push(response, BUFFER_SIZE);

        size_t retlen;
        if ((ret = ssl_sts_connection_recv(ssl, ptr, BUFFER_SIZE, &retlen)) != 0)
            return 1;

        response->size = array_size(response) - (BUFFER_SIZE - retlen);

        ret = sts_parse_reply(reply, response->data, response->size);
        if (ret != STSE_INCOMPLETE_CONTENT && ret != STSE_INCOMPLETE_HEADER) {
            break;
        }
    }

    if (ret != STSE_SUCCESS) {
        return 1;
    }

    if (reply->status_code != 200) {
        fprintf(stderr, "Unexpected status code %u\n", reply->status_code);
        return 1;
    }

    if (reply->sequence_number != expected_seq_number) {
        fprintf(stderr, "Sequence number doesn't match expected sequence number\n");
        return 1;
    }

    return 0;
}

static int auth_login_finish(struct sts_connection *sts, struct ssl_sts_connection *ssl)
{
    const char url[] = "/Auth/LoginFinish";
    const size_t url_len = sizeof(url) - 1;

    array_uint8_t content;
    array_init(&content);
    array_reserve(&content, 1024);

    appendf(&content, "<Request>\n");
    appendf(&content, "<Language>en</Language>\n");
    appendf(&content, "</Request>\n");

    array_uint8_t request;
    array_init(&request);
    array_reserve(&request, 1024);
    int ret = sts_write_request_with_sequence_number(
        &request,
        url, url_len,
        ++sts->seq_number,
        TIMEOUT_MS,
        content.data, content.size);

    array_reset(&content);

    if (ret != 0) {
        array_reset(&request);
        return ret;
    }

    ret = ssl_sts_connection_send(ssl, request.data, request.size);
    array_reset(&request);

    if (ret != 0) {
        return ret;
    }

    struct sts_reply reply = {0};
    array_uint8_t response;
    array_init(&response);
    array_reserve(&response, 1024);

    if ((ret = recv_sts_response(ssl, &response, &reply, sts->seq_number)) != 0) {
        array_reset(&response);
        return ret;
    }

    if (reply.status_code != 200) {
        fprintf(stderr, "Reply to '%s' failed with status %u\n", url, reply.status_code);
        array_reset(&response);
        return 1;
    }

    struct str reply_content = s_from_unsigned_utf8(reply.content, reply.content_length);

    struct str auth_type;
    if (find_between(&auth_type, &reply_content, "<AuthType>", "</AuthType>") == 0) {
        // This mean we need to complete a 2fa.
        struct str sms = s_from_c_str("SMS");
        struct str totp = s_from_c_str("Totp");
        struct str email = s_from_c_str("Email");
        if (s_cmp(&auth_type, &totp) == 0) {
            ret = PORTAL_ERR_2FA_REQUIRE_TOTP;
        } else if (s_cmp(&auth_type, &email) == 0) {
            ret = PORTAL_ERR_2FA_REQUIRE_EMAIL;
        } else if (s_cmp(&auth_type, &sms) == 0) {
            ret = PORTAL_ERR_2FA_REQUIRE_SMS;
        } else {
            fprintf(stderr, "Unknown AuthType: '%.*s'\n", (int)auth_type.len, auth_type.ptr);
            ret = 1;
        }

        array_reset(&response);
        return ret;
    }

    // In case we don't need to do the 2fa, the reply has the following format:
    // <Reply>
    // <UserId>{GUID}</UserId>
    // <UserCenter>{Integer}</UserCenter>
    // <UserName>{Gw2 Username}</UserName>
    // <ResumeToken>{GUID}</ResumeToken>
    // <EmailVerified>{0|1}</EmailVerified>
    // </Reply>

    struct str user_id;
    if (find_between(&user_id, &reply_content, "<UserId>", "</UserId>") != 0) {
        fprintf(stderr, "Failed to find UserId\n");
        array_reset(&response);
        return 1;
    }

    if (!s_parse_uuid(&user_id, &sts->user_id)) {
        fprintf(stderr, "Failed to parse the user_id uuid '%.*s'\n", (int)user_id.len, user_id.ptr);
        array_reset(&response);
        return 1;
    }

    array_reset(&response);
    return 0;
}

static int read_user_code(char *buffer, size_t size, size_t *ret)
{
    printf("Enter code: ");

    if (fgets(buffer, (int)size, stdin) == NULL)
        return 1;

    // The complete line didn't fit in the buffer.
    const char *newline;
    if ((newline = strchr(buffer, '\n')) == NULL)
        return 1;
    *ret = newline - buffer;
    return 0;
}

static int auth2f_upgrade_totp(
    struct sts_connection *sts,
    struct ssl_sts_connection *ssl,
    const char *otp,
    size_t otp_len,
    int remember_me)
{
    const char url[] = "/Auth2f/Upgrade";
    const size_t url_len = sizeof(url) - 1;

    array_uint8_t content;
    array_init(&content);
    array_reserve(&content, 1024);

    appendf(&content, "<Request>\n");
    appendf(&content, "<Otp>%.*s</Otp>\n", (int)otp_len, otp);
    if (remember_me != 0)
        appendf(&content, "<WhitelistIp/>\n");
    appendf(&content, "</Request>\n");

    array_uint8_t request;
    array_init(&request);
    array_reserve(&request, 1024);

    int ret = sts_write_request_with_sequence_number(
        &request,
        url, url_len,
        ++sts->seq_number,
        TIMEOUT_MS,
        content.data, content.size);

    array_reset(&content);

    if (ret != 0) {
        array_reset(&request);
        return ret;
    }

    ret = ssl_sts_connection_send(ssl, request.data, request.size);
    array_reset(&request);

    if (ret != 0) {
        return ret;
    }

    struct sts_reply reply = {0};
    array_uint8_t response;
    array_init(&response);
    array_reserve(&response, 1024);

    if ((ret = recv_sts_response(ssl, &response, &reply, sts->seq_number)) != 0) {
        array_reset(&response);
        return ret;
    }

    if (reply.status_code != 200) {
        fprintf(stderr, "Reply to '%s' failed with status %u\n", url, reply.status_code);
        array_reset(&response);
        return 1;
    }

    struct str reply_content = s_from_unsigned_utf8(reply.content, reply.content_length);

    struct str user_id;
    if (find_between(&user_id, &reply_content, "<UserId>", "</UserId>") != 0) {
        fprintf(stderr, "Failed to find UserId\n");
        array_reset(&response);
        return 1;
    }

    if (!s_parse_uuid(&user_id, &sts->user_id)) {
        fprintf(stderr, "Failed to parse the user_id uuid '%.*s'\n", (int)user_id.len, user_id.ptr);
        array_reset(&response);
        return 1;
    }

    array_reset(&response);
    return 0;
}

static int auth_list_game_accounts(struct sts_connection *sts, struct ssl_sts_connection *ssl)
{
    const char url[] = "/Auth/ListMyGameAccounts";
    const size_t url_len = sizeof(url) - 1;

    array_uint8_t content;
    array_init(&content);
    array_reserve(&content, 1024);
    appendf(&content, "<Request>\n");
    appendf(&content, "<GameCode>gw1</GameCode>\n");
    appendf(&content, "</Request>\n");

    array_uint8_t request;
    array_init(&request);
    array_reserve(&request, 1024);
    int ret = sts_write_request_with_sequence_number(
        &request,
        url, url_len,
        ++sts->seq_number,
        TIMEOUT_MS,
        content.data, content.size);

    array_reset(&content);

    if (ret != 0) {
        array_reset(&request);
        return ret;
    }

    ret = ssl_sts_connection_send(ssl, request.data, request.size);
    array_reset(&request);

    if (ret != 0) {
        return ret;
    }

    struct sts_reply reply = {0};
    array_uint8_t response;
    array_init(&response);
    array_reserve(&response, 1024);

    if ((ret = recv_sts_response(ssl, &response, &reply, sts->seq_number)) != 0) {
        array_reset(&response);
        return ret;
    }

    if (reply.status_code != 200) {
        fprintf(stderr, "Reply to '%s' failed with status %u\n", url, reply.status_code);
        array_reset(&response);
        return 1;
    }

    // The reply has the following format:
    // <Reply type="array">
    // <Row>
    // <GameCode>gw1</GameCode>
    // <Alias>gw1</Alias>
    // <Created>{ISO8601 time}</Created>
    // </Row>
    // </Reply>

    array_reset(&response);
    return 0;
}

static int auth_request_game_token(struct sts_connection *sts, struct ssl_sts_connection *ssl)
{
    const char url[] = "/Auth/RequestGameToken";
    const size_t url_len = sizeof(url) - 1;

    // It seems that the 'GameCode' and 'AccountAlias' values are directly
    // taked from the previous packet. It returns an array of such information.

    array_uint8_t content;
    array_init(&content);
    array_reserve(&content, 1024);
    appendf(&content, "<Request>\n");
    appendf(&content, "<GameCode>gw1</GameCode>\n");
    appendf(&content, "<AccountAlias>gw1</AccountAlias>\n");
    appendf(&content, "</Request>\n");

    array_uint8_t request;
    array_init(&request);
    array_reserve(&request, 1024);
    int ret = sts_write_request_with_sequence_number(
        &request,
        url, url_len,
        ++sts->seq_number,
        TIMEOUT_MS,
        content.data, content.size);

    array_reset(&content);

    if (ret != 0) {
        array_reset(&request);
        return ret;
    }

    ret = ssl_sts_connection_send(ssl, request.data, request.size);
    array_reset(&request);

    if (ret != 0) {
        return ret;
    }

    struct sts_reply reply = {0};
    array_uint8_t response;
    array_init(&response);
    array_reserve(&response, 1024);

    if ((ret = recv_sts_response(ssl, &response, &reply, sts->seq_number)) != 0) {
        array_reset(&response);
        return ret;
    }

    if (reply.status_code != 200) {
        fprintf(stderr, "Reply to '%s' failed with status %u\n", url, reply.status_code);
        array_reset(&response);
        return 1;
    }

    // The reply has the following format.
    // We need the game token to connect to Guild Wars 1.
    // <Reply>
    // <Token>{UUID}</Token>
    // </Reply>
    struct str reply_content = s_from_unsigned_utf8(reply.content, reply.content_length);

    struct str token;
    if (find_between(&token, &reply_content, "<Token>", "</Token>") != 0) {
        fprintf(stderr, "Failed to find Token\n");
        array_reset(&response);
        return 1;
    }

    if (!s_parse_uuid(&token, &sts->token)) {
        fprintf(stderr, "Failed to parse the token uuid '%.*s'\n", (int)token.len, token.ptr);
        array_reset(&response);
        return 1;
    }

    array_reset(&response);
    return 0;
}

#if 0
static int sts_ping(struct sts_connection *sts, struct ssl_sts_connection *ssl)
{
    (void)sts;

    const char url[] = "/Sts/Ping";
    const size_t url_len = sizeof(url) - 1;

    array_uint8_t request;
    array_init(&request);

    const uint8_t content[] = "";
    int ret = sts_write_request(&request, url, url_len, content, 0);

    if (ret != 0) {
        array_reset(&request);
        return ret;
    }

    ret = ssl_sts_connection_send(ssl, request.data, request.size);
    array_reset(&request);

    return ret;
}
#endif

int portal_login(struct portal_login_result *result, const char *username, const char *password, const char *secrets)
{
    int ret;

    struct sts_connection sts;
    sts_connection_init(&sts);

    // Open socket and send the initial message, following ANet proprietary
    // "sts" protocol.
    if ((ret = sts_connection_connect(&sts, "cligate.1.ncplatform.net.")) != 0) {
        sts_connection_free(&sts);
        return ret;
    }

    mbedtls_entropy_context entropy;
    mbedtls_entropy_init(&entropy);

    struct ssl_sts_connection ssl;
    ssl_sts_connection_init(&ssl);

    if ((ret = sts_connection_start_tls(&sts, &ssl)) != 0) {
        goto cleanup;
    }

    // We "seed" the ssl structure effectively allowing it to generate
    // a random private key.
    if ((ret = ssl_sts_connection_seed(&ssl, &entropy)) != 0) {
        goto cleanup;
    }

    // We init the SRP necessary data. Note that the password isn't
    // kept in memory by the `struct ssl_sts_connection` structure.
    if ((ret = ssl_sts_connection_init_srp(&ssl, username, password)) != 0) {
        goto cleanup;
    }

    // We do the handshake process. After this handshake, we will have
    // a secured connection with initialized ciphers. We will then be
    // able to send encrypted application data.
    if ((ret = ssl_sts_connection_handshake(&ssl)) != 0) {
        goto cleanup;
    }

    // At this point, we have a secured connection and we are authenticated.
    // We need to use the STS protocol, over TLS-SRP to retrieve a user id and
    // a game token.

    if ((ret = auth_login_finish(&sts, &ssl)) != 0) {
        // This is not necessarily a unrecoverable error. We may need
        // to send a code, because of 2fa.
        if (ret == PORTAL_ERR_2FA_REQUIRE_TOTP) {
            fprintf(stderr, "2fa requires TOTP code\n");
        } else if (ret == PORTAL_ERR_2FA_REQUIRE_EMAIL) {
            fprintf(stderr, "2fa requires Email code\n");
        } else if (ret == PORTAL_ERR_2FA_REQUIRE_SMS) {
            fprintf(stderr, "2fa requires SMS code\n");
        } else {
            goto cleanup;
        }

        char otp[32];
        size_t otp_len;

        if (secrets != NULL && ret == PORTAL_ERR_2FA_REQUIRE_TOTP) {
            uint32_t code;
            if (!totp(secrets, 6, &code)) {
                fprintf(stderr, "Failed to generate the 2fa code\n");
                goto cleanup;
            }

            if ((ret = snprintf(otp, sizeof(otp), "%06d", code)) <= 0) {
                fprintf(stderr, "Failed to stringnify the 2fa code\n");
                goto cleanup;
            }

            otp_len = (size_t)ret;
        } else {
            if ((ret = read_user_code(otp, sizeof(otp), &otp_len)) != 0) {
                goto cleanup;
            }
        }

        const int remember_me = 1;
        if ((ret = auth2f_upgrade_totp(&sts, &ssl, otp, otp_len, remember_me)) != 0) {
            goto cleanup;
        }
    }

    if ((ret = auth_list_game_accounts(&sts, &ssl)) != 0) {
        goto cleanup;
    }

    if ((ret = auth_request_game_token(&sts, &ssl)) != 0) {
        goto cleanup;
    }

    result->token = sts.token;
    result->user_id = sts.user_id;

cleanup:
    // TODO: This connection seems to need to stay open even after successful auth.
    // Its naughty, but for now leave the pointer hanging instead of closing the conn.
    // ssl_sts_connection_free(&ssl);
    sts_connection_free(&sts);
    mbedtls_entropy_free(&entropy);
    if (ret != 0)
        ret = 1;
    return ret;
}

int portal_login_dummy(struct portal_login_result *result, const char *username, const char *password, const char *secret)
{
    struct str token = s_from_c_str("d8b9bf5d-90b1-4cbd-9b76-88da7be763b6");
    struct str user_id = s_from_c_str("fa520ee2-4419-4eb4-ae49-6e9abe6ef24f");
    if (!s_parse_uuid(&token, &result->token)) {
        return 1;
    }
    if (!s_parse_uuid(&user_id, &result->user_id)) {
        return 1;
    }
    return 0;
}
