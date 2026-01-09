#ifdef WEBGATE_LOGIN_C
#error "login.c included more than once"
#endif
#define WEBGATE_LOGIN_C

#define WEBGATE_ERR_UKNOWN_ERROR      1
#define WEBGATE_ERR_2FA_REQUIRE_TOTP  2
#define WEBGATE_ERR_2FA_REQUIRE_EMAIL 3
#define WEBGATE_ERR_2FA_REQUIRE_SMS   4

struct webgate_response {
    long status_code;
    array_uint8_t content;
};

struct webgate_context {
    CURL *handle;
    char auth_token[UUID_STRING_LENGTH + 1];
    char alias[32];
    uint32_t client_version;
    struct webgate_response resp;
    struct uuid user_id;
    struct uuid token;
};

static struct str GAME_CODE = {
    .ptr = "gw1",
    .len = sizeof("gw1") - 1,
};

int webgate_init(void)
{
    CURLcode err;
    if ((err = curl_global_init(CURL_GLOBAL_DEFAULT)) != CURLE_OK) {
        log_error("Failed to initialize curl, code: %d", err);
        return 1;
    }
    return 0;
}

void webgate_cleanup(void)
{
    curl_global_cleanup();
}

size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    UNUSED_PARAMETER(size);

    struct webgate_response *resp = (struct webgate_response *) userdata;

    uint8_t *buffer;
    if ((buffer = array_push(&resp->content, nmemb)) == NULL) {
        return 0;
    }

    memcpy(buffer, ptr, nmemb);
    return nmemb;
}

int webgate_request(struct webgate_context *ctx, const char *path, array_uint8_t *post_data, struct webgate_response *resp)
{
    int err;
    CURLcode result;

    char url[256];
    err = snprintf(url, sizeof(url), "https://webgate.ncplatform.net%s", path);
    if (err < 0 || sizeof(url) <= err) {
        return 1;
    }

    char auth[256];
    err = snprintf(auth, sizeof(auth), "Authorization: Arena %s", ctx->auth_token);
    if (err < 0 || sizeof(auth) <= err) {
        log_error("Failed to create the 'Authorization' header");
        return 1;
    }

    char user_agent[256];
    err = snprintf(user_agent, sizeof(user_agent), "User-Agent: Gw/%" PRIu32 ".0 (Win32)", ctx->client_version);
    if (err < 0 || sizeof(user_agent) <= err) {
        log_error("Failed to create the 'User-Agent' header");
        return 1;
    }

    struct curl_slist *headers = NULL;
    if ((headers = curl_slist_append(headers, user_agent)) == NULL ||
        (headers = curl_slist_append(headers, auth)) == NULL)
    {
        curl_slist_free_all(headers);
        return 1;
    }

    if (post_data != NULL) {
        if ((headers = curl_slist_append(headers, "Content-Type: application/xml")) == NULL) {
            curl_slist_free_all(headers);
            return 1;
        }
    }

    if ((result = curl_easy_setopt(ctx->handle, CURLOPT_HTTPHEADER, headers)) != CURLE_OK) {
        log_error("curl_easy_setopt(CURLOPT_HTTPHEADER) failed %s", curl_easy_strerror(result));
        goto cleanup;
    }
    if ((result = curl_easy_setopt(ctx->handle, CURLOPT_URL, url)) != CURLE_OK) {
        log_error("curl_easy_setopt(CURLOPT_URL) failed %s", curl_easy_strerror(result));
        goto cleanup;
    }
    if ((result = curl_easy_setopt(ctx->handle, CURLOPT_WRITEDATA, resp)) != CURLE_OK) {
        log_error("curl_easy_setopt(CURLOPT_WRITEDATA) failed %s", curl_easy_strerror(result));
        goto cleanup;
    }

    if (post_data != NULL) {
        if ((size_t) LONG_MAX < post_data->size) {
            log_error("Can't send more than %ld bytes, got %zu", LONG_MAX, post_data->size);
            goto cleanup;
        }
        if ((result = curl_easy_setopt(ctx->handle, CURLOPT_POSTFIELDSIZE, (long) post_data->size)) != CURLE_OK) {
            log_error("curl_easy_setopt(CURLOPT_POSTFIELDSIZE) failed %s", curl_easy_strerror(result));
            goto cleanup;
        }
        if ((result = curl_easy_setopt(ctx->handle, CURLOPT_POSTFIELDS, post_data->data)) != CURLE_OK) {
            log_error("curl_easy_setopt(CURLOPT_POSTFIELDS) failed %s", curl_easy_strerror(result));
            goto cleanup;
        }
    }

    array_clear(&resp->content);
    if ((result = curl_easy_perform(ctx->handle)) != CURLE_OK) {
        log_error("curl_easy_perform failed %s", curl_easy_strerror(result));
        goto cleanup;
    }

    if ((result = curl_easy_getinfo(ctx->handle, CURLINFO_RESPONSE_CODE, &resp->status_code)) != CURLE_OK) {
        goto cleanup;
    }

cleanup:
    curl_slist_free_all(headers);
    return result != CURLE_OK;
}

