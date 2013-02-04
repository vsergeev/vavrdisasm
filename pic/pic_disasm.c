#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <byte_stream.h>
#include <disasm_stream.h>
#include <instruction.h>

#include "pic_instruction_set.h"
#include "pic_support.h"

/******************************************************************************/
/* PIC Baseline / Midrange / Midrange Enhanced Disassembly Stream Support */
/******************************************************************************/

struct disasm_stream_pic_state {
    /* Architecture */
    int subarch;
    /* 2-byte opcode buffer */
    uint8_t data[2];
    uint32_t address[2];
    unsigned int len;
    /* EOF encountered flag */
    int eof;
};

static int disasm_stream_pic_init(struct DisasmStream *self, int subarch) {
    /* Allocate stream state */
    self->state = malloc(sizeof(struct disasm_stream_pic_state));
    if (self->state == NULL) {
        self->error = "Error allocating disasm stream state!";
        return STREAM_ERROR_ALLOC;
    }
    /* Initialize stream state */
    memset(self->state, 0, sizeof(struct disasm_stream_pic_state));
    ((struct disasm_stream_pic_state *)self->state)->subarch = subarch;

    /* Reset the error to NULL */
    self->error = NULL;

    /* Initialize the input stream */
    if (self->in->stream_init(self->in) < 0) {
        self->error = "Error in input stream initialization!";
        return STREAM_ERROR_INPUT;
    }

    return 0;
}

/* Wrapper functions for different sub-architectures */

int disasm_stream_pic_baseline_init(struct DisasmStream *self) {
    return disasm_stream_pic_init(self, PIC_ARCH_BASELINE);
}

int disasm_stream_pic_midrange_init(struct DisasmStream *self) {
    return disasm_stream_pic_init(self, PIC_ARCH_MIDRANGE);
}

int disasm_stream_pic_midrange_enhanced_init(struct DisasmStream *self) {
    return disasm_stream_pic_init(self, PIC_ARCH_MIDRANGE_ENHANCED);
}

int disasm_stream_pic_close(struct DisasmStream *self) {
    /* Free stream state memory */
    free(self->state);

    /* Close input stream */
    if (self->in->stream_close(self->in) < 0) {
        self->error = "Error in input stream close!";
        return STREAM_ERROR_INPUT;
    }

    return 0;
}

/******************************************************************************/
/* Core of the PIC Disassembler */
/******************************************************************************/

static int util_bits_data_from_mask(uint16_t data, uint16_t mask) {
    uint16_t result;
    int i, j;

    result = 0;

    /* Sweep through mask from bits 0 to 15 */
    for (i = 0, j = 0; i < 16; i++) {
        /* If mask bit is set */
        if (mask & (1 << i)) {
            /* If data bit is set */
            if (data & (1 << i))
                result |= (1 << j);
            j++;
        }
    }

    return result;
}

static struct picInstructionInfo *util_iset_lookup_by_opcode(int subarch, uint16_t opcode) {
    int i, j;

    uint16_t instructionBits;

    for (i = 0; i < PIC_TOTAL_INSTRUCTIONS[subarch]; i++) {
        instructionBits = opcode;

        /* Mask out the operands from the opcode */
        for (j = 0; j < PIC_Instruction_Sets[subarch][i].numOperands; j++)
            instructionBits &= ~(PIC_Instruction_Sets[subarch][i].operandMasks[j]);

        /* Compare left over instruction bits with the instruction mask */
        if (instructionBits == PIC_Instruction_Sets[subarch][i].instructionMask)
            return &PIC_Instruction_Sets[subarch][i];
    }

    return NULL;
}

static struct picInstructionInfo *util_iset_lookup_by_mnemonic(int subarch, char *mnemonic) {
    int i;

    for (i = 0; i < PIC_TOTAL_INSTRUCTIONS[subarch]; i++) {
        if (strcmp(mnemonic, PIC_Instruction_Sets[subarch][i].mnemonic) == 0)
            return &PIC_Instruction_Sets[subarch][i];
    }

    return NULL;
}

static int util_opbuffer_len_consecutive(struct disasm_stream_pic_state *state) {
    int i, lenConsecutive;

    lenConsecutive = 0;
    for (i = 0; i < state->len; i++) {
        /* If there is a greater than 1 byte gap between addresses */
        if (i > 0 && (state->address[i] - state->address[i-1]) != 1)
            break;
        lenConsecutive++;
    }

    return lenConsecutive;
}

