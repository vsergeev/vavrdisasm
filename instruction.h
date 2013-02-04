#ifndef INSTRUCTION_H
#define INSTRUCTION_H

struct instruction {
    uint32_t address;
    unsigned int width;
    void *instructionDisasm;
    int (*print_origin)(struct instruction *, FILE *, int flags);
    int (*print_address)(struct instruction *, FILE *, int flags);
    int (*print_opcodes)(struct instruction *, FILE *, int flags);
    int (*print_mnemonic)(struct instruction *, FILE *, int flags);
    int (*print_operands)(struct instruction *, FILE *, int flags);
    int (*print_comment)(struct instruction *, FILE *, int flags);
};

#endif
