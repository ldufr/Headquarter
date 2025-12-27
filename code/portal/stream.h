#ifdef PORTAL_STREAM_H
#error "stream.h included more than once"
#endif
#define PORTAL_STREAM_H

void appendv(array_uint8_t *buffer, const char *fmt, va_list args);
void appendf(array_uint8_t *buffer, const char *fmt, ...);

void array_add_be_uint16(array_uint8_t *buffer, uint16_t value);

uint32_t be24dec(const void *pp);

int chk_stream_read8(const uint8_t **data, size_t *length, uint8_t *out);
int chk_stream_read16(const uint8_t **data, size_t *length, uint16_t *out);
int chk_stream_read32(const uint8_t **data, size_t *length, uint32_t *out);
int chk_stream_read(const uint8_t **data, size_t *length, uint8_t *out, size_t out_len);
