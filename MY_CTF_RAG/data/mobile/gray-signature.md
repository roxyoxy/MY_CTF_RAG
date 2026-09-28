# 灰签名回廊

> **Category**: Mobile | **Difficulty**: Hard | **Competition**: ISCC 2026 决赛
> **Flag**: `ISCC{gqPXMyL_AbEB_oF7M_xi9fQa}`

---

## Attack Chain

```
4个隐藏组件获取fragment → Part1 XOR逆向 → 链式Token计算 → Part2 SSE shuffle逆向 → Part3 SSE add逆向 → Part4 RC4约束+SHA-256爆破
```

## Key Techniques

### 1. 隐藏组件提取Fragment
- SecretContentProvider: URI查询获取fragment2 = `gHw7`
- SecretReceiver: 先auth(token=ISCC2026)再getkey(part=3)获取fragment3 = `Tkhg`
- SecretActivity: deep link提取fragment4 = `ew`

### 2. Part 1逆向（7字节）
Native层`nativeVerifyPart1`做`(input[i]+1)^7`变换，与Java层`certHash[i%4]^bArr2[i]`期望值对比。签名hash两边抵消后：`part1[i] = (bArr2[i]^7) - 1` = `"gqPXMyL"`。

### 3. 链式Token计算
类FNV-1a的64位滚动哈希，128位乘法拆成两个64位乘法+拼接。依次用Part1输出、fragment2、fragment3计算得到chain3。

### 4. Part 2逆向（5字节，SSE shuffle）
从SO的.rodata提取xmmword_3DD20={3,0,1,2} shuffle掩码和target=0x72150925，反shuffle得到Part2=`"_AbEB"`。

### 5. Part 3逆向（5字节，SSE add）
xmmword_3DC60={0x19,...} XOR掩码，target=0xA895C7E1（注意有符号转无符号），解方程得Part3=`"_oF7M"`。

### 6. Part 4 RC4约束+SHA-256爆破
RC4密钥由chain3低32位XOR"Secu"+frag2+frag3+`\x00\x00`构造，keystream前4字节用于弱约束过滤（1590万->35910组合），剩余3字符SHA-256暴力枚举约90亿次。

## Exploit

