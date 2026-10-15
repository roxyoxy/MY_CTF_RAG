# justctf 2025 private~1

## prospector

- task has no libc, no got, no bss
- use `read()` to find writable address in memory in one session (not required, after some initial testing I increased amount of bits that can be leaked via `player->score`)
- then ROP using gadgets from ld.so (not that many of them are available)
- 1b bruteforce
