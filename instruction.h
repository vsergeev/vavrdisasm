#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <stdint.h>

struct instruction {
    void *instructionDisasm;

    /* Get the address */
    uint32_t (*get_address)(struct instruction *);
    /* Get the width in bytes */
    unsigned int (*get_width)(struct instruction *);
    /* Get the number of operands */
    unsigned int (*get_num_operands)(struct instruction *);
    /* Get the raw opcodes, stored in dest, get_width() number of bytes */
    void (*get_opcodes)(struct instruction *, uint8_t *dest);


    /* Get a formatted string, stored in dest of max size */

    /* Formatted origin directive for this instruction */
    int (*get_str_origin)(struct instruction *, char *dest, int size, int flags);
    /* Formatted address label */
    int (*get_str_address_label)(struct instruction *, char *dest, int size, int flags);
    /* Formatted address */
    int (*get_str_address)(struct instruction *, char *dest, int size, int flags);
    /* Formatted opcodes */
    int (*get_str_opcodes)(struct instruction *, char *dest, int size, int flags);
    /* Formattted mnemonic */
    int (*get_str_mnemonic)(struct instruction *, char *dest, int size, int flags);
    /* Formatted operand of index index */
    int (*get_str_operand)(struct instruction *, char *dest, int size, int index, int flags);
    /* Formatted comment (could be a destination address */
    int (*get_str_comment)(struct instruction *, char *dest, int size, int flags);

    /* Free the allocated disassembled instruction */
    void (*free)(struct instruction *);
};

#endif
