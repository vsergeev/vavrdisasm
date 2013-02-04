#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <print_stream.h>
#include <instruction.h>
#include <print_stream_file.h>

/******************************************************************************/
/* File Print Stream Support */
/******************************************************************************/

/* Print Stream State */
struct print_stream_file_state {
    /* Print Option Bit Flags */
    unsigned int flags;
    /* Origin Initiailized */
    int origin_initialized;
    /* Next Expected address */
    uint32_t next_address;
};

int print_stream_file_init(struct PrintStream *self, int flags) {
    /* Allocate stream state */
    self->state = malloc(sizeof(struct print_stream_file_state));
    if (self->state == NULL) {
        self->error = "Error allocating format stream state!";
        return STREAM_ERROR_ALLOC;
    }

    /* Initialize stream state */
    memset(self->state, 0, sizeof(struct print_stream_file_state));
    ((struct print_stream_file_state *)self->state)->flags = flags;

    /* Reset the error to NULL */
    self->error = NULL;

    /* Initialize the input stream */
    if (self->in->stream_init(self->in) < 0) {
        self->error = "Error in input stream initialization!";
        return STREAM_ERROR_INPUT;
    }

    return 0;
}

int print_stream_file_close(struct PrintStream *self) {
    /* Free stream state memory */
    free(self->state);

    /* Close input stream */
    if (self->in->stream_close(self->in) < 0) {
        self->error = "Error in input stream close!";
        return STREAM_ERROR_INPUT;
    }

    return 0;
}

int print_stream_file_read(struct PrintStream *self, FILE *out) {
    struct print_stream_file_state *state = (struct print_stream_file_state *)self->state;
    struct instruction instr;
    char str[128];
    int i, ret;

    /* Read a disassembled instruction */
    ret = self->in->stream_read(self->in, &instr);
    switch (ret) {
        case 0:
            break;
        case STREAM_EOF:
            return STREAM_EOF;
        default:
            self->error = "Error in disasm stream read!";
            return STREAM_ERROR_INPUT;
    }

    #define print_str() do { if (fputs(str, out) < 0) goto fputs_error; } while(0)
    #define print_comma() do { if (fputs(", ", out) < 0) goto fputs_error; } while(0)
    #define print_newline() do { if (fputs("\n", out) < 0) goto fputs_error; } while(0)
    #define print_tab() do { if (fputs("\t", out) < 0) goto fputs_error; } while (0)

    /* If this is the very first instruction, or there is a discontinuity in
     * the instruction address */
    if (!(state->origin_initialized) || instr.get_address(&instr) != state->next_address) {
        /* Print an origin directive if we're outputting assembly */
        if (state->flags & PRINT_FLAG_ASSEMBLY) {
            instr.get_str_origin(&instr, str, sizeof(str), state->flags);
            print_str();
            print_newline();
        }
        state->origin_initialized = 1;
    }
    /* Update next expected address */
    state->next_address = instr.get_address(&instr) + instr.get_width(&instr);

    /* Print an address label if we're printing assembly */
    if (state->flags & PRINT_FLAG_ASSEMBLY) {
        instr.get_str_address_label(&instr, str, sizeof(str), state->flags);
        print_str();
        print_tab();
    /* Or print an normal address */
    } else if (state->flags & PRINT_FLAG_ADDRESSES) {
        instr.get_str_address(&instr, str, sizeof(str), state->flags);
        print_str();
        print_tab();
    }

    /* Print the opcodes */
    if (state->flags & PRINT_FLAG_OPCODES) {
        instr.get_str_opcodes(&instr, str, sizeof(str), state->flags);
        print_str();
        print_tab();
    }

    /* Print the mnemonic */
    instr.get_str_mnemonic(&instr, str, sizeof(str), state->flags);
    print_str();
    print_tab();

    /* Print the operands */
    for (i = 0; i < instr.get_num_operands(&instr); i++) {
        /* Print dat comma, yea */
        if (i > 0 && i < instr.get_num_operands(&instr))
            print_comma();

        /* Print this operand index i */
        instr.get_str_operand(&instr, str, sizeof(str), i, state->flags);
        print_str();
    }

    /* Print a comment (e.g. destination address comment) */
    if (state->flags & PRINT_FLAG_DESTINATION_COMMENT) {
        if (instr.get_str_comment(&instr, str, sizeof(str), state->flags) > 0) {
            print_tab();
            print_str();
        }
    }

    /* Print a newline */
    print_newline();

    /* Free the allocated disassembled instruction */
    instr.free(&instr);

    return 0;

    fputs_error:
    self->error = "Error writing to output file!";
    return STREAM_ERROR_OUTPUT;
}

