#ifndef AVR_H
#define AVR_H

#include <stdio.h>
#include <disasm_stream.h>
#include <print_stream.h>

/* AVR Disassembly Stream Support */
int disasm_stream_avr_init(struct DisasmStream *self);
int disasm_stream_avr_close(struct DisasmStream *self);
int disasm_stream_avr_read(struct DisasmStream *self, void *idisasm);

/* AVR Print Stream State/Options */
struct print_stream_avr_state {
    /* Print Option Bit Flags */
    unsigned int flags;
    /* Address label prefix */
    char address_label_prefix[8];
    /* Previous address observed */
    uint32_t prev_address;
    /* Previous instruction width observed */
    uint32_t prev_instr_width;
};

/* AVR Print Stream Option Flags */
enum {
    AVR_PRINT_FLAG_ADDRESS_LABELS          = (1<<0),
    AVR_PRINT_FLAG_ADDRESSES               = (1<<1),
    AVR_PRINT_FLAG_DEST_ADDR_COMMENT       = (1<<2),
    AVR_PRINT_FLAG_DATA_HEX                = (1<<3),
    AVR_PRINT_FLAG_DATA_BIN                = (1<<4),
    AVR_PRINT_FLAG_DATA_DEC                = (1<<5),
    AVR_PRINT_FLAG_OPCODES                 = (1<<6),
};

/* AVR Print Stream Support */
int print_stream_avr_init(struct PrintStream *self);
int print_stream_avr_close(struct PrintStream *self);
int print_stream_avr_read(struct PrintStream *self, FILE *out);

#endif

