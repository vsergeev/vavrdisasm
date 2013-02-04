#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <byte_stream.h>

/******************************************************************************/
/* ASCII Hex Stream Support */
/******************************************************************************/

struct byte_stream_asciihex_state {
    uint32_t address;
};

int byte_stream_asciihex_init(struct ByteStream *self) {
    /* Allocate stream state */
    self->state = malloc(sizeof(struct byte_stream_asciihex_state));
    if (self->state == NULL) {
        self->error = "Error allocating opcode stream state!";
        return STREAM_ERROR_ALLOC;
    }
    /* Initialize stream state */
    memset(self->state, 0, sizeof(struct byte_stream_asciihex_state));

    /* Reset error string to NULL */
    self->error = NULL;

    /* Initialize the input stream */
    /* FILE *in; assumed to have been opened */

    return 0;
}

int byte_stream_asciihex_close(struct ByteStream *self) {
    /* Free stream state memory */
    free(self->state);

    /* Close input stream */
    fclose(self->in);

    return 0;
}

static uint8_t util_hex2num(char c) {
    switch (c) {
        case '0' ... '9': return (c - '0');
        case 'a' ... 'f': return (c - 'a') + 10;
        case 'A' ... 'F': return (c - 'A') + 10;
    }
    return 0;
}

int byte_stream_asciihex_read(struct ByteStream *self, uint8_t *data, uint32_t *address) {
    struct byte_stream_asciihex_state *state = (struct byte_stream_asciihex_state *)self->state;

    int bytes_read;
    char hexstr[3];

    /* Read an ascii hex string */
    bytes_read = fread(hexstr, 1, 3, self->in);
    if (bytes_read == 3) {
        /* Check for valid space delimited hex string "xx " or "xx\t" or "xx\n" etc. */
        if (isxdigit(hexstr[0]) && isxdigit(hexstr[1]) && isspace(hexstr[2])) {
            *data = util_hex2num(hexstr[0])*16 + util_hex2num(hexstr[1]);
            *address = state->address;
            state->address++;
            return 0;
        } else {
            self->error = "Error reading file!";
        }
    } else if (bytes_read == 2) {
        /* Check for valid hex byte "xx" */
        if (isxdigit(hexstr[0]) && isxdigit(hexstr[1])) {
            *data = util_hex2num(hexstr[0])*16 + util_hex2num(hexstr[1]);
            *address = state->address;
            state->address++;
            return 0;
        } else {
            self->error = "Error reading file!";
        }
    } else {
        /* Check for short-count, indicating EOF or error */
        if (feof(self->in))
            return STREAM_EOF;
        else if (ferror(self->in))
            self->error = "Error reading file!";
    }

    return STREAM_ERROR_INPUT;
}

