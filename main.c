#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "opcode_stream.h"
#include "disasm_stream.h"
#include "format_stream.h"

//#define DEBUG_OPCODE_STREAM
//#define DEBUG_DISASM_STREAM
//#define DEBUG_FORMAT_STREAM

/* Supported file types */
enum {
    FILE_TYPE_ATMEL_GENERIC,
    FILE_TYPE_INTEL_HEX,
    FILE_TYPE_MOTOROLA_SRECORD,
    FILE_TYPE_BINARY,
};

/* AVRASM format prefixes */
char *format_prefixes_avrasm[] = {
    [AVR_FORMAT_PREFIX_REGISTER]            = "R",  /* mov R0, R2 */
    [AVR_FORMAT_PREFIX_IO_REGISTER]         = "$",  /* out $39, R16 */
    [AVR_FORMAT_PREFIX_DATA_HEX]            = "0x", /* ldi R16, 0x3d */
    [AVR_FORMAT_PREFIX_DATA_BIN]            = "0b", /* ldi R16, 0b00111101 */
    [AVR_FORMAT_PREFIX_DATA_DEC]            = "",   /* ldi R16, 61 */
    [AVR_FORMAT_PREFIX_BIT]                 = "",   /* cbi $12, 7 */
    [AVR_FORMAT_PREFIX_ADDRESS_LABEL]       = "A_", /* A_0010: ... */
    [AVR_FORMAT_PREFIX_ABSOLUTE_ADDRESS]    = "0x", /* call 0x1234 */
    [AVR_FORMAT_PREFIX_RELATIVE_ADDRESS]    = ".",  /* rjmp .4 */
    [AVR_FORMAT_PREFIX_DES_ROUND]           = "0x", /* des 0x01 */
    [AVR_FORMAT_PREFIX_RAW_WORD]            = "0x", /* .dw 0xabcd */
    [AVR_FORMAT_PREFIX_RAW_BYTE]            = "0x", /* .db 0xab */
    };
/* Objdump format prefixes */
char *format_prefixes_objdump[] = {
    [AVR_FORMAT_PREFIX_REGISTER]            = "r",  /* mov r0, r2 */
    [AVR_FORMAT_PREFIX_IO_REGISTER]         = "0x", /* out 0x39, r16 */
    [AVR_FORMAT_PREFIX_DATA_HEX]            = "0x", /* ldi r16, 0x3d */
    [AVR_FORMAT_PREFIX_DATA_BIN]            = "0b", /* ldi r16, 0b00111101 */
    [AVR_FORMAT_PREFIX_DATA_DEC]            = "",   /* ldi r16, 61 */
    [AVR_FORMAT_PREFIX_BIT]                 = "",   /* cbi 0x12, 7 */
    [AVR_FORMAT_PREFIX_ADDRESS_LABEL]       = "A_", /* A_0010: ... */
    [AVR_FORMAT_PREFIX_ABSOLUTE_ADDRESS]    = "0x", /* call 0x1234 */
    [AVR_FORMAT_PREFIX_RELATIVE_ADDRESS]    = ".",  /* rjmp .4 */
    [AVR_FORMAT_PREFIX_DES_ROUND]           = "",   /* des 1 */
    [AVR_FORMAT_PREFIX_RAW_WORD]            = "0x", /* .dw 0xabcd */
    [AVR_FORMAT_PREFIX_RAW_BYTE]            = "0x", /* .db 0xab */
    };


/* getopt flags for some long options that don't have a short option equivalent */
static int no_addresses = 0;            /* Flag for --no-addresses */
static int no_destination_comments = 0; /* Flag for --no-destination-comments */
static int no_opcodes = 0;              /* Flag for --no-opcodes */
static int data_base = 0;               /* Base of data constants (hexadecimal, binary, decimal) */

static struct option long_options[] = {
    {"address-label", required_argument, NULL, 'l'},
    {"file-type", required_argument, NULL, 't'},
    {"out-file", required_argument, NULL, 'o'},
    {"data-base-hex", no_argument, &data_base, AVR_FORMAT_FLAG_DATA_HEX},
    {"data-base-bin", no_argument, &data_base, AVR_FORMAT_FLAG_DATA_BIN},
    {"data-base-dec", no_argument, &data_base, AVR_FORMAT_FLAG_DATA_DEC},
    {"no-opcodes", no_argument, &no_opcodes, 1},
    {"no-addresses", no_argument, &no_addresses, 1},
    {"no-destination-comments", no_argument, &no_destination_comments, 1},
    {"help", no_argument, NULL, 'h'},
    {"version", no_argument, NULL, 'v'},
    {NULL, 0, NULL, 0}
};

