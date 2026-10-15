# scriptctf-2025_EaaS

# EaaS | NoobMaster

- Description: Email as a Service! Have fun...

eaas.zip:attachments/eaas.zip

# Write-up

AES-CBC bit flipping attack. 

Let's take a look at how AES-CBC works:


    For the first block: encrypt(first_16_bytes) XOR IV (Initial Vector)
    For the second block: encrypt(16_through_32_bytes) XOR FIRST_BLOCK_CIPHERTEXT
    For the third block: encrypt(32_through_48_bytes) XOR SECOND_BLOCK_CIPHERTEXT and so on... The decryption is the same, just call the decrypt function instead of encrypt.

Since it XORs with a ciphertext block after encrypting, if we can control the ciphertext (which we do), we can cause the second block to decrypt into arbitrary text! This would cause the first block to turn into random garbage, but the second block is in our control! If we carefully create our payload, we start with `16 random bytes + 'a,' + our email's first byte XOR 1 + rest of email` and provide this as input for "password". This would pass the first check and give us the encrypted value. Then, we can XOR the third byte with 1. This would cause the first block to be garbage, but the second block will now decrypt into our email! The random garbage does not matter because the second block starts with a comma, which would count it as another recipient. We duplicate the payload but instead of using our email, we use `@script.sorcerers` and follow the same steps. Final payload looks like: `16 random bytes + 'a,' + our email's first byte XOR 1 + rest of email + ',' + 16 random bytes + '@sbript.sorcerers'` (the `c` XOR 1 = `b`).

Then, we take the ciphertext and modify the specific bytes accordingly.

Solve at `solve/solve.py`

# Flag: scriptCTF{CBC_1s_s3cur3_r1ght?}