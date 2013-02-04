#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include <byte_stream.h>
#include <disasm_stream.h>
#include <print_stream.h>

/* File Support */
#include "file/file_support.h"

/* AVR Support */
#include "avr/avr_support.h"

/* Supported file types */
enum {
    FILE_TYPE_ATMEL_GENERIC,
    FILE_TYPE_INTEL_HEX,
    FILE_TYPE_MOTOROLA_SRECORD,
    FILE_TYPE_BINARY,
    FILE_TYPE_ASCII_HEX,
};

/* Supported architectures */
enum {
    ARCH_AVR8,
    ARCH_PIC_MIDRANGE,
};

/* getopt flags for some long options that don't have a short option equivalent */
static int no_addresses = 0;            /* Flag for --no-addresses */
static int no_destination_comments = 0; /* Flag for --no-destination-comments */
static int no_opcodes = 0;              /* Flag for --no-opcodes */
static int assembly = 0;                /* Flag for --assembly */
static int data_base = 0;               /* Base of data constants (hexadecimal, binary, decimal) */

/* Supported data constant bases */
enum {
    DATA_BASE_HEX,
    DATA_BASE_BIN,
    DATA_BASE_DEC,
};

static struct option long_options[] = {
    {"file-type", required_argument, NULL, 't'},
    {"out-file", required_argument, NULL, 'o'},
    {"assembly", no_argument, &assembly, 1},
    {"data-base-hex", no_argument, &data_base, DATA_BASE_HEX},
    {"data-base-bin", no_argument, &data_base, DATA_BASE_BIN},
    {"data-base-dec", no_argument, &data_base, DATA_BASE_DEC},
    {"no-opcodes", no_argument, &no_opcodes, 1},
    {"no-addresses", no_argument, &no_addresses, 1},
    {"no-destination-comments", no_argument, &no_destination_comments, 1},
    {"help", no_argument, NULL, 'h'},
    {"version", no_argument, NULL, 'v'},
    {NULL, 0, NULL, 0}
};

