#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "avr_instruction_set.h"
#include <print_stream.h>
#include "avr.h"

/* AVR Print Stream Prefix String Indices */
enum {
    AVR_PRINT_PREFIX_REGISTER,
    AVR_PRINT_PREFIX_IO_REGISTER,
    AVR_PRINT_PREFIX_DATA_HEX,
    AVR_PRINT_PREFIX_DATA_BIN,
    AVR_PRINT_PREFIX_DATA_DEC,
    AVR_PRINT_PREFIX_BIT,
    AVR_PRINT_PREFIX_ABSOLUTE_ADDRESS,
    AVR_PRINT_PREFIX_RELATIVE_ADDRESS,
    AVR_PRINT_PREFIX_DES_ROUND,
    AVR_PRINT_PREFIX_RAW_WORD,
    AVR_PRINT_PREFIX_RAW_BYTE,
};

/* AVRASM format prefixes */
char *avr_format_prefixes_avrasm[] = {
    [AVR_PRINT_PREFIX_REGISTER]            = "R",  /* mov R0, R2 */
    [AVR_PRINT_PREFIX_IO_REGISTER]         = "$",  /* out $39, R16 */
    [AVR_PRINT_PREFIX_DATA_HEX]            = "0x", /* ldi R16, 0x3d */
    [AVR_PRINT_PREFIX_DATA_BIN]            = "0b", /* ldi R16, 0b00111101 */
    [AVR_PRINT_PREFIX_DATA_DEC]            = "",   /* ldi R16, 61 */
    [AVR_PRINT_PREFIX_BIT]                 = "",   /* cbi $12, 7 */
    [AVR_PRINT_PREFIX_ABSOLUTE_ADDRESS]    = "0x", /* call 0x1234 */
    [AVR_PRINT_PREFIX_RELATIVE_ADDRESS]    = ".",  /* rjmp .4 */
    [AVR_PRINT_PREFIX_DES_ROUND]           = "0x", /* des 0x01 */
    [AVR_PRINT_PREFIX_RAW_WORD]            = "0x", /* .dw 0xabcd */
    [AVR_PRINT_PREFIX_RAW_BYTE]            = "0x", /* .db 0xab */
    };
/* Objdump format prefixes */
char *avr_format_prefixes_objdump[] = {
    [AVR_PRINT_PREFIX_REGISTER]            = "r",  /* mov r0, r2 */
    [AVR_PRINT_PREFIX_IO_REGISTER]         = "0x", /* out 0x39, r16 */
    [AVR_PRINT_PREFIX_DATA_HEX]            = "0x", /* ldi r16, 0x3d */
    [AVR_PRINT_PREFIX_DATA_BIN]            = "0b", /* ldi r16, 0b00111101 */
    [AVR_PRINT_PREFIX_DATA_DEC]            = "",   /* ldi r16, 61 */
    [AVR_PRINT_PREFIX_BIT]                 = "",   /* cbi 0x12, 7 */
    [AVR_PRINT_PREFIX_ABSOLUTE_ADDRESS]    = "0x", /* call 0x1234 */
    [AVR_PRINT_PREFIX_RELATIVE_ADDRESS]    = ".",  /* rjmp .4 */
    [AVR_PRINT_PREFIX_DES_ROUND]           = "",   /* des 1 */
    [AVR_PRINT_PREFIX_RAW_WORD]            = "0x", /* .dw 0xabcd */
    [AVR_PRINT_PREFIX_RAW_BYTE]            = "0x", /* .db 0xab */
    };

/* Default prefixes are AVRASM style */
char **avr_format_prefixes = avr_format_prefixes_avrasm;

/* Default address field width, e.g. 4 -> 0x0004 */
int avr_address_width = 4;

/******************************************************************************/
/* AVR Print Stream Support */
/******************************************************************************/

int print_stream_avr_init(struct PrintStream *self) {
    /* Allocate stream state */
    self->state = malloc(sizeof(struct print_stream_avr_state));
    if (self->state == NULL) {
        self->error = "Error allocating format stream state!";
        return STREAM_ERROR_ALLOC;
    }

    /* Initialize stream state */
    memset(self->state, 0, sizeof(struct print_stream_avr_state));

    /* Reset the error to NULL */
    self->error = NULL;

    /* Initialize the input stream */
    if (self->in->stream_init(self->in) < 0) {
        self->error = "Error in input stream initialization!";
        return STREAM_ERROR_INPUT;
    }

    return 0;
}

int print_stream_avr_close(struct PrintStream *self) {
    /* Free stream state memory */
    free(self->state);

    /* Close input stream */
    if (self->in->stream_close(self->in) < 0) {
        self->error = "Error in input stream close!";
        return STREAM_ERROR_INPUT;
    }

    return 0;
}

