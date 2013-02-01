#!/usr/bin/python2

import os
import subprocess

### Run vavrdisasm through a fuzzer 50 times, to check the robustness of its
### format decoding and disassembly logic

test1_passed = False

for i in range(50):
    # Create a random 16384 byte file
    bogus_data = os.urandom(16384)
    with open('bogus.bin', 'w') as bogus_f:
        bogus_f.write(bogus_data)

    # Convert it to Atmel Generic format
    with open('bogus.gen', 'w') as bogus_gen_f:
        for i in range(len(bogus_data)/2):
            bogus_gen_f.write("%06x:%02x%02x\n" % (i, ord(bogus_data[2*i+1]), ord(bogus_data[2*i])))

    del bogus_data

    # Convert it to Intel HEX and Motorola S-Record formats with objcopy
    subprocess.check_call(["objcopy", "-I", "binary", "-O", "ihex", "bogus.bin", "bogus.hex"])
    subprocess.check_call(["objcopy", "-I", "binary", "-O", "srec", "bogus.bin", "bogus.srec"])

    # Generate disassemblies for each file format
    bogus_bin_disasm = subprocess.check_output(["./vavrdisasm", "-t", "binary", "bogus.bin"])
    bogus_gen_disasm = subprocess.check_output(["./vavrdisasm", "-t", "generic", "bogus.gen"])
    bogus_ihex_disasm = subprocess.check_output(["./vavrdisasm", "-t", "ihex", "bogus.hex"])
    bogus_srec_disasm = subprocess.check_output(["./vavrdisasm", "-t", "srec", "bogus.srec"])

    if (len(bogus_gen_disasm) != len(bogus_bin_disasm) or bogus_gen_disasm != bogus_bin_disasm):
        print "Error! Atmel Generic disassembly differs from Binary disassembly!"
        print "Please inform the author!"
        os.exit(-1)
    if (len(bogus_ihex_disasm) != len(bogus_bin_disasm) or bogus_ihex_disasm != bogus_bin_disasm):
        print "Error! Intel Hex disassembly differs from Binary disassembly!"
        print "Please inform the author!"
        os.exit(-1)
    if (len(bogus_srec_disasm) != len(bogus_bin_disasm) or bogus_srec_disasm != bogus_bin_disasm):
        print "Error! Motorola S-Record disassembly differs from Binary disassembly!"
        print "Please inform the author!"
        os.exit(-1)

# Clean up files
os.remove("bogus.bin")
os.remove("bogus.gen")
os.remove("bogus.hex")
os.remove("bogus.srec")

test1_passed = True

print "SUCCESS fuzzing test passed"

### Compare vavrdisasm output with avr-objdump 50 times

test2_passed = False

