#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "libGIS-1.0.5/srecord.h"

#include <byte_stream.h>

/******************************************************************************/
/* Motorola S-Record file support */
/******************************************************************************/

struct byte_stream_srecord_state {
    SRecord sRec;
    unsigned int availBytes;
};

int byte_stream_srecord_init(struct ByteStream *self) {
    /* Allocate stream state */
    self->state = malloc(sizeof(struct byte_stream_srecord_state));
    if (self->state == NULL) {
        self->error = "Error allocating opcode stream state!";
        return STREAM_ERROR_ALLOC;
    }
    /* Initialize stream state */
    memset(self->state, 0, sizeof(struct byte_stream_srecord_state));

    /* Reset error string to NULL */
    self->error = NULL;

    /* Initialize the input stream */
    /* FILE *in; assumed to have been opened */

    return 0;
}

int byte_stream_srecord_close(struct ByteStream *self) {
    /* Free stream state memory */
    free(self->state);

    /* Close input stream */
    fclose(self->in);

    return 0;
}

int byte_stream_srecord_read(struct ByteStream *self, uint8_t *data, uint32_t *address) {
    struct byte_stream_srecord_state *state = (struct byte_stream_srecord_state *)self->state;
    int ret;

    if (state->availBytes == 0) {
        do {
            /* Read the next record */
            ret = Read_SRecord(&(state->sRec), self->in);
            switch (ret) {
                case SRECORD_OK:
                    break;
                case SRECORD_ERROR_NEWLINE:
                    continue;
                case SRECORD_ERROR_EOF:
                    return STREAM_EOF;
                case SRECORD_ERROR_FILE:
                    self->error = "Error reading Motorola S-Record formatted file!";
                    return STREAM_ERROR_INPUT;
                case SRECORD_ERROR_INVALID_RECORD:
                    self->error = "Invalid Motorola S-Record formatted file!";
                    return STREAM_ERROR_INPUT;
                default:
                    self->error = "Unknown error reading Motorola S-Record formatted file!";
                    return STREAM_ERROR_INPUT;
            }

        /* Continue reading until we get a data record */
        } while (state->sRec.type != SRECORD_TYPE_S1 && state->sRec.type != SRECORD_TYPE_S2 && state->sRec.type != SRECORD_TYPE_S3);

        /* Update our available bytes counter */
        state->availBytes = state->sRec.dataLen;
    }

    if (state->availBytes) {
       /* Copy over next low byte (assuming little-endian) */
       *data = state->sRec.data[state->sRec.dataLen - state->availBytes];
       *address = (uint32_t)state->sRec.address + (uint32_t)(state->sRec.dataLen - state->availBytes);
       state->availBytes--;
    }

    return 0;
}

