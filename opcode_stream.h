#ifndef OPCODE_STREAM_H
#define OPCODE_STREAM_H

#include <stdint.h>
#include <stdio.h>

#include "stream_error.h"

struct OpcodeStream {
    /* Input stream */
    FILE *in;
    /* Stream state */
    void *state;
    /* Error string */
    char *error;

    /* Init function */
    int (*stream_init)(struct OpcodeStream *self);
    /* Close function */
    int (*stream_close)(struct OpcodeStream *self);
    /* Output function */
    int (*stream_read)(struct OpcodeStream *self, uint8_t *data, uint32_t *address, unsigned int *len, unsigned int n);
};

/* Atmel Generic Opcode Stream Support */
int opcode_stream_generic_init(struct OpcodeStream *self);
int opcode_stream_generic_close(struct OpcodeStream *self);
int opcode_stream_generic_read(struct OpcodeStream *self, uint8_t *data, uint32_t *address, unsigned int *len, unsigned int n);

/* Intel HEX Opcode Stream Support */
int opcode_stream_ihex_init(struct OpcodeStream *self);
int opcode_stream_ihex_close(struct OpcodeStream *self);
int opcode_stream_ihex_read(struct OpcodeStream *self, uint8_t *data, uint32_t *address, unsigned int *len, unsigned int n);

/* Motorola S-Record Opcode Stream Support */
int opcode_stream_srecord_init(struct OpcodeStream *self);
int opcode_stream_srecord_close(struct OpcodeStream *self);
int opcode_stream_srecord_read(struct OpcodeStream *self, uint8_t *data, uint32_t *address, unsigned int *len, unsigned int n);

/* Binary Opcode Stream Support */
int opcode_stream_binary_init(struct OpcodeStream *self);
int opcode_stream_binary_close(struct OpcodeStream *self);
int opcode_stream_binary_read(struct OpcodeStream *self, uint8_t *data, uint32_t *address, unsigned int *len, unsigned int n);

/* Debug Opcode Stream Support */
int opcode_stream_debug_init(struct OpcodeStream *self);
int opcode_stream_debug_close(struct OpcodeStream *self);
int opcode_stream_debug_read(struct OpcodeStream *self, uint8_t *data, uint32_t *address, unsigned int *len, unsigned int n);

/* Opcode Stream File Test */
int test_opcode_stream(FILE *in, int (*stream_init)(struct OpcodeStream *self), int (*stream_close)(struct OpcodeStream *self), int (*stream_read)(struct OpcodeStream *self, uint8_t *data, uint32_t *address, unsigned int *len, unsigned int n));

#endif

