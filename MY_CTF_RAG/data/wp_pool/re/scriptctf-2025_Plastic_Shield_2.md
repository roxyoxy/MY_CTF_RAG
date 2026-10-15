# scriptctf 2025 Plastic Shield 2


# Plastic Shield 2 | Ashray Shah

- Description: Okay! Fixed last time's issue. Seriously though, I swear this one is unbreakable.

plastic-shield-2:attachments/plastic-shield-2

# Writeup

This time, the entire input is indeed hashed. However, only the first three hex characters (nibbles) of the hash are used for the key and IV, the rest being filled with zeros. This lets us take the encoded data, and brute force it, since there is only 16^3 possible combinations (4096).

# Flag: scriptCTF{00p513_n07_4641n!}