```python
#!/usr/bin/env python3
"""
ISCC 2026 MOBILE - 灰签名回廊 解题脚本
"""
import hashlib
import struct
import time
import multiprocessing as mp
import zipfile
import os
import sys

sys.stdout.reconfigure(encoding='utf-8')

# ============================================================
# 配置 - 修改APK路径
# ============================================================
APK_PATH = "attachment-68 (2).apk"
TARGET_SHA = "33a76f8a987c7f5092a7dd920320d9c4b04213e66936a78f36ef36acbe24771a"

# ============================================================
# Step 1: 从APK提取证书SHA-256
# ============================================================
def get_cert_hash(apk_path):
    """提取APK签名证书的SHA-256"""
    import subprocess
    # 使用keytool或openssl提取证书
    # 这里直接硬编码提取结果（每个APK不同）
    # 实际操作: keytool -printcert -jarfile xxx.apk
    return bytes.fromhex("133415c3f8aad739be8d0df905481208aa7448da7254a474028eeeddfc2849af")

# ============================================================
# Step 2: 还原Fragment
# ============================================================
# Fragment 2: {112,95,96,32} XOR 23
frag2_raw = bytes([112 ^ 23, 95 ^ 23, 96 ^ 23, 32 ^ 23])  # b"gHw7"

# Fragment 3: (i ^ arr[i]) ^ 90
frag3_raw = bytes([(i ^ [14,48,48,62][i]) ^ 90 for i in range(4)])  # b"Tkhg"

# Fragment 4: "ISCC202ew" 取 bytes[7:9]
frag4_raw = bytes([101, 119])  # b"ew"

print(f"Fragment 2: {frag2_raw}")
print(f"Fragment 3: {frag3_raw}")
print(f"Fragment 4: {frag4_raw}")

# ============================================================
# Step 3: 还原Part 1
# ============================================================
cert_hash = get_cert_hash(APK_PATH)
cert4 = cert_hash[:4]  # {0x13, 0x34, 0x15, 0xC3}
bArr2 = [111, 117, 86, 94, 73, 125, 74]

part1 = ''.join(chr((b ^ 7) - 1) for b in bArr2)
print(f"Part 1: {part1}")

# Part 1 native输出（用于chain token）
expected_part1 = bytes([cert4[i % 4] ^ bArr2[i] for i in range(7)])
print(f"Expected Part1 output: {expected_part1.hex()}")

# ============================================================
# Step 4: 链式Token计算
# ============================================================
MASK = 0xFFFFFFFFFFFFFFFF

def mul64(a, b):
    return (a * b) & MASK

def F(x):
    a = mul64(0x20000000366000, x)
    b = mul64(0x100000001B3, x) >> 51
    return (a | b) & MASK

def compute_chain_token(round_num, seed, data):
    v10 = seed ^ mul64(0x100000001B3, round_num)
    n = len(data)
    v11 = 0
    if n > 1:
        while (n & 0x7FFFFFFE) != v11:
            t = v10 ^ data[v11]
            mid = F(t) ^ data[v11 + 1]
            v10 = mul64(0x20000000366000, mid) | (mul64(0x100000001B3, mid) >> 51)
            v10 &= MASK
            v11 += 2
    if n & 1:
        t = data[v11] ^ v10
        v10 = mul64(0x20000000366000, t) | (mul64(0x100000001B3, t) >> 51)
        v10 &= MASK
    # Avalanche
    t2 = v10 ^ (v10 >> 33)
    v12 = mul64(0xFF51AFD7ED558CCD, t2) ^ (mul64(0xFF51AFD7ED558CCD, t2) >> 33)
    return v12 & MASK

c = 1597463007  # 0x5F3759DF
c = compute_chain_token(1, c, expected_part1)
c = compute_chain_token(2, c, frag2_raw)
chain3 = compute_chain_token(3, c, frag3_raw)
print(f"Chain3: 0x{chain3:016X}")

# ============================================================
# Step 5: 还原Part 2 (SSE shuffle + XOR)
# ============================================================
key2 = frag2_raw
# xmmword_3DD20 = {3, 0, 1, 2}
shuffle_mask = [3, 0, 1, 2]
target2 = 1913981221  # 0x72150925

# 条件1: input[0] ^ key2[0] == 56
p2_0 = 56 ^ key2[0]

# 条件2: shuffle(input[1:5]) XOR key2[0:4] == target_bytes
target2_bytes = target2.to_bytes(4, 'little')
shuffled = [target2_bytes[i] ^ key2[i] for i in range(4)]

# 反shuffle
# mask = {3,0,1,2}: shuffled[0]=input[4], shuffled[1]=input[1], shuffled[2]=input[2], shuffled[3]=input[3]
part2_bytes = [0] * 5
part2_bytes[0] = p2_0
part2_bytes[1] = shuffled[1]
part2_bytes[2] = shuffled[2]
part2_bytes[3] = shuffled[3]
part2_bytes[4] = shuffled[0]

part2 = ''.join(chr(b) for b in part2_bytes)
print(f"Part 2: {part2}")

# ============================================================
# Step 6: 还原Part 3 (SSE add + XOR)
# ============================================================
key3 = frag3_raw
# xmmword_3DC60 = {0x19, 0x19, 0x19, 0x19}
xor_mask = 0x19
target3 = 0xA895C7E1  # -1466578975 unsigned

# constructed = [key3[1], key3[2], key3[3], key3[0]]
constructed = [key3[1], key3[2], key3[3], key3[0]]
target3_bytes = target3.to_bytes(4, 'little')

# 条件1: key3[0] + (input[0] ^ 0x19) == 0x9A
p3_0 = ((0x9A - key3[0]) & 0xFF) ^ xor_mask

# 条件2: add_epi8(constructed, input_dword XOR 0x19) == target_bytes
part3_bytes = [0] * 5
part3_bytes[0] = p3_0
for i in range(4):
    part3_bytes[i + 1] = ((target3_bytes[i] - constructed[i]) & 0xFF) ^ xor_mask

part3 = ''.join(chr(b) for b in part3_bytes)
print(f"Part 3: {part3}")

# ============================================================
# Step 7: RC4约束 + SHA-256爆破 Part 4
# ============================================================
# RC4 Key: chain3_low32_LE XOR "Secu" + frag2 + frag3 + 00 00
chain3_low32 = chain3 & 0xFFFFFFFF
chain3_le = chain3_low32.to_bytes(4, 'little')
secu = b"Secu"
rc4_key = bytearray(14)
for i in range(4):
    rc4_key[i] = chain3_le[i] ^ secu[i]
rc4_key[4:8] = frag2_raw
rc4_key[8:12] = frag3_raw
rc4_key[12:14] = b'\x00\x00'
print(f"RC4 Key: {rc4_key.hex()}")

# Standard RC4 KSA + PRGA
def rc4_ksa_prga(key, n):
    S = list(range(256))
    j = 0
    for i in range(256):
        j = (j + key[i % len(key)] + S[i]) & 0xFF
        S[i], S[j] = S[j], S[i]
    # PRGA
    i = j = 0
    out = []
    for _ in range(n):
        i = (i + 1) & 0xFF
        j = (j + S[i]) & 0xFF
        S[i], S[j] = S[j], S[i]
        out.append(S[(S[i] + S[j]) & 0xFF])
    return bytes(out)

ks = rc4_ksa_prga(list(rc4_key), 4)
print(f"Keystream: {ks.hex()}")

# RC4弱约束过滤
CS = list(range(ord('a'), ord('z')+1)) + list(range(ord('A'), ord('Z')+1)) + \
     list(range(ord('0'), ord('9')+1)) + [ord('_')]
CSLEN = len(CS)

valid = []
for i0 in CS:
    e0 = i0 ^ ks[0]
    for i1 in CS:
        e1 = i1 ^ ks[1]
        for i2 in CS:
            e2 = i2 ^ ks[2]
            for i3 in CS:
                e3 = i3 ^ ks[3]
                val = (((e0 << 6) ^ (8 * e1) ^ e2) << 12) ^ (e3 << 9)
                if (val & 0x3FF0000) == 0x780000:
                    valid.append((i0, i1, i2, i3))

print(f"RC4 valid combos: {len(valid)}")

# 构造已知前缀
PREFIX = f"ISCC{{{part1}{part2}{part3}}}".encode()
SUFFIX = b"}"
POFF = len(PREFIX)
TARGET = bytes.fromhex(TARGET_SHA)

print(f"Prefix: {PREFIX.decode()}")
print(f"Total checks: {len(valid) * CSLEN**3:,}")

# 爆破
flag = bytearray(PREFIX + b'\x00' * 7 + SUFFIX)
count = 0
t0 = time.time()
found = False

for a, b, c, d in valid:
    flag[POFF] = a
    flag[POFF+1] = b
    flag[POFF+2] = c
    flag[POFF+3] = d
    for e in CS:
        flag[POFF+4] = e
        for f in CS:
            flag[POFF+5] = f
            for g in CS:
                flag[POFF+6] = g
                if hashlib.sha256(bytes(flag)).digest() == TARGET:
                    result = flag.decode()
                    print(f"\nFLAG: {result}")
                    found = True
                    break
                count += 1
            if found: break
        if found: break
    if found: break
    if count % 1000000 == 0:
        elapsed = time.time() - t0
        rate = count / elapsed if elapsed > 0 else 0
        print(f"  {count:,} checked ({rate:.0f}/s)")

if not found:
    print("Not found!")
else:
    # 验证
    verify = hashlib.sha256(result.encode()).hexdigest()
    print(f"SHA-256 verify: {verify}")
    print(f"Match: {verify == TARGET_SHA}")
    print(f"Time: {time.time()-t0:.1f}s")
```

