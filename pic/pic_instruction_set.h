#ifndef PIC_INSTRUCTION_SET_H
#define PIC_INSTRUCTION_SET_H

#include <stdint.h>

/* Indicies for the .DW and .DB raw word / byte "instructions" */
#define PIC_ISET_INDEX_WORD(arch)     (PIC_TOTAL_INSTRUCTIONS[arch]-2)
#define PIC_ISET_INDEX_BYTE(arch)     (PIC_TOTAL_INSTRUCTIONS[arch]-1)

/* Enumeration for all types of PIC Operands */
enum {
    OPERAND_NONE,
    OPERAND_REGISTER,
    OPERAND_REGISTER_DEST,
    OPERAND_BIT,
    OPERAND_LITERAL,
    OPERAND_ABSOLUTE_ADDRESS,
    OPERAND_RAW_WORD,
    OPERAND_RAW_BYTE,
    /* Midrange Enhanced operands */
    OPERAND_RELATIVE_ADDRESS,
    OPERAND_SIGNED_LITERAL,
    OPERAND_FSR_INDEX,
    OPERAND_INCREMENT_MODE,
    OPERAND_INDF_INDEX
};

/* Enumeration for supported PIC sub-architectures */
enum {
    PIC_ARCH_BASELINE,
    PIC_ARCH_MIDRANGE,
    PIC_ARCH_MIDRANGE_ENHANCED,
    PIC_ARCH_PIC18,
};

/* Structure for each instruction entry in the instruction set */
struct picInstructionInfo {
    char mnemonic[7];
    unsigned int width;
    uint16_t instructionMask;
    int numOperands;
    uint16_t operandMasks[3];
    int operandTypes[3];
};

/* Structure for a disassembled instruction */
struct picInstructionDisasm {
    uint32_t address;
    uint8_t opcode[2];
    struct picInstructionInfo *instructionInfo;
    int32_t operandDisasms[3];
};

extern struct picInstructionInfo *PIC_Instruction_Sets[];
extern int PIC_TOTAL_INSTRUCTIONS[];

#endif