static void util_opbuffer_shift(struct disasm_stream_pic_state *state, int n) {
    int i, j;

    for (i = 0; i < n; i++) {
        /* Shift the data and address slots down by one */
        for (j = 0; j < sizeof(state->data) - 1; j++) {
            state->data[j] = state->data[j+1];
            state->address[j] = state->address[j+1];
        }
        state->data[j] = 0x00;
        state->address[j] = 0x00;
        /* Update the opcode buffer length */
        if (state->len > 0)
            state->len--;
    }
}

static int32_t util_disasm_operand(struct picInstructionInfo *instruction, uint32_t operand, int index) {
    int32_t operandDisasm;
    uint32_t msb;

    switch (instruction->operandTypes[index]) {
        case OPERAND_SIGNED_LITERAL:
        case OPERAND_RELATIVE_ADDRESS:
            /* We got lucky, because it turns out that in all of the masks for
             * relative jumps / signed literals, the bits occupy the lowest
             * positions continuously (no breaks in the bit string). */
            /* Calculate the most significant bit of this signed data */
            msb = (instruction->operandMasks[index] + 1) >> 1;

            /* If the sign bit is set */
            if (operand & msb) {
                /* Manually sign-extend to the 32-bit container */
                operandDisasm = (int32_t) ( ( ~operand + 1 ) & instruction->operandMasks[index] );
                operandDisasm = -operandDisasm;
            } else {
                operandDisasm = (int32_t) ( operand & instruction->operandMasks[index] );
            }
            /* Multiply by two to point to a byte address */
            operandDisasm *= 2;

            break;
        default:
            /* Copy the operand with no additional processing */
            operandDisasm = operand;
            break;
    }

    return operandDisasm;
}

extern uint32_t pic_instruction_get_address(struct instruction *instr);
extern unsigned int pic_instruction_get_width(struct instruction *instr);
extern unsigned int pic_instruction_get_num_operands(struct instruction *instr);
extern void pic_instruction_get_opcodes(struct instruction *instr, uint8_t *dest);
extern int pic_instruction_get_str_origin(struct instruction *instr, char *dest, int size, int flags);
extern int pic_instruction_get_str_address_label(struct instruction *instr, char *dest, int size, int flags);
extern int pic_instruction_get_str_address(struct instruction *instr, char *dest, int size, int flags);
extern int pic_instruction_get_str_opcodes(struct instruction *instr, char *dest, int size, int flags);
extern int pic_instruction_get_str_mnemonic(struct instruction *instr, char *dest, int size, int flags);
extern int pic_instruction_get_str_operand(struct instruction *instr, char *dest, int size, int index, int flags);
extern int pic_instruction_get_str_comment(struct instruction *instr, char *dest, int size, int flags);
extern void pic_instruction_free(struct instruction *instr);

