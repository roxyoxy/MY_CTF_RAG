# scriptctf-2025_Vault3

# Vault | ConnorChang

- Description: Welcome to Connor's Fantastic Vault, where you can store all you valuables. We have gone through many renovations in our security and hopefully there should be no more hacks in our system. (Note: this binary uses glibc version 2.30 without tcache)

- vault:attachments/vault
- libc.so.6:attachments/libc.so.6
- ld.so.2:attachments/ld.so.2

# Writeup

binary basically has 8 byte overflow which allows for some unlinking shenanigins. heres my lazy writeup for now: 
(smthing like heap overflow -> overwrite next chunk's size flag to make it seem like cur chunk is free -> set 
prev chunk size to 0x80 -> change fd and bk pointers to be at vaults with them pointing back at correct memory -> that will make vaults point 0x18 before itself -> overwrite it so that vaults points 8 bytes before __free_hook -> overwrite that with b'/bin/sh\x00' + p64(libc.sym.system) -> shell :)

# Flag - scriptCTF{y_d0_u_k33p_h4ck1n6_my_v4ul7:c}
