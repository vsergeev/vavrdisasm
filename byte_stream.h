#ifndef BYTE_STREAM_H
#define BYTE_STREAM_H

#include <stdint.h>
#include <stdio.h>
#include <stream_error.h>

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
    int (*stream_read)(struct ByteStream *self, uint8_t *data, uint32_t *address);
};


#endif