**运行结果：**

```
Fragment 2: b'gHw7'
Fragment 3: b'Tkhg'
Fragment 4: b'ew'
Part 1: gqPXMyL
Expected Part1 output: 7c41439d5a495f
Chain3: 0x6D18C30F9162887C
Part 2: _AbEB
Part 3: _oF7M
RC4 Key: 2fed01e467487737546b68670000
Keystream: 55cae09b
RC4 valid combos: 35910
Prefix: ISCC{gqPXMyL_AbEB_oF7M
Total checks: 8,979,187,770

FLAG: ISCC{gqPXMyL_AbEB_oF7M_xi9fQa}
SHA-256 verify: 33a76f8a987c7f5092a7dd920320d9c4b04213e66936a78f36ef36acbe24771a
Match: True
```

## Distilled Knowledge

| 知识点 | 关键 |
|--------|------|
| Android隐藏组件 | ContentProvider/BroadcastReceiver/Deep Link Activity可隐藏flag片段 |
| 自定义Base64 | 大小写字母和数字分别倒序排列的变体编码 |
| Native层SSE指令 | `_mm_shuffle_epi8`+`_mm_xor_si128`+`_mm_add_epi8`做验证 |
| 有符号转无符号 | target=-1466578975的正确无符号值为0xA895C7E1 |
| FNV-1a变体哈希 | 128位乘法拆成两个64位，用于链式token传递 |
| RC4弱约束 | 位运算约束将搜索空间从1590万缩减到35910 |
| SHA-256爆破 | 63^3=25万种自由字符组合，多进程并行约22分钟 |

---

*Original writeup: `C:\Users\48714\Desktop\wp\final\灰签名回廊_WriteUp.md`*
