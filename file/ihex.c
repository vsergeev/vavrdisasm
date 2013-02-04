#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "libGIS-1.0.5/ihex.h"

#include <byte_stream.h>

/******************************************************************************/
/* Intel HEX file support */
/******************************************************************************/

struct byte_stream_ihex_state {
    IHexRecord iRec;
    unsigned int availBytes;
};

int byte_stream_ihex_init(struct ByteStream *self) {
    /* Allocate stream state */
    self->state = malloc(sizeof(struct byte_stream_ihex_state));
    if (self->state == NULL) {
        self->error = "Error allocating opcode stream state!";
        return STREAM_ERROR_ALLOC;
    }
    /* Initialize stream state */
    memset(self->state, 0, sizeof(struct byte_stream_ihex_state));

    /* Reset error string to NULL */
    self->error = NULL;

    /* Initialize the input stream */
    /* FILE *in; assumed to have been opened */

    return 0;
}

int byte_stream_ihex_close(struct ByteStream *self) {
    /* Free stream state memory */
    free(self->state);

    /* Close input stream */
    fclose(self->in);

    return 0;
}

int byte_stream_ihex_read(struct ByteStream *self, uint8_t *data, uint32_t *address) {
    struct byte_stream_ihex_state *state = (struct byte_stream_ihex_state *)self->state;
    int ret;

    if (state->availBytes == 0) {
        do {
            /* Read the next record */
            ret = Read_IHexRecord(&(state->iRec), self->in);
            switch (ret) {
                case IHEX_OK:
                    break;
                case IHEX_ERROR_NEWLINE:
                    continue;
                case IHEX_ERROR_EOF:
                    return STREAM_EOF;
                case IHEX_ERROR_FILE:
                    self->error = "Error reading Intel HEX formatted file!";
                    return STREAM_ERROR_INPUT;
                case IHEX_ERROR_INVALID_RECORD:
                    self->error = "Invalid Intel HEX formatted file!";
                    return STREAM_ERROR_INPUT;
                default:
                    self->error = "Unknown error reading Intel HEX formatted file!";
                    return STREAM_ERROR_INPUT;
            }

        /* Continue reading until we get a data record */
        } while(state->iRec.type != IHEX_TYPE_00);

        /* Update our available bytes counter */
        state->availBytes = state->iRec.dataLen;
    }

    if (state->availBytes) {
        /* Copy over next low byte (assuming little-endian) */
        *data = state->iRec.data[ (state->iRec.dataLen - state->availBytes) ];
        *address = (uint32_t)state->iRec.address + (uint32_t)(state->iRec.dataLen - state->availBytes);
        state->availBytes--;
    }

    return 0;
}

