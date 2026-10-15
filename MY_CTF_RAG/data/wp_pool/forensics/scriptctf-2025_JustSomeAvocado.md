# scriptctf-2025_JustSomeAvocado

# Just Some Avocado | Connor Chang

Description: just an innocent little avocado!

avocado.jpg:attachments/avocado.jpg

# Writeup

You can use binwalk to extract a zip file. The zip file has a password that can be brute forced with hashcat or john the ripper. After unipping that zip file, you will get a wav file and another zip file. The new zip file cannot be brute forced with hashcat or john, but putting the wav file into a spectogram will reveal the password to the zip file and give you the flag.

# Flag - scriptCTF{1_l0ve_d41_v3r0n}