int webgate_session_create(struct webgate_context *ctx)
{
    int err;

    // When creating a new session, we still send the "Authentication" header,
    // but with a token `0`.
    STATIC_ASSERT(ARRAY_SIZE("0") <= ARRAY_SIZE(ctx->auth_token));
    memcpy(ctx->auth_token, "0", 2);

    array_clear(&ctx->resp.content);

    const char *path = "/session/create.xml";
    if ((err = webgate_request(ctx, path, NULL, &ctx->resp)) != 0) {
        return err;
    }

    if (ctx->resp.status_code != 200) {
        log_error("Request to '%s' failed, http status: %ld", path, ctx->resp.status_code);
        return 1;
    }

    struct str reply_content = s_from_unsigned_utf8(ctx->resp.content.data, ctx->resp.content.size);
    struct str session_token;
    if (find_between(&session_token, &reply_content, "<Session>", "</Session>") != 0) {
        return 1;
    }

    if (ARRAY_SIZE(ctx->auth_token) <= session_token.len) {
        return 1;
    }

    memcpy(ctx->auth_token, session_token.ptr, session_token.len);
    ctx->auth_token[session_token.len] = 0;
    return 0;
}

int webgate_user_login(struct webgate_context *ctx, const char *username, const char *password)
{
    int err;

    // @Cleanup: Figure out the correct buffer size
    char b64password[100] = "";
    if ((err = b64encode(b64password, sizeof(b64password), password, strlen(password))) != 0) {
        return 0;
    }

    array_uint8_t content;
    array_init(&content);
    array_reserve(&content, 1024);

    appendf(&content, "<Request>\n");
    appendf(&content, "\t<GameCode>%.*s</GameCode>\n", (int) GAME_CODE.len, GAME_CODE.ptr);
    appendf(&content, "\t<LoginName>%s</LoginName>\n", username);
    appendf(&content, "\t<Provider>Portal</Provider>\n");
    appendf(&content, "\t<Password>%s</Password>\n", b64password);
    appendf(&content, "</Request>\n");

    array_clear(&ctx->resp.content);

    const char *path = "/users/login.xml";
    err = webgate_request(ctx, path, &content, &ctx->resp);
    array_reset(&content);
    if (err != 0) {
        return err;
    }

    if (ctx->resp.status_code != 200) {
        log_error("Request to '%s' failed, http status: %ld", path, ctx->resp.status_code);
        return 1;
    }

    struct str reply_content = s_from_unsigned_utf8(ctx->resp.content.data, ctx->resp.content.size);

    struct str auth_type;
    if (find_between(&auth_type, &reply_content, "<AuthType>", "</AuthType>") == 0) {
        // This mean we need to complete a 2fa.
        struct str sms = s_from_c_str("SMS");
        struct str totp = s_from_c_str("Totp");
        struct str email = s_from_c_str("Email");
        if (s_cmp(&auth_type, &totp) == 0) {
            err = WEBGATE_ERR_2FA_REQUIRE_TOTP;
        } else if (s_cmp(&auth_type, &email) == 0) {
            err = WEBGATE_ERR_2FA_REQUIRE_EMAIL;
        } else if (s_cmp(&auth_type, &sms) == 0) {
            err = WEBGATE_ERR_2FA_REQUIRE_SMS;
        } else {
            log_error("Unknown AuthType: '%.*s'", (int) auth_type.len, auth_type.ptr);
            err = 1;
        }
        return err;
    }

    struct str user_id;
    if (find_between(&user_id, &reply_content, "<UserId>", "</UserId>") != 0) {
        return 1;
    }

    if (!s_parse_uuid(&user_id, &ctx->user_id)) {
        log_error("Couldn't print the user id '%.*s' as uuid", (int) user_id.len, user_id.ptr);
        return 1;
    }

    return 0;
}

