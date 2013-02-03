#include <stdint.h>
#include <stdio.h>
#include "avr_disasm_stream.h"

struct byte_stream_debug_state {
    uint8_t *data;
    uint32_t *address;
    unsigned int len;
    int index;
};

static int test_print_stream_run(char *name, uint8_t *test_data, uint32_t *test_address, unsigned int test_len, char **format_prefixes, int format_address_width, int format_flags) {
    struct ByteStream bs;
    struct DisasmStream ds;
    struct PrintStream ps;
    int ret;

    printf("Running test \"%s\"\n", name);

    /* Setup a debug Opcode Stream */
    bs.in = NULL;
    bs.stream_init = byte_stream_debug_init;
    bs.stream_close = byte_stream_debug_close;
    bs.stream_read = byte_stream_debug_read;

    /* Setup the AVR Disasm Stream */
    ds.in = &bs;
    ds.stream_init = disasm_stream_avr_init;
    ds.stream_close = disasm_stream_avr_close;
    ds.stream_read = disasm_stream_avr_read;

    /* Setup the Print Stream */
    ps.in = &ds;
    ps.stream_init = print_stream_avr_init;
    ps.stream_close = print_stream_avr_close;
    ps.stream_read = print_stream_avr_read;

    /* Initialize the stream */
    printf("\tps.stream_init(): %d\n", ps.stream_init(&ps));
    if (ps.error != NULL) {
        printf("\t\tError: %s\n", ps.error);
        return -1;
    }

    /* Load the Opcode Stream with the test vector */
    struct byte_stream_debug_state *state;
    state = (struct byte_stream_debug_state *)bs.state;
    state->data = test_data;
    state->address = test_address;
    state->len = test_len;

    /* Load the Print Stream options */
    struct print_stream_avr_state *options;
    options = (struct print_stream_avr_state *)ps.state;
    options->prefixes = format_prefixes;
    options->address_width = format_address_width;
    options->flags = format_flags;

    /* Read formatted disassembled instructions until EOF */
    while ( (ret = ps.stream_read(&ps, stdout)) != STREAM_EOF ) {
        if (ps.error != NULL)
            printf("\t\tError: %s\n", ps.error);
        if (ret < 0) {
            printf("\tps.stream_read(): %d\n", ret);
            return -1;
        }
    }

    /* Close the stream */
    printf("\tps.stream_close(): %d\n", ps.stream_close(&ps));
    if (ps.error != NULL) {
        printf("\t\tError: %s\n", ps.error);
        return -1;
    }

    printf("\n");

    return 0;
}

void test_print_stream(void) {
    int i;
    int numTests = 0, passedTests = 0;

    /* Sample Program */
    /* rjmp l1; l1: ser r16; out 0x17, r16; dec r16; rjmp l2; jmp 0x2abab4;
     * cbi 0x12, 7; ldi r16, 0xaf; ret; nop; st Y, r2; st Y+, r2; st -Y, r2;
     * std y+5, r2; l2: st X, r3; st X+, r3; st -X, r3; st Y, r4; st Y+, r4;
     * st -Y, r4; std y+5, r4; .word 0xffff; .byte 0xff */
    uint8_t d[] = {0x00, 0xc0, 0x0f, 0xef, 0x07, 0xbb, 0x0a, 0x95, 0x0a, 0xc0, 0xad, 0x94, 0x5a, 0x5d, 0x97, 0x98, 0x0f, 0xea, 0x08, 0x95, 0x00, 0x00, 0x28, 0x82, 0x29, 0x92, 0x2a, 0x92, 0x2d, 0x82, 0x3c, 0x92, 0x3d, 0x92, 0x3e, 0x92, 0x48, 0x82, 0x49, 0x92, 0x4a, 0x92, 0x4d, 0x82, 0xff, 0xff, 0xff};
    uint32_t a[sizeof(d)]; for (i = 0; i < sizeof(d); i++) a[i] = i;

    /* Typical Options */
    int format_address_width = 4;

    /* Check typical options */
    {
        int format_flags = AVR_FORMAT_FLAG_ADDRESSES | AVR_FORMAT_FLAG_DEST_ADDR_COMMENT | AVR_FORMAT_FLAG_DATA_HEX | AVR_FORMAT_FLAG_OPCODES;

        if (test_print_stream_run("Typical Options", &d[0], &a[0], sizeof(d), format_prefixes, format_address_width, format_flags) == 0)
            passedTests++;
        numTests++;
    }

    /* Check data type bin */
    {
        int format_flags = AVR_FORMAT_FLAG_ADDRESSES | AVR_FORMAT_FLAG_DEST_ADDR_COMMENT | AVR_FORMAT_FLAG_DATA_BIN | AVR_FORMAT_FLAG_OPCODES;

        if (test_print_stream_run("Data Type Bin", &d[0], &a[0], sizeof(d), format_prefixes, format_address_width, format_flags) == 0)
            passedTests++;
        numTests++;
    }

    /* Check data type dec */
    {
        int format_flags = AVR_FORMAT_FLAG_ADDRESSES | AVR_FORMAT_FLAG_DEST_ADDR_COMMENT | AVR_FORMAT_FLAG_DATA_DEC | AVR_FORMAT_FLAG_OPCODES;

        if (test_print_stream_run("Data Type Dec", &d[0], &a[0], sizeof(d), format_prefixes, format_address_width, format_flags) == 0)
            passedTests++;
        numTests++;
    }

    /* Check no original opcode */
    {
        int format_flags = AVR_FORMAT_FLAG_ADDRESSES | AVR_FORMAT_FLAG_DEST_ADDR_COMMENT | AVR_FORMAT_FLAG_DATA_HEX;

        if (test_print_stream_run("No Original Opcode", &d[0], &a[0], sizeof(d), format_prefixes, format_address_width, format_flags) == 0)
            passedTests++;
        numTests++;
    }

    /* Check no addresses, no destination comments */
    {
        int format_flags = AVR_FORMAT_FLAG_DATA_HEX;

        if (test_print_stream_run("No Addresses, No Destination Comments", &d[0], &a[0], sizeof(d), format_prefixes, format_address_width, format_flags) == 0)
            passedTests++;
        numTests++;
    }

    /* Check address labels */
    {
        int format_flags = AVR_FORMAT_FLAG_ADDRESS_LABELS | AVR_FORMAT_FLAG_DEST_ADDR_COMMENT | AVR_FORMAT_FLAG_DATA_HEX;

        if (test_print_stream_run("Address Labels", &d[0], &a[0], sizeof(d), format_prefixes, format_address_width, format_flags) == 0)
            passedTests++;
        numTests++;
    }

    printf("%d / %d tests passed.\n", passedTests, numTests);
}

