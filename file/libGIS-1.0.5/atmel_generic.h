#ifndef ATMEL_GENERIC_H
#define ATMEL_GENERIC_H
/**
 * \file atmel_generic.h
 * \brief Low-level utility functions to create, read, write, and print Atmel Generic binary records.
 * \author Vanya A. Sergeev <vsergeev@gmail.com>
 * \date February 2011
 * \version 1.0.5
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* General definition of the Atmel Generic record specifications. */
enum _AtmelGenericDefinitions {
	/* 16 should be plenty of space to read in an Atmel Generic record (11 bytes in reality) */
	ATMEL_GENERIC_RECORD_BUFF_SIZE = 16,
	/* Offsets and lengths of various fields in an Atmel Generic record */
	ATMEL_GENERIC_ADDRESS_LEN = 6,
	ATMEL_GENERIC_DATA_LEN = 4,
	/* The separator character, a colon, that separates the data and address fields in the Atmel Generic records */
	ATMEL_GENERIC_SEPARATOR_OFFSET = 6,
	ATMEL_GENERIC_SEPARATOR = ':',
};

/**
 * All of the possible error codes the Atmel Generic record utility functions may return.
 */
enum AtmelGenericErrors {
	ATMEL_GENERIC_OK = 0, 				/**< Error code for success or no error. */
	ATMEL_GENERIC_ERROR_FILE = -1, 			/**< Error code for error while reading from or writing to a file. You may check errno for the exact error if this error code is encountered. */
	ATMEL_GENERIC_ERROR_EOF = -2, 			/**< Error code for encountering end-of-file when reading from a file. */
	ATMEL_GENERIC_ERROR_INVALID_RECORD = -3, 	/**< Error code for error if an invalid record was read. */
	ATMEL_GENERIC_ERROR_INVALID_ARGUMENTS = -4, 	/**< Error code for error from invalid arguments passed to function. */
	ATMEL_GENERIC_ERROR_NEWLINE = -5, 		/**< Error code for encountering a newline with no record when reading from a file. */
};

/**
 * Structure to hold the fields of an Atmel Generic record.
 */
typedef struct {
	uint32_t address; 	/**< The 24-bit address field of the record. */
	uint16_t data; 		/**< The 16-bit data field of the record. */
} AtmelGenericRecord;

/**
 * Sets all of the record fields of an Atmel Generic record structure.
 * Note that the Atmel Generic record only supports 24-bit addresses.
 * \param address The 24-bit address of the data.
 * \param data The 16-bit word of data.
 * \param genericRecord A pointer to the target Atmel Generic record structure where these fields will be set.
 * \return ATMEL_GENERIC_OK on success, otherwise one of the ATMEL_GENERIC_ERROR_ error codes.
 * \retval ATMEL_GENERIC_OK on success.
 * \retval ATMEL_GENERIC_ERROR_INVALID_ARGUMENTS if the record pointer is NULL.
*/
int New_AtmelGenericRecord(uint32_t address, uint16_t data, AtmelGenericRecord *genericRecord);

/**
 * Reads an Atmel Generic record from an opened file.
 * \param genericRecord A pointer to the Atmel Generic record structure that will store the read record.
 * \param in A file pointer to an opened file that can be read.
 * \return ATMEL_GENERIC_OK on success, otherwise one of the ATMEL_GENERIC_ERROR_ error codes.
 * \retval ATMEL_GENERIC_OK on success.
 * \retval ATMEL_GENERIC_ERROR_INVALID_ARGUMENTS if the record pointer or file pointer is NULL.
 * \retval ATMEL_GENERIC_ERROR_EOF if end-of-file has been reached.
 * \retval ATMEL_GENERIC_ERROR_FILE if a file reading error has occured.
 * \retval ATMEL_GENERIC_INVALID_RECORD if the record read is invalid (record did not match specifications).
*/
int Read_AtmelGenericRecord(AtmelGenericRecord *genericRecord, FILE *in);

/**
 * Writes an Atmel Generic record to an opened file.
 * Note that the Atmel Generic record only supports 24-bit addresses, so only 24-bits of the address stored in the Atmel Generic record structure that genericRecord points to will be written.
 * \param genericRecord A pointer to the Atmel Generic record structure.
 * \param out A file pointer to an opened file that can be written to.
 * \return ATMEL_GENERIC_OK on success, otherwise one of the ATMEL_GENERIC_ERROR_ error codes.
 * \retval ATMEL_GENERIC_OK on success. 
 * \retval ATMEL_GENERIC_ERROR_INVALID_ARGUMENTS if the record pointer or file pointer is NULL.
 * \retval ATMEL_GENERIC_ERROR_FILE if a file writing error has occured.
*/
int Write_AtmelGenericRecord(const AtmelGenericRecord *genericRecord, FILE *out);

/**
 * Prints the contents of an Atmel Generic record structure to stdout.
 * The record dump consists of the address and data fields of the record.
 * \param genericRecord A pointer to the Atmel Generic record structure.
 * \return Always returns ATMEL_GENERIC_OK (success).
 * \retval ATMEL_GENERIC_OK on success.
*/
void Print_AtmelGenericRecord(const AtmelGenericRecord *genericRecord);

#endif
