#ifndef INSTRUCTION_H
#define INSTRUCTION_H

struct instruction {
    uint32_t address;
    unsigned int width;
    void *instructionDisasm;
    int (*print_origin)(struct instruction *, FILE *, int flags);
    int (*print)(struct instruction *, FILE *, int flags);
};

#endif
