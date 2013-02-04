#ifndef DISASM_STREAM_H
#define DISASM_STREAM_H

#include <stdio.h>
#include <byte_stream.h>
#include <instruction.h>
#include <stream_error.h>

struct DisasmStream {
    /* Input stream */
    struct ByteStream *in;
    /* Stream state */
    void *state;
    /* Error */
    char *error;

    /* Init function */
    int (*stream_init)(struct DisasmStream *self);
    /* Close function */
    int (*stream_close)(struct DisasmStream *self);
    /* Output function */
    int (*stream_read)(struct DisasmStream *self, struct instruction *instr);
};

#endif

