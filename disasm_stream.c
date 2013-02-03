#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "stream_error.h"
#include "opcode_stream.h"
#include "disasm_stream.h"

#include "avr_instruction_set.h"

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
};

int disasm_stream_avr_init(struct DisasmStream *self) {
    struct disasm_stream_avr_state *state;

    /* Allocate stream state */
    self->state = malloc(sizeof(struct disasm_stream_avr_state));
    if (self->state == NULL) {
        self->error = "Error allocating disasm stream state!";
        return STREAM_ERROR_ALLOC;
    }
    /* Initialize stream state */
    state = (struct disasm_stream_avr_state *)self->state;
    state->len = 0;
    state->eof = 0;

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

static struct avrInstructionInfo *util_iset_lookup_by_mnemonic(char *m) {
    int i;

    for (i = 0; i < AVR_TOTAL_INSTRUCTIONS; i++) {
        if (strcmp(m, AVR_Instruction_Set[i].mnemonic) == 0)
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

int disasm_stream_avr_read(struct DisasmStream *self, void *idisasm) {
    struct disasm_stream_avr_state *state = (struct disasm_stream_avr_state *)self->state;
    struct avrInstructionDisasm *instrDisasm = (struct avrInstructionDisasm *)idisasm;

    int decodeAttempts, lenConsecutive;

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
            memset(instrDisasm, 0, sizeof(struct avrInstructionDisasm));
            instrDisasm->address = state->address[0];
            instrDisasm->opcode[0] = state->data[0];
            instrDisasm->instructionSetEntry = &AVR_Instruction_Set[AVR_ISET_INDEX_BYTE];
            instrDisasm->operandDisasms[0] = (int32_t)state->data[0];
            instrDisasm->width = 1;
            /* Shift out the processed byte(s) from our opcode buffer */
            util_opbuffer_shift(state, 1);
            return 0;
        }

        /* Two or more consecutive bytes */
        if (lenConsecutive >= 2) {
            uint16_t opcode; uint32_t operand;
            struct avrInstructionInfo *instruction;
            int i;

            /* Assemble the 16-bit opcode from little-endian input */
            opcode = (uint16_t)(state->data[1] << 8) | (uint16_t)(state->data[0]);
            /* Look up the instruction in our instruction set */
            if ( (instruction = util_iset_lookup_by_opcode(opcode)) == NULL) {
                /* This should never happen because of the .DW instruction that
                 * matches any 16-bit opcode */
                self->error = "Error, catastrophic failure! Malformed instruction set!";
                return STREAM_ERROR_FAILURE;
            }

            /* If this is a 16-bit wide instruction */
            if (instruction->operandTypes[0] != OPERAND_LONG_ABSOLUTE_ADDRESS &&
                instruction->operandTypes[1] != OPERAND_LONG_ABSOLUTE_ADDRESS) {
                /* Decode and return the 16-bit instruction */
                memset(instrDisasm, 0, sizeof(struct avrInstructionDisasm));
                instrDisasm->address = state->address[0];
                instrDisasm->opcode[0] = state->data[0]; instrDisasm->opcode[1] = state->data[1];
                instrDisasm->width = 2;
                instrDisasm->instructionSetEntry = instruction;
                /* Disassemble the operands */
                for (i = 0; i < instruction->numOperands; i++) {
                    /* Extract the operand bits */
                    operand = util_bits_data_from_mask(opcode, instruction->operandMasks[i]);
                    /* Disassemble the operand */
                    instrDisasm->operandDisasms[i] = util_disasm_operand(operand, instruction->operandTypes[i]);
                }
                /* Shift out the processed byte(s) from our opcode buffer */
                util_opbuffer_shift(state, 2);
                return 0;

            /* Else, this is a 32-bit wide instruction */
            } else {
                /* We have read the complete 32-bit instruction */
                if (lenConsecutive == 4) {
                    /* Decode and return the 32-bit instruction */
                    memset(instrDisasm, 0, sizeof(struct avrInstructionDisasm));
                    instrDisasm->address = state->address[0];
                    instrDisasm->opcode[0] = state->data[0]; instrDisasm->opcode[1] = state->data[1];
                    instrDisasm->opcode[2] = state->data[2]; instrDisasm->opcode[3] = state->data[3];
                    instrDisasm->width = 4;
                    instrDisasm->instructionSetEntry = instruction;
                    /* Disassemble the operands */
                    for (i = 0; i < instruction->numOperands; i++) {
                        /* Extract the operand bits */
                        operand = util_bits_data_from_mask(opcode, instruction->operandMasks[i]);

                        /* Append the extra bits if it's a long operand */
                        if (instruction->operandTypes[i] == OPERAND_LONG_ABSOLUTE_ADDRESS)
                            operand = (uint32_t)(operand << 16) | (uint32_t)(state->data[3] << 8) | (uint32_t)(state->data[2]);

                        /* Disassemble the operand */
                        instrDisasm->operandDisasms[i] = util_disasm_operand(operand, instruction->operandTypes[i]);
                    }
                    /* Shift out the processed byte(s) from our opcode buffer */
                    util_opbuffer_shift(state, 4);
                    return 0;

                /* Edge case: when input stream changes address with 3 or 2
                 * undecoded long instruction bytes */
                    /* Three lone bytes at some address or at EOF */
                    /* Two lone bytes at some address or at EOF */
                } else if ((lenConsecutive == 3 && (state->len > 3 || state->eof)) ||
                           (lenConsecutive == 2 && (state->len > 2 || state->eof))) {
                    /* Return a raw .DW word "instruction" */
                    memset(instrDisasm, 0, sizeof(struct avrInstructionDisasm));
                    instrDisasm->address = state->address[0];
                    instrDisasm->opcode[0] = state->data[0]; instrDisasm->opcode[1] = state->data[1];
                    instrDisasm->width = 2;
                    instrDisasm->instructionSetEntry = &AVR_Instruction_Set[AVR_ISET_INDEX_WORD];
                    instrDisasm->operandDisasms[0] = (uint16_t)(state->data[1] << 8) | (uint16_t)state->data[0];
                    /* Shift out the processed byte(s) from our opcode buffer */
                    util_opbuffer_shift(state, 2);
                    return 0;
                }

                /* Otherwise, read another byte into our opcode buffer below */
            }
        }

        uint8_t readData; uint32_t readAddr; unsigned int readLen;
        int ret;

        /* Read the next data byte from the opcode stream */
        ret = self->in->stream_read(self->in, &readData, &readAddr, &readLen, 1);
        switch (ret) {
            case 0:
                break;
            case STREAM_EOF:
                /* Record encountered EOF */
                state->eof = 1;
                break;
            default:
                self->error = "Error in opcode stream read!";
                return STREAM_ERROR_INPUT;
        }

        /* If we successfully read a byte */
        if (readLen) {
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

/******************************************************************************/
/* Disasm Stream Test Instrumentation */
/******************************************************************************/

struct opcode_stream_debug_state {
    uint8_t *data;
    uint32_t *address;
    unsigned int len;
    int index;
};

static int debug_disasm_stream(uint8_t *test_data, uint32_t *test_address, unsigned int test_len, struct avrInstructionDisasm *output_instrDisasms, unsigned int *output_len) {
    struct OpcodeStream os;
    struct DisasmStream ds;
    int ret;

    /* Setup a debug Opcode Stream */
    os.in = NULL;
    os.stream_init = opcode_stream_debug_init;
    os.stream_close = opcode_stream_debug_close;
    os.stream_read = opcode_stream_debug_read;

    /* Setup the AVR Disasm Stream */
    ds.in = &os;
    ds.stream_init = disasm_stream_avr_init;
    ds.stream_close = disasm_stream_avr_close;
    ds.stream_read = disasm_stream_avr_read;

    /* Initialize the stream */
    printf("\tds.stream_init(): %d\n", ds.stream_init(&ds));
    if (ds.error != NULL) {
        printf("\t\tError: %s\n", ds.error);
        return -1;
    }

    /* Load the Opcode Stream with the test vector */
    struct opcode_stream_debug_state *state;
    state = (struct opcode_stream_debug_state *)os.state;
    state->data = test_data;
    state->address = test_address;
    state->len = test_len;

    *output_len = 0;

    for (; ret != STREAM_EOF; output_instrDisasms++) {
        /* Disassemble an instruction */
        ret = ds.stream_read(&ds, output_instrDisasms);

        printf("\tds.stream_read(): %d\n", ret);
        if (ds.error != NULL)
            printf("\t\tError: %s\n", ds.error);

        if (ret == 0) {
            printf("\t\t%04x: Instruction: %s | Operands: %d, 0x%02x 0x%02x\n", output_instrDisasms->address, output_instrDisasms->instructionSetEntry->mnemonic, output_instrDisasms->instructionSetEntry->numOperands, output_instrDisasms->operandDisasms[0], output_instrDisasms->operandDisasms[1]);
            *output_len = *output_len + 1;
        } else if (ret == -1) {
            printf("\t\tEOF encountered\n");
        }
    }

    /* Close the stream */
    printf("\tds.stream_close(): %d\n", ds.stream_close(&ds));
    if (ds.error != NULL) {
        printf("\t\tError: %s\n", ds.error);
        return -1;
    }

    printf("\n");

    return 0;
}

/******************************************************************************/
/* Disasm Stream Unit Tests */
/******************************************************************************/

static int test_disasm_unit_test_run(char *name, uint8_t *test_data, uint32_t *test_address, unsigned int test_len, struct avrInstructionDisasm *expected_instrDisasms, unsigned int expected_len) {
    int ret, i, j;
    struct avrInstructionDisasm instrDisasms[16];
    unsigned int len;

    printf("Running test \"%s\"\n", name);

    /* Run the Disasm Stream on the test vectors */
    ret = debug_disasm_stream(test_data, test_address, test_len, &instrDisasms[0], &len);

    /* Check return value */
    if (ret != 0) {
        printf("\tFAILURE ret == 0\n\n");
        return -1;
    }
    printf("\tSUCCESS ret == 0\n");

    /* Compare number of disassembled instructions */
    if (len != expected_len) {
        printf("\tFAILURE len == expected_len\n\n");
        return -1;
    }
    printf("\tSUCCESS len == expected_len\n");

    /* Compare each disassembled instruction */
    for (i = 0; i < expected_len; i++) {
        /* Compare instruction address */
        if (instrDisasms[i].address != expected_instrDisasms[i].address) {
            printf("\tFAILURE instr %d address:\t0x%04x, \texpected 0x%04x\n\n", i, instrDisasms[i].address, expected_instrDisasms[i].address);
            return -1;
        }
        printf("\tSUCCESS instr %d address:\t0x%04x, \texpected 0x%04x\n", i, instrDisasms[i].address, expected_instrDisasms[i].address);

        /* Compare instruction width */
        if (instrDisasms[i].width != expected_instrDisasms[i].width) {
            printf("\tFAILURE instr %d width:\t\t%d, \t\texpected %d\n\n", i, instrDisasms[i].width, expected_instrDisasms[i].width);
            return -1;
        }
        printf("\tSUCCESS instr %d width:\t\t%d, \t\texpected %d\n", i, instrDisasms[i].width, expected_instrDisasms[i].width);

        /* Compare instruction identified */
        if (instrDisasms[i].instructionSetEntry != expected_instrDisasms[i].instructionSetEntry) {
            printf("\tFAILURE instr %d entry:  \t%s, \t\texpected %s\n\n", i, instrDisasms[i].instructionSetEntry->mnemonic, expected_instrDisasms[i].instructionSetEntry->mnemonic);
            return -1;
        }
        printf("\tSUCCESS instr %d entry:  \t%s, \t\texpected %s\n", i, instrDisasms[i].instructionSetEntry->mnemonic, expected_instrDisasms[i].instructionSetEntry->mnemonic);

        /* Compare disassembled operands */
        for (j = 0; j < AVR_MAX_NUM_OPERANDS; j++) {
           if (instrDisasms[i].operandDisasms[j] != expected_instrDisasms[i].operandDisasms[j]) {
                printf("\tFAILURE instr %d operand %d:\t0x%04x, \texpected 0x%04x\n\n", i, j, instrDisasms[i].operandDisasms[j], expected_instrDisasms[i].operandDisasms[j]);
                return -1;
            }
            printf("\tSUCCESS instr %d operand %d:\t0x%04x, \texpected 0x%04x\n", i, j, instrDisasms[i].operandDisasms[j], expected_instrDisasms[i].operandDisasms[j]);
        }
    }

    printf("\tSUCCESS all checks passed!\n\n");
    return 0;
}

void test_disasm_stream_unit_tests(void) {
    int numTests = 0, passedTests = 0;
    struct avrInstructionInfo *(*lookup)(char *) = util_iset_lookup_by_mnemonic;

    /* Check Sample Program */
    /* rjmp .0; ser R16; out $17, R16; out $18, R16; dec R16; rjmp .-6 */
    {
        uint8_t d[] =  {0x00, 0xc0, 0x0f, 0xef, 0x07, 0xbb, 0x08, 0xbb, 0x0a, 0x95, 0xfd, 0xcf};
        uint32_t a[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b};
        struct avrInstructionDisasm dis[] = {
                                                {0x00, {0}, 2, lookup("rjmp"), {0, 0x00}},
                                                {0x02, {0}, 2, lookup("ser"), {16, 0x00}},
                                                {0x04, {0}, 2, lookup("out"), {0x17, 0x10}},
                                                {0x06, {0}, 2, lookup("out"), {0x18, 0x10}},
                                                {0x08, {0}, 2, lookup("dec"), {16, 0x00}},
                                                {0x0a, {0}, 2, lookup("rjmp"), {-6, 0x00}},
                                            };
        if (test_disasm_unit_test_run("Sample Program", &d[0], &a[0], sizeof(d), &dis[0], sizeof(dis)/sizeof(dis[0])) == 0)
            passedTests++;
        numTests++;
    }

    /* Check 32-bit Instructions */
    /* jmp 0x2abab4; call 0x1f00e; sts 0x1234, r2; lds r3, 0x6780 */
    {
        uint8_t d[] =  {0xad, 0x94, 0x5a, 0x5d, 0x0e, 0x94, 0x07, 0xf8, 0x20, 0x92, 0x34, 0x12, 0x30, 0x90, 0x80, 0x67};
        uint32_t a[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
        struct avrInstructionDisasm dis[] = {
                                                {0x00, {0}, 4, lookup("jmp"), {0x2abab4, 0x00}},
                                                {0x04, {0}, 4, lookup("call"), {0x1f00e, 0x00}},
                                                {0x08, {0}, 4, lookup("sts"), {0x2468, 2}},
                                                {0x0c, {0}, 4, lookup("lds"), {3, 0xcf00}},
                                            };
        if (test_disasm_unit_test_run("32-bit Instructions", &d[0], &a[0], sizeof(d), &dis[0], sizeof(dis)/sizeof(dis[0])) == 0)
            passedTests++;
        numTests++;
    }

    /* Check EOF lone byte */
    /* Lone byte due to EOF */
    {
        uint8_t d[] = {0x18};
        uint32_t a[] = {0x500};
        struct avrInstructionDisasm dis[] = {
                                                {0x500, {0}, 1, lookup(".db"), {0x18, 0x00}},
                                            };
        if (test_disasm_unit_test_run("EOF Lone Byte", &d[0], &a[0], sizeof(d), &dis[0], sizeof(dis)/sizeof(dis[0])) == 0)
            passedTests++;
        numTests++;
    }

    /* Check boundary lone byte */
    /* Lone byte due to address change */
    {
        uint8_t d[] = {0x18, 0x12, 0x33};
        uint32_t a[] = {0x500, 0x502, 0x503};
        struct avrInstructionDisasm dis[] = {
                                                {0x500, {0}, 1, lookup(".db"), {0x18, 0x00}},
                                                {0x502, {0}, 2, lookup("cpi"), {0x11, 0x32}},
                                            };
        if (test_disasm_unit_test_run("Boundary Lone Byte", &d[0], &a[0], sizeof(d), &dis[0], sizeof(dis)/sizeof(dis[0])) == 0)
            passedTests++;
        numTests++;
    }

    /* Check EOF lone wide instruction */
    /* Call instruction 0x94ae 0xab XX cut short by EOF */
    {
        uint8_t d[] = {0xae, 0x94, 0xab};
        uint32_t a[] = {0x500, 0x501, 0x502};
        struct avrInstructionDisasm dis[] = {   {0x500, {0}, 2, lookup(".dw"), {0x94ae, 0x00}},
                                                {0x502, {0}, 1, lookup(".db"), {0xab, 0x00}},
                                            };
        if (test_disasm_unit_test_run("EOF Lone Wide Instruction", &d[0], &a[0], sizeof(d), &dis[0], sizeof(dis)/sizeof(dis[0])) == 0)
            passedTests++;
        numTests++;
    }

    /* Check boundary lone wide instruction */
    /* Call instruction 500: 0x94ae | 504: 0xab 0xcd cut short by address change */
    {
        uint8_t d[] = {0xae, 0x94, 0xab, 0xcd};
        uint32_t a[] = {0x500, 0x501, 0x504, 0x505};
        struct avrInstructionDisasm dis[] = {   {0x500, {0}, 2, lookup(".dw"), {0x94ae, 0x00}},
                                                {0x504, {0}, 2, lookup("rjmp"), {-0x4aa, 0x00}},
                                            };
        if (test_disasm_unit_test_run("Boundary Lone Wide Instruction", &d[0], &a[0], sizeof(d), &dis[0], sizeof(dis)/sizeof(dis[0])) == 0)
            passedTests++;
        numTests++;
    }

    printf("%d / %d tests passed.\n", passedTests, numTests);
}