static int util_avr_print_operand(FILE *out, struct avrInstructionDisasm *instrDisasm, int operandIndex, struct print_stream_avr_state *options) {
    switch (instrDisasm->instructionSetEntry->operandTypes[operandIndex]) {
        case OPERAND_REGISTER:
        case OPERAND_REGISTER_STARTR16:
        case OPERAND_REGISTER_EVEN_PAIR:
        case OPERAND_REGISTER_EVEN_PAIR_STARTR24:
            if (fprintf(out, "%s%d", avr_format_prefixes[AVR_PRINT_PREFIX_REGISTER], instrDisasm->operandDisasms[operandIndex]) < 0) return -1;
            break;
        case OPERAND_IO_REGISTER:
            if (fprintf(out, "%s%02x", avr_format_prefixes[AVR_PRINT_PREFIX_IO_REGISTER], instrDisasm->operandDisasms[operandIndex]) < 0) return -1;
            break;
        case OPERAND_BIT:
            if (fprintf(out, "%s%d", avr_format_prefixes[AVR_PRINT_PREFIX_BIT], instrDisasm->operandDisasms[operandIndex]) < 0) return -1;
            break;
        case OPERAND_DES_ROUND:
            if (fprintf(out, "%s%d", avr_format_prefixes[AVR_PRINT_PREFIX_DES_ROUND], instrDisasm->operandDisasms[operandIndex]) < 0) return -1;
            break;
        case OPERAND_RAW_WORD:
            if (fprintf(out, "%s%04x", avr_format_prefixes[AVR_PRINT_PREFIX_RAW_WORD], instrDisasm->operandDisasms[operandIndex]) < 0) return -1;
            break;
        case OPERAND_RAW_BYTE:
            if (fprintf(out, "%s%02x", avr_format_prefixes[AVR_PRINT_PREFIX_RAW_BYTE], instrDisasm->operandDisasms[operandIndex]) < 0) return -1;
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
            if (fprintf(out, "Y+%d", instrDisasm->operandDisasms[operandIndex]) < 0) return -1;
            break;
        case OPERAND_ZPQ:
            if (fprintf(out, "Z+%d", instrDisasm->operandDisasms[operandIndex]) < 0) return -1;
            break;
        case OPERAND_DATA:
            if (options->flags & AVR_PRINT_FLAG_DATA_BIN) {
                /* Data representation binary */
                char binary[9];
                int i;
                for (i = 0; i < 8; i++) {
                    if (instrDisasm->operandDisasms[operandIndex] & (1 << i))
                        binary[7-i] = '1';
                    else
                        binary[7-i] = '0';
                }
                binary[8] = '\0';
                if (fprintf(out, "%s%s", avr_format_prefixes[AVR_PRINT_PREFIX_DATA_BIN], binary) < 0) return -1;
            } else if (options->flags & AVR_PRINT_FLAG_DATA_DEC) {
                /* Data representation decimal */
                if (fprintf(out, "%s%d", avr_format_prefixes[AVR_PRINT_PREFIX_DATA_DEC], instrDisasm->operandDisasms[operandIndex]) < 0) return -1;
            } else {
                /* Default to data representation hex */
                if (fprintf(out, "%s%02x", avr_format_prefixes[AVR_PRINT_PREFIX_DATA_HEX], instrDisasm->operandDisasms[operandIndex]) < 0) return -1;
            }
            break;
        case OPERAND_LONG_ABSOLUTE_ADDRESS:
            /* Divide the address by two to render a word address */
            if (fprintf(out, "%s%0*x", avr_format_prefixes[AVR_PRINT_PREFIX_ABSOLUTE_ADDRESS], avr_address_width, instrDisasm->operandDisasms[operandIndex] / 2) < 0) return -1;
            break;
        case OPERAND_BRANCH_ADDRESS:
        case OPERAND_RELATIVE_ADDRESS:
            /* If we have address labels turned on, replace the relative
             * address with the appropriate address label */
            if (options->flags & AVR_PRINT_FLAG_ADDRESS_LABELS) {
                if (fprintf(out, "%s%0*x", options->address_label_prefix, avr_address_width, instrDisasm->operandDisasms[operandIndex] + instrDisasm->address + 2) < 0) return -1;
            } else {
                /* Print a plus sign for positive relative addresses, printf
                 * will insert a minus sign for negative relative addresses. */
                if (instrDisasm->operandDisasms[operandIndex] >= 0) {
                    if (fprintf(out, "%s+%d", avr_format_prefixes[AVR_PRINT_PREFIX_RELATIVE_ADDRESS], instrDisasm->operandDisasms[operandIndex]) < 0) return -1;
                } else {
                    if (fprintf(out, "%s%d", avr_format_prefixes[AVR_PRINT_PREFIX_RELATIVE_ADDRESS], instrDisasm->operandDisasms[operandIndex]) < 0) return -1;
                }
            }
            break;
        default:
            break;
    }
    return 0;
}

