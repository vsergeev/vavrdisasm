## ABOUT vAVRdisasm

vAVRdisasm is an 8-bit Atmel AVR firmware disassembler. This single-pass disassembler can read Atmel Generic, Intel HEX8, and Motorola S-Record formatted files containing valid AVR program binaries.

It supports all 142 8-bit AVR instructions as defined by the [Atmel AVR Instruction Set](http://www.atmel.com/atmel/acrobat/doc0856.pdf) revision 0856I-AVR-07/10.

vAVRdisasm features a handful of formatting options, including:

  * Printing the instruction address alongside disassembly, enabled by default
  * Printing the destination address of relative branch/jump/call instructions as comments alongside disassembly, enabled by default
  * Printing the original opcode data alongside disassembly, enabled by default
  * Ghetto Address Labels (see [“Ghetto Address Labels”](#ghetto-address-labels) section)
  * Formatting data constants in different bases (hexadecimal, binary, decimal)
  * .DW data word directive for data not recognized as an instruction during disassembly
  * Piped input and output

vAVRdisasm should work on most *nix platforms, including a [Cygwin](http://www.cygwin.com/) or [MinGW](http://www.mingw.org/) environment. vAVRdisasm was written by Vanya A. Sergeev, and tested with the GNU C Compiler on Linux. Feel free to send any ideas or suggestions to vsergeev at gmail dot com.

## LICENSE

vAVRdisasm is released under the GNU General Public License Version 3.

    You should have received a copy of the GNU General Public License
    along with this program; see the file "COPYING".  If not, see
    <http://www.gnu.org/licenses/>.

## COMPILING vAVRdisasm

Simply running,

    $ make

 in the vAVRdisasm project directory should compile vAVRdisasm on most
*nix systems, including a Cygwin or MinGW environment. The Makefile is
configured to use GCC to compile vAVRdisasm.

vAVRdisasm should have no problem being compiled with "gmake".

## USAGE

    Usage: ./vavrdisasm <option(s)> <file>
    Disassembles AVR program file <file>. Use - for standard input.
    
    vAVRdisasm version 3.0 - 02/01/2013.
    Written by Vanya A. Sergeev - <vsergeev@gmail.com>.
    
    Additional Options:
      -o, --out-file <file>         Write to file instead of standard output.
    
      -t, --file-type <type>        Specify file type of the program file.
    
      -l, --address-label <prefix>  Create ghetto address labels with
                                      the specified label prefix.
    
      --data-base-hex               Represent data constants in hexadecimal
                                      (default).
      --data-base-bin               Represent data constants in binary.
      --data-base-dec               Represent data constants in decimal.
    
      --no-addresses                Do not display address alongside disassembly.
      --no-opcodes                  Do not display original opcode alongside
                                      disassembly.
      --no-destination-comments     Do not display destination address comments
                                      of relative branch/jump/call instructions.
    
      -h, --help                    Display this usage/help.
      -v, --version                 Display the program's version.
    
    Supported file types:
      Atmel Generic         generic
      Intel HEX8            ihex
      Motorola S-Record     srec
      Raw Binary            binary
    

## USING vAVRdisasm

### File Input

For most purposes:

    $ vavrdisasm <AVR program file>

Example:

    $ vavrdisasm sampleprogram.hex

Use `-` for standard input.

### Option `-t` or `--file-type`
vAVRdisasm will auto-recognize Atmel Generic, Intel HEX8, and Motorola S-Record files. However, the `-t` or `--file-type` option can be used to explicitly select the file format, and to specify a raw binary input file.

Example:

    $ vavrdisasm -t binary sampleprogram

The file type argument for this option can be "generic", "ihex", "srecord", or "binary", for Atmel Generic, Intel HEX8, Motorola S-Record, and raw binary files, respectively.

### Option `-o` or `--out-file` <<output file>>
Specify an output file for writing instead of the standard output. The output file `-` is also synonymous for standard output.

### Options `--data-base-hex`, `--data-base-bin`, `--data-base-dec`
vAVRdisasm will default to formatting data constants in hexadecimal. However, data constants can be represented in a different base with one of the following options: `--data-base-hex`, `--data-base-bin`, and `--data-base-dec`.

### Options `--no-addresses`, `--no-destination-comments`, `--no-opcodes`
By default, vAVRdisasm will print the instruction addresses alongside disassembly, the original opcodes alongside disassembly,and  destination comments for relative branch, jump, and call instructions. These formatting options can be disabled with the `--no-addresses`, `--no-opcodes`, and `--no-destination-comments` options.

### Option `-l` or `--address-label`
See the [Ghetto Address Labels](#ghetto-address-labels) section.

### Options `-h` or `--help`, `-v` or `--version`
The `-h` or `--help` option will print a brief usage summary, including program options and supported file types.
The `-v` or `--version` option will print the program's version.

If you encounter any bugs or problems, please submit an issue on [GitHub](https://github.com/vsergeev/vAVRdisasm/issues) or notify the author by email, vsergeev at gmail dot com.

## Ghetto Address Labels

vAVRdisasm supports a unique formatting feature: Ghetto Address Labels, which few, if not any, single-pass disassemblers implement.

With the `-l` or `--address-label` option and a supplied prefix, vAVRdisasm will print a label containing the non-numerical supplied prefix and the address of the disassembled instruction at every instruction. Also, all relative branch, jump, and call instructions will be formatted to jump to their designated address label.

This feature enables direct re-assembly of the vAVRdisasm's disassembly. This can be especially useful for quick modification to the AVR program assembly code without having to manually format the disassembly or adjust the relative branch, jump, and call distances with every modification to the disassembly.

The `-l` or `--address-label` option overrides the default printing of the addresses alongside disassembly.

Example:

    $ vavrdisasm -l “A_” sampleprogram.hex

vAVRdisasm's disassembly will include address labels that will look like this: A_0000:

For sample disassembly outputs by vAVRdisasm, see the [Sample Disassembly Outputs](#sample-disassembly-outputs) section.

## Shortcomings

  * vAVRdisasm does not display alternate versions of the same encoded instruction. This means that the "cbr" instruction will never be displayed in the disassembly, because the "andi" instruction precedes it in priority.

These features do not affect the accuracy of the disassembler's output, and may be supported in future versions of vAVRdisasm.

## Source Code

vAVRdisasm uses [libGIS](/projects/libgis/), a free Atmel Generic, Intel HEX, and Motorola S-Record Parser library to parse formatted files containing AVR program binaries. libGIS is available for free under both MIT and Public Domain licenses [here](/projects/libgis/). libGIS is compiled into vAVRdisasm---it does not need to be obtained separately.

## Sample Disassembly Outputs

These output samples, produced by vAVRdisasm, are a disassembly of the program from the ["Notice's Guide to AVR Development"](http://www.atmel.com/dyn/resources/prod_documents/novice.pdf) article in the Atmel Applications Journal.

    $ vavrdisasm sampleprogram.hex
       0:   c0 00       rjmp    .+0 ; 0x2
       2:   ef 0f       ser R16
       4:   bb 07       out $17, R16
       6:   bb 08       out $18, R16
       8:   95 0a       dec R16
       a:   cf fd       rjmp    .-6 ; 0x6

    $ vavrdisasm --no-opcodes sampleprogram.hex
       0:   rjmp    .+0 ; 0x2
       2:   ser R16
       4:   out $17, R16
       6:   out $18, R16
       8:   dec R16
       a:   rjmp    .-6 ; 0x6

    $ vavrdisasm --no-destination-comments sampleprogram.hex
       0:   c0 00       rjmp    .+0
       2:   ef 0f       ser R16
       4:   bb 07       out $17, R16
       6:   bb 08       out $18, R16
       8:   95 0a       dec R16
       a:   cf fd       rjmp    .-6

    $ vavrdisasm --no-addresses sampleprogram.hex
    c0 00       rjmp    .+0 ; 0x2
    ef 0f       ser R16
    bb 07       out $17, R16
    bb 08       out $18, R16
    95 0a       dec R16
    cf fd       rjmp    .-6 ; 0x6

    $ vavrdisasm -l "A_" sampleprogram.hex
    .org 0x0000
    A_0000: rjmp    A_0002  ; 0x2
    A_0002: ser R16
    A_0004: out $17, R16
    A_0006: out $18, R16
    A_0008: dec R16
    A_000a: rjmp    A_0006  ; 0x6

The above program sample was modified slightly to illustrate vAVRdisasm’s ability to represent data constants in different bases:

    $ vavrdisasm --data-base-bin sampleprogram2.hex
       0:   c0 00       rjmp    .+0 ; 0x2
       2:   ef 0f       ser R16
       4:   bb 07       out $17, R16
       6:   ea 0f       ldi R16, 0b10101111
       8:   bb 08       out $18, R16
       a:   95 0a       dec R16
       c:   cf fd       rjmp    .-6 ; 0x8

