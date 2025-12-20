#ifdef WEBGATE_LOGIN_C
#error "login.c included more than once"
#endif
#define WEBGATE_LOGIN_C

struct webgate_response {
    long status_code;
    array_uint8_t content;
};

struct webgate_context {
    CURL *handle;
    char auth_token[UUID_STRING_LENGTH];
    uint32_t client_version;
    struct webgate_response resp;
    struct uuid user_id;
    struct uuid token;
};

int webgate_init(void)
{
    CURLcode err;
    if ((err = curl_global_init(CURL_GLOBAL_DEFAULT)) != CURLE_OK) {
        fprintf(stderr, "Failed to initialize curl, code: %d\n", err);
        return 1;
    }
    return 0;
}

void webgate_cleanup()
{
    curl_global_cleanup();
}

size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
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
        return 1;
    }

    char user_agent[256];
    err = snprintf(user_agent, sizeof(user_agent), "User-Agent: Gw/%" PRIu32 ".0 (Win32)", ctx->client_version);
    if (err < 0 || sizeof(user_agent) <= err) {
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
        fprintf(stderr, "curl_easy_setopt(CURLOPT_HTTPHEADER) failed %s\n", curl_easy_strerror(result));
        goto cleanup;
    }
    if ((result = curl_easy_setopt(ctx->handle, CURLOPT_URL, url)) != CURLE_OK) {
        fprintf(stderr, "curl_easy_setopt(CURLOPT_URL) failed %s\n", curl_easy_strerror(result));
        goto cleanup;
    }
    if ((result = curl_easy_setopt(ctx->handle, CURLOPT_WRITEDATA, resp)) != CURLE_OK) {
        fprintf(stderr, "curl_easy_setopt(CURLOPT_WRITEDATA) failed %s\n", curl_easy_strerror(result));
        goto cleanup;
    }

    if (post_data != NULL) {
        if ((size_t) LONG_MAX < post_data->size) {
            fprintf(stderr, "Can't send more than %ld bytes, got %zu\n", LONG_MAX, post_data->size);
            goto cleanup;
        }
        if ((result = curl_easy_setopt(ctx->handle, CURLOPT_POSTFIELDSIZE, (long) post_data->size)) != CURLE_OK) {
            fprintf(stderr, "curl_easy_setopt(CURLOPT_POSTFIELDSIZE) failed %s\n", curl_easy_strerror(result));
            goto cleanup;
        }
        if ((result = curl_easy_setopt(ctx->handle, CURLOPT_POSTFIELDS, post_data->data)) != CURLE_OK) {
            fprintf(stderr, "curl_easy_setopt(CURLOPT_POSTFIELDS) failed %s\n", curl_easy_strerror(result));
            goto cleanup;
        }
    }

    array_clear(&resp->content);
    if ((result = curl_easy_perform(ctx->handle)) != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform failed %s\n", curl_easy_strerror(result));
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
    if ((err = webgate_request(ctx, "/session/create.xml", NULL, &ctx->resp)) != 0) {
        return err;
    }

    if (ctx->resp.status_code != 200) {
        fprintf(stderr, "Failed to create the session, http status: %ld\n", ctx->resp.status_code);
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
    appendf(&content, "\t<GameCode>gw1</GameCode>\n");
    appendf(&content, "\t<LoginName>%s</LoginName>\n", username);
    appendf(&content, "\t<Provider>Portal</Provider>\n");
    appendf(&content, "\t<Password>%s</Password>\n", b64password);
    appendf(&content, "</Request>\n");

    array_clear(&ctx->resp.content);
    if ((err = webgate_request(ctx, "/users/login.xml", &content, &ctx->resp)) != 0) {
        return err;
    }

    if (ctx->resp.status_code != 200) {
        fprintf(stderr, "Failed to create the session, http status: %ld\n", ctx->resp.status_code);
        return 1;
    }

    // <Reply>
    // <UserId>{GUID}</UserId>
    // <UserCenter>5</UserCenter>
    // <UserName>{string}</UserName>
    // <Parts/>
    // <ResumeToken>{GUID}</ResumeToken>
    // <LoginName>{email}</LoginName>
    // <Provider>Portal</Provider>
    // <EmailVerified>1</EmailVerified>
    // </Reply>

    struct str reply_content = s_from_unsigned_utf8(ctx->resp.content.data, ctx->resp.content.size);
    struct str user_id;
    if (find_between(&user_id, &reply_content, "<UserId>", "</UserId>") != 0) {
        return 1;
    }

    if (!s_parse_uuid(&user_id, &ctx->user_id)) {
        fprintf(stderr, "Couldn't print the user id '%.*s' as uuid\n", (int) user_id.len, user_id.ptr);
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
    appendf(&content, "\t<GameCode>gw1</GameCode>\n");
    appendf(&content, "</Request>\n");

    array_clear(&ctx->resp.content);
    if ((err = webgate_request(ctx, "/my_account/game_accounts.xml", &content, &ctx->resp)) != 0) {
        return err;
    }

    if (ctx->resp.status_code != 200) {
        fprintf(stderr, "Failed to create the session, http status: %ld\n", ctx->resp.status_code);
        return 1;
    }

    // <Reply type="array">
    // <Row>
    // <GameCode>gw1</GameCode>
    // <Alias>gw1</Alias>
    // <Created>{timestamp}</Created>
    // </Row>
    // </Reply>

    /*
    struct str reply_content = s_from_unsigned_utf8(ctx->resp.content.data, ctx->resp.content.size);
    struct str user_id;
    if (find_between(&user_id, &reply_content, "<UserId>", "</UserId>") != 0) {
        return 1;
    }

    if (!s_parse_uuid(&user_id, &ctx->user_id)) {
        fprintf(stderr, "Couldn't print the user id '%.*s' as uuid\n", (int) user_id.len, user_id.ptr);
        return 1;
    }
    */

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
        fprintf(stderr, "Failed to create a webgate session, err: %d\n", err);
        goto cleanup;
    }

    if ((err = webgate_user_login(&ctx, username, password)) != 0) {
        fprintf(stderr, "Failed to do a webgate login, err: %d\n", err);
        goto cleanup;
    }

    if ((err = webgate_list_game_accounts(&ctx)) != 0) {
        fprintf(stderr, "Failed to list the game accounts, err: %d\n", err);
        goto cleanup;
    }

cleanup:
    curl_easy_cleanup(ctx.handle);
    array_reset(&ctx.resp.content);
    return err;
}
