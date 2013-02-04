#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <byte_stream.h>

/******************************************************************************/
/* Binary Byte Stream Support */
/******************************************************************************/

struct byte_stream_binary_state {
    uint32_t address;
};

int byte_stream_binary_init(struct ByteStream *self) {
    /* Allocate stream state */
    self->state = malloc(sizeof(struct byte_stream_binary_state));
    if (self->state == NULL) {
        self->error = "Error allocating opcode stream state!";
        return STREAM_ERROR_ALLOC;
    }
    /* Initialize stream state */
    memset(self->state, 0, sizeof(struct byte_stream_binary_state));

    /* Reset error string to NULL */
    self->error = NULL;

    /* Initialize the input stream */
    /* FILE *in; assumed to have been opened */

    return 0;
}

int byte_stream_binary_close(struct ByteStream *self) {
    /* Free stream state memory */
    free(self->state);

    /* Close input stream */
    fclose(self->in);

    return 0;
}

int byte_stream_binary_read(struct ByteStream *self, uint8_t *data, uint32_t *address) {
    struct byte_stream_binary_state *state = (struct byte_stream_binary_state *)self->state;
    int bytes_read;

    /* Read a byte */
    bytes_read = fread(data, 1, 1, self->in);
    if (bytes_read == 1) {
        *address = state->address;
        state->address++;
    } else {
    /* Check for short-count, indicating EOF or error */
        if (feof(self->in))
            return STREAM_EOF;
        else if (ferror(self->in)) {
            self->error = "Error reading file!";
            return STREAM_ERROR_INPUT;
        }
    }

    return 0;
}