int print_stream_avr_read(struct PrintStream *self, FILE *out) {
    struct print_stream_avr_state *options = (struct print_stream_avr_state *)self->state;
    struct avrInstructionDisasm instrDisasm;
    int ret, i;

    /* Read a disassembled instruction */
    ret = self->in->stream_read(self->in, (void *)&instrDisasm);
    switch (ret) {
        case 0:
            break;
        case STREAM_EOF:
            return STREAM_EOF;
        default:
            self->error = "Error in disasm stream read!";
            return STREAM_ERROR_INPUT;
    }

    /* If we're printing address labels (implying we want to output
     * assemble-able code) and we've jumped in addresses, drop an .org
     * directive */
    if (options->flags & AVR_PRINT_FLAG_ADDRESS_LABELS) {
        /* If we haven't dropped an .org directive before */
        if (options->prev_instr_width == 0) {
            if (fprintf(out, ".org %s%0*x\n", avr_format_prefixes[AVR_PRINT_PREFIX_ABSOLUTE_ADDRESS], avr_address_width, instrDisasm.address) < 0)
                goto fprintf_error;
            options->prev_address = instrDisasm.address;
            options->prev_instr_width = instrDisasm.width;
        } else {
            /* Drop an .org directive if the address changed beyond previous
             * instruction width */
            if (instrDisasm.address - options->prev_address > options->prev_instr_width) {
                if (fprintf(out, ".org %s%0*x\n", avr_format_prefixes[AVR_PRINT_PREFIX_ABSOLUTE_ADDRESS], avr_address_width, instrDisasm.address) < 0)
                    goto fprintf_error;
            }
            options->prev_address = instrDisasm.address;
            options->prev_instr_width = instrDisasm.width;
        }
    }

    /* Print an address label like "A_0010:" or a plain address like "0010:" */
    if (options->flags & AVR_PRINT_FLAG_ADDRESS_LABELS)
        ret = fprintf(out, "%s%0*x:\t", options->address_label_prefix, avr_address_width, instrDisasm.address);
    else if (options->flags & AVR_PRINT_FLAG_ADDRESSES)
        ret = fprintf(out, "%*x:\t", avr_address_width, instrDisasm.address);

    if (ret < 0)
        goto fprintf_error;

    /* Print original opcode, if we're not generating address labels */
    if (options->flags & AVR_PRINT_FLAG_OPCODES && !(options->flags & AVR_PRINT_FLAG_ADDRESS_LABELS)) {
        /* If this is an 8-bit data byte */
        if (instrDisasm.width == 1)
            ret = fprintf(out, "%02x         \t", instrDisasm.opcode[0]);

        /* If this is a 16-bit instruction */
        else if (instrDisasm.width == 2)
            ret = fprintf(out, "%02x %02x      \t", instrDisasm.opcode[1], instrDisasm.opcode[0]);

        /* If this is a 32-bit instruction */
        else if (instrDisasm.width == 4)
            ret = fprintf(out, "%02x %02x %02x %02x\t", instrDisasm.opcode[3], instrDisasm.opcode[2], instrDisasm.opcode[1], instrDisasm.opcode[0]);
    }

    if (ret < 0)
        goto fprintf_error;

    /* Print instruction mnemonic */
    if (fprintf(out, "%s\t", instrDisasm.instructionSetEntry->mnemonic) < 0)
        goto fprintf_error;

    /* Print instruction operands */
    for (i = 0; i < instrDisasm.instructionSetEntry->numOperands; i++) {
        /* Print dat comma, yea */
        if (i > 0 && i < instrDisasm.instructionSetEntry->numOperands) {
            if (fprintf(out, ", ") < 0)
                goto fprintf_error;
        }

        /* Print the operand */
        if (util_avr_print_operand(out, &instrDisasm, i, options) < 0)
            goto fprintf_error;
    }

    /* Print the destination address comment */
    if (options->flags & AVR_PRINT_FLAG_DEST_ADDR_COMMENT) {
        for (i = 0; i < instrDisasm.instructionSetEntry->numOperands; i++) {
            if ( instrDisasm.instructionSetEntry->operandTypes[i] == OPERAND_BRANCH_ADDRESS ||
                 instrDisasm.instructionSetEntry->operandTypes[i] == OPERAND_RELATIVE_ADDRESS) {
                if (fprintf(out, "\t; %s%x", avr_format_prefixes[AVR_PRINT_PREFIX_ABSOLUTE_ADDRESS], instrDisasm.operandDisasms[i] + instrDisasm.address + 2) < 0)
                    goto fprintf_error;
                break;
            }
        }
    }

    /* Print a newline */
    if (fprintf(out, "\n") < 0)
        goto fprintf_error;

    return 0;

    fprintf_error:
    self->error = "Error writing to output file!";
    return STREAM_ERROR_OUTPUT;
}

