# scriptctf 2025 Vault2

# Vault2 | Connor Chang

- Description: Welcome to Connor's Fantastic Vault, where you can store all your valuables. All valuables are encrypted before stored, to ensure noone's data is compromised if anything goes wrong! (Note: submit the flag in the format `scriptCTF{0xdeadbeefdeadbeef}`)

vault:attachments/vault

# Writeup

The key insight is that after `free(vaults[idx])`, vaults\[idx\] is never set to 0. You can alloc and free a vault, and change the user_id. User_id is now at the same memory address as the vault you originally alloced and printing the `user_id ^ slab_cookie`. Xoring this with user_id and the original leaked value gives you the flag.

# Flag - scriptCTF{0x31415926beef007}
