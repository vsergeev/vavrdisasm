#!/usr/bin/python2

import os
import sys
import subprocess

# Check that we have avr-binutils to execute the tests below
try:
    subprocess.check_output(['avr-as', '--version'])
    subprocess.check_output(['avr-ld', '--version'])
    subprocess.check_output(['avr-objcopy', '--version'])
except OSError:
    print "Error! avr-binutils required to run tests."
    sys.exit(-1)

NUM_ITERATIONS_TEST1 = 50
NUM_ITERATIONS_TEST2 = 50
NUM_ITERATIONS_TEST3 = 50

################################################################################
# Run vavrdisasm through a fuzzer, to check the robustness of its format
# decoding and disassembly logic, and compare its disassembly output across
# different input file formats
################################################################################

test1_passed = True

print "Starting fuzzing test"

for i in range(NUM_ITERATIONS_TEST1):
    print "\tTest 1 Iteration %d / %d" % (i+1, NUM_ITERATIONS_TEST1)

    # Create a random 16384 byte file
    bogus_data = os.urandom(16384)
    with open('bogus1.bin', 'w') as bogus_f:
        bogus_f.write(bogus_data)

    # Convert it to Atmel Generic format
    with open('bogus1.gen', 'w') as bogus_gen_f:
        for i in range(len(bogus_data)/2):
            bogus_gen_f.write("%06x:%02x%02x\n" % (i, ord(bogus_data[2*i+1]), ord(bogus_data[2*i])))

    del bogus_data

    # Convert it to Intel HEX and Motorola S-Record formats with objcopy
    subprocess.check_call(["objcopy", "-I", "binary", "-O", "ihex", "bogus1.bin", "bogus1.hex"])
    subprocess.check_call(["objcopy", "-I", "binary", "-O", "srec", "bogus1.bin", "bogus1.srec"])

    # Generate disassemblies for each file format
    bogus_bin_disasm = subprocess.check_output(["./vavrdisasm", "-t", "binary", "bogus1.bin"])
    bogus_gen_disasm = subprocess.check_output(["./vavrdisasm", "-t", "generic", "bogus1.gen"])
    bogus_ihex_disasm = subprocess.check_output(["./vavrdisasm", "-t", "ihex", "bogus1.hex"])
    bogus_srec_disasm = subprocess.check_output(["./vavrdisasm", "-t", "srec", "bogus1.srec"])

    if (len(bogus_gen_disasm) != len(bogus_bin_disasm) or bogus_gen_disasm != bogus_bin_disasm):
        print "Error! Atmel Generic disassembly differs from Binary disassembly!"
        print "Please send bogus1.gen and bogus1.bin to the author!"
        test1_passed = False
        break
    if (len(bogus_ihex_disasm) != len(bogus_bin_disasm) or bogus_ihex_disasm != bogus_bin_disasm):
        print "Error! Intel Hex disassembly differs from Binary disassembly!"
        print "Please send bogus1.hex and bogus1.bin to the author!"
        test1_passed = False
        break
    if (len(bogus_srec_disasm) != len(bogus_bin_disasm) or bogus_srec_disasm != bogus_bin_disasm):
        print "Error! Motorola S-Record disassembly differs from Binary disassembly!"
        print "Please send bogus1.srec and bogus1.bin to the author!"
        test1_passed = False
        break

if test1_passed:
    # Clean up files
    os.remove("bogus1.bin")
    os.remove("bogus1.gen")
    os.remove("bogus1.hex")
    os.remove("bogus1.srec")

    print "SUCCESS fuzzing test\n"
else:
    print "FAILURE fuzzing test\n"

################################################################################
# Compare vavrdisasm output with avr-objdump output
################################################################################

test2_passed = True

print "Starting avr-objdump compare test"

