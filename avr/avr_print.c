#include <stdint.h>
#include <stdio.h>

#include <print_stream.h>
#include <instruction.h>

#include "avr_instruction_set.h"

/* AVRASM format prefixes */
#define AVR_PREFIX_REGISTER             "R"  /* mov R0, R2 */
#define AVR_PREFIX_IO_REGISTER          "$"  /* out $39, R16 */
#define AVR_PREFIX_DATA_HEX             "0x" /* ldi R16, 0x3d */
#define AVR_PREFIX_DATA_BIN             "0b" /* ldi R16, 0b00111101 */
#define AVR_PREFIX_DATA_DEC             ""   /* ldi R16, 61 */
#define AVR_PREFIX_BIT                  ""   /* cbi $12, 7 */
#define AVR_PREFIX_ABSOLUTE_ADDRESS     "0x" /* call 0x1234 */
#define AVR_PREFIX_RELATIVE_ADDRESS     "."  /* rjmp .4 */
#define AVR_PREFIX_DES_ROUND            "0x" /* des 0x01 */
#define AVR_PREFIX_RAW_WORD             "0x" /* .dw 0xabcd */
#define AVR_PREFIX_RAW_BYTE             "0x" /* .db 0xab */
#define AVR_PREFIX_ADDRESS_LABEL        "A_" /* A_0004: */

/* Address filed width, e.g. 4 -> 0x0004 */
#define AVR_ADDRESS_WIDTH               4

int avr_instruction_print_origin(struct instruction *instr, FILE *out, int flags) {
    /* Print an origin directive if we're outputting assembly */
    if (flags & PRINT_FLAG_ASSEMBLY) {
        if (fprintf(out, ".org %s%0*x\n", AVR_PREFIX_ABSOLUTE_ADDRESS, AVR_ADDRESS_WIDTH, instr->address) < 0)
            return -1;
    }
    return 0;
}

