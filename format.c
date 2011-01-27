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
 * format.c - Formatting of disassembled instructions, with regard to the several
 *  formatting features this disassembler supports.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "format.h"

/* Formats a disassembled operand with its prefix (such as 'R' to indicate a register) into the
 * pointer to a C-string strOperand, which must be free'd after it has been used.
 * I decided to format the disassembled operands individually into strings for maximum flexibility,
 * and so that the printing of the formatted operand is not hard coded into the format operand code.
 * If an addressLabelPrefix is specified in formattingOptions (option is set and string is not NULL), 
 * it will print the relative branch/jump/call with this prefix and the destination address as the label. */
static int formatDisassembledOperand(char **strOperand, int operandNum, const disassembledInstruction dInstruction, formattingOptions fOptions);


/* Prints a disassembled instruction, formatted with options set in the formattingOptions structure. */
int printDisassembledInstruction(FILE *out, const disassembledInstruction dInstruction, formattingOptions fOptions) {
	int retVal, i;
	char *strOperand;
	
	/* If we just found a long instruction, there is nothing to be printed yet, since we don't
	 * have the entire long address ready yet. */
	if (AVR_Long_Instruction_State == AVR_LONG_INSTRUCTION_FOUND)
		return 0;
	
	/* If address labels are enabled, then we use an address label prefix as set in the
	 * string addressLabelPrefix, because labels need to start with non-numerical character
	 * for best compatibility with AVR assemblers. */	
	if (fOptions.options & FORMAT_OPTION_ADDRESS_LABEL) 
		retVal = fprintf(out, "%s%0*X: %s ", fOptions.addressLabelPrefix, fOptions.addressFieldWidth, dInstruction.address, dInstruction.instruction->mnemonic);
	/* Just print the address that the instruction is located at, without address labels. */
	else if (fOptions.options & FORMAT_OPTION_ADDRESS) 
		retVal = fprintf(out, "%4X:\t%s ", dInstruction.address, dInstruction.instruction->mnemonic);
	else 
		retVal = fprintf(out, "%s ", dInstruction.instruction->mnemonic);

	if (retVal < 0)
		return ERROR_FILE_WRITING_ERROR;

	for (i = 0; i < dInstruction.instruction->numOperands; i++) {
		/* If we're not on the first operand, but not on the last one either, print a comma separating
		 * the operands. */
		if (i > 0 && i != dInstruction.instruction->numOperands) {
			if (fprintf(out, ", ") < 0)
				return ERROR_FILE_WRITING_ERROR;
		}
		/* Format the disassembled operand into the string strOperand, and print it */
		retVal = formatDisassembledOperand(&strOperand, i, dInstruction, fOptions);
		if (retVal < 0)
			return retVal;
		/* Print the operand and free if it's not NULL */
		if (strOperand != NULL) {
			fprintf(out, "%s", strOperand);
			free(strOperand);
		}
	}

	/* If we want a comment specifying what address the rjmp or rcall would take us,
	 * i.e. rcall .+4 ; 0x56
	 */
	if (fOptions.options & FORMAT_OPTION_DESTINATION_ADDRESS_COMMENT) {
		for (i = 0; i < dInstruction.instruction->numOperands; i++) {
			/* This is only done for operands with relative addresses,
			 * where the destination address is relative from the address 
			 * the PC is on. */
			if (dInstruction.instruction->operandTypes[i] == OPERAND_BRANCH_ADDRESS ||
				dInstruction.instruction->operandTypes[i] == OPERAND_RELATIVE_ADDRESS) {
					if (fprintf(out, "%1s\t; %s%X", "", OPERAND_PREFIX_ABSOLUTE_ADDRESS, dInstruction.address+dInstruction.operands[i]+2) < 0)
						return ERROR_FILE_WRITING_ERROR;
					/* Stop after one relative address operand */
					break;
			}
		}
	}
	
	fprintf(out, "\n");
	return 0;
}

/* Formats a disassembled operand with its prefix (such as 'R' to indicate a register) into the
 * pointer to a C-string strOperand, which must be free'd after it has been used.
 * I decided to format the disassembled operands individually into strings for maximum flexibility,
 * and so that the printing of the formatted operand is not hard coded into the format operand code.
 * If an addressLabelPrefix is specified in formattingOptions (option is set and string is not NULL), 
 * it will print the relative branch/jump/call with this prefix and the destination address as the label. */
