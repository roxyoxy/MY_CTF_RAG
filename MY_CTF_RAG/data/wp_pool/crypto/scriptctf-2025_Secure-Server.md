# scriptctf-2025_Secure-Server

# Secure Server | NoobMaster

- Description: John Doe uses this secure server where plaintext is never shared. Our Forensics Analyst was able to capture this traffic and the source code for the server. Can you recover John Doe's secrets?

- files.zip: /attachments/files.zip

# Write-up

Via the packer capture, you can find out all the input/output transmitted. Based on the source code, you are given:
* `flag XOR key1`
* `flag XOR key1 XOR key2`
* `flag XOR key2`
XORing these three together yields the flag (the keys cancel out and `flag XOR flag XOR flag = flag`)

# Flag - scriptCTF{x0r_1s_not_s3cur3!!!!}