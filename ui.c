/*
 * vAVRdisasm - AVR program disassembler.
 * Written by Vanya A. Sergeev - <vsergeev@gmail.com>
 *
 * Copyright (C) 2007-2011 Vanya A. Sergeev
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA. 
 *
 * ui.c - Main user interface to AVR disassembler. Takes care of program
 *  arguments, setting disassembly formatting options, and program file
 *  type recognition. 
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "file.h"
#include "errorcodes.h"

/* Flags for some long options that don't have a short option equivilant */
static int no_addresses = 0;			/* Flag for --no-addresses */
static int no_destination_comments = 0;		/* Flag for --no-destination-comments */
static int data_base = 0;			/* Base of data constants (hexadecimal, binary, decimal) */

static struct option long_options[] = {
	{"address-label", required_argument, NULL, 'l'},
	{"file-type", required_argument, NULL, 't'},
	{"out-file", required_argument, NULL, 'o'},
	{"data-base-hex", no_argument, &data_base, FORMAT_OPTION_DATA_HEX},
	{"data-base-bin", no_argument, &data_base, FORMAT_OPTION_DATA_BIN},
	{"data-base-dec", no_argument, &data_base, FORMAT_OPTION_DATA_DEC},
	{"no-addresses", no_argument, &no_addresses, 1},
	{"no-destination-comments", no_argument, &no_destination_comments, 1},
	{"help", no_argument, NULL, 'h'},
	{"version", no_argument, NULL, 'v'},
	{NULL, 0, NULL, 0}
};

static void printUsage(FILE *stream, const char *programName) {
	fprintf(stream, "Usage: %s <option(s)> <file>\n", programName);
	fprintf(stream, " Disassembles AVR program file <file>. Use - for standard input.\n");
	fprintf(stream, " Written by Vanya A. Sergeev - <vsergeev@gmail.com>.\n\n");
	fprintf(stream, " Additional Options:\n\
  -o, --out-file <output file>	Write to output file instead of standard output.\n\
  -t, --file-type <type>	Specify the file type of the object file.\n\
  -l, --address-label <prefix> 	Create ghetto address labels with \n\
				the specified label prefix.\n\
  --data-base-hex		Represent data constants in hexadecimal\n\
				(default).\n\
  --data-base-bin		Represent data constants in binary.\n\
  --data-base-dec		Represent data constants in decimal.\n\
  --no-addresses		Do not display the address alongside\n\
				disassembly.\n\
  --no-destination-comments	Do not display the destination address\n\
				comments of relative branch/jump/call\n\
				instructions.\n\
  -h, --help			Display this usage/help.\n\
  -v, --version			Display the program's version.\n\n");
	fprintf(stream, "Supported file types:\n\
  Atmel Generic - File Type: generic\n\
  Auto-recognized with .gen file extension.\n\n\
  Intel HEX8 - File Type: ihex\n\
  Auto-recognized with .hex, .ihex, and .ihx file extensions.\n\n\
  Motorola S-Record - File Type: srecord\n\
  Auto-recognized with .srec and .sre file extensions.\n\n");
}

static void printVersion(FILE *stream) {
	fprintf(stream, "vAVRdisasm version 1.8 - 01/26/2011.\n");
	fprintf(stream, "Written by Vanya Sergeev - <vsergeev@gmail.com>\n");
}
	
