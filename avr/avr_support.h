#ifndef AVR_SUPPORT_H
#define AVR_SUPPORT_H

#include <disasm_stream.h>
#include <instruction.h>

/* AVR Disassembly Stream Support */
int disasm_stream_avr_init(struct DisasmStream *self);
int disasm_stream_avr_close(struct DisasmStream *self);
int disasm_stream_avr_read(struct DisasmStream *self, struct instruction *instr);

#endif

