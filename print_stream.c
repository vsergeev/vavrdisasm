#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <print_stream.h>
#include <instruction.h>

/******************************************************************************/
/* Print Stream Support */
/******************************************************************************/

int print_stream_init(struct PrintStream *self, int flags) {
    /* Allocate stream state */
    self->state = malloc(sizeof(struct print_stream_state));
    if (self->state == NULL) {
        self->error = "Error allocating format stream state!";
        return STREAM_ERROR_ALLOC;
    }

    /* Initialize stream state */
    memset(self->state, 0, sizeof(struct print_stream_state));
    ((struct print_stream_state *)self->state)->flags = flags;

    /* Reset the error to NULL */
    self->error = NULL;

    /* Initialize the input stream */
    if (self->in->stream_init(self->in) < 0) {
        self->error = "Error in input stream initialization!";
        return STREAM_ERROR_INPUT;
    }

    return 0;
}

int print_stream_close(struct PrintStream *self) {
    /* Free stream state memory */
    free(self->state);

    /* Close input stream */
    if (self->in->stream_close(self->in) < 0) {
        self->error = "Error in input stream close!";
        return STREAM_ERROR_INPUT;
    }

    return 0;
}

int print_stream_read(struct PrintStream *self, FILE *out) {
    struct print_stream_state *state = (struct print_stream_state *)self->state;
    struct instruction instr;
    int ret;

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

    /* If this is the very first instruction, or there is a discontinuity in
     * the instruction address */
    if (!(state->origin_initialized) || instr.address != state->next_address) {
        /* Print an origin directive if we're outputting assembly */
        if (instr.print_origin(&instr, out, state->flags) < 0)
            goto fprintf_error;
        state->origin_initialized = 1;
    }

    /* Update next expected address */
    state->next_address = instr.address + instr.width;

    /* Print the instruction */
    if (instr.print(&instr, out, state->flags) < 0)
        goto fprintf_error;

    /* Print a newline */
    if (fprintf(out, "\n") < 0)
        goto fprintf_error;

    return 0;

    fprintf_error:
    self->error = "Error writing to output file!";
    return STREAM_ERROR_OUTPUT;
}

