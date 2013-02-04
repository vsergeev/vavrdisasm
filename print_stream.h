#ifndef PRINT_STREAM_H
#define PRINT_STREAM_H

#include <stdint.h>
#include <stdio.h>
#include <disasm_stream.h>
#include <stream_error.h>
#include <instruction.h>

struct PrintStream {
    /* Input stream */
    struct DisasmStream *in;
    /* Stream state */
    void *state;
    /* Error */
    char *error;

    /* Init function */
    int (*stream_init)(struct PrintStream *self, int flags);
    /* Close function */
    int (*stream_close)(struct PrintStream *self);
    /* Output function */
    int (*stream_read)(struct PrintStream *self, FILE *out);
};

#endif

