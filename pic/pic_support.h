#ifndef PIC_SUPPORT_H
#define PIC_SUPPORT_H

#include <disasm_stream.h>
#include <instruction.h>

/* PIC Disassembly Stream Supoprt */
int disasm_stream_pic_close(struct DisasmStream *self);
int disasm_stream_pic_read(struct DisasmStream *self, struct instruction *instr);

/* PIC Baseline Disassembly Stream Support */
int disasm_stream_pic_baseline_init(struct DisasmStream *self);
#define disasm_stream_pic_baseline_close    disasm_stream_pic_close
#define disasm_stream_pic_baseline_read     disasm_stream_pic_read
/* PIC Midrange Disassembly Stream Support */
int disasm_stream_pic_midrange_init(struct DisasmStream *self);
#define disasm_stream_pic_midrange_close    disasm_stream_pic_close
#define disasm_stream_pic_midrange_read     disasm_stream_pic_read
/* PIC Midrange Enhanced Disassembly Stream Support */
int disasm_stream_pic_midrange_enhanced_init(struct DisasmStream *self);
#define disasm_stream_pic_midrange_enhanced_close    disasm_stream_pic_close
#define disasm_stream_pic_midrange_enhanced_read     disasm_stream_pic_read

/* PIC Instruction Print Support */
int pic_instruction_print_origin(struct instruction *instr, FILE *out, int flags);
int pic_instruction_print(struct instruction *instr, FILE *out, int flags);

#endif

