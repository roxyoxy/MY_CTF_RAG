# justctf 2025 private~3

# tape

- 32bit binary
- IO pwning
- write @ FILE structure on the heap
- rewind(FILE) available
- heap hardened, disabled vtable overwrite, disabled wide_data modification
- seccomp preventing executing shell (open, read, write is the expected solution)