int webgate_list_game_accounts(struct webgate_context *ctx)
{
    int err;

    array_uint8_t content;
    array_init(&content);
    array_reserve(&content, 256);

    appendf(&content, "<Request>\n");
    appendf(&content, "\t<GameCode>%.*s</GameCode>\n", (int) GAME_CODE.len, GAME_CODE.ptr);
    appendf(&content, "</Request>\n");

    array_clear(&ctx->resp.content);

    const char *path = "/my_account/game_accounts.xml";
    err = webgate_request(ctx, path, &content, &ctx->resp);
    array_reset(&content);
    if (err != 0) {
        return err;
    }

    if (ctx->resp.status_code != 200) {
        log_error("Request to '%s' failed, http status: %ld", path, ctx->resp.status_code);
        return 1;
    }

    // <Reply type="array">
    // <Row>
    // <GameCode>gw1</GameCode>
    // <Alias>gw1</Alias>
    // <Created>{timestamp}</Created>
    // </Row>
    // </Reply>

    struct str reply_content = s_from_unsigned_utf8(ctx->resp.content.data, ctx->resp.content.size);

    for (;;) {
        struct str row;
        if (find_between(&row, &reply_content, "<Row>", "</Row>") != 0)
            return 1;

        size_t skip_count = (row.ptr - reply_content.ptr) + row.len;
        reply_content.len -= skip_count;
        reply_content.ptr += skip_count;

        struct str game_code;
        if (find_between(&game_code, &row, "<GameCode>", "</GameCode>") != 0)
            continue;
        struct str alias;
        if (find_between(&alias, &row, "<Alias>", "</Alias>") != 0)
            continue;

        if (s_cmp(&game_code, &GAME_CODE) == 0) {
            if (ARRAY_SIZE(ctx->alias) <= alias.len) {
                log_error(
                    "Found alias '%.*s', but it's too large for the buffer. Maximum %zu, got %zu",
                    (int) alias.len, alias.ptr,
                    ARRAY_SIZE(ctx->alias),
                    alias.len
                );
                return 1;
            }

            memcpy(ctx->alias, alias.ptr, alias.len);
            ctx->alias[alias.len] = 0;
            break;
        }
    }

    return 0;
}

int webgate_upgrade_login(struct webgate_context *ctx, const char *otp, size_t otp_len, bool remember_me)
{
    int err;

    array_uint8_t content;
    array_init(&content);
    array_reserve(&content, 256);

    appendf(&content, "<Request>\n");
    appendf(&content, "\t<Otp>%.*s</Otp>\n", (int) otp_len, otp);
    if (remember_me) {
        appendf(&content, "\t<AllowlistIp/>\n");
    }
    appendf(&content, "</Request>\n");

    array_clear(&ctx->resp.content);

    const char *path = "/my_account/upgrade_login.xml";
    err = webgate_request(ctx, path, &content, &ctx->resp);
    array_reset(&content);
    if (err != 0) {
        return err;
    }

    if (ctx->resp.status_code != 200) {
        log_error("Request to '%s' failed, http status: %ld", path, ctx->resp.status_code);
        return 1;
    }

    struct str reply_content = s_from_unsigned_utf8(ctx->resp.content.data, ctx->resp.content.size);

    struct str user_id;
    if (find_between(&user_id, &reply_content, "<UserId>", "</UserId>") != 0) {
        return 1;
    }

    if (!s_parse_uuid(&user_id, &ctx->user_id)) {
        log_error("Couldn't print the user id '%.*s' as uuid", (int) user_id.len, user_id.ptr);
        return 1;
    }

    return 0;
}

