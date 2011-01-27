/*
 *  srecord.h
 *  Utility functions to create, read, write, and print Motorola S-Record binary records.
 *
 *  Written by Vanya A. Sergeev <vsergeev@gmail.com>
 *  Version 1.0.3 - October 2009
 *
 */

#include "srecord.h"

/* Lengths of the ascii hex encoded address fields of different SRecord types */ 
static int SRecord_Address_Lengths[] = {
	4, // S0
	4, // S1
	6, // S2
	8, // S3
	8, // S4
	4, // S5
	6, // S6
	8, // S7
	6, // S8
	4, // S9
};

/* Initializes a new IHexRecord structure that the paramater ihexRecord points to with the passed
 * S-Record type, up to 32-bit integer address, 8-bit data array, and size of 8-bit data array. */
int New_SRecord(int type, uint32_t address, uint8_t data[], int dataLen, SRecord *srec) {
	/* Data length size check, assertion of srec */
	if (dataLen < 0 || dataLen > SRECORD_MAX_DATA_LEN/2 || srec == NULL)
		return SRECORD_ERROR_INVALID_ARGUMENTS;
	
	srec->type = type;
	srec->address = address;
	memcpy(srec->data, data, dataLen);
	srec->dataLen = dataLen;
	srec->checksum = Checksum_SRecord(*srec);
	
	return SRECORD_OK;	
}