static void printUsage(const char *programName) {
    printf("Usage: %s <option(s)> <file>\n", programName);
    printf("Disassembles AVR program file <file>. Use - for standard input.\n\n");
    printf("vAVRdisasm version 3.0 - 02/01/2013.\n");
    printf("Written by Vanya A. Sergeev - <vsergeev@gmail.com>.\n\n");
    printf("Additional Options:\n\
  -o, --out-file <file>         Write to file instead of standard output.\n\
\n\
  -t, --file-type <type>        Specify file type of the program file.\n\
\n\
  -l, --address-label <prefix>  Create ghetto address labels with\n\
                                  the specified label prefix.\n\
\n\
  --data-base-hex               Represent data constants in hexadecimal\n\
                                  (default).\n\
  --data-base-bin               Represent data constants in binary.\n\
  --data-base-dec               Represent data constants in decimal.\n\
\n\
  --no-addresses                Do not display address alongside disassembly.\n\
  --no-opcodes                  Do not display original opcode alongside\n\
                                  disassembly.\n\
  --no-destination-comments     Do not display destination address comments\n\
                                  of relative branch/jump/call instructions.\n\
\n\
  -h, --help                    Display this usage/help.\n\
  -v, --version                 Display the program's version.\n\n");
    printf("Supported file types:\n\
  Atmel Generic         generic\n\
  Intel HEX8            ihex\n\
  Motorola S-Record     srec\n\
  Raw Binary            binary\n\n");
}

static void printVersion(void) {
    printf("vAVRdisasm version 3.0 - 02/01/2013.\n");
    printf("Written by Vanya Sergeev - <vsergeev@gmail.com>\n");
}

void print_stream_error_trace(struct FormatStream *fs, struct DisasmStream *ds, struct OpcodeStream *os) {
    if (fs->error == NULL)
        fprintf(stderr, "\tFormat Stream Error: No error\n");
    else
        fprintf(stderr, "\tFormat Stream Error: %s\n", fs->error);

    if (ds->error == NULL)
        fprintf(stderr, "\tDisasm Stream Error: No error\n");
    else
        fprintf(stderr, "\tDisasm Stream Error: %s\n", ds->error);

    if (os->error == NULL)
        fprintf(stderr, "\tOpcode Stream Error: No error\n");
    else
        fprintf(stderr, "\tOpcode Stream Error: %s\n", os->error);

    fprintf(stderr, "\tPlease file an issue at https://github.com/vsergeev/vAVRdisasm/issues\n\tor email the author!");
}

