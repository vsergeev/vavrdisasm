#ifndef FORMAT_STREAM_H
#define FORMAT_STREAM_H

#include <stdint.h>
#include <stdio.h>

#include "disasm_stream.h"
#include "stream_error.h"

#include "avr_instruction_set.h"

struct FormatStream {
    /* Input stream */
    struct DisasmStream *in;
    /* Stream state */
    void *state;
    /* Error */
    char *error;

    /* Init function */
    int (*stream_init)(struct FormatStream *self);
    /* Close function */
    int (*stream_close)(struct FormatStream *self);
    /* Output function */
    int (*stream_read)(struct FormatStream *self, FILE *out);
};

/* AVR Format Stream Support */
int format_stream_avr_init(struct FormatStream *self);
int format_stream_avr_close(struct FormatStream *self);
int format_stream_avr_read(struct FormatStream *self, FILE *out);

/* Format Stream Test */
void test_format_stream(void);

/* AVR Format Stream State/Options */
struct format_stream_avr_state {
    char **prefixes;
    unsigned int address_width;
    unsigned int flags;
    /* Previous address observed */
    uint32_t prev_address;
    /* Previous instruction width observed */
    uint32_t prev_instr_width;
};

/* AVR Format Stream Prefix String Indices */
enum {
    AVR_FORMAT_PREFIX_REGISTER,
    AVR_FORMAT_PREFIX_IO_REGISTER,
    AVR_FORMAT_PREFIX_DATA_HEX,
    AVR_FORMAT_PREFIX_DATA_BIN,
    AVR_FORMAT_PREFIX_DATA_DEC,
    AVR_FORMAT_PREFIX_BIT,
    AVR_FORMAT_PREFIX_ADDRESS_LABEL,
    AVR_FORMAT_PREFIX_ABSOLUTE_ADDRESS,
    AVR_FORMAT_PREFIX_RELATIVE_ADDRESS,
    AVR_FORMAT_PREFIX_DES_ROUND,
    AVR_FORMAT_PREFIX_RAW_WORD,
    AVR_FORMAT_PREFIX_RAW_BYTE,
};

/* AVR Format Stream Option Flags */
enum {
    AVR_FORMAT_FLAG_ADDRESS_LABELS          = (1<<0),
    AVR_FORMAT_FLAG_ADDRESSES               = (1<<1),
    AVR_FORMAT_FLAG_DEST_ADDR_COMMENT       = (1<<2),
    AVR_FORMAT_FLAG_DATA_HEX                = (1<<3),
    AVR_FORMAT_FLAG_DATA_BIN                = (1<<4),
    AVR_FORMAT_FLAG_DATA_DEC                = (1<<5),
    AVR_FORMAT_FLAG_OPCODES                 = (1<<6),
};

#endif