static void printUsage(const char *programName) {
    printf("Usage: %s <option(s)> <file>\n", programName);
    printf("Disassembles program file <file>. Use - for standard input.\n\n");
    printf("vAVRdisasm version 3.0 - 02/01/2013.\n");
    printf("Written by Vanya A. Sergeev - <vsergeev@gmail.com>.\n\n");
    printf("Additional Options:\n\
  -o, --out-file <file>         Write to file instead of standard output.\n\
\n\
  -t, --file-type <type>        Specify file type of the program file.\n\
\n\
  --assembly                    Produce assemble-able code with address labels.\n\
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
  Atmel Generic             generic\n\
  Intel HEX8                ihex\n\
  Motorola S-Record         srec\n\
  Raw Binary                binary\n\
  ASCII Hex                 ascii\n\n");
}

static void printVersion(void) {
    printf("vAVRdisasm version 3.0 - 02/01/2013.\n");
    printf("Written by Vanya Sergeev - <vsergeev@gmail.com>\n");
}

void print_stream_error_trace(struct PrintStream *ps, struct DisasmStream *ds, struct ByteStream *bs) {
    if (ps->error == NULL)
        fprintf(stderr, "\tPrint Stream Error: No error\n");
    else
        fprintf(stderr, "\tPrint Stream Error: %s\n", ps->error);

    if (ds->error == NULL)
        fprintf(stderr, "\tDisasm Stream Error: No error\n");
    else
        fprintf(stderr, "\tDisasm Stream Error: %s\n", ds->error);

    if (bs->error == NULL)
        fprintf(stderr, "\tByte Stream Error: No error\n");
    else
        fprintf(stderr, "\tByte Stream Error: %s\n", bs->error);

    fprintf(stderr, "\tPlease file an issue at https://github.com/vsergeev/vAVRdisasm/issues\n\tor email the author!\n\n");
}

int main(int argc, const char *argv[]) {
    /* User Options */
    int optc;
    char arch_str[16] = {0};
    char file_type_str[8] = {0};
    char file_out_str[4096] = {0};

    /* Input / Output files */
    FILE *file_in = NULL, *file_out = NULL;

    /* Disassembler Streams */
    int file_type = 0;
    int arch = ARCH_AVR8;
    int flags = 0;
    struct ByteStream bs;
    struct DisasmStream ds;
    struct PrintStream ps;
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
            case 'a':
                strncpy(arch_str, optarg, sizeof(arch_str));
                break;
            case 't':
                strncpy(file_type_str, optarg, sizeof(file_type_str));
                break;
            case 'o':
                if (strcmp(optarg, "-") != 0)
                    strncpy(file_out_str, optarg, sizeof(file_out_str));
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
        else if (strcasecmp(file_type_str, "ascii") == 0)
            file_type = FILE_TYPE_ASCII_HEX;
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
    if (file_out_str[0] != '\0') {
        file_out = fopen(file_out_str, "w");
        if (file_out == NULL) {
            perror("Error opening output file for writing");
            goto cleanup_exit_failure;
        }
    } else {
    /* Otherwise, default the output file to stdout */
        file_out = stdout;
    }

    /*** Debug ***/

    #ifdef DEBUG_BYTE_STREAM
        /* Test opcode stream */
        if (file_type == FILE_TYPE_ATMEL_GENERIC)
            test_byte_stream(file_in, byte_stream_generic_init, byte_stream_generic_close, byte_stream_generic_read);
        else if (file_type == FILE_TYPE_INTEL_HEX)
            test_byte_stream(file_in, byte_stream_ihex_init, byte_stream_ihex_close, byte_stream_ihex_read);
        else if (file_type == FILE_TYPE_MOTOROLA_SRECORD)
            test_byte_stream(file_in, byte_stream_srecord_init, byte_stream_srecord_close, byte_stream_srecord_read);
        else if (file_type == FILE_TYPE_BINARY)
            test_byte_stream(file_in, byte_stream_binary_init, byte_stream_binary_close, byte_stream_binary_read);
        goto cleanup_exit_success;
    #elif defined DEBUG_DISASM_STREAM
        /* Test Disasm Stream */
        test_disasm_stream_unit_tests();
        goto cleanup_exit_success;
    #elif defined DEBUG_PRINT_STREAM
        /* Test Print Stream */
        test_print_stream();
        goto cleanup_exit_success;
    #endif

    /*** Setup Formatting Flags ***/
    if (!no_addresses)
        flags |= PRINT_FLAG_ADDRESSES;
    if (!no_destination_comments)
        flags |= PRINT_FLAG_DESTINATION_COMMENT;
    if (!no_opcodes)
        flags |= PRINT_FLAG_OPCODES;

    if (data_base == DATA_BASE_BIN)
        flags |= PRINT_FLAG_DATA_BIN;
    else if (data_base == DATA_BASE_DEC)
        flags |= PRINT_FLAG_DATA_DEC;
    else
        flags |= PRINT_FLAG_DATA_HEX;

    if (assembly)
        flags |= PRINT_FLAG_ASSEMBLY;

    /*** Setup disassembler streams ***/

    /* Setup the Byte Stream */
    bs.in = file_in;
    if (file_type == FILE_TYPE_ATMEL_GENERIC) {
        bs.stream_init = byte_stream_generic_init;
        bs.stream_close = byte_stream_generic_close;
        bs.stream_read = byte_stream_generic_read;
    } else if (file_type == FILE_TYPE_INTEL_HEX) {
        bs.stream_init = byte_stream_ihex_init;
        bs.stream_close = byte_stream_ihex_close;
        bs.stream_read = byte_stream_ihex_read;
    } else if (file_type == FILE_TYPE_MOTOROLA_SRECORD) {
        bs.stream_init = byte_stream_srecord_init;
        bs.stream_close = byte_stream_srecord_close;
        bs.stream_read = byte_stream_srecord_read;
    } else if (file_type == FILE_TYPE_ASCII_HEX) {
        bs.stream_init = byte_stream_asciihex_init;
        bs.stream_close = byte_stream_asciihex_close;
        bs.stream_read = byte_stream_asciihex_read;
    } else {
        bs.stream_init = byte_stream_binary_init;
        bs.stream_close = byte_stream_binary_close;
        bs.stream_read = byte_stream_binary_read;
    }

    /* Setup the Disasm Stream */
    ds.in = &bs;
    if (arch == ARCH_AVR8) {
        ds.stream_init = disasm_stream_avr_init;
        ds.stream_close = disasm_stream_avr_close;
        ds.stream_read = disasm_stream_avr_read;
    }

    /* Setup the Print Stream */
    ps.in = &ds;
    ps.stream_init = print_stream_init;
    ps.stream_close = print_stream_close;
    ps.stream_read = print_stream_read;

    /* Initialize streams */
    if ((ret = ps.stream_init(&ps, flags)) < 0) {
        fprintf(stderr, "Error initializing streams! Error code: %d\n", ret);
        print_stream_error_trace(&ps, &ds, &bs);
        goto cleanup_exit_failure;
    }

    /* Read from Print Stream until EOF */
    while ( (ret = ps.stream_read(&ps, file_out)) != STREAM_EOF ) {
        if (ret < 0) {
            fprintf(stderr, "Error occured during disassembly! Error code: %d\n", ret);
            print_stream_error_trace(&ps, &ds, &bs);
            goto cleanup_exit_failure;
        }
    }

    /* Close streams */
    if ((ret = ps.stream_close(&ps)) < 0) {
        fprintf(stderr, "Error closing streams! Error code: %d\n", ret);
        print_stream_error_trace(&ps, &ds, &bs);
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
    exit(EXIT_FAILURE);
}

