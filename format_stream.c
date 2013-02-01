#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "stream_error.h"
#include "format_stream.h"
#include "avr_instruction_set.h"

/******************************************************************************/
/* AVR Format Stream Support */
/******************************************************************************/

int format_stream_avr_init(struct FormatStream *self) {
    struct format_stream_avr_state *state;

    /* Allocate stream state */
    self->state = malloc(sizeof(struct format_stream_avr_state));
    if (self->state == NULL) {
        self->error = "Error allocating format stream state!";
        return STREAM_ERROR_ALLOC;
    }
    /* Initialize stream state */
    state = (struct format_stream_avr_state *)self->state;
    state->prefixes = NULL;
    state->address_width = 0;
    state->flags = 0;
    state->prev_address = 0;
    state->prev_instr_width = 0;

    /* Reset the error to NULL */
    self->error = NULL;

    /* Initialize the input stream */
    if (self->in->stream_init(self->in) < 0) {
        self->error = "Error in input stream initialization!";
        return STREAM_ERROR_INPUT;
    }

    return 0;
}

int format_stream_avr_close(struct FormatStream *self) {
    /* Free stream state memory */
    free(self->state);

    /* Close input stream */
    if (self->in->stream_close(self->in) < 0) {
        self->error = "Error in input stream close!";
        return STREAM_ERROR_INPUT;
    }

    return 0;
}

int util_format_print_operand(FILE *out, int32_t operandDisasm, int operandType, struct format_stream_avr_state *options) {
    switch (operandType) {
        case OPERAND_REGISTER:
        case OPERAND_REGISTER_STARTR16:
        case OPERAND_REGISTER_EVEN_PAIR:
        case OPERAND_REGISTER_EVEN_PAIR_STARTR24:
            if (fprintf(out, "%s%d", options->prefixes[AVR_FORMAT_PREFIX_REGISTER], operandDisasm) < 0) return -1;
            break;
        case OPERAND_IO_REGISTER:
            if (fprintf(out, "%s%02x", options->prefixes[AVR_FORMAT_PREFIX_IO_REGISTER], operandDisasm) < 0) return -1;
            break;
        case OPERAND_BIT:
            if (fprintf(out, "%s%d", options->prefixes[AVR_FORMAT_PREFIX_BIT], operandDisasm) < 0) return -1;
            break;
        case OPERAND_DES_ROUND:
            if (fprintf(out, "%s%d", options->prefixes[AVR_FORMAT_PREFIX_DES_ROUND], operandDisasm) < 0) return -1;
            break;
        case OPERAND_RAW_WORD:
            if (fprintf(out, "%s%04x", options->prefixes[AVR_FORMAT_PREFIX_RAW_WORD], operandDisasm) < 0) return -1;
            break;
        case OPERAND_RAW_BYTE:
            if (fprintf(out, "%s%02x", options->prefixes[AVR_FORMAT_PREFIX_RAW_BYTE], operandDisasm) < 0) return -1;
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
            if (fprintf(out, "Y+%d", operandDisasm) < 0) return -1;
            break;
        case OPERAND_ZPQ:
            if (fprintf(out, "Z+%d", operandDisasm) < 0) return -1;
            break;
        case OPERAND_DATA:
            if (options->flags & AVR_FORMAT_FLAG_DATA_BIN) {
                /* Data representation binary */
                char binary[9];
                int i;
                for (i = 0; i < 8; i++) {
                    if (operandDisasm & (1 << i))
                        binary[7-i] = '1';
                    else
                        binary[7-i] = '0';
                }
                binary[8] = '\0';
                if (fprintf(out, "%s%s", options->prefixes[AVR_FORMAT_PREFIX_DATA_BIN], binary) < 0) return -1;
            } else if (options->flags & AVR_FORMAT_FLAG_DATA_DEC) {
                /* Data representation decimal */
                if (fprintf(out, "%s%d", options->prefixes[AVR_FORMAT_PREFIX_DATA_DEC], operandDisasm) < 0) return -1;
            } else {
                /* Default to data representation hex */
                if (fprintf(out, "%s%02x", options->prefixes[AVR_FORMAT_PREFIX_DATA_HEX], operandDisasm) < 0) return -1;
            }
            break;
        case OPERAND_LONG_ABSOLUTE_ADDRESS:
            /* Divide the address by two to render a word address */
            if (fprintf(out, "%s%0*x", options->prefixes[AVR_FORMAT_PREFIX_ABSOLUTE_ADDRESS], options->address_width, operandDisasm / 2) < 0) return -1;
            break;
        case OPERAND_BRANCH_ADDRESS:
        case OPERAND_RELATIVE_ADDRESS:
            /* If we have address labels turned on, replace the relative
             * address with the appropriate address label */
            if (options->flags & AVR_FORMAT_FLAG_ADDRESS_LABELS) {
                if (fprintf(out, "%s%0*x", options->prefixes[AVR_FORMAT_PREFIX_ADDRESS_LABEL], options->address_width, operandDisasm) < 0) return -1;
            } else {
                /* Print a plus sign for positive relative addresses, printf
                 * will insert a minus sign for negative relative addresses. */
                if (operandDisasm >= 0) {
                    if (fprintf(out, "%s+%d", options->prefixes[AVR_FORMAT_PREFIX_RELATIVE_ADDRESS], operandDisasm) < 0) return -1;
                } else {
                    if (fprintf(out, "%s%d", options->prefixes[AVR_FORMAT_PREFIX_RELATIVE_ADDRESS], operandDisasm) < 0) return -1;
                }
            }
            break;
        default:
            break;
    }
    return 0;
}

