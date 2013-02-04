#include <stdint.h>
#include <stdio.h>

#include <print_stream.h>
#include <instruction.h>

#include "pic_instruction_set.h"

/* PIC format prefixes */
#define PIC_PREFIX_REGISTER             "0x"    /* clrf 0x25 */
#define PIC_PREFIX_BIT                  ""      /* bsf 0x32, 0 */
#define PIC_PREFIX_DATA_HEX             "0x"    /* movlw 0x6 */
#define PIC_PREFIX_DATA_BIN             "b'"    /* movlw b'00000110' */
#define PIC_SUFFIX_DATA_BIN             "'"
#define PIC_PREFIX_DATA_DEC             ""      /* movlw 6 */
#define PIC_PREFIX_ABSOLUTE_ADDRESS     "0x"    /* call 0xb6 */
#define PIC_PREFIX_RELATIVE_ADDRESS     "."
#define PIC_PREFIX_INDF_INDEX           "INDF"  /* INDF0, INDF1, ... */
#define PIC_PREFIX_RAW_WORD             "0x"    /* data 0xabcd */
#define PIC_PREFIX_RAW_BYTE             "0x"    /* .db 0xab */
#define PIC_PREFIX_ADDRESS_LABEL        "A_"    /* A_0004: */

/* Address filed width, e.g. 4 -> 0x0004 */
#define PIC_ADDRESS_WIDTH               4

int pic_instruction_print_origin(struct instruction *instr, FILE *out, int flags) {
    /* Print an origin directive if we're outputting assembly */
    if (flags & PRINT_FLAG_ASSEMBLY) {
        if (fprintf(out, "org %s%0*x\n", PIC_PREFIX_ABSOLUTE_ADDRESS, PIC_ADDRESS_WIDTH, instr->address) < 0)
            return -1;
    }
    return 0;
}

int pic_instruction_print(struct instruction *instr, FILE *out, int flags) {
    struct picInstructionDisasm *instrDisasm = (struct picInstructionDisasm *)instr->instructionDisasm;
    int i;

    /* Print an address label if we're outputting assembly */
    if (flags & PRINT_FLAG_ASSEMBLY) {
        if (fprintf(out, "%s%0*x:\t", PIC_PREFIX_ADDRESS_LABEL, PIC_ADDRESS_WIDTH, instr->address) < 0)
            return -1;

    /* Print address */
    } else if (flags & PRINT_FLAG_ADDRESSES) {
        if (fprintf(out, "%*x:\t", PIC_ADDRESS_WIDTH, instr->address) < 0)
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
                if (fprintf(out, "%s%d", PIC_PREFIX_REGISTER, instrDisasm->operandDisasms[i]) < 0) return -1;
                break;
            case OPERAND_REGISTER_DEST:
                if (instrDisasm->operandDisasms[i] == 0) {
                    if (fprintf(out, "W") < 0) return -1;
                } else {
                    if (fprintf(out, "F") < 0) return -1;
                }
                break;
            case OPERAND_BIT:
                if (fprintf(out, "%s%d", PIC_PREFIX_BIT, instrDisasm->operandDisasms[i]) < 0) return -1;
                break;
            case OPERAND_RAW_WORD:
                if (fprintf(out, "%s%04x", PIC_PREFIX_RAW_WORD, instrDisasm->operandDisasms[i]) < 0) return -1;
                break;
            case OPERAND_RAW_BYTE:
                if (fprintf(out, "%s%02x", PIC_PREFIX_RAW_BYTE, instrDisasm->operandDisasms[i]) < 0) return -1;
                break;
            case OPERAND_ABSOLUTE_ADDRESS:
                /* If we have address labels turned on, replace the relative
                 * address with the appropriate address label */
                if (flags & PRINT_FLAG_ASSEMBLY) {
                    if (fprintf(out, "%s%0*x", PIC_PREFIX_ADDRESS_LABEL, PIC_ADDRESS_WIDTH, instrDisasm->operandDisasms[i]) < 0) return -1;
                } else {
                    if (fprintf(out, "%s%0*x", PIC_PREFIX_ABSOLUTE_ADDRESS, PIC_ADDRESS_WIDTH, instrDisasm->operandDisasms[i]) < 0) return -1;
                }
                break;
            case OPERAND_LITERAL:
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
                    if (fprintf(out, "%s%s%s", PIC_PREFIX_DATA_BIN, binary, PIC_SUFFIX_DATA_BIN) < 0) return -1;
                } else if (flags & PRINT_FLAG_DATA_DEC) {
                    /* Data representation decimal */
                    if (fprintf(out, "%s%d", PIC_PREFIX_DATA_DEC, instrDisasm->operandDisasms[i]) < 0) return -1;
                } else {
                    /* Default to data representation hex */
                    if (fprintf(out, "%s%02x", PIC_PREFIX_DATA_HEX, instrDisasm->operandDisasms[i]) < 0) return -1;
                }
                break;
		    /* Mid-range Enhanced Operands */
            case OPERAND_RELATIVE_ADDRESS:
                /* If we have address labels turned on, replace the relative
                 * address with the appropriate address label */
                if (flags & PRINT_FLAG_ASSEMBLY) {
                    if (fprintf(out, "%s%0*x", PIC_PREFIX_ADDRESS_LABEL, PIC_ADDRESS_WIDTH, instrDisasm->operandDisasms[i] + instrDisasm->address + 2) < 0) return -1;
                } else {
                    /* Print a plus sign for positive relative addresses, printf
                     * will insert a minus sign for negative relative addresses. */
                    if (instrDisasm->operandDisasms[i] >= 0) {
                        if (fprintf(out, "%s+%d", PIC_PREFIX_RELATIVE_ADDRESS, instrDisasm->operandDisasms[i]) < 0) return -1;
                    } else {
                        if (fprintf(out, "%s%d", PIC_PREFIX_RELATIVE_ADDRESS, instrDisasm->operandDisasms[i]) < 0) return -1;
                    }
                }
                break;
            case OPERAND_FSR_INDEX:
                if (fprintf(out, "%d", instrDisasm->operandDisasms[i]) < 0) return -1;
                break;

            case OPERAND_INDF_INDEX:
            case OPERAND_SIGNED_LITERAL:

            default:
                break;
        }
    }

    /* Print destination address comment */
    if (flags & PRINT_FLAG_DESTINATION_COMMENT) {
        for (i = 0; i < instrDisasm->instructionInfo->numOperands; i++) {
            if (instrDisasm->instructionInfo->operandTypes[i] == OPERAND_RELATIVE_ADDRESS) {
                if (fprintf(out, "\t; %s%x", PIC_PREFIX_ABSOLUTE_ADDRESS, instrDisasm->operandDisasms[i] + instrDisasm->address + 2) < 0)
                    return -1;
            }
        }
    }

    return 0;
}

