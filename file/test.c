#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include <byte_stream.h>

/******************************************************************************/
/* Byte Stream File Test */
/******************************************************************************/

int test_byte_stream(FILE *in, int (*stream_init)(struct ByteStream *self), int (*stream_close)(struct ByteStream *self), int (*stream_read)(struct ByteStream *self, uint8_t *data, uint32_t *address)) {
    struct ByteStream os;
    uint8_t data;
    uint32_t address;
    int i, ret;

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

    /* Read 100 bytes */
    for (i = 0; i < 100; i++) {
        ret = os.stream_read(&os, &data, &address);
        printf("os.stream_read(): %d\n", ret);
        if (ret == STREAM_EOF)
            break;
        else if (ret < 0) {
            printf("\tError: %s\n", os.error);
            break;
        }
        printf("\t%08x:%02x\n", address, data);
    }

    /* Close the stream */
    printf("os.stream_close(): %d\n", os.stream_close(&os));
    if (os.error != NULL) {
        printf("\tError: %s\n", os.error);
        return -1;
    }

    return 0;
}