for i in range(50):
    # Create a random 16384 byte file
    bogus_data = os.urandom(16384)
    with open('bogus.bin', 'w') as bogus_f:
        bogus_f.write(bogus_data)
    del bogus_data

    # Generate disassemblies
    objdump_disasm = subprocess.check_output(["avr-objdump", "-b", "binary", "-m", "avr", "-D", "bogus.bin"])
    vavrdisasm_disasm = subprocess.check_output(["./vavrdisasm", "-t", "binary", "--no-destination-comments", "bogus.bin"])

    # Split disassemblies by newlines
    objdump_disasm = objdump_disasm.split("\n")
    vavrdisasm_disasm = vavrdisasm_disasm.split("\n")

    # Map and filter lambdas for post-processing
    lam_lowercase_and_split = lambda x: x.lower().split("\t")
    lam_trim_dest_comments = lambda x: x[0:4]
    lam_filter_non_asm = lambda x: len(x) >= 3
    lam_add_empty_operands = lambda x: [ x[0], x[1], x[2], '' ] if len(x) == 3 else x
    lam_strip_spaces = lambda x: [y.strip() for y in x]
    lam_opcode_big_endian = lambda x: [x[0], ' '.join(x[1].split(' ')[::-1]), x[2], x[3]]
    lam_sub_word_directive = lambda x: [ x[0], x[1], ".dw" if x[2] == ".word" else x[2], x[3]]
    lam_sub_word_addr_byte_addr = lambda x: [ x[0], x[1], x[2], "0x%x" % (int(x[3], 0)*2) if (x[2] == "call" or x[2] == "jmp") else x[3] ]
    lam_add_des_operand_prefix = lambda x: [ x[0], x[1], x[2], ("0x" + x[3]) if x[2] == "des" else x[3] ]

    ### Post-process objdump disassembly

    # Trim lines leading up to disassembly
    for j in range(len(objdump_disasm)):
        if ("<.data>:" in objdump_disasm[j]):
            break;
    objdump_disasm = objdump_disasm[j+1:]

    # Lower case all lines and split into fields
    objdump_disasm = map(lam_lowercase_and_split, objdump_disasm)
    # Trim destination comments
    objdump_disasm = map(lam_trim_dest_comments, objdump_disasm)
    # Filter out any non-asm lines (usually last line)
    objdump_disasm = filter(lam_filter_non_asm, objdump_disasm)
    # Add empty operands for no operand instructions
    objdump_disasm = map(lam_add_empty_operands, objdump_disasm)
    # Strip spaces
    objdump_disasm = map(lam_strip_spaces, objdump_disasm)
    # Format opcode bytes into big-endian order
    objdump_disasm = map(lam_opcode_big_endian, objdump_disasm)
    # Substitute .word for .dw
    objdump_disasm = map(lam_sub_word_directive, objdump_disasm)
    # Add "0x" prefix to des instruction operands
    objdump_disasm = map(lam_add_des_operand_prefix, objdump_disasm)

    # objdump_disasm is now an array of string arrays, [ [address, opcode, mneumonic, operands], ... ]

    ### Post-processing vavrdisasm diassembly

    # Lower case all lines and split into fields
    vavrdisasm_disasm = map(lam_lowercase_and_split, vavrdisasm_disasm)
    # Filter out any non-asm lines (usually last line)
    vavrdisasm_disasm = filter(lam_filter_non_asm, vavrdisasm_disasm)
    # Strip spaces
    vavrdisasm_disasm = map(lam_strip_spaces, vavrdisasm_disasm)
    # Replace call/jmp word addresses with byte addresses
    vavrdisasm_disasm = map(lam_sub_word_addr_byte_addr, vavrdisasm_disasm)

    # vavrdisasm_disasm is now an array of string arrays, [ [address, opcode, mneumonic, operands], ... ]

    test2_passed = True

    ### Compare disassembly outputs
    for i in range(min(len(objdump_disasm), len(vavrdisasm_disasm))):
        if objdump_disasm[i] != vavrdisasm_disasm[i]:
            # If the opcodes are the same, and this is an std / lds identity,
            # skip it
            if objdump_disasm[i][1] == vavrdisasm_disasm[i][1] and objdump_disasm[i][2] == 'std' and vavrdisasm_disasm[i][2] == 'lds':
                continue
            # If the opcodes are the same, and this is the ldi / ser identity,
            # skip it
            if objdump_disasm[i][1] == vavrdisasm_disasm[i][1] and objdump_disasm[i][2] == 'ldi' and vavrdisasm_disasm[i][2] == 'ser' and \
               vavrdisasm_disasm[i][3] + ", 0xff" == objdump_disasm[i][3]:
                continue

            print "Found mismatch!"
            print "objdump:   ", objdump_disasm[i]
            print "vavrdisasm:", vavrdisasm_disasm[i]
            print "Please inform the author!"
            print ""
            test2_passed = False

# Clean up files
os.remove("bogus.bin")

if test2_passed:
    print "SUCCESS avr-objdump compare test passed"
else:
    print "FAILURE avr-objdump compare test passed"

if test1_passed and test2_passed:
    print "SUCCESS all tests pasts"

