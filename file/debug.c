#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <byte_stream.h>

/******************************************************************************/
/* Debug Byte Stream Support */
/******************************************************************************/

struct byte_stream_debug_state {
    uint8_t *data;
    uint32_t *address;
    unsigned int len;
    int index;
};

int byte_stream_debug_init(struct ByteStream *self) {
    /* Allocate stream state */
    self->state = malloc(sizeof(struct byte_stream_debug_state));
    if (self->state == NULL) {
        self->error = "Error allocating opcode stream state!";
        return STREAM_ERROR_ALLOC;
    }
    /* Initialize stream state */
    memset(self->state, 0, sizeof(struct byte_stream_debug_state));

    /* Reset error string to NULL */
    self->error = NULL;

    /* Initialize the input stream */
    /* NULL file input */

    return 0;
}

int byte_stream_debug_close(struct ByteStream *self) {
    /* Free stream state memory */
    free(self->state);

    /* Close input stream */
    /* NULL file input */

    return 0;
}

int byte_stream_debug_read(struct ByteStream *self, uint8_t *data, uint32_t *address) {
    struct byte_stream_debug_state *state = (struct byte_stream_debug_state *)self->state;

    /* If we have no more data left in our test vector */
    if (state->len == 0)
        return STREAM_EOF;

    *data = state->data[state->index];
    *address = state->address[state->index];
    state->index++;
    state->len--;

    return 0;
}

