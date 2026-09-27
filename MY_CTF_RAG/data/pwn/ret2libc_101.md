# ret2libc 101

The binary has NX enabled and no win function, so we pivot to ret2libc.
First leak the libc base address by calling puts on a GOT entry, then
return to main for a second pass. Compute the offset for libc-2.31 and
build the chain: system, exit, and /bin/sh.

Steps:
1. Overflow the buffer and pop rdi with the GOT address of puts.
2. Call puts to print the leaked address of the resolved function.
3. Subtract the known offset inside libc-2.31 to recover the libc base.
4. ROP chain: pop rdi -> /bin/sh, ret alignment, call system.

The one_gadget tool can shorten the final chain when constraints allow.
