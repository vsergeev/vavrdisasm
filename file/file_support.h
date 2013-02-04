#include <stdint.h>

/* Atmel Generic Byte Stream Support */
int byte_stream_generic_init(struct ByteStream *self);
int byte_stream_generic_close(struct ByteStream *self);
int byte_stream_generic_read(struct ByteStream *self, uint8_t *data, uint32_t *address);

/* Intel HEX Byte Stream Support */
int byte_stream_ihex_init(struct ByteStream *self);
int byte_stream_ihex_close(struct ByteStream *self);
int byte_stream_ihex_read(struct ByteStream *self, uint8_t *data, uint32_t *address);

/* Motorola S-Record Byte Stream Support */
int byte_stream_srecord_init(struct ByteStream *self);
int byte_stream_srecord_close(struct ByteStream *self);
int byte_stream_srecord_read(struct ByteStream *self, uint8_t *data, uint32_t *address);

/* Binary Byte Stream Support */
int byte_stream_binary_init(struct ByteStream *self);
int byte_stream_binary_close(struct ByteStream *self);
int byte_stream_binary_read(struct ByteStream *self, uint8_t *data, uint32_t *address);

/* ASCII Hex Stream Support */
int byte_stream_asciihex_init(struct ByteStream *self);
int byte_stream_asciihex_close(struct ByteStream *self);
int byte_stream_asciihex_read(struct ByteStream *self, uint8_t *data, uint32_t *address);

/* Debug Byte Stream Support */
int byte_stream_debug_init(struct ByteStream *self);
int byte_stream_debug_close(struct ByteStream *self);
int byte_stream_debug_read(struct ByteStream *self, uint8_t *data, uint32_t *address);

/* Byte Stream File Test */
int test_byte_stream(FILE *in, int (*stream_init)(struct ByteStream *self), int (*stream_close)(struct ByteStream *self), int (*stream_read)(struct ByteStream *self, uint8_t *data, uint32_t *address));

