#include <stdint.h>
#include <stdio.h>

#include <opcode_stream.h>

/******************************************************************************/
/* Disasm Stream Test Instrumentation */
/******************************************************************************/

struct byte_stream_debug_state {
    uint8_t *data;
    uint32_t *address;
    unsigned int len;
    int index;
};

static int debug_disasm_stream(uint8_t *test_data, uint32_t *test_address, unsigned int test_len, struct avrInstructionDisasm *output_instrDisasms, unsigned int *output_len) {
    struct ByteStream bs;
    struct DisasmStream ds;
    int ret;

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

    /* Initialize the stream */
    printf("\tds.stream_init(): %d\n", ds.stream_init(&ds));
    if (ds.error != NULL) {
        printf("\t\tError: %s\n", ds.error);
        return -1;
    }

    /* Load the Opcode Stream with the test vector */
    struct byte_stream_debug_state *state;
    state = (struct byte_stream_debug_state *)bs.state;
    state->data = test_data;
    state->address = test_address;
    state->len = test_len;

    *output_len = 0;

    for (; ret != STREAM_EOF; output_instrDisasms++) {
        /* Disassemble an instruction */
        ret = ds.stream_read(&ds, output_instrDisasms);

        printf("\tds.stream_read(): %d\n", ret);
        if (ds.error != NULL)
            printf("\t\tError: %s\n", ds.error);

        if (ret == 0) {
            printf("\t\t%04x: Instruction: %s | Operands: %d, 0x%02x 0x%02x\n", output_instrDisasms->address, output_instrDisasms->instructionSetEntry->mnemonic, output_instrDisasms->instructionSetEntry->numOperands, output_instrDisasms->operandDisasms[0], output_instrDisasms->operandDisasms[1]);
            *output_len = *output_len + 1;
        } else if (ret == -1) {
            printf("\t\tEOF encountered\n");
        }
    }

    /* Close the stream */
    printf("\tds.stream_close(): %d\n", ds.stream_close(&ds));
    if (ds.error != NULL) {
        printf("\t\tError: %s\n", ds.error);
        return -1;
    }

    printf("\n");

    return 0;
}

/******************************************************************************/
/* AVR Disasm Stream Unit Tests */
/******************************************************************************/

static int test_disasm_avr_unit_test_run(char *name, uint8_t *test_data, uint32_t *test_address, unsigned int test_len, struct avrInstructionDisasm *expected_instrDisasms, unsigned int expected_len) {
    int ret, i, j;
    struct avrInstructionDisasm instrDisasms[16];
    unsigned int len;

    printf("Running test \"%s\"\n", name);

    /* Run the Disasm Stream on the test vectors */
    ret = debug_disasm_stream(test_data, test_address, test_len, &instrDisasms[0], &len);

    /* Check return value */
    if (ret != 0) {
        printf("\tFAILURE ret == 0\n\n");
        return -1;
    }
    printf("\tSUCCESS ret == 0\n");

    /* Compare number of disassembled instructions */
    if (len != expected_len) {
        printf("\tFAILURE len == expected_len\n\n");
        return -1;
    }
    printf("\tSUCCESS len == expected_len\n");

    /* Compare each disassembled instruction */
    for (i = 0; i < expected_len; i++) {
        /* Compare instruction address */
        if (instrDisasms[i].address != expected_instrDisasms[i].address) {
            printf("\tFAILURE instr %d address:\t0x%04x, \texpected 0x%04x\n\n", i, instrDisasms[i].address, expected_instrDisasms[i].address);
            return -1;
        }
        printf("\tSUCCESS instr %d address:\t0x%04x, \texpected 0x%04x\n", i, instrDisasms[i].address, expected_instrDisasms[i].address);

        /* Compare instruction width */
        if (instrDisasms[i].width != expected_instrDisasms[i].width) {
            printf("\tFAILURE instr %d width:\t\t%d, \t\texpected %d\n\n", i, instrDisasms[i].width, expected_instrDisasms[i].width);
            return -1;
        }
        printf("\tSUCCESS instr %d width:\t\t%d, \t\texpected %d\n", i, instrDisasms[i].width, expected_instrDisasms[i].width);

        /* Compare instruction identified */
        if (instrDisasms[i].instructionSetEntry != expected_instrDisasms[i].instructionSetEntry) {
            printf("\tFAILURE instr %d entry:  \t%s, \t\texpected %s\n\n", i, instrDisasms[i].instructionSetEntry->mnemonic, expected_instrDisasms[i].instructionSetEntry->mnemonic);
            return -1;
        }
        printf("\tSUCCESS instr %d entry:  \t%s, \t\texpected %s\n", i, instrDisasms[i].instructionSetEntry->mnemonic, expected_instrDisasms[i].instructionSetEntry->mnemonic);

        /* Compare disassembled operands */
        for (j = 0; j < AVR_MAX_NUM_OPERANDS; j++) {
           if (instrDisasms[i].operandDisasms[j] != expected_instrDisasms[i].operandDisasms[j]) {
                printf("\tFAILURE instr %d operand %d:\t0x%04x, \texpected 0x%04x\n\n", i, j, instrDisasms[i].operandDisasms[j], expected_instrDisasms[i].operandDisasms[j]);
                return -1;
            }
            printf("\tSUCCESS instr %d operand %d:\t0x%04x, \texpected 0x%04x\n", i, j, instrDisasms[i].operandDisasms[j], expected_instrDisasms[i].operandDisasms[j]);
        }
    }

    printf("\tSUCCESS all checks passed!\n\n");
    return 0;
}