int avr_instruction_print(struct instruction *instr, FILE *out, int flags) {
    struct avrInstructionDisasm *instrDisasm = (struct avrInstructionDisasm *)instr->instructionDisasm;
    int i;

    /* Print an address label if we're outputting assembly */
    if (flags & PRINT_FLAG_ASSEMBLY) {
        if (fprintf(out, "%s%0*x:\t", AVR_PREFIX_ADDRESS_LABEL, AVR_ADDRESS_WIDTH, instr->address) < 0)
            return -1;

    /* Print address */
    } else if (flags & PRINT_FLAG_ADDRESSES) {
        if (fprintf(out, "%*x:\t", AVR_ADDRESS_WIDTH, instr->address) < 0)
            return -1;
    }

    /* Print original opcodes */
    if (flags & PRINT_FLAG_OPCODES) {
        if (instrDisasm->instructionInfo->width == 1) {
            if (fprintf(out, "%02x         \t", instrDisasm->opcode[0]) < 0)
                return -1;
        } else if (instrDisasm->instructionInfo->width == 2) {
            if (fprintf(out, "%02x %02x      \t", instrDisasm->opcode[1], instrDisasm->opcode[0]) < 0)
                return -1;
        } else if (instrDisasm->instructionInfo->width == 4) {
            if (fprintf(out, "%02x %02x %02x %02x\t", instrDisasm->opcode[3], instrDisasm->opcode[2], instrDisasm->opcode[1], instrDisasm->opcode[0]) < 0)
                return -1;
        }
    }

    /* Print mnemonic */
    if (fprintf(out, "%s\t", instrDisasm->instructionInfo->mnemonic) < 0)
        return -1;

    /* Print operands */
    for (i = 0; i < instrDisasm->instructionInfo->numOperands; i++) {
        /* Print dat comma, yea */
        if (i > 0 && i < instrDisasm->instructionInfo->numOperands) {
            if (fprintf(out, ", ") < 0)
                return -1;
        }

        /* Print the operand */
        switch (instrDisasm->instructionInfo->operandTypes[i]) {
            case OPERAND_REGISTER:
            case OPERAND_REGISTER_STARTR16:
            case OPERAND_REGISTER_EVEN_PAIR:
            case OPERAND_REGISTER_EVEN_PAIR_STARTR24:
                if (fprintf(out, "%s%d", AVR_PREFIX_REGISTER, instrDisasm->operandDisasms[i]) < 0) return -1;
                break;
            case OPERAND_IO_REGISTER:
                if (fprintf(out, "%s%02x", AVR_PREFIX_IO_REGISTER, instrDisasm->operandDisasms[i]) < 0) return -1;
                break;
            case OPERAND_BIT:
                if (fprintf(out, "%s%d", AVR_PREFIX_BIT, instrDisasm->operandDisasms[i]) < 0) return -1;
                break;
            case OPERAND_DES_ROUND:
                if (fprintf(out, "%s%d", AVR_PREFIX_DES_ROUND, instrDisasm->operandDisasms[i]) < 0) return -1;
                break;
            case OPERAND_RAW_WORD:
                if (fprintf(out, "%s%04x", AVR_PREFIX_RAW_WORD, instrDisasm->operandDisasms[i]) < 0) return -1;
                break;
            case OPERAND_RAW_BYTE:
                if (fprintf(out, "%s%02x", AVR_PREFIX_RAW_BYTE, instrDisasm->operandDisasms[i]) < 0) return -1;
                break;
            case OPERAND_X:
                if (fprintf(out, "X") < 0) return -1;
                break;
            case OPERAND_XP:
                if (fprintf(out, "X+") < 0) return -1;
                break;
            case OPERAND_MX:
                if (fprintf(out, "-X") < 0) return -1;
                break;
            case OPERAND_Y:
                if (fprintf(out, "Y") < 0) return -1;
                break;
            case OPERAND_YP:
                if (fprintf(out, "Y+") < 0) return -1;
                break;
            case OPERAND_MY:
                if (fprintf(out, "-Y") < 0) return -1;
                break;
            case OPERAND_Z:
                if (fprintf(out, "Z") < 0) return -1;
                break;
            case OPERAND_ZP:
                if (fprintf(out, "Z+") < 0) return -1;
                break;
            case OPERAND_MZ:
                if (fprintf(out, "-Z") < 0) return -1;
                break;
            case OPERAND_YPQ:
                if (fprintf(out, "Y+%d", instrDisasm->operandDisasms[i]) < 0) return -1;
                break;
            case OPERAND_ZPQ:
                if (fprintf(out, "Z+%d", instrDisasm->operandDisasms[i]) < 0) return -1;
                break;
            case OPERAND_DATA:
                if (flags & PRINT_FLAG_DATA_BIN) {
                    /* Data representation binary */
                    char binary[9];
                    int i;
                    for (i = 0; i < 8; i++) {
                        if (instrDisasm->operandDisasms[i] & (1 << i))
                            binary[7-i] = '1';
                        else
                            binary[7-i] = '0';
                    }
                    binary[8] = '\0';
                    if (fprintf(out, "%s%s", AVR_PREFIX_DATA_BIN, binary) < 0) return -1;
                } else if (flags & PRINT_FLAG_DATA_DEC) {
                    /* Data representation decimal */
                    if (fprintf(out, "%s%d", AVR_PREFIX_DATA_DEC, instrDisasm->operandDisasms[i]) < 0) return -1;
                } else {
                    /* Default to data representation hex */
                    if (fprintf(out, "%s%02x", AVR_PREFIX_DATA_HEX, instrDisasm->operandDisasms[i]) < 0) return -1;
                }
                break;
            case OPERAND_LONG_ABSOLUTE_ADDRESS:
                /* Divide the address by two to render a word address */
                if (fprintf(out, "%s%0*x", AVR_PREFIX_ABSOLUTE_ADDRESS, AVR_ADDRESS_WIDTH, instrDisasm->operandDisasms[i] / 2) < 0) return -1;
                break;
            case OPERAND_BRANCH_ADDRESS:
            case OPERAND_RELATIVE_ADDRESS:
                /* If we have address labels turned on, replace the relative
                 * address with the appropriate address label */
                if (flags & PRINT_FLAG_ASSEMBLY) {
                    if (fprintf(out, "%s%0*x", AVR_PREFIX_ADDRESS_LABEL, AVR_ADDRESS_WIDTH, instrDisasm->operandDisasms[i] + instrDisasm->address + 2) < 0) return -1;
                } else {
                    /* Print a plus sign for positive relative addresses, printf
                     * will insert a minus sign for negative relative addresses. */
                    if (instrDisasm->operandDisasms[i] >= 0) {
                        if (fprintf(out, "%s+%d", AVR_PREFIX_RELATIVE_ADDRESS, instrDisasm->operandDisasms[i]) < 0) return -1;
                    } else {
                        if (fprintf(out, "%s%d", AVR_PREFIX_RELATIVE_ADDRESS, instrDisasm->operandDisasms[i]) < 0) return -1;
                    }
                }
                break;
            default:
                break;
        }
    }

    /* Print destination address comment */
    if (flags & PRINT_FLAG_DESTINATION_COMMENT) {
        for (i = 0; i < instrDisasm->instructionInfo->numOperands; i++) {
            if ( instrDisasm->instructionInfo->operandTypes[i] == OPERAND_BRANCH_ADDRESS ||
                 instrDisasm->instructionInfo->operandTypes[i] == OPERAND_RELATIVE_ADDRESS) {
                if (fprintf(out, "\t; %s%x", AVR_PREFIX_ABSOLUTE_ADDRESS, instrDisasm->operandDisasms[i] + instrDisasm->address + 2) < 0)
                    return -1;
            }
        }
    }

    return 0;
}

