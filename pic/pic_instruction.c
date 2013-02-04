#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <instruction.h>
#include <print_stream_file.h>

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

uint32_t pic_instruction_get_address(struct instruction *instr);
unsigned int pic_instruction_get_width(struct instruction *instr);
unsigned int pic_instruction_get_num_operands(struct instruction *instr);
void pic_instruction_get_opcodes(struct instruction *instr, uint8_t *dest);
int pic_instruction_get_str_origin(struct instruction *instr, char *dest, int size, int flags);
int pic_instruction_get_str_address_label(struct instruction *instr, char *dest, int size, int flags);
int pic_instruction_get_str_address(struct instruction *instr, char *dest, int size, int flags);
int pic_instruction_get_str_opcodes(struct instruction *instr, char *dest, int size, int flags);
int pic_instruction_get_str_mnemonic(struct instruction *instr, char *dest, int size, int flags);
int pic_instruction_get_str_operand(struct instruction *instr, char *dest, int size, int index, int flags);
int pic_instruction_get_str_comment(struct instruction *instr, char *dest, int size, int flags);
void pic_instruction_free(struct instruction *instr);


uint32_t pic_instruction_get_address(struct instruction *instr) {
    struct picInstructionDisasm *instructionDisasm = (struct picInstructionDisasm *)instr->instructionDisasm;
    return instructionDisasm->address;
}

unsigned int pic_instruction_get_width(struct instruction *instr) {
    struct picInstructionDisasm *instructionDisasm = (struct picInstructionDisasm *)instr->instructionDisasm;
    return instructionDisasm->instructionInfo->width;
}

unsigned int pic_instruction_get_num_operands(struct instruction *instr) {
    struct picInstructionDisasm *instructionDisasm = (struct picInstructionDisasm *)instr->instructionDisasm;
    return instructionDisasm->instructionInfo->numOperands;
}

void pic_instruction_get_opcodes(struct instruction *instr, uint8_t *dest) {
    struct picInstructionDisasm *instructionDisasm = (struct picInstructionDisasm *)instr->instructionDisasm;
    int i;

    for (i = 0; i < instructionDisasm->instructionInfo->width; i++)
        *dest++ = instructionDisasm->opcode[i];
}

int pic_instruction_get_str_origin(struct instruction *instr, char *dest, int size, int flags) {
    struct picInstructionDisasm *instructionDisasm = (struct picInstructionDisasm *)instr->instructionDisasm;
    return snprintf(dest, size, "org %s%0*x", PIC_PREFIX_ABSOLUTE_ADDRESS, PIC_ADDRESS_WIDTH, instructionDisasm->address);
}

int pic_instruction_get_str_address_label(struct instruction *instr, char *dest, int size, int flags) {
    struct picInstructionDisasm *instructionDisasm = (struct picInstructionDisasm *)instr->instructionDisasm;
    return snprintf(dest, size, "%s%0*x:", PIC_PREFIX_ADDRESS_LABEL, PIC_ADDRESS_WIDTH, instructionDisasm->address);
}

int pic_instruction_get_str_address(struct instruction *instr, char *dest, int size, int flags) {
    struct picInstructionDisasm *instructionDisasm = (struct picInstructionDisasm *)instr->instructionDisasm;
    return snprintf(dest, size, "%*x:", PIC_ADDRESS_WIDTH, instructionDisasm->address);
}

int pic_instruction_get_str_opcodes(struct instruction *instr, char *dest, int size, int flags) {
    struct picInstructionDisasm *instructionDisasm = (struct picInstructionDisasm *)instr->instructionDisasm;

    if (instructionDisasm->instructionInfo->width == 1)
        return snprintf(dest, size, "%02x         ", instructionDisasm->opcode[0]);
    else if (instructionDisasm->instructionInfo->width == 2)
        return snprintf(dest, size, "%02x %02x      ", instructionDisasm->opcode[1], instructionDisasm->opcode[0]);

    return snprintf(dest, size, "");
}

int pic_instruction_get_str_mnemonic(struct instruction *instr, char *dest, int size, int flags) {
    struct picInstructionDisasm *instructionDisasm = (struct picInstructionDisasm *)instr->instructionDisasm;
    return snprintf(dest, size, "%s", instructionDisasm->instructionInfo->mnemonic);
}