int disasm_stream_pic_read(struct DisasmStream *self, struct instruction *instr) {
    struct disasm_stream_pic_state *state = (struct disasm_stream_pic_state *)self->state;

    int decodeAttempts, lenConsecutive;

    /* Setup the function pointers of the instruction structure */
    instr->get_address = pic_instruction_get_address;
    instr->get_width = pic_instruction_get_width;
    instr->get_num_operands = pic_instruction_get_num_operands;
    instr->get_opcodes = pic_instruction_get_opcodes;
    instr->get_str_origin = pic_instruction_get_str_origin;
    instr->get_str_address_label = pic_instruction_get_str_address_label;
    instr->get_str_address = pic_instruction_get_str_address;
    instr->get_str_opcodes = pic_instruction_get_str_opcodes;
    instr->get_str_mnemonic = pic_instruction_get_str_mnemonic;
    instr->get_str_operand = pic_instruction_get_str_operand;
    instr->get_str_comment = pic_instruction_get_str_comment;
    instr->free = pic_instruction_free;

    for (decodeAttempts = 0; decodeAttempts < 3; decodeAttempts++) {
        /* Count the number of consective bytes in our opcode buffer */
        lenConsecutive = util_opbuffer_len_consecutive(state);

        /* If we decoded all bytes and reached EOF, return EOF too */
        if (lenConsecutive == 0 && state->len == 0 && state->eof)
            return STREAM_EOF;

        /* Edge case: when input stream changes address with 1 undecoded
         * byte */
            /* One lone byte at some address or at EOF */
        if (lenConsecutive == 1 && (state->len > 1 || state->eof)) {
            /* Decode a raw .DB byte "instruction" */
            instr->instructionDisasm = malloc(sizeof(struct picInstructionDisasm));
            if (instr->instructionDisasm == NULL) {
                self->error = "Error allocating memory for disassembled instruction!";
                return STREAM_ERROR_FAILURE;
            }
            struct picInstructionDisasm *instructionDisasm = (struct picInstructionDisasm *)(instr->instructionDisasm);

            memset(instructionDisasm, 0, sizeof(struct picInstructionDisasm));
            instructionDisasm->address = state->address[0];
            instructionDisasm->opcode[0] = state->data[0];
            instructionDisasm->instructionInfo = &PIC_Instruction_Sets[state->subarch][PIC_ISET_INDEX_BYTE(state->subarch)];
            instructionDisasm->operandDisasms[0] = (int32_t)state->data[0];
            /* Shift out the processed byte(s) from our opcode buffer */
            util_opbuffer_shift(state, 1);
            return 0;
        }

        /* Two consecutive bytes */
        if (lenConsecutive == 2) {
            uint16_t opcode;
            uint32_t operand;
            struct picInstructionInfo *instructionInfo;
            int i;

            /* Assemble the 16-bit opcode from little-endian input */
            opcode = (uint16_t)(state->data[1] << 8) | (uint16_t)(state->data[0]);
            /* Look up the instruction in our instruction set */
            if ( (instructionInfo = util_iset_lookup_by_opcode(state->subarch, opcode)) == NULL) {
                /* This should never happen because of the .DW instruction that
                 * matches any 16-bit opcode */
                self->error = "Error, catastrophic failure! Malformed instruction set!";
                return STREAM_ERROR_FAILURE;
            }

            /* Decode and return the 16-bit instruction */
            instr->instructionDisasm = malloc(sizeof(struct picInstructionDisasm));
            if (instr->instructionDisasm == NULL) {
                self->error = "Error allocating memory for disassembled instruction!";
                return STREAM_ERROR_FAILURE;
            }
            struct picInstructionDisasm *instructionDisasm = (struct picInstructionDisasm *)(instr->instructionDisasm);

            memset(instructionDisasm, 0, sizeof(struct picInstructionDisasm));
            instructionDisasm->address = state->address[0];
            instructionDisasm->opcode[0] = state->data[0];
            instructionDisasm->opcode[1] = state->data[1];
            instructionDisasm->instructionInfo = instructionInfo;
            /* Disassemble the operands */
            for (i = 0; i < instructionInfo->numOperands; i++) {
                /* Extract the operand bits */
                operand = util_bits_data_from_mask(opcode, instructionInfo->operandMasks[i]);
                /* Disassemble the operand */
                instructionDisasm->operandDisasms[i] = util_disasm_operand(instructionInfo, operand, i);
            }
            /* Shift out the processed byte(s) from our opcode buffer */
            util_opbuffer_shift(state, 2);
            return 0;
        }
        /* Otherwise, read another byte into our opcode buffer below */

        uint8_t readData;
        uint32_t readAddr;
        int ret;

        /* Read the next data byte from the opcode stream */
        ret = self->in->stream_read(self->in, &readData, &readAddr);
        if (ret == STREAM_EOF) {
            /* Record encountered EOF */
            state->eof = 1;
        } else if (ret < 0) {
            self->error = "Error in opcode stream read!";
            return STREAM_ERROR_INPUT;
        } else if (ret == 0) {
            /* If we have an opcode buffer overflow (this should never happen
             * if the decoding logic above is correct) */
            if (state->len == sizeof(state->data)) {
                self->error = "Error, catastrophic failure! Opcode buffer overflowed!";
                return STREAM_ERROR_FAILURE;
            }

            /* Append the data / address to our opcode buffer */
            state->data[state->len] = readData;
            state->address[state->len] = readAddr;
            state->len++;
        }
    }

    /* We should have returned an instruction above */
    self->error = "Error, catastrophic failure! No decoding logic invoked!";
    return STREAM_ERROR_FAILURE;

    return 0;
}

