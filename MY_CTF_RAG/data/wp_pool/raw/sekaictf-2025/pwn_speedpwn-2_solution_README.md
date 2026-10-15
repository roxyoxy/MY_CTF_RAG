# pwn_speedpwn-2_solution_README

## Writeup

Clear OOB where we can write anywhere in heap. We dont have leak, but PIE is disabled. We can use tcache_perthread to allocate a chunk into bss, a few qwords after got. Now, we can change free_got to printf_plt. this allows us to get leaks using format string. After this, we change free to system and pop a shell.
