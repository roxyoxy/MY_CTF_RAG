# justctf-2025_private~2

## shellcode_printer

- task allocs rwx memory, pointer to this memory is stored on stack
- fmt string vulnerability is present
- use fmt string vuln to write shellcode to the allocated memory
- each write advances the pointer by 2 bytes
- last shellcode instruction should jump to the start of the shellcode
- profit
