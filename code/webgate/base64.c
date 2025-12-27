#ifdef WEBGATE_BASE64_C
#error "base64.c included more than once"
#endif
#define WEBGATE_BASE64_C

static size_t b64bufsize(size_t len)
{
    size_t triplets = (len + 2) / 3;
    return (4 * triplets) + 1;
}

static int b64encode(char *output, size_t olen, const char *input, size_t ilen)
{
    static const char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/="; 

    if (olen < b64bufsize(ilen))
        return 1;

    for (size_t idx = 0; idx < ilen; idx += 3) {
        unsigned int value;
        value = input[idx] << 16 |
                (idx + 1 < ilen ? input[idx + 1] << 8 : 0) |
                (idx + 2 < ilen ? input[idx + 2] : 0);
        unsigned int idx0 = (value & 0xfc0000) >> 18;
        unsigned int idx1 = (value & 0x3f000) >> 12;
        unsigned int idx2 = (value & 0xfc0) >> 6;
        unsigned int idx3 = (value & 0x3f);
        *output++ = table[idx0];
        *output++ = table[idx1];
        *output++ = idx + 1 < ilen ? table[idx2] : '=';
        *output++ = idx + 2 < ilen ? table[idx3] : '=';
    }
    *output = 0;
    return 0;
}
