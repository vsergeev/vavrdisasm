/*
 *  atmel_generic.c
 *  Utility functions to create, read, write, and print Atmel Generic binary records.
 *
 *  Written by Vanya A. Sergeev <vsergeev@gmail.com>
 *  Version 1.0.5 - February 2011
 *
 */

#include "atmel_generic.h"

/* Initializes a new AtmelGenericRecord structure that the paramater genericRecord points to with the passed
 * 24-bit integer address, and 16-bit data word. */
int New_AtmelGenericRecord(uint32_t address, uint16_t data, AtmelGenericRecord *genericRecord) {
	/* Assert genericRecord pointer */
	if (genericRecord == NULL)
		return ATMEL_GENERIC_ERROR_INVALID_ARGUMENTS;
	
	genericRecord->address = address;
	genericRecord->data = data;
	return ATMEL_GENERIC_OK;
}


/* Utility function to read an Atmel Generic record from a file */
int Read_AtmelGenericRecord(AtmelGenericRecord *genericRecord, FILE *in) {
	char recordBuff[ATMEL_GENERIC_RECORD_BUFF_SIZE];
	int i;
	
	/* Check our record pointer and file pointer */
	if (genericRecord == NULL || in == NULL)
		return ATMEL_GENERIC_ERROR_INVALID_ARGUMENTS;
		
	if (fgets(recordBuff, ATMEL_GENERIC_RECORD_BUFF_SIZE, in) == NULL) {
			/* In case we hit EOF, don't report a file error */
			if (feof(in) != 0)
				return ATMEL_GENERIC_ERROR_EOF;
			else
				return ATMEL_GENERIC_ERROR_FILE;
	}
	/* Null-terminate the string at the first sign of a \r or \n */
	for (i = 0; i < (int)strlen(recordBuff); i++) {
		if (recordBuff[i] == '\r' || recordBuff[i] == '\n') {
			recordBuff[i] = 0;
			break;
		}
	}

	/* Check if we hit a newline */
	if (strlen(recordBuff) == 0)
		return ATMEL_GENERIC_ERROR_NEWLINE;
	
	/* Size check that the record has the address, data, and start code */
	if (strlen(recordBuff) < ATMEL_GENERIC_ADDRESS_LEN + ATMEL_GENERIC_DATA_LEN + 1)
		return ATMEL_GENERIC_ERROR_INVALID_RECORD;
	
	/* Check for the record "start code" (the colon that separates the address and data */
	if (recordBuff[ATMEL_GENERIC_SEPARATOR_OFFSET] != ATMEL_GENERIC_SEPARATOR)
		return ATMEL_GENERIC_ERROR_INVALID_RECORD;
	
	/* Replace the colon "start code" with a 0 so we can convert the ascii hex encoded
	 * address up to this point */
	recordBuff[ATMEL_GENERIC_SEPARATOR_OFFSET] = 0;
	genericRecord->address = strtol(recordBuff, (char **)NULL, 16);
	
	/* Convert the rest of the data past the colon, this string has been null terminated at
	 * the end already */
	genericRecord->data = strtol(recordBuff+ATMEL_GENERIC_SEPARATOR_OFFSET+1, (char **)NULL, 16);
	
	return ATMEL_GENERIC_OK;
}

/* Utility function to write an Atmel Generic record to a file */
int Write_AtmelGenericRecord(const AtmelGenericRecord *genericRecord, FILE *out) {
	/* Check our record pointer and file pointer */
	if (genericRecord == NULL || out == NULL)
		return ATMEL_GENERIC_ERROR_INVALID_ARGUMENTS;
	
	if (fprintf(out, "%2.6X%c%2.4X\r\n", genericRecord->address, ATMEL_GENERIC_SEPARATOR, genericRecord->data) < 0)
		return ATMEL_GENERIC_ERROR_FILE;
	
	return ATMEL_GENERIC_OK;
}

/* Utility function to print the information stored in an Atmel Generic record */
void Print_AtmelGenericRecord(const AtmelGenericRecord *genericRecord) {
	printf("Atmel Generic Address: \t0x%2.6X\n", genericRecord->address);
	printf("Atmel Generic Data: \t0x%2.4X\n", genericRecord->data); 
}

