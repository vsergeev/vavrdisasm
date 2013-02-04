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

/* Print Stream State */
struct print_stream_state {
    /* Print Option Bit Flags */
    unsigned int flags;
    /* Origin Initiailized */
    int origin_initialized;
    /* Next Expected address */
    uint32_t next_address;
};

/* Print Stream Option Flags */
enum {
    PRINT_FLAG_ASSEMBLY                = (1<<0),
    PRINT_FLAG_ADDRESSES               = (1<<1),
    PRINT_FLAG_DESTINATION_COMMENT     = (1<<2),
    PRINT_FLAG_DATA_HEX                = (1<<3),
    PRINT_FLAG_DATA_BIN                = (1<<4),
    PRINT_FLAG_DATA_DEC                = (1<<5),
    PRINT_FLAG_OPCODES                 = (1<<6),
};

/* Print Stream Support */
int print_stream_init(struct PrintStream *self, int flags);
int print_stream_close(struct PrintStream *self);
int print_stream_read(struct PrintStream *self, FILE *out);

#endif