void test_disasm_stream_avr_unit_tests(void) {
    int numTests = 0, passedTests = 0;
    struct avrInstructionInfo *(*lookup)(char *) = util_iset_lookup_by_mnemonic;

    /* Check Sample Program */
    /* rjmp .0; ser R16; out $17, R16; out $18, R16; dec R16; rjmp .-6 */
    {
        uint8_t d[] =  {0x00, 0xc0, 0x0f, 0xef, 0x07, 0xbb, 0x08, 0xbb, 0x0a, 0x95, 0xfd, 0xcf};
        uint32_t a[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b};
        struct avrInstructionDisasm dis[] = {
                                                {0x00, {0}, 2, lookup("rjmp"), {0, 0x00}},
                                                {0x02, {0}, 2, lookup("ser"), {16, 0x00}},
                                                {0x04, {0}, 2, lookup("out"), {0x17, 0x10}},
                                                {0x06, {0}, 2, lookup("out"), {0x18, 0x10}},
                                                {0x08, {0}, 2, lookup("dec"), {16, 0x00}},
                                                {0x0a, {0}, 2, lookup("rjmp"), {-6, 0x00}},
                                            };
        if (test_disasm_avr_unit_test_run("Sample Program", &d[0], &a[0], sizeof(d), &dis[0], sizeof(dis)/sizeof(dis[0])) == 0)
            passedTests++;
        numTests++;
    }

    /* Check 32-bit Instructions */
    /* jmp 0x2abab4; call 0x1f00e; sts 0x1234, r2; lds r3, 0x6780 */
    {
        uint8_t d[] =  {0xad, 0x94, 0x5a, 0x5d, 0x0e, 0x94, 0x07, 0xf8, 0x20, 0x92, 0x34, 0x12, 0x30, 0x90, 0x80, 0x67};
        uint32_t a[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
        struct avrInstructionDisasm dis[] = {
                                                {0x00, {0}, 4, lookup("jmp"), {0x2abab4, 0x00}},
                                                {0x04, {0}, 4, lookup("call"), {0x1f00e, 0x00}},
                                                {0x08, {0}, 4, lookup("sts"), {0x2468, 2}},
                                                {0x0c, {0}, 4, lookup("lds"), {3, 0xcf00}},
                                            };
        if (test_disasm_avr_unit_test_run("32-bit Instructions", &d[0], &a[0], sizeof(d), &dis[0], sizeof(dis)/sizeof(dis[0])) == 0)
            passedTests++;
        numTests++;
    }

    /* Check EOF lone byte */
    /* Lone byte due to EOF */
    {
        uint8_t d[] = {0x18};
        uint32_t a[] = {0x500};
        struct avrInstructionDisasm dis[] = {
                                                {0x500, {0}, 1, lookup(".db"), {0x18, 0x00}},
                                            };
        if (test_disasm_avr_unit_test_run("EOF Lone Byte", &d[0], &a[0], sizeof(d), &dis[0], sizeof(dis)/sizeof(dis[0])) == 0)
            passedTests++;
        numTests++;
    }

    /* Check boundary lone byte */
    /* Lone byte due to address change */
    {
        uint8_t d[] = {0x18, 0x12, 0x33};
        uint32_t a[] = {0x500, 0x502, 0x503};
        struct avrInstructionDisasm dis[] = {
                                                {0x500, {0}, 1, lookup(".db"), {0x18, 0x00}},
                                                {0x502, {0}, 2, lookup("cpi"), {0x11, 0x32}},
                                            };
        if (test_disasm_avr_unit_test_run("Boundary Lone Byte", &d[0], &a[0], sizeof(d), &dis[0], sizeof(dis)/sizeof(dis[0])) == 0)
            passedTests++;
        numTests++;
    }

    /* Check EOF lone wide instruction */
    /* Call instruction 0x94ae 0xab XX cut short by EOF */
    {
        uint8_t d[] = {0xae, 0x94, 0xab};
        uint32_t a[] = {0x500, 0x501, 0x502};
        struct avrInstructionDisasm dis[] = {   {0x500, {0}, 2, lookup(".dw"), {0x94ae, 0x00}},
                                                {0x502, {0}, 1, lookup(".db"), {0xab, 0x00}},
                                            };
        if (test_disasm_avr_unit_test_run("EOF Lone Wide Instruction", &d[0], &a[0], sizeof(d), &dis[0], sizeof(dis)/sizeof(dis[0])) == 0)
            passedTests++;
        numTests++;
    }

    /* Check boundary lone wide instruction */
    /* Call instruction 500: 0x94ae | 504: 0xab 0xcd cut short by address change */
    {
        uint8_t d[] = {0xae, 0x94, 0xab, 0xcd};
        uint32_t a[] = {0x500, 0x501, 0x504, 0x505};
        struct avrInstructionDisasm dis[] = {   {0x500, {0}, 2, lookup(".dw"), {0x94ae, 0x00}},
                                                {0x504, {0}, 2, lookup("rjmp"), {-0x4aa, 0x00}},
                                            };
        if (test_disasm_avr_unit_test_run("Boundary Lone Wide Instruction", &d[0], &a[0], sizeof(d), &dis[0], sizeof(dis)/sizeof(dis[0])) == 0)
            passedTests++;
        numTests++;
    }

    printf("%d / %d tests passed.\n", passedTests, numTests);
}