/* Utility function to read an S-Record from a file */
int Read_SRecord(SRecord *srec, FILE *in) {
	char recordBuff[SRECORD_RECORD_BUFF_SIZE];
	/* A temporary buffer to hold ascii hex encoded data, set to the maximum length we would ever need */
	char hexBuff[SRECORD_MAX_ADDRESS_LEN+1];
	int asciiAddressLen, asciiDataLen, dataOffset, fieldDataCount, i;
	
	/* Check our file pointer and the S-Record struct */
	if (in == NULL || srec == NULL)
		return SRECORD_ERROR_INVALID_ARGUMENTS;
	
	if (fgets(recordBuff, SRECORD_RECORD_BUFF_SIZE, in) == NULL) {
			/* In case we hit EOF, don't report a file error */
			if (feof(in) != 0)
				return SRECORD_ERROR_EOF;
			else
				return SRECORD_ERROR_FILE;
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
		return SRECORD_ERROR_NEWLINE;
	
	/* Size check for type and count fields */
	if (strlen(recordBuff) < SRECORD_TYPE_LEN + SRECORD_COUNT_LEN)
		return SRECORD_ERROR_INVALID_RECORD;
	
	/* Check for the S-Record start code at the beginning of every record */
	if (recordBuff[SRECORD_START_CODE_OFFSET] != SRECORD_START_CODE)
		return SRECORD_ERROR_INVALID_RECORD;
	
	/* Copy the ascii hex encoding of the type field into hexBuff, convert it into a usable integer */
	strncpy(hexBuff, recordBuff+SRECORD_TYPE_OFFSET, SRECORD_TYPE_LEN);
	hexBuff[SRECORD_TYPE_LEN] = 0;
	srec->type = strtol(hexBuff, (char **)NULL, 16);
	
	/* Copy the ascii hex encoding of the count field into hexBuff, convert it to a usable integer */
	strncpy(hexBuff, recordBuff+SRECORD_COUNT_OFFSET, SRECORD_COUNT_LEN);
	hexBuff[SRECORD_COUNT_LEN] = 0;
	fieldDataCount = strtol(hexBuff, (char **)NULL, 16);
	
	/* Check that our S-Record type is valid */
	if (srec->type < SRECORD_TYPE_S0 || srec->type > SRECORD_TYPE_S9)
		return SRECORD_ERROR_INVALID_RECORD;
	/* Get the ascii hex address length of this particular S-Record type */
	asciiAddressLen = SRecord_Address_Lengths[srec->type];
	
	/* Size check for address field */
	if (strlen(recordBuff) < (unsigned int)(SRECORD_ADDRESS_OFFSET+asciiAddressLen))
		return SRECORD_ERROR_INVALID_RECORD;
		
	/* Copy the ascii hex encoding of the count field into hexBuff, convert it to a usable integer */
	strncpy(hexBuff, recordBuff+SRECORD_ADDRESS_OFFSET, asciiAddressLen);
	hexBuff[asciiAddressLen] = 0;
	srec->address = strtol(hexBuff, (char **)NULL, 16);
	
	/* Compute the ascii-hex data length by subtracting the remaining field lengths from the S-Record 
	 * count field (times 2 to account for the number of characters used in ascii hex encoding) */
	asciiDataLen = (fieldDataCount*2) - asciiAddressLen - SRECORD_CHECKSUM_LEN;
	/* Bailout if we get an invalid data length */
	if (asciiDataLen < 0 || asciiDataLen > SRECORD_MAX_DATA_LEN)
		return SRECORD_ERROR_INVALID_RECORD;
		
	/* Size check for final data field and checksum field */
	if (strlen(recordBuff) < (unsigned int)(SRECORD_ADDRESS_OFFSET+asciiAddressLen+asciiDataLen+SRECORD_CHECKSUM_LEN))
		return SRECORD_ERROR_INVALID_RECORD;
		
	dataOffset = SRECORD_ADDRESS_OFFSET+asciiAddressLen;
	
	/* Loop through each ascii hex byte of the data field, pull it out into hexBuff,
	 * convert it and store the result in the data buffer of the S-Record */
	for (i = 0; i < asciiDataLen/2; i++) {
		/* Times two i because every byte is represented by two ascii hex characters */
		strncpy(hexBuff, recordBuff+dataOffset+2*i, SRECORD_ASCII_HEX_BYTE_LEN);
		hexBuff[SRECORD_ASCII_HEX_BYTE_LEN] = 0;
		srec->data[i] = strtol(hexBuff, (char **)NULL, 16);
	}
	/* Real data len is divided by two because every byte is represented by two ascii hex characters */
	srec->dataLen = asciiDataLen/2;
	
	/* Copy out the checksum ascii hex encoded byte, and convert it back to a usable integer */
	strncpy(hexBuff, recordBuff+dataOffset+asciiDataLen, SRECORD_CHECKSUM_LEN);
	hexBuff[SRECORD_CHECKSUM_LEN] = 0;
	srec->checksum = strtol(hexBuff, (char **)NULL, 16);

	if (srec->checksum != Checksum_SRecord(*srec))
		return SRECORD_ERROR_INVALID_RECORD;
	
	return SRECORD_OK;
}

/* Utility function to write an S-Record to a file */
int Write_SRecord(const SRecord srec, FILE *out) {
	char strAddress[SRECORD_MAX_ADDRESS_LEN+1];
	int asciiAddressLen, asciiAddressOffset, fieldDataCount, i;
	
	/* Check our file pointer */
	if (out == NULL)
		return SRECORD_ERROR_INVALID_ARGUMENTS;
	
	/* Check that the type and data length is within range */
	if (srec.type < SRECORD_TYPE_S0 || srec.type > SRECORD_TYPE_S9 || srec.dataLen > SRECORD_MAX_DATA_LEN/2)
		return SRECORD_ERROR_INVALID_RECORD;
	
	/* Compute the record count, address and checksum lengths are halved because record count
	 * is the number of bytes left in the record, not the length of the ascii hex representation */
	fieldDataCount = SRecord_Address_Lengths[srec.type]/2 + srec.dataLen + SRECORD_CHECKSUM_LEN/2;
	
	asciiAddressLen = SRecord_Address_Lengths[srec.type];
	/* The offset of the ascii hex encoded address from zero, this is used so we only write as
	 * many bytes of the address as permitted by the S-Record type. */
	asciiAddressOffset = SRECORD_MAX_ADDRESS_LEN-asciiAddressLen;
	
	if (fprintf(out, "%c%1.1X%2.2X", SRECORD_START_CODE, srec.type, fieldDataCount) < 0)
		return SRECORD_ERROR_FILE;

	/* Write the ascii hex representation of the address, starting from the offset to only
	 * write as much of the address as permitted by the S-Record type (calculated above). */
	/* Fix the hex to be 8 hex digits of precision, to fit all 32-bits, including zeros */
	snprintf(strAddress, sizeof(strAddress), "%2.8X", srec.address);
	if (fprintf(out, "%s", strAddress+asciiAddressOffset) < 0)
		return SRECORD_ERROR_FILE;
	
	/* Write each byte of the data, guaranteed to be two hex ascii characters each since
	 * srec.data[i] has the type of uint8_t */
	for (i = 0; i < srec.dataLen; i++) {
		if (fprintf(out, "%2.2X", srec.data[i]) < 0)
			return SRECORD_ERROR_FILE;
	}
		
	/* Last but not least, the checksum */
	if (fprintf(out, "%2.2X\r\n", Checksum_SRecord(srec)) < 0)
		return SRECORD_ERROR_FILE;
	
	return SRECORD_OK;
}


/* Utility function to print (formatted) the information stored in an S-Record */
void Print_SRecord(const SRecord srec) {
	int i;
	printf("S-Record Type: S%d\n", srec.type);
	printf("S-Record Address: 0x%2.8X\n", srec.address);
	printf("S-Record Data: {");
	for (i = 0; i < srec.dataLen; i++) {
		printf("0x%02X, ", srec.data[i]);
	}
	printf("}\n");
	printf("S-Record Checksum: 0x%2.2X\n", srec.checksum);
}

/* Utility function to calculate the checksum of an S-Record */
uint8_t Checksum_SRecord(const SRecord srec) {
	uint8_t checksum;
	int fieldDataCount, i;
	
	/* Compute the record count, address and checksum lengths are halved because record count
	 * is the number of bytes left in the record, not the length of the ascii hex representation */
	fieldDataCount = SRecord_Address_Lengths[srec.type]/2 + srec.dataLen + SRECORD_CHECKSUM_LEN/2;

	/* Add the count, address, and data fields together */
	checksum = fieldDataCount;
	/* Add each byte of the address individually */
	checksum += (uint8_t)(srec.address & 0x000000FF);
	checksum += (uint8_t)((srec.address & 0x0000FF00) >> 8);
	checksum += (uint8_t)((srec.address & 0x00FF0000) >> 16);
	checksum += (uint8_t)((srec.address & 0xFF000000) >> 24);
	for (i = 0; i < srec.dataLen; i++)
		checksum += srec.data[i];
	
	/* One's complement the checksum */
	checksum = ~checksum;
	
	return checksum;
}