int formatDisassembledOperand(char **strOperand, int operandNum, const disassembledInstruction dInstruction, formattingOptions fOptions) {
	char binary[9];
	int retVal;

	switch (dInstruction.instruction->operandTypes[operandNum]) {
		case OPERAND_NONE:
		case OPERAND_REGISTER_GHOST:
			strOperand = NULL;
			retVal = 0;
			break;
		case OPERAND_REGISTER:
		case OPERAND_REGISTER_STARTR16:
		case OPERAND_REGISTER_EVEN_PAIR_STARTR24:
		case OPERAND_REGISTER_EVEN_PAIR:
			retVal = asprintf(strOperand, "%s%d", OPERAND_PREFIX_REGISTER, dInstruction.operands[operandNum]);
			break;
		case OPERAND_DATA:
		case OPERAND_COMPLEMENTED_DATA:
			if (fOptions.options & FORMAT_OPTION_DATA_BIN) {
				int i;
				for (i = 7; i >= 0; i--) {
					if (dInstruction.operands[operandNum] & (1<<i))
						binary[7-i] = '1';
					else
						binary[7-i] = '0';
				}
				binary[8] = '\0'; 
				retVal = asprintf(strOperand, "%s%s", OPERAND_PREFIX_DATA_BIN, binary);
			} else if (fOptions.options & FORMAT_OPTION_DATA_DEC) {
				retVal = asprintf(strOperand, "%s%d", OPERAND_PREFIX_DATA_DEC, dInstruction.operands[operandNum]);
			} else {
				retVal = asprintf(strOperand, "%s%02X", OPERAND_PREFIX_DATA_HEX, dInstruction.operands[operandNum]);
			}
			break;
		case OPERAND_BIT:
			retVal = asprintf(strOperand, "%s%d", OPERAND_PREFIX_BIT, dInstruction.operands[operandNum]);
			break;
		case OPERAND_BRANCH_ADDRESS:
		case OPERAND_RELATIVE_ADDRESS:
			/* If we have an address label, print it, otherwise just print the
			 * relative distance to the destination address. */
			if ((fOptions.options & FORMAT_OPTION_ADDRESS_LABEL) && fOptions.addressLabelPrefix != NULL) { 
				retVal = asprintf(strOperand, "%s%0*X", fOptions.addressLabelPrefix, fOptions.addressFieldWidth, dInstruction.address+dInstruction.operands[operandNum]+2);
			} else {
				/* Check if the operand is greater than 0 so we can print the + sign. */
				if (dInstruction.operands[operandNum] > 0) {
					retVal = asprintf(strOperand, "%s+%d", OPERAND_PREFIX_BRANCH_ADDRESS, dInstruction.operands[operandNum]);
				} else {
				/* Since the operand variable is signed, the negativeness of the distance
				 * to the destination address has been taken care of in disassembleOperands() */
					retVal = asprintf(strOperand, "%s%d", OPERAND_PREFIX_BRANCH_ADDRESS, dInstruction.operands[operandNum]);
				}
			}
			break;
		case OPERAND_LONG_ABSOLUTE_ADDRESS:
			retVal = asprintf(strOperand, "%s%0*X", OPERAND_PREFIX_ABSOLUTE_ADDRESS, fOptions.addressFieldWidth, AVR_Long_Instruction_Data);
			break;
		case OPERAND_IO_REGISTER:
			retVal = asprintf(strOperand, "%s%02X", OPERAND_PREFIX_IO_REGISTER, dInstruction.operands[operandNum]);
			break;	
		case OPERAND_WORD_DATA:
			retVal = asprintf(strOperand, "%s%0*X", OPERAND_PREFIX_WORD_DATA, fOptions.addressFieldWidth, dInstruction.operands[operandNum]);
			break;
		case OPERAND_DES_ROUND:
			retVal = asprintf(strOperand, "%s%02X", OPERAND_PREFIX_WORD_DATA, dInstruction.operands[operandNum]);
			break;

		case OPERAND_YPQ:
			retVal = asprintf(strOperand, "Y+%d", dInstruction.operands[operandNum]);
			break;
		case OPERAND_ZPQ:
			retVal = asprintf(strOperand, "Z+%d", dInstruction.operands[operandNum]);
			break;
		case OPERAND_X:
			retVal = asprintf(strOperand, "X");
			break;
		case OPERAND_XP:
			retVal = asprintf(strOperand, "X+");
			break;
		case OPERAND_MX:
			retVal = asprintf(strOperand, "-X");
			break;
		case OPERAND_Y:
			retVal = asprintf(strOperand, "Y");
			break;
		case OPERAND_YP:
			retVal = asprintf(strOperand, "Y+");
			break;
		case OPERAND_MY:
			retVal = asprintf(strOperand, "-Y");
			break;
		case OPERAND_Z:
			retVal = asprintf(strOperand, "Z");
			break;
		case OPERAND_ZP:
			retVal = asprintf(strOperand, "Z+");
			break;
		case OPERAND_MZ:
			retVal = asprintf(strOperand, "-Z");
			break;

		/* This is impossible by normal operation. */
		default:
			return ERROR_UNKNOWN_OPERAND; 
	}
	if (retVal < 0)
		return ERROR_MEMORY_ALLOCATION_ERROR;
	return 0;
}
