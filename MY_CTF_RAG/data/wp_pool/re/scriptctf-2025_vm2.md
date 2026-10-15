# scriptctf 2025 vm2

# vm2 | Connor Chang

- Description: my friend tells me that he made the vm smarter and that i have to write a script in his weird vm code

reverse_me:attachments/reverse_me

# Writeup
The new vm added some new opperations that weren't used in the check.bin from vm. Reversing the `reverse_me` shows us that we need to implement a palindrome checker in the custom binary. The restrictions for the # of ops and # of instructions are a bit strict, but an example of a working script can be found in solve/solve.py.

# Flag - scriptCTF{m4yb3_h4rd3r_ch4ll_n3x7_71m3}
