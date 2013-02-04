#ifndef AVR_INSTRUCTION_SET_H
#define AVR_INSTRUCTION_SET_H

#include <stdint.h>

/* Indicies for the .DW and .DB raw word / byte "instructions" */
#define AVR_ISET_INDEX_WORD     (AVR_TOTAL_INSTRUCTIONS-2)
#define AVR_ISET_INDEX_BYTE     (AVR_TOTAL_INSTRUCTIONS-1)

/* Enumeration for all types of AVR Operands */
enum {
    OPERAND_NONE,
    OPERAND_REGISTER, OPERAND_REGISTER_STARTR16,
    OPERAND_REGISTER_EVEN_PAIR, OPERAND_REGISTER_EVEN_PAIR_STARTR24,
    OPERAND_BRANCH_ADDRESS, OPERAND_RELATIVE_ADDRESS, OPERAND_LONG_ABSOLUTE_ADDRESS,
    OPERAND_IO_REGISTER, OPERAND_DATA, OPERAND_DES_ROUND, OPERAND_BIT,
    OPERAND_X, OPERAND_XP, OPERAND_MX,
    OPERAND_Y, OPERAND_YP, OPERAND_MY, OPERAND_YPQ,
    OPERAND_Z, OPERAND_ZP, OPERAND_MZ, OPERAND_ZPQ,
    OPERAND_RAW_WORD, OPERAND_RAW_BYTE,
};

/* Structure for each entry in the instruction set */
struct avrInstructionInfo {
    char mnemonic[7];
    unsigned int width;
    uint16_t instructionMask;
    int numOperands;
    uint16_t operandMasks[2];
    int operandTypes[2];
};

/* Structure for a disassembled instruction */
struct avrInstructionDisasm {
    uint32_t address;
    uint8_t opcode[4];
    struct avrInstructionInfo *instructionInfo;
    int32_t operandDisasms[2];
};

extern struct avrInstructionInfo AVR_Instruction_Set[];
extern int AVR_TOTAL_INSTRUCTIONS;

#endif