for i in range(NUM_ITERATIONS_TEST2):
    print "\tTest 2 Iteration %d / %d" % (i+1, NUM_ITERATIONS_TEST2)

    # Create a random 16384 byte file
    bogus_data = os.urandom(16384)
    with open('bogus2.bin', 'w') as bogus_f:
        bogus_f.write(bogus_data)
    del bogus_data

    # Generate disassemblies
    objdump_disasm = subprocess.check_output(["avr-objdump", "-b", "binary", "-m", "avr", "-D", "bogus2.bin"])
    vavrdisasm_disasm = subprocess.check_output(["./vavrdisasm", "-t", "binary", "--no-destination-comments", "bogus2.bin"])

    # Split disassemblies by newlines
    objdump_disasm = objdump_disasm.split("\n")
    vavrdisasm_disasm = vavrdisasm_disasm.split("\n")

    # Map and filter lambdas for post-processing
    func_lowercase_and_split = lambda x: x.lower().split("\t")
    func_trim_dest_comments = lambda x: x[0:4]
    func_filter_non_asm = lambda x: len(x) >= 3
    func_add_empty_operands = lambda x: [ x[0], x[1], x[2], '' ] if len(x) == 3 else x
    func_strip_spaces = lambda x: [y.strip() for y in x]
    func_opcode_big_endian = lambda x: [x[0], ' '.join(x[1].split(' ')[::-1]), x[2], x[3]]
    func_sub_word_directive = lambda x: [ x[0], x[1], ".word" if x[2] == ".dw" else x[2], x[3]]
    func_sub_word_addr_byte_addr = lambda x: [ x[0], x[1], x[2], "0x%x" % (int(x[3], 0)*2) if (x[2] == "call" or x[2] == "jmp") else x[3] ]
    func_remove_des_operand_prefix = lambda x: [ x[0], x[1], x[2], x[3].replace("0x", "") if x[2] == "des" else x[3] ]
    func_sub_ioreg_operand_prefix = lambda x: [ x[0], x[1], x[2], x[3].replace("$", "0x") ]

    ### Post-process objdump disassembly

    # Trim lines leading up to disassembly
    for j in range(len(objdump_disasm)):
        if ("<.data>:" in objdump_disasm[j]):
            break;
    objdump_disasm = objdump_disasm[j+1:]

    # Lower case all lines and split into fields
    # Trim destination comments
    # Filter out any non-asm lines (usually last line)
    # Add empty operands for no operand instructions
    # Strip spaces
    # Format opcode bytes into big-endian order
    for (f,g) in [  \
                    (map, func_lowercase_and_split), \
                    (map, func_trim_dest_comments), \
                    (filter, func_filter_non_asm), \
                    (map, func_add_empty_operands), \
                    (map, func_strip_spaces), \
                    (map, func_opcode_big_endian), \
                 ]:
        objdump_disasm = f(g, objdump_disasm)

    # objdump_disasm is now an array of string arrays, [ [address, opcode, mneumonic, operands], ... ]

    ### Post-processing vavrdisasm diassembly

    # Lower case all lines and split into fields
    # Filter out any non-asm lines (usually last line)
    # Strip spaces
    # Replace call/jmp word addresses with byte addresses
    # Remove 0x prefix of des operands
    # Replace I/O register prefix $ with 0x
    # Replace .dw for .word
    for (f,g) in [  \
                    (map, func_lowercase_and_split), \
                    (filter, func_filter_non_asm), \
                    (map, func_strip_spaces), \
                    (map, func_sub_word_addr_byte_addr), \
                    (map, func_remove_des_operand_prefix), \
                    (map, func_sub_ioreg_operand_prefix),
                    (map, func_sub_word_directive), \
                ]:
        vavrdisasm_disasm = f(g, vavrdisasm_disasm)

    # vavrdisasm_disasm is now an array of string arrays, [ [address, opcode, mneumonic, operands], ... ]

    ### Compare disassembly outputs
    for i in range(min(len(objdump_disasm), len(vavrdisasm_disasm))):
        if objdump_disasm[i] != vavrdisasm_disasm[i]:
            # If the opcodes are the same, and this is an std / lds identity,
            # skip it
            if objdump_disasm[i][1] == vavrdisasm_disasm[i][1] and \
               objdump_disasm[i][2] == 'std' and vavrdisasm_disasm[i][2] == 'lds':
                continue
            # If the opcodes are the same, and this is the ldi / ser identity,
            # skip it
            if objdump_disasm[i][1] == vavrdisasm_disasm[i][1] and \
               objdump_disasm[i][2] == 'ldi' and vavrdisasm_disasm[i][2] == 'ser' and \
               vavrdisasm_disasm[i][3] + ", 0xff" == objdump_disasm[i][3]:
                continue
            # If objdump failed to handle a 32-bit instruction cutoff at EOF,
            # skip it
            if objdump_disasm[i][0] == "3ffe:" and "is out of bounds" in \
               objdump_disasm[i][2] and vavrdisasm_disasm[i][2] == '.word':
                continue

            print "Found mismatch!"
            print "objdump:   ", objdump_disasm[i]
            print "vavrdisasm:", vavrdisasm_disasm[i]
            print "Please send bogus2.bin to the author!"
            print ""
            test2_passed = False
            break

