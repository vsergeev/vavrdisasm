#ifndef SRECORD_H
#define SRECORD_H
/**
 * \file srecord.h
 * \brief Low-level utility functions to create, read, write, and print Motorola S-Record binary records.
 * \author Vanya A. Sergeev <vsergeev@gmail.com>
 * \date February 2011
 * \version 1.0.5
 */
 
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* General definition of the S-Record specification */
enum _SRecordDefinitions {
	/* 768 should be plenty of space to read in an S-Record */
	SRECORD_RECORD_BUFF_SIZE = 768,
	/* Offsets and lengths of various fields in an S-Record record */
	SRECORD_TYPE_OFFSET = 1,
	SRECORD_TYPE_LEN = 1,
	SRECORD_COUNT_OFFSET = 2,
	SRECORD_COUNT_LEN = 2,
	SRECORD_ADDRESS_OFFSET = 4,
	SRECORD_CHECKSUM_LEN = 2,
	/* Maximum ascii hex length of the S-Record data field */
	SRECORD_MAX_DATA_LEN = 64,
	/* Maximum ascii hex length of the S-Record address field */
	SRECORD_MAX_ADDRESS_LEN = 8,
	/* Ascii hex length of a single byte */
	SRECORD_ASCII_HEX_BYTE_LEN = 2,
	/* Start code offset and value */
	SRECORD_START_CODE_OFFSET = 0,
	SRECORD_START_CODE = 'S',
};

/**
 * All possible error codes the Motorola S-Record utility functions may return.
 */
enum SRecordErrors {
	SRECORD_OK = 0, 			/**< Error code for success or no error. */
	SRECORD_ERROR_FILE = -1, 		/**< Error code for error while reading from or writing to a file. You may check errno for the exact error if this error code is encountered. */
	SRECORD_ERROR_EOF = -2, 		/**< Error code for encountering end-of-file when reading from a file. */
	SRECORD_ERROR_INVALID_RECORD = -3, 	/**< Error code for error if an invalid record was read. */
	SRECORD_ERROR_INVALID_ARGUMENTS = -4, 	/**< Error code for error from invalid arguments passed to function. */
	SRECORD_ERROR_NEWLINE = -5, 		/**< Error code for encountering a newline with no record when reading from a file. */
};

/**
 * Motorola S-Record Types S0-S9
 */
enum SRecordTypes {
	SRECORD_TYPE_S0 = 0, /**< Header record, although there is an official format it is often made proprietary by third-parties. 16-bit address normally set to 0x0000 and header information is stored in the data field. This record is unnecessary and commonly not used. */
	SRECORD_TYPE_S1, /**< Data record with 16-bit address */
	SRECORD_TYPE_S2, /**< Data record with 24-bit address */
	SRECORD_TYPE_S3, /**< Data record with 32-bit address */
	SRECORD_TYPE_S4, /**< Extension by LSI Logic, Inc. See their specification for more details. */
	SRECORD_TYPE_S5, /**< 16-bit address field that contains the number of S1, S2, and S3 (all data) records transmitted. No data field. */
	SRECORD_TYPE_S6, /**< 24-bit address field that contains the number of S1, S2, and S3 (all data) records transmitted. No data field. */
	SRECORD_TYPE_S7, /**< Termination record for S3 data records. 32-bit address field contains address of the entry point after the S-Record file has been processed. No data field. */
	SRECORD_TYPE_S8, /**< Termination record for S2 data records. 24-bit address field contains address of the entry point after the S-Record file has been processed. No data field. */
	SRECORD_TYPE_S9, /**< Termination record for S1 data records. 16-bit address field contains address of the entry point after the S-Record file has been processed. No data field. */
};

/**
 * Structure to hold the fields of a Motorola S-Record record.
 */
typedef struct {
	uint32_t address; 			/**< The address field. This can be 16, 24, or 32 bits depending on the record type. */
	uint8_t data[SRECORD_MAX_DATA_LEN/2]; 	/**< The 8-bit array data field, which has a maximum size of 32 bytes. */
	int dataLen; 				/**< The number of bytes of data stored in this record. */
	int type; 				/**< The Motorola S-Record type of this record (S0-S9). */
	uint8_t checksum; 			/**< The checksum of this record. */
} SRecord;

/**
 * Sets all of the record fields of a Motorola S-Record structure.
 * \param type The Motorola S-Record type (integer value of 0 through 9).
 * \param address The 32-bit address of the data. The actual size of the address (16-,24-, or 32-bits) when written to a file depends on the S-Record type.
 * \param data A pointer to the 8-bit array of data.
 * \param dataLen The size of the 8-bit data array.
 * \param srec A pointer to the target Motorola S-Record structure where these fields will be set.
 * \return SRECORD_OK on success, otherwise one of the SRECORD_ERROR_ error codes.
 * \retval SRECORD_OK on success.
 * \retval SRECORD_ERROR_INVALID_ARGUMENTS if the record pointer is NULL, or if the length of the 8-bit data array is out of range (less than zero or greater than the maximum data length allowed by record specifications, see SRecord.data).
*/
int New_SRecord(int type, uint32_t address, const uint8_t *data, int dataLen, SRecord *srec);

/**
 * Reads a Motorola S-Record record from an opened file.
 * \param srec A pointer to the Motorola S-Record structure that will store the read record.
 * \param in A file pointer to an opened file that can be read.
 * \return SRECORD_OK on success, otherwise one of the SRECORD_ERROR_ error codes.
 * \retval SRECORD_OK on success.
 * \retval SRECORD_ERROR_INVALID_ARGUMENTS if the record pointer or file pointer is NULL.
 * \retval SRECORD_ERROR_EOF if end-of-file has been reached.
 * \retval SRECORD_ERROR_FILE if a file reading error has occured.
 * \retval SRECORD_INVALID_RECORD if the record read is invalid (record did not match specifications or record checksum was invalid).
*/
int Read_SRecord(SRecord *srec, FILE *in);

/**
 * Writes a Motorola S-Record to an opened file.
 * \param srec A pointer to the Motorola S-Record structure.
 * \param out A file pointer to an opened file that can be written to.
 * \return SRECORD_OK on success, otherwise one of the SRECORD_ERROR_ error codes.
 * \retval SRECORD_OK on success. 
 * \retval SRECORD_ERROR_INVALID_ARGUMENTS if the record pointer or file pointer is NULL.
 * \retval SRECORD_ERROR_INVALID_RECORD if the record's data length (the SRecord.dataLen variable of the record) is out of range (greater than the maximum data length allowed by record specifications, see SRecord.data).
 * \retval SRECORD_ERROR_FILE if a file writing error has occured.
*/
int Write_SRecord(const SRecord *srec, FILE *out);

/**
 * Prints the contents of a Motorola S-Record structure to stdout.
 * The record dump consists of the type, address, entire data array, and checksum fields of the record.
 * \param srec A pointer to the Motorola S-Record structure.
 * \return Always returns SRECORD_OK (success).
 * \retval SRECORD_OK on success.
*/
void Print_SRecord(const SRecord *srec);

/**
 * Calculates the checksum of a Motorola S-Record SRecord structure.
 * See the Motorola S-Record specifications for more details on the checksum calculation.
 * \param srec A pointer to the Motorola S-Record structure.
 * \return The 8-bit checksum.
*/
uint8_t Checksum_SRecord(const SRecord *srec);

#endif
