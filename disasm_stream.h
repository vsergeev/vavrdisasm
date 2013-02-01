#ifndef DISASM_STREAM_H
#define DISASM_STREAM_H

#include <stdint.h>
#include <stdio.h>

#include "opcode_stream.h"
#include "stream_error.h"

#include "avr_instruction_set.h"

struct DisasmStream {
    /* Input stream */
    struct OpcodeStream *in;
    /* Stream state */
    void *state;
    /* Error */
    char *error;

    /* Init function */
    int (*stream_init)(struct DisasmStream *self);
    /* Close function */
    int (*stream_close)(struct DisasmStream *self);
    /* Output function */
    int (*stream_read)(struct DisasmStream *self, void *idisasm);
};

/* AVR Disassembly Stream Support */
int disasm_stream_avr_init(struct DisasmStream *self);
int disasm_stream_avr_close(struct DisasmStream *self);
int disasm_stream_avr_read(struct DisasmStream *self, void *idisasm);

/* Disasm Stream Unit Tests */
void test_disasm_stream_unit_tests(void);

#endif

