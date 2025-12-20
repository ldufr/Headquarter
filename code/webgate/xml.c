#ifdef WEBGATE_XML_C
#error "xml.c included more than once"
#endif
#define WEBGATE_XML_C

struct str {
    const char *ptr;
    size_t      len;
};

struct str s_from_unsigned_utf8(const uint8_t *ptr, size_t len)
{
    return (struct str){(const char *)ptr, len};
}

struct str s_from_signed_utf8(const char *ptr, size_t len)
{
    return (struct str){ptr, len};
}

struct str s_from_c_str(const char *ptr)
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

static void appendv(array_uint8_t *buffer, const char *fmt, va_list args)
{
    va_list args_copy;
    va_copy(args_copy, args);

    int ret = vsnprintf(NULL, 0, fmt, args);
    if (ret < 0) {
        va_end(args_copy);
        abort();
    }

    // We need to allocate one more bytes, because  of `vsnprintf`.
    // We will pop this "\0" byte later.
    uint8_t *write_ptr = array_push(buffer, (size_t)ret + 1);
    vsnprintf((char *)write_ptr, (size_t)ret + 1, fmt, args_copy);
    (void)array_pop(buffer);
    va_end(args_copy);
}

static void appendf(array_uint8_t *buffer, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    appendv(buffer, fmt, args);
    va_end(args);
}