int main(int argc, const char *argv[]) {
    int optc;
    /* Input / Output files */
    char file_type_str[8] = {0};
    char *file_out_path = NULL;
    FILE *file_in = NULL, *file_out = NULL;
    int file_type = 0;
    /* Formatting options */
    int format_flags;
    int format_address_width;
    char **format_prefixes;
    char format_address_label[8] = {0};
    /* Disassembler Streams */
    struct OpcodeStream os;
    struct DisasmStream ds;
    struct FormatStream fs;
    int ret;

    /* Parse command line options */
    while (1) {
        optc = getopt_long(argc, (char * const *)argv, "o:t:l:hv", long_options, NULL);
        if (optc == -1)
            break;
        switch (optc) {
            /* Long option */
            case 0:
                break;
            case 'l':
                strncpy(format_address_label, optarg, sizeof(format_address_label));
                break;
            case 't':
                strncpy(file_type_str, optarg, sizeof(file_type_str));
                break;
            case 'o':
                if (strcmp(optarg, "-") != 0)
                    file_out_path = strdup(optarg);
                break;
            case 'h':
                printUsage(argv[0]);
                exit(EXIT_SUCCESS);
            case 'v':
                printVersion();
                exit(EXIT_SUCCESS);
            default:
                printUsage(argv[0]);
                exit(EXIT_SUCCESS);
        }
    }

    /* If there are no more arguments left */
    if (optind == argc) {
        fprintf(stderr, "Error: No program file specified! Use - for standard input.\n\n");
        printUsage(argv[0]);
        goto cleanup_exit_failure;
    }

    /*** Open input file ***/

    /* Support reading from stdin with filename "-" */
    if (strcmp(argv[optind], "-") == 0) {
        file_in = stdin;
    } else {
    /* Otherwise, open the specified input file */
        file_in = fopen(argv[optind], "r");
        if (file_in == NULL) {
            perror("Error: Cannot open program file for disassembly");
            goto cleanup_exit_failure;
        }
    }

    /*** Determine input file type ***/

    /* If a file type was specified */
    if (file_type_str[0] != '\0') {
        if (strcasecmp(file_type_str, "generic") == 0)
            file_type = FILE_TYPE_ATMEL_GENERIC;
        else if (strcasecmp(file_type_str, "ihex") == 0)
            file_type = FILE_TYPE_INTEL_HEX;
        else if (strcasecmp(file_type_str, "srec") == 0)
            file_type = FILE_TYPE_MOTOROLA_SRECORD;
        else if (strcasecmp(file_type_str, "binary") == 0)
            file_type = FILE_TYPE_BINARY;
        else {
            fprintf(stderr, "Unknown file type %s.\n", file_type_str);
            fprintf(stderr, "See program help/usage for supported file types.\n");
            goto cleanup_exit_failure;
        }
    } else {
    /* Otherwise, attempt to auto-detect file type by first character */
        int c;
        c = fgetc(file_in);
        /* Intel HEX8 record statements start with : */
        if ((char)c == ':')
            file_type = FILE_TYPE_INTEL_HEX;
        /* Motorola S-Record record statements start with S */
        else if ((char)c == 'S')
            file_type = FILE_TYPE_MOTOROLA_SRECORD;
        /* Atmel Generic record statements start with a ASCII hex digit */
        else if ( ((char)c >= '0' && (char)c <= '9') || ((char)c >= 'a' && (char)c <= 'f') || ((char)c >= 'A' && (char)c <= 'F') )
            file_type = FILE_TYPE_ATMEL_GENERIC;
        else {
            fprintf(stderr, "Unable to auto-recognize file type by first character.\n");
            fprintf(stderr, "Please specify file type with -t / --file-type option.\n");
            goto cleanup_exit_failure;
        }
        ungetc(c, file_in);
    }

    /*** Open output file ***/

    /* If an output file was specified */
    if (file_out_path != NULL) {
        file_out = fopen(file_out_path, "w");
        if (file_out == NULL) {
            perror("Error opening output file for writing");
            goto cleanup_exit_failure;
        }
        free(file_out_path);
        file_out_path = NULL;
    } else {
    /* Otherwise, default the output file to stdout */
        file_out = stdout;
    }

    /*** Formatting Options ***/

    /* Set up formatting flags */
    format_flags = 0;

    if (!no_addresses)
        format_flags |= AVR_FORMAT_FLAG_ADDRESSES;
    if (!no_destination_comments)
        format_flags |= AVR_FORMAT_FLAG_DEST_ADDR_COMMENT;
    if (!no_opcodes)
        format_flags |= AVR_FORMAT_FLAG_OPCODES;

    if (data_base == AVR_FORMAT_FLAG_DATA_BIN)
        format_flags |= AVR_FORMAT_FLAG_DATA_BIN;
    else if (data_base == AVR_FORMAT_FLAG_DATA_DEC)
        format_flags |= AVR_FORMAT_FLAG_DATA_DEC;
    else
        format_flags |= AVR_FORMAT_FLAG_DATA_HEX;

    /* Set up formatting prefixes */
    format_prefixes = format_prefixes_avrasm;

    if (format_address_label[0] != '\0') {
        format_flags |= AVR_FORMAT_FLAG_ADDRESS_LABELS;
        format_prefixes[AVR_FORMAT_PREFIX_ADDRESS_LABEL] = &format_address_label[0];
    }

    /* Set up formatting address width */
    format_address_width = 4;

    /*** Debug ***/

    #ifdef DEBUG_OPCODE_STREAM
        /* Test opcode stream */
        if (file_type == FILE_TYPE_ATMEL_GENERIC)
            test_opcode_stream(file_in, opcode_stream_generic_init, opcode_stream_generic_close, opcode_stream_generic_read);
        else if (file_type == FILE_TYPE_INTEL_HEX)
            test_opcode_stream(file_in, opcode_stream_ihex_init, opcode_stream_ihex_close, opcode_stream_ihex_read);
        else if (file_type == FILE_TYPE_MOTOROLA_SRECORD)
            test_opcode_stream(file_in, opcode_stream_srecord_init, opcode_stream_srecord_close, opcode_stream_srecord_read);
        else if (file_type == FILE_TYPE_BINARY)
            test_opcode_stream(file_in, opcode_stream_binary_init, opcode_stream_binary_close, opcode_stream_binary_read);
        goto cleanup_exit_success;
    #elif defined DEBUG_DISASM_STREAM
        /* Test Disasm Stream */
        test_disasm_stream_unit_tests();
        goto cleanup_exit_success;
    #elif defined DEBUG_FORMAT_STREAM
        /* Test Format Stream */
        test_format_stream();
        goto cleanup_exit_success;
    #endif

    /*** Setup disassembler streams ***/

    /* Setup the Opcode Stream */
    os.in = file_in;
    if (file_type == FILE_TYPE_ATMEL_GENERIC) {
        os.stream_init = opcode_stream_generic_init;
        os.stream_close = opcode_stream_generic_close;
        os.stream_read = opcode_stream_generic_read;
    } else if (file_type == FILE_TYPE_INTEL_HEX) {
        os.stream_init = opcode_stream_ihex_init;
        os.stream_close = opcode_stream_ihex_close;
        os.stream_read = opcode_stream_ihex_read;
    } else if (file_type == FILE_TYPE_MOTOROLA_SRECORD) {
        os.stream_init = opcode_stream_srecord_init;
        os.stream_close = opcode_stream_srecord_close;
        os.stream_read = opcode_stream_srecord_read;
    } else {
        os.stream_init = opcode_stream_binary_init;
        os.stream_close = opcode_stream_binary_close;
        os.stream_read = opcode_stream_binary_read;
    }

    /* Setup the Disasm Stream */
    ds.in = &os;
    ds.stream_init = disasm_stream_avr_init;
    ds.stream_close = disasm_stream_avr_close;
    ds.stream_read = disasm_stream_avr_read;

    /* Setup the Format Stream */
    fs.in = &ds;
    fs.stream_init = format_stream_avr_init;
    fs.stream_close = format_stream_avr_close;
    fs.stream_read = format_stream_avr_read;

    /* Initialize streams */
    if ((ret = fs.stream_init(&fs)) < 0) {
        fprintf(stderr, "Error initializing streams! Error code: %d\n", ret);
        print_stream_error_trace(&fs, &ds, &os);
        goto cleanup_exit_failure;
    }

    /* Load format options into the Format Stream */
    struct format_stream_avr_state *options;
    options = (struct format_stream_avr_state *)fs.state;
    options->flags = format_flags;
    options->address_width = format_address_width;
    options->prefixes = format_prefixes;

    /* Read from Format Stream until EOF */
    while ( (ret = fs.stream_read(&fs, file_out)) != STREAM_EOF ) {
        if (ret < 0) {
            fprintf(stderr, "Error occured during disassembly! Error code: %d\n", ret);
            print_stream_error_trace(&fs, &ds, &os);
            goto cleanup_exit_failure;
        }
    }

    /* Close streams */
    if ((ret = fs.stream_close(&fs)) < 0) {
        fprintf(stderr, "Error closing streams! Error code: %d\n", ret);
        print_stream_error_trace(&fs, &ds, &os);
        goto cleanup_exit_failure;
    }

    cleanup_exit_success:
    if (file_out != stdout && file_out != NULL)
        fclose(file_out);
    exit(EXIT_SUCCESS);

    cleanup_exit_failure:
    if (file_in != stdin && file_in != NULL)
        fclose(file_in);
    if (file_out != stdout && file_out != NULL)
        fclose(file_out);
    if (file_out_path != NULL)
        free(file_out_path);
    exit(EXIT_FAILURE);
}

