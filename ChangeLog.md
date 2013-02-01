  * Release 3.0 - 02/01/2013
      * Complete rewrite of vAVRdisasm to an opcode->disasm->format stream architecture.
      * Added decoding support for malformed input that might have single bytes on EOF or address boundaries.
      * Added binary file support with `-t binary` or `--file-type binary` option.
      * Added comprehensive fuzzing and avr-objdump comparison test (see crazy_test.py or `make test`).
      * Modified default disassembly output to include original opcodes alongside disassembled instructions.
  * Release 2.0 - 09/24/2011
      * Changed address operand formatting for LDS, STS, JMP, and CALL instructions from byte addreses to word addresses, to make vAVRdisasm's output compatible with AVR assemblers.
      * Fixed signed relative branch/jump decoding: jumps in the reverse direction are now correctly decoded.
      * Thanks to Graham Carnell for the above two fixes!
      * Upgraded license from GPLv2 to GPLv3.
  * Release 1.9 - 04/03/2011
      * CRITICAL FIX: Fixed S-Record reading bug that was ignoring valid data records.
      * Added output file support by `-o` / `--out-file` <output file> option.
      * Added standard input support with the "-" file argument, meaning the disassembler now supports piped input.
      * Improved Atmel Generic / Intel HEX8 / Motorola S-Record auto-detection by first character rather than file extension.
      * Thanks to Thomas for all four of the above fixes and suggestions!
      * Added printing of original opcode data alongside disassembly with `--original` option.
  * Release 1.8 - 01/26/2011
      * Fixed address decoding for LDS, STS, JMP, and CALL instructions. Reversed the modification from release 1.7.
      * Added support for XCH, LAS, LAC, LAT instructions, bringing the disassembler up to date with AVR Instruction Set revision 0856I - 07/10.
  * Release 1.7 - 05/27/2010
      * Fixed address decoding for LDS, STS, JMP, and CALL instructions. Previously, vAVRdisasm was printing the disassembled address operands as twice the value they should have been for these instructions.
  * Release 1.6 - 02/04/2010
      * Fixed the number-of-operands field for the SPM instruction. This bug was causing vAVRdisasm to crash as it was attempting to format a non-existing operand during disassembly.
      * Updated the README.
  * Release 1.5 - 08/25/2009
      * Renamed source files to make more sense and for better organization of code.
      * Added support for DES, SPM #2, LDS (16-bit), and STS (16-bit) instructions, bringing the disassembler to support the AVR instruction set up to revision 0856H - 04/09.
  * Release 1.4 - 06/27/2009
      * Fixed handling of newlines (sometimes found at the end of program files) so  an "invalid record" error doesn't appear when a newline is read.
      * CRITICAL FIX: Fixed reading and disassembly of odd byte length records in Intel HEX8 and Motorola S-Record files. Special thanks to Ahmed for discovery and patch!
  * Release 1.3 - 01/08/2009
      * Fixed a few small bugs/typos for cleaner compilation.
      * CRITICAL FIX: Corrected the absolute address calculation, used in instructions like absolute jump.
  * Release 1.2 - 01/06/2007
      * Added formatting of data constants in different bases (hexadecimal, binary, decimal).
      * Fixed a small bug/typo: first operand of "out" instruction is actually an I/O register.
  * Release 1.0 - 01/03/2007
      * Initial release.

