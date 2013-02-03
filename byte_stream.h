#ifndef BYTE_STREAM_H
#define BYTE_STREAM_H

#include <stdint.h>
#include <stdio.h>
#include "stream_error.h"

struct ByteStream {
    /* Input stream */
    FILE *in;
    /* Stream state */
    void *state;
    /* Error string */
    char *error;

    /* Init function */
    int (*stream_init)(struct ByteStream *self);
    /* Close function */
    int (*stream_close)(struct ByteStream *self);
    /* Output function */
    int (*stream_read)(struct ByteStream *self, uint8_t *data, uint32_t *address, unsigned int *len, unsigned int n);
};

/* Atmel Generic Byte Stream Support */
int byte_stream_generic_init(struct ByteStream *self);
int byte_stream_generic_close(struct ByteStream *self);
int byte_stream_generic_read(struct ByteStream *self, uint8_t *data, uint32_t *address, unsigned int *len, unsigned int n);

/* Intel HEX Byte Stream Support */
int byte_stream_ihex_init(struct ByteStream *self);
int byte_stream_ihex_close(struct ByteStream *self);
int byte_stream_ihex_read(struct ByteStream *self, uint8_t *data, uint32_t *address, unsigned int *len, unsigned int n);

/* Motorola S-Record Byte Stream Support */
int byte_stream_srecord_init(struct ByteStream *self);
int byte_stream_srecord_close(struct ByteStream *self);
int byte_stream_srecord_read(struct ByteStream *self, uint8_t *data, uint32_t *address, unsigned int *len, unsigned int n);

/* Binary Byte Stream Support */
int byte_stream_binary_init(struct ByteStream *self);
int byte_stream_binary_close(struct ByteStream *self);
int byte_stream_binary_read(struct ByteStream *self, uint8_t *data, uint32_t *address, unsigned int *len, unsigned int n);

/* Debug Byte Stream Support */
int byte_stream_debug_init(struct ByteStream *self);
int byte_stream_debug_close(struct ByteStream *self);
int byte_stream_debug_read(struct ByteStream *self, uint8_t *data, uint32_t *address, unsigned int *len, unsigned int n);

/* Byte Stream File Test */
int test_byte_stream(FILE *in, int (*stream_init)(struct ByteStream *self), int (*stream_close)(struct ByteStream *self), int (*stream_read)(struct ByteStream *self, uint8_t *data, uint32_t *address, unsigned int *len, unsigned int n));

#endif

