#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <byte_stream.h>
#include <disasm_stream.h>
#include <instruction.h>

#include "avr_instruction_set.h"
#include "avr_support.h"

/******************************************************************************/
/* AVR Disassembly Stream Support */
/******************************************************************************/

struct disasm_stream_avr_state {
    /* 4-byte opcode buffer */
    uint8_t data[4];
    uint32_t address[4];
    unsigned int len;
    /* EOF encountered flag */
    int eof;
    /* Disassembled instruction */
    struct avrInstructionDisasm instrDisasm;
};

int disasm_stream_avr_init(struct DisasmStream *self) {
    /* Allocate stream state */
    self->state = malloc(sizeof(struct disasm_stream_avr_state));
    if (self->state == NULL) {
        self->error = "Error allocating disasm stream state!";
        return STREAM_ERROR_ALLOC;
    }
    /* Initialize stream state */
    memset(self->state, 0, sizeof(struct disasm_stream_avr_state));

    /* Reset the error to NULL */
    self->error = NULL;

    /* Initialize the input stream */
    if (self->in->stream_init(self->in) < 0) {
        self->error = "Error in input stream initialization!";
        return STREAM_ERROR_INPUT;
    }

    return 0;
}

int disasm_stream_avr_close(struct DisasmStream *self) {
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
/* Core of the AVR Disassembler */
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

static struct avrInstructionInfo *util_iset_lookup_by_opcode(uint16_t opcode) {
    int i, j;

    uint16_t instructionBits;

    for (i = 0; i < AVR_TOTAL_INSTRUCTIONS; i++) {
        instructionBits = opcode;

        /* Mask out the operands from the opcode */
        for (j = 0; j < AVR_Instruction_Set[i].numOperands; j++)
            instructionBits &= ~(AVR_Instruction_Set[i].operandMasks[j]);

        /* Compare left over instruction bits with the instruction mask */
        if (instructionBits == AVR_Instruction_Set[i].instructionMask)
            return &AVR_Instruction_Set[i];
    }

    return NULL;
}

static struct avrInstructionInfo *util_iset_lookup_by_mnemonic(char *mnemonic) {
    int i;

    for (i = 0; i < AVR_TOTAL_INSTRUCTIONS; i++) {
        if (strcmp(mnemonic, AVR_Instruction_Set[i].mnemonic) == 0)
            return &AVR_Instruction_Set[i];
    }

    return NULL;
}

static int util_opbuffer_len_consecutive(struct disasm_stream_avr_state *state) {
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

static void util_opbuffer_shift(struct disasm_stream_avr_state *state, int n) {
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

static int32_t util_disasm_operand(uint32_t operand, int operandType) {
    int32_t operandDisasm;

    switch (operandType) {
        case OPERAND_BRANCH_ADDRESS:
            /* Relative branch address is 7 bits, two's complement form */

            /* If the sign bit is set */
            if (operand & (1 << 6)) {
                /* Manually sign-extend to the 32-bit container */
                operandDisasm = (int32_t) ( ( ~operand + 1 ) & 0x7f );
                operandDisasm = -operandDisasm;
            } else {
                operandDisasm = (int32_t) ( operand & 0x7f );
            }
            /* Multiply by two to point to a byte address */
            operandDisasm *= 2;

            break;
        case OPERAND_RELATIVE_ADDRESS:
             /* Relative address is 12 bits, two's complement form */

            /* If the sign bit is set */
            if (operand & (1 << 11)) {
                /* Manually sign-extend to the 32-bit container */
                operandDisasm = (int32_t) ( ( ~operand + 1 ) & 0xfff );
                operandDisasm = -operandDisasm;
            } else {
                operandDisasm = (int32_t) ( operand & 0xfff );
            }
            /* Multiply by two to point to a byte address */
            operandDisasm *= 2;

            break;
        case OPERAND_LONG_ABSOLUTE_ADDRESS:
            /* Multiply by two to point to a byte address */
            operandDisasm = operand * 2;
            break;
        case OPERAND_REGISTER_STARTR16:
            /* Register offset from R16 */
            operandDisasm = 16 + operand;
            break;
        case OPERAND_REGISTER_EVEN_PAIR:
            /* Register even */
            operandDisasm = operand*2;
            break;
        case OPERAND_REGISTER_EVEN_PAIR_STARTR24:
            /* Register even offset from R24 */
            operandDisasm = 24 + operand*2;
            break;
        default:
            /* Copy the operand with no additional processing */
            operandDisasm = operand;
            break;
    }

    return operandDisasm;
}

int disasm_stream_avr_read(struct DisasmStream *self, struct instruction *instr) {
    struct disasm_stream_avr_state *state = (struct disasm_stream_avr_state *)self->state;

    int decodeAttempts, lenConsecutive;

    /* Fill disassembled instruction pointer and print functions in instruction
     * structure */
    instr->instructionDisasm = (void *)&(state->instrDisasm);
    instr->print_origin = avr_instruction_print_origin;
    instr->print = avr_instruction_print;

    for (decodeAttempts = 0; decodeAttempts < 5; decodeAttempts++) {
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
            memset(&(state->instrDisasm), 0, sizeof(struct avrInstructionDisasm));
            state->instrDisasm.address = state->address[0];
            state->instrDisasm.opcode[0] = state->data[0];
            state->instrDisasm.instructionInfo = &AVR_Instruction_Set[AVR_ISET_INDEX_BYTE];
            state->instrDisasm.operandDisasms[0] = (int32_t)state->data[0];
            /* Shift out the processed byte(s) from our opcode buffer */
            util_opbuffer_shift(state, 1);

            /* Fill the instruction structure */
            instr->address = state->instrDisasm.address;
            instr->width = state->instrDisasm.instructionInfo->width;
            return 0;
        }

        /* Two or more consecutive bytes */
        if (lenConsecutive >= 2) {
            uint16_t opcode;
            uint32_t operand;
            struct avrInstructionInfo *instructionInfo;
            int i;

            /* Assemble the 16-bit opcode from little-endian input */
            opcode = (uint16_t)(state->data[1] << 8) | (uint16_t)(state->data[0]);
            /* Look up the instruction in our instruction set */
            if ( (instructionInfo = util_iset_lookup_by_opcode(opcode)) == NULL) {
                /* This should never happen because of the .DW instruction that
                 * matches any 16-bit opcode */
                self->error = "Error, catastrophic failure! Malformed instruction set!";
                return STREAM_ERROR_FAILURE;
            }

            /* If this is a 16-bit wide instruction */
            if (instructionInfo->width == 2) {
                /* Decode and return a 16-bit instruction */
                memset(&(state->instrDisasm), 0, sizeof(struct avrInstructionDisasm));
                state->instrDisasm.address = state->address[0];
                state->instrDisasm.opcode[0] = state->data[0]; state->instrDisasm.opcode[1] = state->data[1];
                state->instrDisasm.instructionInfo = instructionInfo;
                /* Disassemble the operands */
                for (i = 0; i < instructionInfo->numOperands; i++) {
                    /* Extract the operand bits */
                    operand = util_bits_data_from_mask(opcode, instructionInfo->operandMasks[i]);
                    /* Disassemble the operand */
                    state->instrDisasm.operandDisasms[i] = util_disasm_operand(operand, instructionInfo->operandTypes[i]);
                }
                /* Shift out the processed byte(s) from our opcode buffer */
                util_opbuffer_shift(state, 2);

                /* Fill the instruction structure */
                instr->address = state->instrDisasm.address;
                instr->width = state->instrDisasm.instructionInfo->width;

                return 0;

            /* Else, this is a 32-bit wide instruction */
            } else {
                /* We have read the complete 32-bit instruction */
                if (lenConsecutive == 4) {
                    /* Decode a 32-bit instruction */
                    memset(&(state->instrDisasm), 0, sizeof(struct avrInstructionDisasm));
                    state->instrDisasm.address = state->address[0];
                    state->instrDisasm.opcode[0] = state->data[0]; state->instrDisasm.opcode[1] = state->data[1];
                    state->instrDisasm.opcode[2] = state->data[2]; state->instrDisasm.opcode[3] = state->data[3];
                    state->instrDisasm.instructionInfo = instructionInfo;
                    /* Disassemble the operands */
                    for (i = 0; i < instructionInfo->numOperands; i++) {
                        /* Extract the operand bits */
                        operand = util_bits_data_from_mask(opcode, instructionInfo->operandMasks[i]);

                        /* Append the extra bits if it's a long operand */
                        if (instructionInfo->operandTypes[i] == OPERAND_LONG_ABSOLUTE_ADDRESS)
                            operand = (uint32_t)(operand << 16) | (uint32_t)(state->data[3] << 8) | (uint32_t)(state->data[2]);

                        /* Disassemble the operand */
                        state->instrDisasm.operandDisasms[i] = util_disasm_operand(operand, instructionInfo->operandTypes[i]);
                    }
                    /* Shift out the processed byte(s) from our opcode buffer */
                    util_opbuffer_shift(state, 4);

                    /* Fill the instruction structure */
                    instr->address = state->instrDisasm.address;
                    instr->width = state->instrDisasm.instructionInfo->width;

                    return 0;

                /* Edge case: when input stream changes address with 3 or 2
                 * undecoded long instruction bytes */
                    /* Three lone bytes at some address or at EOF */
                    /* Two lone bytes at some address or at EOF */
                } else if ((lenConsecutive == 3 && (state->len > 3 || state->eof)) ||
                           (lenConsecutive == 2 && (state->len > 2 || state->eof))) {
                    /* Return a raw .DW word "instruction" */
                    memset(&(state->instrDisasm), 0, sizeof(struct avrInstructionDisasm));
                    state->instrDisasm.address = state->address[0];
                    state->instrDisasm.opcode[0] = state->data[0]; state->instrDisasm.opcode[1] = state->data[1];
                    state->instrDisasm.instructionInfo = &AVR_Instruction_Set[AVR_ISET_INDEX_WORD];
                    /* Disassemble the operands */
                    for (i = 0; i < instructionInfo->numOperands; i++) {
                        /* Extract the operand bits */
                        operand = util_bits_data_from_mask(opcode, instructionInfo->operandMasks[i]);
                        /* Disassemble the operand */
                        state->instrDisasm.operandDisasms[i] = util_disasm_operand(operand, instructionInfo->operandTypes[i]);
                    }
                    /* Shift out the processed byte(s) from our opcode buffer */
                    util_opbuffer_shift(state, 2);

                    /* Fill the instruction structure */
                    instr->address = state->instrDisasm.address;
                    instr->width = state->instrDisasm.instructionInfo->width;

                    return 0;
                }

                /* Otherwise, read another byte into our opcode buffer below */
            }
        }

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

