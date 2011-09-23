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
 * file.h - Header file to Routines to take care of Atmel Generic, Intel HEX, 
 *  and Motorola S-Record file disassembly. Interface to avr_disasm.c to and 
 *  format.c to swiftly disassemble and print an assembled instruction.
 *
 */
 
#ifndef FILE_DISASM_H
#define FILE_DISASM_H

#include <stdio.h>
#include "format.h"

/* Reads a record from an Atmel Generic formatted file, formats the assembled
 * instruction data into an assembledInsruction structure, then passes the
 * assembled instruction data to disassembleAndPrint() for disassembly and printing.
 * Loops until all records have been read and processed. */
int disassembleGenericFile(FILE *fileOut, FILE *fileIn, formattingOptions fOptions);

/* Reads a record from an Intel Hex formatted file, formats the assembled
 * instruction data into an assembledInsruction structure, then passes the
 * assembled instruction data to disassembleAndPrint() for disassembly and printing.
 * Loops until all records have been read and processed. */
int disassembleIHexFile(FILE *fileOut, FILE *fileIn, formattingOptions fOptions);

/* Reads a record from an Motorola S-Record formatted file, formats the assembled
 * instruction data into an assembledInsruction structure, then passes the
 * assembled instruction data to disassembleAndPrint() for disassembly and printing.
 * Loops until all records have been read and processed. */
int disassembleSRecordFile(FILE *fileOut, FILE *fileIn, formattingOptions fOptions);

/* Disassemble an assembled instruction, and print its disassembly
 * to fileOut. Alert user of errors. */
int disassembleAndPrint(FILE *fileOut, const assembledInstruction *aInstruction, formattingOptions fOptions);

#endif
