# Heap use-after-free notes

Classic use-after-free in a menu-driven heap challenge. The program frees
a note object but keeps the dangling pointer in the global table, so a
follow-up allocation of the same size reclaims the slot and lets us
overwrite the vtable or a function pointer.

Exploit path:
1. Allocate two notes of the same fastbin size, then free the first.
2. Allocate a controlled string that lands on the freed chunk.
3. Trigger the dangling call to hijack control flow.

tcache poisoning in glibc 2.31 requires a valid next pointer check, so
overwrite __free_hook or __malloc_hook with system and pass /bin/sh.