int main (int argc, const char *argv[]) {
	int optc;
	FILE *fileIn, *fileOut;
	char fileType[8], *fileExtension;
	int (*disassembleFile)(FILE *, FILE *, formattingOptions);
	formattingOptions fOptions;

	/* Recent flag options */
	fOptions.options = 0;
	/* Set default address field width for this version. */
	fOptions.addressFieldWidth = 3;
	/* Default to stdout output file */
	fileOut = stdout;

	fileType[0] = '\0';	
	while (1) {
		optc = getopt_long(argc, (char * const *)argv, "o:t:l:hv", long_options, NULL);
		if (optc == -1)
			break;
		switch (optc) {
			/* Long option */
			case 0:
				break;
			case 'l':
				fOptions.options |= FORMAT_OPTION_ADDRESS_LABEL;
				strncpy(fOptions.addressLabelPrefix, optarg, sizeof(fOptions.addressLabelPrefix));
				break;
			case 't':
				strncpy(fileType, optarg, sizeof(fileType));
				break;
			case 'o':
				if (strcmp(optarg, "-") != 0)
					fileOut = fopen(optarg, "w");
				break;
			case 'h':
				printUsage(stderr, argv[0]);
				exit(EXIT_SUCCESS);	
			case 'v':
				printVersion(stderr);
				exit(EXIT_SUCCESS);
			default:
				printUsage(stdout, argv[0]);
				exit(EXIT_SUCCESS);
		}
	}

	if (!no_addresses)
		fOptions.options |= FORMAT_OPTION_ADDRESS;
	if (!no_destination_comments)
		fOptions.options |= FORMAT_OPTION_DESTINATION_ADDRESS_COMMENT;

	if (data_base == FORMAT_OPTION_DATA_BIN)
		fOptions.options |= FORMAT_OPTION_DATA_BIN;
	else if (data_base == FORMAT_OPTION_DATA_DEC)
		fOptions.options |= FORMAT_OPTION_DATA_DEC;
	else
		fOptions.options |= FORMAT_OPTION_DATA_HEX;

	if (fileOut == NULL) {
		perror("Error: Cannot open output file for writing");
		exit(EXIT_FAILURE);
	}
		
	if (optind == argc) {
		fprintf(stderr, "Error: No program file specified! Use - for standard input.\n\n");
		printUsage(stderr, argv[0]);
		exit(EXIT_FAILURE);
	}

	/* If no file type was specified, try to auto-recognize the file type by extension. */
	if (fileType[0] == '\0' && strlen(argv[optind]) > 1) {
		fileExtension = rindex(argv[optind], '.');
		if (fileExtension == NULL) {
			fprintf(stderr, "Unable to auto-recognize file type by extension.\n");
			fprintf(stderr, "Please specify file type with -t,--file-type option.\n");
			exit(EXIT_FAILURE);
		}
		/* To skip the dot */
		fileExtension++;
		if (strcasecmp(fileExtension, "gen") == 0) 
			strcpy(fileType, "generic");
		else if (strcasecmp(fileExtension, "ihx") == 0)
			strcpy(fileType, "ihex");
		else if (strcasecmp(fileExtension, "hex") == 0)
			strcpy(fileType, "ihex");
		else if (strcasecmp(fileExtension, "ihex") == 0)
			strcpy(fileType, "ihex");
		else if (strcasecmp(fileExtension, "srec") == 0)
			strcpy(fileType, "srecord");
		else if (strcasecmp(fileExtension, "sre") == 0)
			strcpy(fileType, "srecord");
		else {
			fprintf(stderr, "Unable to auto-recognize file type by extension.\n");
			fprintf(stderr, "Please specify file type with -t,--file-type option.\n");
			exit(EXIT_FAILURE);
		}
	}

	/* I check the fileType string here and set the disassembleFile
	 * pointer so I don't have to do any file type error checking after
	 * I've opened the file, this means cleaner error handling. */
	if (strcasecmp(fileType, "generic") == 0)
		disassembleFile = disassembleGenericFile;
	else if (strcasecmp(fileType, "ihex") == 0)
		disassembleFile = disassembleIHexFile;
	else if (strcasecmp(fileType, "srecord") == 0)
		disassembleFile = disassembleSRecordFile;
	else {
		if (fileType[0] != '\0')
			fprintf(stderr, "Unknown file type %s.\n", fileType);
		else
			fprintf(stderr, "Unspecified file type.\n");
		fprintf(stderr, "See program help/usage for supported file types.\n");
		exit(EXIT_FAILURE);
	}

	/* Support reading from stdin with filename "-" */
	if (strcmp(argv[optind], "-") == 0) {
		fileIn = stdin;
	} else {
		fileIn = fopen(argv[optind], "r");
		if (fileIn == NULL) {
			perror("Error: Cannot open program file for disassembly");
			exit(EXIT_FAILURE);
		}
	}

	disassembleFile(fileOut, fileIn, fOptions);

	fclose(fileOut);	
	fclose(fileIn);

	exit(EXIT_SUCCESS);
}

