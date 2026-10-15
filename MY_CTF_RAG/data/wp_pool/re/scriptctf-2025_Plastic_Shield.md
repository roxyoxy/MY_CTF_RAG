# scriptctf-2025_Plastic Shield


# Plastic Shield | Ashray Shah

- Description: OPSec is useless unless you do it correctly.

plastic-shield:attachments/plastic-shield

# Writeup

At first glance, the binary seems to be hashing the password (Blake2b) and using result to decrypt a prestored hash (AES). However, after some static analysis or just messing around with the program, it becomes obvious that the program only hashes one letter, and the rest of the password is unused. It grabs the password in the 6/10ths position (rounding down) and uses that. This means that we only have to brute force all ascii letters, until we find one that works. The password ends up being the letter "\`".

# Flag: scriptCTF{20_cau541i71e5_d3f3n5es_d0wn}