int pic_instruction_get_str_operand(struct instruction *instr, char *dest, int size, int index, int flags) {
    struct picInstructionDisasm *instructionDisasm = (struct picInstructionDisasm *)instr->instructionDisasm;

    if (index < 0 || index > instructionDisasm->instructionInfo->numOperands - 1)
        return snprintf(dest, size, "");

    /* Print the operand */
    switch (instructionDisasm->instructionInfo->operandTypes[index]) {
        case OPERAND_REGISTER:
            return snprintf(dest, size, "%s%d", PIC_PREFIX_REGISTER, instructionDisasm->operandDisasms[index]);
            break;
        case OPERAND_REGISTER_DEST:
            if (instructionDisasm->operandDisasms[index] == 0) {
                return snprintf(dest, size, "W");
            } else {
                return snprintf(dest, size, "F");
            }
            break;
        case OPERAND_BIT:
            return snprintf(dest, size, "%s%d", PIC_PREFIX_BIT, instructionDisasm->operandDisasms[index]);
            break;
        case OPERAND_RAW_WORD:
            return snprintf(dest, size, "%s%04x", PIC_PREFIX_RAW_WORD, instructionDisasm->operandDisasms[index]);
            break;
        case OPERAND_RAW_BYTE:
            return snprintf(dest, size, "%s%02x", PIC_PREFIX_RAW_BYTE, instructionDisasm->operandDisasms[index]);
            break;
        case OPERAND_ABSOLUTE_ADDRESS:
            /* If we have address labels turned on, replace the relative
             * address with the appropriate address label */
            if (flags & PRINT_FLAG_ASSEMBLY) {
                return snprintf(dest, size, "%s%0*x", PIC_PREFIX_ADDRESS_LABEL, PIC_ADDRESS_WIDTH, instructionDisasm->operandDisasms[index]);
            } else {
                return snprintf(dest, size, "%s%0*x", PIC_PREFIX_ABSOLUTE_ADDRESS, PIC_ADDRESS_WIDTH, instructionDisasm->operandDisasms[index]);
            }
            break;
        case OPERAND_LITERAL:
            if (flags & PRINT_FLAG_DATA_BIN) {
                /* Data representation binary */
                char binary[9];
                int i;
                for (i = 0; i < 8; i++) {
                    if (instructionDisasm->operandDisasms[index] & (1 << i))
                        binary[7-i] = '1';
                    else
                        binary[7-i] = '0';
                }
                binary[8] = '\0';
                return snprintf(dest, size, "%s%s%s", PIC_PREFIX_DATA_BIN, binary, PIC_SUFFIX_DATA_BIN);
            } else if (flags & PRINT_FLAG_DATA_DEC) {
                /* Data representation decimal */
                return snprintf(dest, size, "%s%d", PIC_PREFIX_DATA_DEC, instructionDisasm->operandDisasms[index]);
            } else {
                /* Default to data representation hex */
                return snprintf(dest, size, "%s%02x", PIC_PREFIX_DATA_HEX, instructionDisasm->operandDisasms[index]);
            }
            break;
        /* Mid-range Enhanced Operands */
        case OPERAND_RELATIVE_ADDRESS:
            /* If we have address labels turned on, replace the relative
             * address with the appropriate address label */
            if (flags & PRINT_FLAG_ASSEMBLY) {
                return snprintf(dest, size, "%s%0*x", PIC_PREFIX_ADDRESS_LABEL, PIC_ADDRESS_WIDTH, instructionDisasm->operandDisasms[index] + instructionDisasm->address + 2);
            } else {
                /* Print a plus sign for positive relative addresses, printf
                 * will insert a minus sign for negative relative addresses. */
                if (instructionDisasm->operandDisasms[index] >= 0) {
                    return snprintf(dest, size, "%s+%d", PIC_PREFIX_RELATIVE_ADDRESS, instructionDisasm->operandDisasms[index]);
                } else {
                    return snprintf(dest, size, "%s%d", PIC_PREFIX_RELATIVE_ADDRESS, instructionDisasm->operandDisasms[index]);
                }
            }
            break;
        case OPERAND_FSR_INDEX:
            return snprintf(dest, size, "%d", instructionDisasm->operandDisasms[index]);
            break;

        case OPERAND_INDF_INDEX:
        case OPERAND_SIGNED_LITERAL:

        default:
            break;
    }

    return snprintf(dest, size, "");
}

int pic_instruction_get_str_comment(struct instruction *instr, char *dest, int size, int flags) {
    struct picInstructionDisasm *instructionDisasm = (struct picInstructionDisasm *)instr->instructionDisasm;
    int i;

    /* Print destination address comment */
    for (i = 0; i < instructionDisasm->instructionInfo->numOperands; i++) {
        if (instructionDisasm->instructionInfo->operandTypes[i] == OPERAND_RELATIVE_ADDRESS)
            return snprintf(dest, size, "; %s%x", PIC_PREFIX_ABSOLUTE_ADDRESS, instructionDisasm->operandDisasms[i] + instructionDisasm->address + 2);
    }

    return snprintf(dest, size, "");
}

void pic_instruction_free(struct instruction *instr) {
    free(instr->instructionDisasm);
    instr->instructionDisasm = NULL;
}

