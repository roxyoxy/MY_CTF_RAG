# scriptctf-2025_vm

# vm | Connor Chang

- Description: my friend sent this wierd binary that i cant run. plz help me get his flag

reverse_me:attachments/reverse_me
check.bin:attachments/check.bin

# Writeup

`reverse_me` gives a vm with some custom operations. Using the operations, we can decode `check.bin` to find it to be a bunch of equations checking values of the flag. Using a z3 script can easily find the flag.

# Flag - scriptCTF{5up3r_dup3r_345y_vm_r3v}
