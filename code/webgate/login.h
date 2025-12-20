#ifdef WEBGATE_LOGIN_H
#error "login.h included more than once"
#endif
#define WEBGATE_LOGIN_H

struct webgate_login_result {
    struct uuid user_id;
    struct uuid token;
};

int webgate_login(
    struct webgate_login_result *result,
    const char *username,
    const char *password,
    const char *secrets,
    uint32_t version
    );
