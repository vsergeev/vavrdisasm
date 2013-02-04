#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <instruction.h>
#include <print_stream_file.h>

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

uint32_t avr_instruction_get_address(struct instruction *instr);
unsigned int avr_instruction_get_width(struct instruction *instr);
unsigned int avr_instruction_get_num_operands(struct instruction *instr);
void avr_instruction_get_opcodes(struct instruction *instr, uint8_t *dest);
int avr_instruction_get_str_origin(struct instruction *instr, char *dest, int size, int flags);
int avr_instruction_get_str_address_label(struct instruction *instr, char *dest, int size, int flags);
int avr_instruction_get_str_address(struct instruction *instr, char *dest, int size, int flags);
int avr_instruction_get_str_opcodes(struct instruction *instr, char *dest, int size, int flags);
int avr_instruction_get_str_mnemonic(struct instruction *instr, char *dest, int size, int flags);
int avr_instruction_get_str_operand(struct instruction *instr, char *dest, int size, int index, int flags);
int avr_instruction_get_str_comment(struct instruction *instr, char *dest, int size, int flags);
void avr_instruction_free(struct instruction *instr);


uint32_t avr_instruction_get_address(struct instruction *instr) {
    struct avrInstructionDisasm *instructionDisasm = (struct avrInstructionDisasm *)instr->instructionDisasm;
    return instructionDisasm->address;
}

unsigned int avr_instruction_get_width(struct instruction *instr) {
    struct avrInstructionDisasm *instructionDisasm = (struct avrInstructionDisasm *)instr->instructionDisasm;
    return instructionDisasm->instructionInfo->width;
}

unsigned int avr_instruction_get_num_operands(struct instruction *instr) {
    struct avrInstructionDisasm *instructionDisasm = (struct avrInstructionDisasm *)instr->instructionDisasm;
    return instructionDisasm->instructionInfo->numOperands;
}

void avr_instruction_get_opcodes(struct instruction *instr, uint8_t *dest) {
    struct avrInstructionDisasm *instructionDisasm = (struct avrInstructionDisasm *)instr->instructionDisasm;
    int i;

    for (i = 0; i < instructionDisasm->instructionInfo->width; i++)
        *dest++ = instructionDisasm->opcode[i];
}

int avr_instruction_get_str_origin(struct instruction *instr, char *dest, int size, int flags) {
    struct avrInstructionDisasm *instructionDisasm = (struct avrInstructionDisasm *)instr->instructionDisasm;
    return snprintf(dest, size, ".org %s%0*x", AVR_PREFIX_ABSOLUTE_ADDRESS, AVR_ADDRESS_WIDTH, instructionDisasm->address);
}

int avr_instruction_get_str_address_label(struct instruction *instr, char *dest, int size, int flags) {
    struct avrInstructionDisasm *instructionDisasm = (struct avrInstructionDisasm *)instr->instructionDisasm;
    return snprintf(dest, size, "%s%0*x:", AVR_PREFIX_ADDRESS_LABEL, AVR_ADDRESS_WIDTH, instructionDisasm->address);
}

int avr_instruction_get_str_address(struct instruction *instr, char *dest, int size, int flags) {
    struct avrInstructionDisasm *instructionDisasm = (struct avrInstructionDisasm *)instr->instructionDisasm;
    return snprintf(dest, size, "%*x:", AVR_ADDRESS_WIDTH, instructionDisasm->address);
}

int avr_instruction_get_str_opcodes(struct instruction *instr, char *dest, int size, int flags) {
    struct avrInstructionDisasm *instructionDisasm = (struct avrInstructionDisasm *)instr->instructionDisasm;

    if (instructionDisasm->instructionInfo->width == 1)
        return snprintf(dest, size, "%02x         ", instructionDisasm->opcode[0]);
    else if (instructionDisasm->instructionInfo->width == 2)
        return snprintf(dest, size, "%02x %02x      ", instructionDisasm->opcode[1], instructionDisasm->opcode[0]);
    else if (instructionDisasm->instructionInfo->width == 4)
        return snprintf(dest, size, "%02x %02x %02x %02x", instructionDisasm->opcode[3], instructionDisasm->opcode[2], instructionDisasm->opcode[1], instructionDisasm->opcode[0]);

    return snprintf(dest, size, "");
}

int avr_instruction_get_str_mnemonic(struct instruction *instr, char *dest, int size, int flags) {
    struct avrInstructionDisasm *instructionDisasm = (struct avrInstructionDisasm *)instr->instructionDisasm;
    return snprintf(dest, size, "%s", instructionDisasm->instructionInfo->mnemonic);
}

