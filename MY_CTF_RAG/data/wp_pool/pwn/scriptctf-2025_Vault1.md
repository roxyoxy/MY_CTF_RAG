# scriptctf-2025_Vault1

# Vault | Connor Chang

- Description: Congragulations to be part of Connor's Fantastic Vault's prelaunch. You can feel safe to store whatever you want in our vault without risk of compromise.

vault:attachments/vault

# Writeup

Looking at the binary in ghidra, we can see that the binary takes input uses gets (which allows for buffer overflow), and prints the output using printf (which allows for format string vulnerability). You can use the printf vulnerability to leak adrresses from libc to get the libc base, and to leak the canary. With this information, you can use a pwntools script to perform a ret2libc attack on the buffer overflow to call `system("/bin/sh")`.

# Flag - scriptCTF{g3t5_4nd_pr1n7f==b4d}
