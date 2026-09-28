# Conway's Trap

> **Category**: Reverse | **Difficulty**: Hard | **Competition**: ISCC 2026 决赛
> **Flag**: `ISCC{NocwosxmveVar{i9uEtwc5E}`

---

## Attack Chain

```
逆向Conway变换还原seed → 发现VEH异常劫持(int3→隐藏函数0x401D00) → 从栈上提取XOR 0xCC加密的校验目标 → 逆向字节级校验算法(ROL3+XOR+ADD)求解flag
```

## Key Techniques

### 1. Conway变换还原Seed

程序将用户输入的hex串解码为16字节，经过5轮Conway变换后与硬编码目标值比较。Conway变换包含两步：
- **相邻字节融合**：对每对`(x, y)`计算`sum=(x+y)&0xFF`, `new_x=sum`, `new_y=ROL8(y^sum, 1)`
- **整体循环左移1位**

变换完全可逆：先反移位（循环右移1），再反字节对融合（从sum还原x、y）。

目标hash `df7b6a5d4da0f5facf32c4ee4b28b792` 反推5轮得seed `0b5e321c68e0e4fb2d972226e8c70f8d`。

### 2. VEH异常劫持 — 隐藏函数

IDA反编译`sub_401CB0`时发现seed和flag指针完全未使用——这是障眼法。真正原因：
- 程序在初始化时将`0x401CB0`首字节改为`0xCC`(int3断点)
- 注册了VEH handler(0x401DC0)，捕获EXCEPTION_BREAKPOINT后将EIP改为`0x401D00`
- **每次"调用"sub_401CB0，实际执行的是0x401D00处的隐藏函数**
- 字符串`DoNotPatchMe`暗示：patch掉int3会导致隐藏函数永远不执行

### 3. 隐藏函数校验逻辑

隐藏函数从栈上恢复参数（seed指针、flag指针、循环下标），**只有第一轮(i=0)执行真正校验**。栈上23字节常量逐字节XOR `0xCC`解密后得到比较目标，校验算法：

```
dl = seed[i & 0xF] ^ flag[i]
dl = (dl + (i * 0x17) & 0xFF) & 0xFF
dl = ROL8(dl, 3) ^ 0xAA
cmp dl, target[i]
```

校验通过后将seed前两字节覆写为`0xDE 0xAD`，主函数检测到DEAD后直接返回成功。

### 4. 逆推Flag

```
对每个 i (0 <= i < 23):
    t = target[i] ^ 0xAA
    t = ROR8(t, 3)
    t = (t - (i * 0x17) & 0xFF) & 0xFF
    flag[i] = seed[i % 16] ^ t
```

## Exploit

```python
#!/usr/bin/env python3
# Conway's Trap solver

def _rol(val, n):
    """8-bit rotate left"""
    return ((val << n) | (val >> (8 - n))) & 0xFF

def _ror(val, n):
    """8-bit rotate right"""
    return ((val >> n) | (val << (8 - n))) & 0xFF

def conway_forward(block):
    """正向一轮 Conway 变换"""
    b = list(block)
    # 相邻字节融合
    for j in range(0, 16, 2):
        acc = (b[j] + b[j+1]) & 0xFF
        b[j+1] = _rol(b[j+1] ^ acc, 1)
        b[j] = acc
    # 整体循环左移1
    head = b[0]
    for j in range(15):
        b[j] = b[j+1]
    b[15] = head
    return bytes(b)

def conway_backward(block):
    """逆向一轮 Conway 变换"""
    # 先逆整体移位（循环右移1）
    tmp = [0] * 16
    tmp[0] = block[15]
    for j in range(1, 16):
        tmp[j] = block[j-1]
    # 再逆字节对融合
    out = [0] * 16
    for j in range(0, 16, 2):
        s = tmp[j]
        t = tmp[j+1]
        orig_y = _ror(t, 1) ^ s
        orig_x = (s - orig_y) & 0xFF
        out[j] = orig_x
        out[j+1] = orig_y
    return bytes(out)

# ──────────────────────────────────────
# Part A: 逆推 seed
# ──────────────────────────────────────
known_hash = "df7b6a5d4da0f5facf32c4ee4b28b792"
cur = bytes.fromhex(known_hash)
for _ in range(5):
    cur = conway_backward(cur)
recovered_seed = cur
print("[*] recovered seed:", recovered_seed.hex())

# 正向校验
check = recovered_seed
for _ in range(5):
    check = conway_forward(check)
assert check.hex() == known_hash
print("[*] seed verified OK")

# ──────────────────────────────────────
# Part B: 逆推 flag
# ──────────────────────────────────────
# 隐藏函数栈上的加密常量（VA 0x401D20）
chipertext = [
    0x4C, 0x24, 0x9D, 0xE3, 0x7D, 0x56, 0x57, 0xDF,
    0xFE, 0x68, 0xB4, 0x44, 0x13, 0x59, 0x23, 0x0E,
    0x11, 0x73, 0x41, 0x67, 0xD8, 0xA3, 0xBA
]
# 异或 0xCC 解密
decrypted = [c ^ 0xCC for c in chipertext]

# 逆向校验公式求解每个 flag 字节
plain = []
for idx in range(23):
    v = decrypted[idx] ^ 0xAA
    v = _ror(v, 3)
    v = (v - ((idx * 0x17) & 0xFF)) & 0xFF
    v ^= recovered_seed[idx % 16]
    plain.append(v)

answer = "ISCC{" + "".join(chr(b) for b in plain) + "}"
print("[*] flag:", answer)

# ──────────────────────────────────────
# Part C: 正向回验
# ──────────────────────────────────────
for idx in range(23):
    dl = recovered_seed[idx % 16] ^ plain[idx]
    dl = (dl + ((idx * 0x17) & 0xFF)) & 0xFF
    dl = _rol(dl, 3)
    dl ^= 0xAA
    assert dl == decrypted[idx], f"mismatch @ {idx}"
print("[*] round-trip check passed")
```

**运行结果：**

```
[*] recovered seed: 0b5e321c68e0e4fb2d972226e8c70f8d
[*] seed verified OK
[*] flag: ISCC{NocwosxmveVar{i9uEtwc5E}
[*] round-trip check passed
```

## Distilled Knowledge

| 知识点 | 关键 |
|--------|------|
| VEH异常劫持 | 运行时写入int3，VEH handler重定向EIP到隐藏函数 |
| 反静态分析 | IDA看不到隐藏分支，patch掉int3反而走入假路径 |
| Conway变换 | 相邻字节融合+循环移位，完全可逆 |
| 栈上数据加密 | 常量XOR 0xCC解密后才可见真实比较目标 |
| 字节级校验逆向 | seed^flag + ADD + ROL3 + XOR链，逐字节独立可逆推 |

---

*Original writeup: `C:\Users\48714\Desktop\wp\final\Conway's Trap_WriteUp.md`*