int avr_instruction_get_str_operand(struct instruction *instr, char *dest, int size, int index, int flags) {
    struct avrInstructionDisasm *instructionDisasm = (struct avrInstructionDisasm *)instr->instructionDisasm;
    int i;

    if (index < 0 || index > instructionDisasm->instructionInfo->numOperands - 1)
        return snprintf(dest, size, "");

    switch (instructionDisasm->instructionInfo->operandTypes[index]) {
        case OPERAND_REGISTER:
        case OPERAND_REGISTER_STARTR16:
        case OPERAND_REGISTER_EVEN_PAIR:
        case OPERAND_REGISTER_EVEN_PAIR_STARTR24:
            return snprintf(dest, size, "%s%d", AVR_PREFIX_REGISTER, instructionDisasm->operandDisasms[index]);
        case OPERAND_IO_REGISTER:
            return snprintf(dest, size, "%s%02x", AVR_PREFIX_IO_REGISTER, instructionDisasm->operandDisasms[index]);
        case OPERAND_BIT:
            return snprintf(dest, size, "%s%d", AVR_PREFIX_BIT, instructionDisasm->operandDisasms[index]);
        case OPERAND_DES_ROUND:
            return snprintf(dest, size, "%s%d", AVR_PREFIX_DES_ROUND, instructionDisasm->operandDisasms[index]);
        case OPERAND_RAW_WORD:
            return snprintf(dest, size, "%s%04x", AVR_PREFIX_RAW_WORD, instructionDisasm->operandDisasms[index]);
        case OPERAND_RAW_BYTE:
            return snprintf(dest, size, "%s%02x", AVR_PREFIX_RAW_BYTE, instructionDisasm->operandDisasms[index]);
        case OPERAND_X:
            return snprintf(dest, size, "X");
        case OPERAND_XP:
            return snprintf(dest, size, "X+");
        case OPERAND_MX:
            return snprintf(dest, size, "-X");
        case OPERAND_Y:
            return snprintf(dest, size, "Y");
        case OPERAND_YP:
            return snprintf(dest, size, "Y+");
        case OPERAND_MY:
            return snprintf(dest, size, "-Y");
        case OPERAND_Z:
            return snprintf(dest, size, "Z");
        case OPERAND_ZP:
            return snprintf(dest, size, "Z+");
        case OPERAND_MZ:
            return snprintf(dest, size, "-Z");
        case OPERAND_YPQ:
            return snprintf(dest, size, "Y+%d", instructionDisasm->operandDisasms[index]);
        case OPERAND_ZPQ:
            return snprintf(dest, size, "Z+%d", instructionDisasm->operandDisasms[index]);
        case OPERAND_DATA:
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
                return snprintf(dest, size, "%s%s", AVR_PREFIX_DATA_BIN, binary);
            } else if (flags & PRINT_FLAG_DATA_DEC) {
                /* Data representation decimal */
                return snprintf(dest, size, "%s%d", AVR_PREFIX_DATA_DEC, instructionDisasm->operandDisasms[index]);
            } else {
                /* Default to data representation hex */
                return snprintf(dest, size, "%s%02x", AVR_PREFIX_DATA_HEX, instructionDisasm->operandDisasms[index]);
            }
        case OPERAND_LONG_ABSOLUTE_ADDRESS:
            /* Divide the address by two to render a word address */
            return snprintf(dest, size, "%s%0*x", AVR_PREFIX_ABSOLUTE_ADDRESS, AVR_ADDRESS_WIDTH, instructionDisasm->operandDisasms[index] / 2);
        case OPERAND_BRANCH_ADDRESS:
        case OPERAND_RELATIVE_ADDRESS:
            /* If we have address labels turned on, replace the relative
             * address with the appropriate address label */
            if (flags & PRINT_FLAG_ASSEMBLY) {
                return snprintf(dest, size, "%s%0*x", AVR_PREFIX_ADDRESS_LABEL, AVR_ADDRESS_WIDTH, instructionDisasm->operandDisasms[index] + instructionDisasm->address + 2);
            } else {
                /* Print a plus sign for positive relative addresses, printf
                 * will insert a minus sign for negative relative addresses. */
                if (instructionDisasm->operandDisasms[index] >= 0) {
                    return snprintf(dest, size, "%s+%d", AVR_PREFIX_RELATIVE_ADDRESS, instructionDisasm->operandDisasms[index]);
                } else {
                    return snprintf(dest, size, "%s%d", AVR_PREFIX_RELATIVE_ADDRESS, instructionDisasm->operandDisasms[index]);
                }
            }
        default:
            break;
    }

    return snprintf(dest, size, "");
}

int avr_instruction_get_str_comment(struct instruction *instr, char *dest, int size, int flags) {
    struct avrInstructionDisasm *instructionDisasm = (struct avrInstructionDisasm *)instr->instructionDisasm;
    int i;

    for (i = 0; i < instructionDisasm->instructionInfo->numOperands; i++) {
        if ( instructionDisasm->instructionInfo->operandTypes[i] == OPERAND_BRANCH_ADDRESS ||
             instructionDisasm->instructionInfo->operandTypes[i] == OPERAND_RELATIVE_ADDRESS) {
            return snprintf(dest, size, "; %s%x", AVR_PREFIX_ABSOLUTE_ADDRESS, instructionDisasm->operandDisasms[i] + instructionDisasm->address + 2);
        }
    }

    return snprintf(dest, size, "");
}

void avr_instruction_free(struct instruction *instr) {
    free(instr->instructionDisasm);
    instr->instructionDisasm = NULL;
}