int format_stream_avr_read(struct FormatStream *self, FILE *out) {
    struct format_stream_avr_state *options = (struct format_stream_avr_state *)self->state;
    struct avrInstructionDisasm instrDisasm;
    int ret, i;

    /* Check that our stream's options were configured */
    if (options->prefixes == NULL) {
        self->error = "Error in format options configuration!";
        return STREAM_ERROR_FAILURE;
    }

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
    if (options->flags & AVR_FORMAT_FLAG_ADDRESS_LABELS) {
        /* If we haven't dropped an .org directive before */
        if (options->prev_instr_width == 0) {
            if (fprintf(out, ".org %s%0*x\n", options->prefixes[AVR_FORMAT_PREFIX_ABSOLUTE_ADDRESS], options->address_width, instrDisasm.address) < 0)
                goto fprintf_error;
            options->prev_address = instrDisasm.address;
            options->prev_instr_width = instrDisasm.width;
        } else {
            /* Drop an .org directive if the address changed beyond previous
             * instruction width */
            if (instrDisasm.address - options->prev_address > options->prev_instr_width) {
                if (fprintf(out, ".org %s%0*x\n", options->prefixes[AVR_FORMAT_PREFIX_ABSOLUTE_ADDRESS], options->address_width, instrDisasm.address) < 0)
                    goto fprintf_error;
            }
            options->prev_address = instrDisasm.address;
            options->prev_instr_width = instrDisasm.width;
        }
    }

    /* Print an address label like "A_0010:" or a plain address like "0010:" */
    if (options->flags & AVR_FORMAT_FLAG_ADDRESS_LABELS)
        ret = fprintf(out, "%s%0*x:\t", options->prefixes[AVR_FORMAT_PREFIX_ADDRESS_LABEL], options->address_width, instrDisasm.address);
    else if (options->flags & AVR_FORMAT_FLAG_ADDRESSES)
        ret = fprintf(out, "%*x:\t", options->address_width, instrDisasm.address);

    if (ret < 0)
        goto fprintf_error;

    /* Print original opcode, if we're not generating address labels */
    if (options->flags & AVR_FORMAT_FLAG_OPCODES && !(options->flags & AVR_FORMAT_FLAG_ADDRESS_LABELS)) {
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

        /* If we have address labels on, adjust relative address operand with
         * the instruction's address when printing it */
        if ( options->flags & AVR_FORMAT_FLAG_ADDRESS_LABELS &&
             (instrDisasm.instructionSetEntry->operandTypes[i] == OPERAND_BRANCH_ADDRESS ||
              instrDisasm.instructionSetEntry->operandTypes[i] == OPERAND_RELATIVE_ADDRESS) ) {
            /* Print the operand */
            if (util_format_print_operand(out, instrDisasm.operandDisasms[i] + instrDisasm.address + 2, instrDisasm.instructionSetEntry->operandTypes[i], options) < 0)
                goto fprintf_error;
            continue;
        }

        /* Print the operand */
        if (util_format_print_operand(out, instrDisasm.operandDisasms[i], instrDisasm.instructionSetEntry->operandTypes[i], options) < 0)
            goto fprintf_error;
    }

    /* Print the destination address comment */
    if (options->flags & AVR_FORMAT_FLAG_DEST_ADDR_COMMENT) {
        for (i = 0; i < instrDisasm.instructionSetEntry->numOperands; i++) {
            if ( instrDisasm.instructionSetEntry->operandTypes[i] == OPERAND_BRANCH_ADDRESS ||
                 instrDisasm.instructionSetEntry->operandTypes[i] == OPERAND_RELATIVE_ADDRESS) {
                if (fprintf(out, "\t; %s%x", options->prefixes[AVR_FORMAT_PREFIX_ABSOLUTE_ADDRESS], instrDisasm.operandDisasms[i] + instrDisasm.address + 2) < 0)
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

/******************************************************************************/
/* Format Stream Test */
/******************************************************************************/

struct opcode_stream_debug_state {
    uint8_t *data;
    uint32_t *address;
    unsigned int len;
    int index;
};

static int test_format_stream_run(char *name, uint8_t *test_data, uint32_t *test_address, unsigned int test_len, char **format_prefixes, int format_address_width, int format_flags) {
    struct OpcodeStream os;
    struct DisasmStream ds;
    struct FormatStream fs;
    int ret;

    printf("Running test \"%s\"\n", name);

    /* Setup a debug Opcode Stream */
    os.in = NULL;
    os.stream_init = opcode_stream_debug_init;
    os.stream_close = opcode_stream_debug_close;
    os.stream_read = opcode_stream_debug_read;

    /* Setup the AVR Disasm Stream */
    ds.in = &os;
    ds.stream_init = disasm_stream_avr_init;
    ds.stream_close = disasm_stream_avr_close;
    ds.stream_read = disasm_stream_avr_read;

    /* Setup the Format Stream */
    fs.in = &ds;
    fs.stream_init = format_stream_avr_init;
    fs.stream_close = format_stream_avr_close;
    fs.stream_read = format_stream_avr_read;

    /* Initialize the stream */
    printf("\tfs.stream_init(): %d\n", fs.stream_init(&fs));
    if (fs.error != NULL) {
        printf("\t\tError: %s\n", fs.error);
        return -1;
    }

    /* Load the Opcode Stream with the test vector */
    struct opcode_stream_debug_state *state;
    state = (struct opcode_stream_debug_state *)os.state;
    state->data = test_data;
    state->address = test_address;
    state->len = test_len;

    /* Load the Format Stream options */
    struct format_stream_avr_state *options;
    options = (struct format_stream_avr_state *)fs.state;
    options->prefixes = format_prefixes;
    options->address_width = format_address_width;
    options->flags = format_flags;

    /* Read formatted disassembled instructions until EOF */
    while ( (ret = fs.stream_read(&fs, stdout)) != STREAM_EOF ) {
        if (fs.error != NULL)
            printf("\t\tError: %s\n", fs.error);
        if (ret < 0) {
            printf("\tfs.stream_read(): %d\n", ret);
            return -1;
        }
    }

    /* Close the stream */
    printf("\tfs.stream_close(): %d\n", fs.stream_close(&fs));
    if (fs.error != NULL) {
        printf("\t\tError: %s\n", fs.error);
        return -1;
    }

    printf("\n");

    return 0;
}

void test_format_stream(void) {
    int i;
    int numTests = 0, passedTests = 0;

    /* Sample Program */
    /* rjmp l1; l1: ser r16; out 0x17, r16; dec r16; rjmp l2; jmp 0x2abab4;
     * cbi 0x12, 7; ldi r16, 0xaf; ret; nop; st Y, r2; st Y+, r2; st -Y, r2;
     * std y+5, r2; l2: st X, r3; st X+, r3; st -X, r3; st Y, r4; st Y+, r4;
     * st -Y, r4; std y+5, r4; .word 0xffff; .byte 0xff */
    uint8_t d[] = {0x00, 0xc0, 0x0f, 0xef, 0x07, 0xbb, 0x0a, 0x95, 0x0a, 0xc0, 0xad, 0x94, 0x5a, 0x5d, 0x97, 0x98, 0x0f, 0xea, 0x08, 0x95, 0x00, 0x00, 0x28, 0x82, 0x29, 0x92, 0x2a, 0x92, 0x2d, 0x82, 0x3c, 0x92, 0x3d, 0x92, 0x3e, 0x92, 0x48, 0x82, 0x49, 0x92, 0x4a, 0x92, 0x4d, 0x82, 0xff, 0xff, 0xff};
    uint32_t a[sizeof(d)]; for (i = 0; i < sizeof(d); i++) a[i] = i;

    /* Typical Options */
    int format_address_width = 4;
    char *format_prefixes[] = {
        [AVR_FORMAT_PREFIX_REGISTER]            = "R",  /* mov R0, R2 */
        [AVR_FORMAT_PREFIX_IO_REGISTER]         = "$",  /* out $39, R16 */
        [AVR_FORMAT_PREFIX_DATA_HEX]            = "0x", /* ldi R16, 0x3d */
        [AVR_FORMAT_PREFIX_DATA_BIN]            = "0b", /* ldi R16, 0b00111101 */
        [AVR_FORMAT_PREFIX_DATA_DEC]            = "",   /* ldi R16, 61 */
        [AVR_FORMAT_PREFIX_BIT]                 = "",   /* cbi $12, 7 */
        [AVR_FORMAT_PREFIX_ADDRESS_LABEL]       = "A_", /* A_0010: ... */
        [AVR_FORMAT_PREFIX_ABSOLUTE_ADDRESS]    = "0x", /* call 0x1234 */
        [AVR_FORMAT_PREFIX_RELATIVE_ADDRESS]    = ".",  /* rjmp .4 */
        [AVR_FORMAT_PREFIX_DES_ROUND]           = "0x", /* des 0x01 */
        [AVR_FORMAT_PREFIX_RAW_WORD]            = "0x", /* .dw 0xabcd */
        [AVR_FORMAT_PREFIX_RAW_BYTE]            = "0x", /* .db 0xab */
        };

    /* Check typical options */
    {
        int format_flags = AVR_FORMAT_FLAG_ADDRESSES | AVR_FORMAT_FLAG_DEST_ADDR_COMMENT | AVR_FORMAT_FLAG_DATA_HEX | AVR_FORMAT_FLAG_OPCODES;

        if (test_format_stream_run("Typical Options", &d[0], &a[0], sizeof(d), format_prefixes, format_address_width, format_flags) == 0)
            passedTests++;
        numTests++;
    }

    /* Check data type bin */
    {
        int format_flags = AVR_FORMAT_FLAG_ADDRESSES | AVR_FORMAT_FLAG_DEST_ADDR_COMMENT | AVR_FORMAT_FLAG_DATA_BIN | AVR_FORMAT_FLAG_OPCODES;

        if (test_format_stream_run("Data Type Bin", &d[0], &a[0], sizeof(d), format_prefixes, format_address_width, format_flags) == 0)
            passedTests++;
        numTests++;
    }

    /* Check data type dec */
    {
        int format_flags = AVR_FORMAT_FLAG_ADDRESSES | AVR_FORMAT_FLAG_DEST_ADDR_COMMENT | AVR_FORMAT_FLAG_DATA_DEC | AVR_FORMAT_FLAG_OPCODES;

        if (test_format_stream_run("Data Type Dec", &d[0], &a[0], sizeof(d), format_prefixes, format_address_width, format_flags) == 0)
            passedTests++;
        numTests++;
    }

    /* Check no original opcode */
    {
        int format_flags = AVR_FORMAT_FLAG_ADDRESSES | AVR_FORMAT_FLAG_DEST_ADDR_COMMENT | AVR_FORMAT_FLAG_DATA_HEX;

        if (test_format_stream_run("No Original Opcode", &d[0], &a[0], sizeof(d), format_prefixes, format_address_width, format_flags) == 0)
            passedTests++;
        numTests++;
    }

    /* Check no addresses, no destination comments */
    {
        int format_flags = AVR_FORMAT_FLAG_DATA_HEX;

        if (test_format_stream_run("No Addresses, No Destination Comments", &d[0], &a[0], sizeof(d), format_prefixes, format_address_width, format_flags) == 0)
            passedTests++;
        numTests++;
    }

    /* Check address labels */
    {
        int format_flags = AVR_FORMAT_FLAG_ADDRESS_LABELS | AVR_FORMAT_FLAG_DEST_ADDR_COMMENT | AVR_FORMAT_FLAG_DATA_HEX;

        if (test_format_stream_run("Address Labels", &d[0], &a[0], sizeof(d), format_prefixes, format_address_width, format_flags) == 0)
            passedTests++;
        numTests++;
    }

    printf("%d / %d tests passed.\n", passedTests, numTests);
}

