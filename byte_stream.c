#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "libGIS-1.0.5/atmel_generic.h"
#include "libGIS-1.0.5/ihex.h"
#include "libGIS-1.0.5/srecord.h"

#include "byte_stream.h"
#include "stream_error.h"

/******************************************************************************/
/* Atmel Generic file support */
/******************************************************************************/

struct byte_stream_generic_state {
    AtmelGenericRecord aRec;
    int availBytes;
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

int byte_stream_generic_read(struct ByteStream *self, uint8_t *data, uint32_t *address, unsigned int *len, unsigned int n) {
    struct byte_stream_generic_state *state = (struct byte_stream_generic_state *)self->state;

    int i, ret;

    /* Return up to n bytes */
    for (i = 0; i < n; ) {
        if (state->availBytes == 2) {
            /* Copy over low byte (assuming little-endian) */
            *data++ = (uint8_t)(state->aRec.data & 0xff);
            *address++ = state->aRec.address*2;
            state->availBytes--;
            i++;
        } else if (state->availBytes == 1) {
            /* Copy over high byte (assuming little-endian) */
            *data++ = (uint8_t)((state->aRec.data >> 8) & 0xff);
            *address++ = state->aRec.address*2 + 1;
            state->availBytes--;
            i++;
        } else {
            /* Read the next two byte record */
            ret = Read_AtmelGenericRecord(&(state->aRec), self->in);
            switch (ret) {
                case ATMEL_GENERIC_ERROR_NEWLINE:
                    continue;
                case ATMEL_GENERIC_OK:
                    break;

                case ATMEL_GENERIC_ERROR_EOF:
                    *len = i;
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

            /* Update our available byte counter */
            state->availBytes = 2;
        }
    }

    *len = i;

    return 0;
}

/******************************************************************************/
/* Intel IHEX file support */
/******************************************************************************/

struct byte_stream_ihex_state {
    IHexRecord iRec;
    int availBytes;
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

int byte_stream_ihex_read(struct ByteStream *self, uint8_t *data, uint32_t *address, unsigned int *len, unsigned int n) {
    int i, ret;
    struct byte_stream_ihex_state *state = (struct byte_stream_ihex_state *)self->state;

    /* Return up to n bytes */
    for (i = 0; i < n; ) {
        if (state->availBytes > 0) {
            /* Copy over next low byte (assuming little-endian) */
            *data++ = state->iRec.data[ (state->iRec.dataLen - state->availBytes) ];
            *address++ = (uint32_t)state->iRec.address + (uint32_t)(state->iRec.dataLen - state->availBytes);
            state->availBytes--;
            i++;
        } else {
            /* Read the next record */
            ret = Read_IHexRecord(&(state->iRec), self->in);
            switch (ret) {
                case IHEX_ERROR_NEWLINE:
                    continue;
                case IHEX_OK:
                    break;

                case IHEX_ERROR_EOF:
                    *len = i;
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

            /* Skip the record if it's not a data record */
            if (state->iRec.type != IHEX_TYPE_00)
                continue;

            /* Update our available bytes counter */
            state->availBytes = state->iRec.dataLen;
        }
    }

    *len = i;

    return 0;
}

/******************************************************************************/
/* Motorola S-Record file support */
/******************************************************************************/

struct byte_stream_srecord_state {
    SRecord sRec;
    int availBytes;
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

int byte_stream_srecord_read(struct ByteStream *self, uint8_t *data, uint32_t *address, unsigned int *len, unsigned int n) {
    int i, ret;
    struct byte_stream_srecord_state *state = (struct byte_stream_srecord_state *)self->state;

    /* Return up to n bytes */
    for (i = 0; i < n; ) {
        if (state->availBytes > 0) {
            /* Copy over next low byte (assuming little-endian) */
            *data++ = state->sRec.data[state->sRec.dataLen - state->availBytes];
            *address++ = (uint32_t)state->sRec.address + (uint32_t)(state->sRec.dataLen - state->availBytes);
            state->availBytes--;
            i++;
        } else {
            /* Read the next record */
            ret = Read_SRecord(&(state->sRec), self->in);
            switch (ret) {
                case SRECORD_ERROR_NEWLINE:
                    continue;
                case SRECORD_OK:
                    break;

                case SRECORD_ERROR_EOF:
                    *len = i;
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

            /* Skip the record if it's not a data record */
            if (state->sRec.type != SRECORD_TYPE_S1 && state->sRec.type != SRECORD_TYPE_S2 && state->sRec.type != SRECORD_TYPE_S3)
                continue;

            /* Update our available bytes counter */
            state->availBytes = state->sRec.dataLen;
        }
    }

    *len = i;

    return 0;
}

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

int byte_stream_binary_read(struct ByteStream *self, uint8_t *data, uint32_t *address, unsigned int *len, unsigned int n) {
    int i, bytes_read;
    struct byte_stream_binary_state *state = (struct byte_stream_binary_state *)self->state;

    /* Return up to n bytes */
    bytes_read = fread(data, 1, n, self->in);
    for (i = 0; i < bytes_read; i++) {
        *address++ = state->address;
        state->address++;
    }
    *len = bytes_read;

    /* Check for short-count, indicating EOF or error */
    if (bytes_read < n) {
        if (feof(self->in))
            return STREAM_EOF;
        else if (ferror(self->in)) {
            self->error = "Error reading file!";
            return STREAM_ERROR_INPUT;
        }
    }

    return 0;
}

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

int byte_stream_debug_read(struct ByteStream *self, uint8_t *data, uint32_t *address, unsigned int *len, unsigned int n) {
    struct byte_stream_debug_state *state = (struct byte_stream_debug_state *)self->state;

    int i;

    /* Return up to n bytes */
    for (i = 0; i < n; i++) {
        /* If we have no more data left in our test vector */
        if (state->len == 0) {
            *len = i;
            return STREAM_EOF;
        }

        /* Copy over the next data and address from our test vector */
        *data++ = state->data[state->index];
        *address++ = state->address[state->index];
        state->index++;
        state->len--;
    }

    *len = i;

    return 0;
}

/******************************************************************************/
/* Byte Stream File Test */
/******************************************************************************/

int test_byte_stream(FILE *in, int (*stream_init)(struct ByteStream *self), int (*stream_close)(struct ByteStream *self), int (*stream_read)(struct ByteStream *self, uint8_t *data, uint32_t *address, unsigned int *len, unsigned int n)) {
    struct ByteStream os;

    uint8_t data[512];
    uint32_t address[512];
    unsigned int len;

    int i;

    /* Setup the Byte Stream */
    os.in = in;
    os.stream_init = stream_init;
    os.stream_close = stream_close;
    os.stream_read = stream_read;

    printf("Running test_byte_stream()\n\n");

    /* Initialize the stream */
    printf("os.stream_init(): %d\n", os.stream_init(&os));
    if (os.error != NULL) {
        printf("\tError: %s\n", os.error);
        return -1;
    }
    printf("\n");

    /* Read 2 bytes */
    printf("os.stream_read(), 2 bytes: %d\n", os.stream_read(&os, &data[0], &address[0], &len, 2));
    if (os.error != NULL)
        printf("\tError: %s\n", os.error);
    printf("\tRead %d bytes: ", len);
    for (i = 0; i < len; i++) {
        printf("%08x:%02x ", address[i], data[i]);
    }
    printf("\n\n");

    /* Read 5 bytes */
    printf("os.stream_read(), 5 bytes: %d\n", os.stream_read(&os, &data[0], &address[0], &len, 5));
    if (os.error != NULL)
        printf("\tError: %s\n", os.error);
    printf("\tRead %d bytes: ", len);
    for (i = 0; i < len; i++) {
        printf("%08x:%02x ", address[i], data[i]);
    }
    printf("\n\n");

    /* Read 1 byte */
    printf("os.stream_read(), 1 bytes: %d\n", os.stream_read(&os, &data[0], &address[0], &len, 1));
    if (os.error != NULL)
        printf("\tError: %s\n", os.error);
    printf("\tRead %d bytes: ", len);
    for (i = 0; i < len; i++) {
        printf("%08x:%02x ", address[i], data[i]);
    }
    printf("\n\n");

    /* Read up to 500 bytes */
    printf("os.stream_read(), 500 bytes: %d\n", os.stream_read(&os, &data[0], &address[0], &len, 500));
    if (os.error != NULL)
        printf("\tError: %s\n", os.error);
    printf("\tRead %d bytes: ", len);
    for (i = 0; i < len; i++) {
        printf("%08x:%02x ", address[i], data[i]);
    }
    printf("\n\n");

    /* Close the stream */
    printf("os.stream_close(): %d\n", os.stream_close(&os));
    if (os.error != NULL) {
        printf("\tError: %s\n", os.error);
        return -1;
    }

    return 0;
}