int webgate_request_game_tokens(struct webgate_context *ctx)
{
    int err;

    array_uint8_t content;
    array_init(&content);
    array_reserve(&content, 256);

    appendf(&content, "<Request>\n");
    appendf(&content, "\t<GameCode>%.*s</GameCode>\n", (int) GAME_CODE.len, GAME_CODE.ptr);
    appendf(&content, "\t<AccountAlias>%s</AccountAlias>\n", ctx->alias);
    appendf(&content, "</Request>\n");

    array_clear(&ctx->resp.content);

    const char *path = "/my_account/token.xml";
    err = webgate_request(ctx, path, &content, &ctx->resp);
    array_reset(&content);
    if (err != 0) {
        return err;
    }

    if (ctx->resp.status_code != 200) {
        log_error("Request to '%s' failed, http status: %ld", path, ctx->resp.status_code);
        return 1;
    }

    struct str reply_content = s_from_unsigned_utf8(ctx->resp.content.data, ctx->resp.content.size);
    struct str token;
    if (find_between(&token, &reply_content, "<Token>", "</Token>") != 0) {
        return 1;
    }

    if (!s_parse_uuid(&token, &ctx->token)) {
        log_error("Couldn't print the token '%.*s' as uuid", (int) token.len, token.ptr);
        return 1;
    }

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

int webgate_login(
    struct webgate_login_result *result,
    const char *username,
    const char *password,
    const char *secrets,
    uint32_t version
    )
{
    int err;
    struct webgate_context ctx = {
        .auth_token = "0",
        .client_version = version,
    };

    ctx.handle = curl_easy_init();
    if (ctx.handle == NULL) {
        return 1;
    }

    if (curl_easy_setopt(ctx.handle, CURLOPT_WRITEFUNCTION, write_callback) != CURLE_OK) {
        curl_easy_cleanup(ctx.handle);
        return 1;
    }

    if ((err = webgate_session_create(&ctx)) != 0) {
        log_error("Failed to create a webgate session, err: %d", err);
        goto cleanup;
    }

    if ((err = webgate_user_login(&ctx, username, password)) != 0) {
        // This is not necessarily a unrecoverable error. We may need
        // to send a code, because of 2fa.
        if (err == WEBGATE_ERR_2FA_REQUIRE_TOTP) {
            fprintf(stdout, "2fa requires TOTP code\n");
        } else if (err == WEBGATE_ERR_2FA_REQUIRE_EMAIL) {
            fprintf(stdout, "2fa requires Email code\n");
        } else if (err == WEBGATE_ERR_2FA_REQUIRE_SMS) {
            fprintf(stdout, "2fa requires SMS code\n");
        } else {
            log_error("Failed to do a webgate login, err: %d", err);
            goto cleanup;
        }

        char otp[32];
        size_t otp_len;

        if (secrets != NULL && err == WEBGATE_ERR_2FA_REQUIRE_TOTP) {
            uint32_t code;
            if (!totp(secrets, 6, &code)) {
                log_error("Failed to generate the 2fa code");
                goto cleanup;
            }

            if ((err = snprintf(otp, sizeof(otp), "%06d", code)) <= 0) {
                log_error("Failed to stringnify the 2fa code");
                goto cleanup;
            }

            otp_len = (size_t)err;
        } else {
            if ((err = read_user_code(otp, sizeof(otp), &otp_len)) != 0) {
                goto cleanup;
            }
        }

        const bool remember_me = true;
        if ((err = webgate_upgrade_login(&ctx, otp, otp_len, remember_me)) != 0) {
            goto cleanup;
        }
    }

    if ((err = webgate_list_game_accounts(&ctx)) != 0) {
        log_error("Failed to list the game accounts, err: %d", err);
        goto cleanup;
    }

    if ((err = webgate_request_game_tokens(&ctx)) != 0) {
        log_error("Failed to request the game tokens, err: %d", err);
        goto cleanup;
    }

    result->user_id = ctx.user_id;
    result->token = ctx.token;

cleanup:
    curl_easy_cleanup(ctx.handle);
    array_reset(&ctx.resp.content);
    return err;
}
