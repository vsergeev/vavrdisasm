#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "libGIS-1.0.5/atmel_generic.h"

#include <byte_stream.h>

/******************************************************************************/
/* Atmel Generic file support */
/******************************************************************************/

struct byte_stream_generic_state {
    AtmelGenericRecord aRec;
    unsigned int availBytes;
};

int byte_stream_generic_init(struct ByteStream *self) {
    /* Allocate stream state */
    self->state = malloc(sizeof(struct byte_stream_generic_state));
    if (self->state == NULL) {
        self->error = "Error allocating opcode stream state!";
        return STREAM_ERROR_ALLOC;
    }
    /* Initialize stream state */
    memset(self->state, 0, sizeof(struct byte_stream_generic_state));

    /* Reset error string to NULL */
    self->error = NULL;

    /* Initialize the input stream */
    /* FILE *in; assumed to have been opened */

    return 0;
}

int byte_stream_generic_close(struct ByteStream *self) {
    /* Free stream state memory */
    free(self->state);

    /* Close input stream */
    fclose(self->in);

    return 0;
}

int byte_stream_generic_read(struct ByteStream *self, uint8_t *data, uint32_t *address) {
    struct byte_stream_generic_state *state = (struct byte_stream_generic_state *)self->state;
    int ret;

    if (state->availBytes == 0) {
            do {
                /* Read the next two byte record */
                ret = Read_AtmelGenericRecord(&(state->aRec), self->in);
                switch (ret) {
                    case ATMEL_GENERIC_OK:
                        break;
                    case ATMEL_GENERIC_ERROR_NEWLINE:
                        continue;
                    case ATMEL_GENERIC_ERROR_EOF:
                        return STREAM_EOF;
                    case ATMEL_GENERIC_ERROR_FILE:
                        self->error = "Error reading Atmel Generic formatted file!";
                        return STREAM_ERROR_INPUT;
                    case ATMEL_GENERIC_ERROR_INVALID_RECORD:
                        self->error = "Invalid Atmel Generic formatted file!";
                        return STREAM_ERROR_INPUT;
                    default:
                        self->error = "Unknown error reading Atmel Generic formatted file!";
                        return STREAM_ERROR_INPUT;
                }
                break;
            /* Keep reading until we get a record... */
            } while (1);

            /* Update our available byte counter */
            state->availBytes = 2;
    }

    if (state->availBytes == 2) {
        /* Copy over low byte (assuming little-endian) */
        *data = (uint8_t)(state->aRec.data & 0xff);
        *address = state->aRec.address*2;
        state->availBytes--;
    } else if (state->availBytes == 1) {
        /* Copy over high byte (assuming little-endian) */
        *data = (uint8_t)((state->aRec.data >> 8) & 0xff);
        *address = state->aRec.address*2 + 1;
        state->availBytes--;
    }

    return 0;
}