if test2_passed:
    # Clean up files
    os.remove("bogus2.bin")

    print "SUCCESS avr-objdump compare test\n"
else:
    print "FAILURE avr-objdump compare test\n"


################################################################################
# Run vavrdisasm on a random binary file, assemble that disassembly with
# avr-as, disassemble it again again with vavrdisasm and compare it to the
# original disasembly
#   binary -> vavrdisasm -> avr-as/avr-ld -> binary -> vavrdisasm
################################################################################

test3_passed = True

print "Starting disassembling assembled disassembly test"

for i in range(NUM_ITERATIONS_TEST3):
    print "\tTest 3 Iteration %d / %d" % (i+1, NUM_ITERATIONS_TEST3)

    # Create a random 16384 byte file
    bogus_data = os.urandom(16384)
    with open('bogus3.bin', 'w') as bogus_f:
        bogus_f.write(bogus_data)
    del bogus_data

    # Generate original disassembly
    vavrdisasm_orig_disasm = subprocess.check_output(["./vavrdisasm", "-t", "binary", "--no-destination-comments", "bogus3.bin"])
    with open('bogus3.bin.orig_disasm', 'w') as bogus_f:
        bogus_f.write(vavrdisasm_orig_disasm)

    # Split disassembly by newline
    vavrdisasm_postproc_disasm = vavrdisasm_orig_disasm.split("\n")

    # Map and filter lambdas for post-processing
    func_field_split = lambda x: x.split("\t")
    func_filter_asm = lambda x: len(x) > 1
    func_strip_spaces = lambda x: [y.strip() for y in x]
    func_add_address_labels = lambda x: [ "L_" + x[0] ] + x[1:]
    func_remove_des_operand_prefix = lambda x: [ x[0], x[1], x[2], x[3].replace("0x", "") if x[2] == "des" else x[3] ] if len(x) == 4 else x
    func_sub_ioreg_operand_prefix = lambda x: [ x[0], x[1], x[2], x[3].replace("$", "0x") ] if len(x) == 4 else x
    func_sub_word_directive = lambda x: [ x[0], x[1], ".word" if x[2] == ".dw" else x[2], x[3]] if len(x) == 4 else x
    func_sub_word_addr_byte_addr = lambda x: [ x[0], x[1], x[2], "0x%x" % (int(x[3], 0)*2) if (x[2] == "call" or x[2] == "jmp") else x[3] ] if len(x) == 4 else x
    func_sub_lds_for_word_directive = lambda x: [ x[0], x[1], ".word" if x[2] == "lds" and len(x[1].split()) == 2 else x[2], "0x"+''.join(x[1].split()) if x[2] == "lds" and len(x[1].split()) == 2 else x[3] ] if len(x) == 4 else x
    func_remove_opcodes = lambda x: [x[0], x[2], x[3]]
    func_recombine_fields = lambda x: ' '.join(x)

    # Split into fields
    # Filter out any non-asm lines (usually last line)
    # Strip spaces
    # Add address labels
    # Remove 0x prefix of des operands
    # Replace I/O register prefix $ with 0x
    # Replace .dw for .word
    # Replace call/jmp word addresses with byte addresses
    # Turn 16 bit lds instructions into .word's because avr-as doesn't support
    #   them (only generates 32-bit lds)
    # Remove opcodes
    # Recombine fields
    for f,g in  [ \
                    (map, func_field_split), \
                    (filter, func_filter_asm), \
                    (map, func_strip_spaces), \
                    (map, func_add_address_labels), \
                    (map, func_remove_des_operand_prefix), \
                    (map, func_sub_ioreg_operand_prefix), \
                    (map, func_sub_word_directive), \
                    (map, func_sub_word_addr_byte_addr), \
                    (map, func_sub_lds_for_word_directive), \
                    (map, func_remove_opcodes), \
                    (map, func_recombine_fields), \
                ]:
        vavrdisasm_postproc_disasm = f(g, vavrdisasm_postproc_disasm)

    # Recombine lines
    vavrdisasm_postproc_disasm = '\n'.join(vavrdisasm_postproc_disasm)

    with open('bogus3.bin.disasm.S', 'w') as bogus_f:
        bogus_f.write(vavrdisasm_postproc_disasm)

    # Assemble with avr-as
    try:
        subprocess.check_output(['avr-as', '-mmcu', 'avrxmega7', '-mall-opcodes', 'bogus3.bin.disasm.S', '-o', 'bogus3.bin.disasm.o'], stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError as e:
        print "Error! Assembly failed!"
        print e.output
        print "Please send bogus3.bin to the author!"
        test3_passed = False
        break

    # Link with avr-ld
    try:
        subprocess.check_output(['avr-ld', '-mavrxmega7', 'bogus3.bin.disasm.o', '-o', 'bogus3.bin.disasm.out'], stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError as e:
        print "Error! Linking failed!"
        print e.output
        print "Please send bogus3.bin to the author!"
        test3_passed = False
        break

    # Convert with avr-objcopy
    try:
        subprocess.check_output(['avr-objcopy', '-O', 'ihex', 'bogus3.bin.disasm.out', 'bogus3.bin.disasm.hex'], stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError as e:
        print "Error! Binary conversion failed!"
        print e.output
        print "Please send bogus3.bin to the author!"
        test3_passed = False
        break

    # Generate disassembly again
    vavrdisasm_second_disasm = subprocess.check_output(["./vavrdisasm", "--no-destination-comments", "bogus3.bin.disasm.hex"])
    with open('bogus3.bin.second_disasm', 'w') as bogus_f:
        bogus_f.write(vavrdisasm_second_disasm)

    # Check for mismatch
    if vavrdisasm_second_disasm != vavrdisasm_orig_disasm:
        print "Error! Disassembled assembled disassembly mismatch!"
        print "Please send bogus3.bin to the author!"
        test3_passed = False
        break

# Clean up files
os.remove("bogus3.bin.orig_disasm")
os.remove("bogus3.bin.second_disasm")
os.remove("bogus3.bin.disasm.S")
os.remove("bogus3.bin.disasm.o")
os.remove("bogus3.bin.disasm.out")
os.remove("bogus3.bin.disasm.hex")

if test3_passed:
    # Clean up files
    os.remove("bogus3.bin")

    print "SUCCESS disassembling assembled disassembly test\n"
else:
    print "FAILURE disassembling assembled disassembly test\n"


if test1_passed and test2_passed and test3_passed:
    print "SUCCESS all tests pasts"
    sys.exit(0)

sys.exit(-1)

