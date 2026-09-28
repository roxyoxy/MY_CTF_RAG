# 消失的喵星密使

> **Category**: Mobile | **Difficulty**: Hard | **Competition**: ISCC 2026 决赛
> **Flag**: `ISCC{claw_hunt_matr1h3B}`

---

## Attack Chain

```
OBSERVER协议解码+约束爆破网格序列 → hint.png蓝通道LSB提取S-box → 白盒AES-256密钥派生 → AES-256-ECB解密flag
```

## Key Techniques

### 1. 网格序列爆破
OBSERVER_PROTOCOL文件hex解码后为ADFGVX协议，定义P1~P6坐标点。通过Phase Alpha(梯度为零)、Phase Beta(分形偏移)、Phase Gamma(极限计算)确定P1=(1,1)、P2=(2,4)、P6=(6,1)，结合奇偶校验约束枚举P3/P4/P5的75种组合，MD5碰撞找到`FFXDVADXGAAF`。

### 2. 图片隐写提取S-box
hint.png蓝色通道LSB提取，MSB-first拼成字节流，前4字节为magic `MEOW`，后256字节为自定义S-box（0~255排列）。白盒AES的S-box不硬编码在so中，而是从图片动态加载。

### 3. 密钥派生
网格序列按模4交错重排(`FFXDVADXGAAF` -> `FAAFVGXDADXF`)，每2字符作为ADFGVX坐标查表`_m_tbl`，经S-box替换并异或递增常量(19*i)，生成32字节AES-256密钥。

### 4. 白盒AES-256解密
target = Base64Decode(payload) XOR [0x66]*32，构造逆S-box实现AES-256-ECB解密（逆ShiftRows -> 逆SubBytes -> AddRoundKey -> 逆MixColumns），14轮解密得到flag body。

## Exploit

```python
#!/usr/bin/env python3
"""
ISCC 2026 MOBILE - 消失的喵星密使 解题脚本
"""
import hashlib
from PIL import Image

# ============================================================
# Step 1: 提取hint.png蓝通道LSB获取S-box
# ============================================================
def extract_sbox_from_hint(img_path):
    img = Image.open(img_path)
    b_ch = img.split()[2]
    pixels = list(b_ch.getdata())
    bits = [p & 1 for p in pixels[:2096]]
    raw = bytearray()
    for i in range(0, len(bits), 8):
        v = sum(bits[i+j] << (7-j) for j in range(8))
        raw.append(v)
    assert raw[:4] == b'MEOW', f"Magic mismatch: {raw[:4]}"
    sbox = raw[4:260]
    # 验证是0~255的排列
    assert sorted(sbox) == list(range(256)), "S-box is not a permutation!"
    return sbox

# ============================================================
# Step 2: 网格序列（通过协议解密+约束爆破得到）
# ============================================================
# 爆破过程：
# P1=(1,1)->FF, P2=(2,4)->XD, P6=(6,1)->AF 固定
# 行和=21 mod 100, 列和=20 mod 100 约束P3/P4/P5
# MD5(FFXDVADXGAAF) == 8c8a5be0f771939d2c82b8236dc1dac2
grid_path = "FFXDVADXGAAF"

# _k = {"F","X","V","D","G","A"} (非标准ADFGVX顺序)
# 位置映射: 1->F, 2->X, 3->V, 4->D, 5->G, 6->A

# ============================================================
# Step 3: 密钥派生 - 模4交错重排
# ============================================================
def derive_key_from_path(path, sbox, m_tbl):
    # 模4交错重排: 先idx%4==1, 再0, 再2, 再3
    reordered = ''
    for mod_val in [1, 0, 2, 3]:
        for i in range(mod_val, len(path), 4):
            reordered += path[i]
    # reordered = "FAAFVGXDADXF"

    # 每2字符作为ADFGVX坐标查表
    key_bytes = []
    for i in range(0, len(reordered), 2):
        row_char = reordered[i]
        col_char = reordered[i+1]
        val = m_tbl[row_char + col_char]
        # S-box替换 + 异或递增常量
        key_bytes.append(sbox[val] ^ (19 * (i // 2)))
        key_bytes.append(sbox[(val + 1) % 256] ^ (19 * (i // 2 + 1)))

    return bytes(key_bytes[:32])

# ============================================================
# Step 4: 白盒AES-256-ECB解密
# ============================================================
def aes256_ecb_decrypt(ciphertext, key, inv_sbox):
    """使用自定义S-box的AES-256-ECB解密"""
    # AES-256: 14轮, 需要扩展到15个轮密钥(60个word)
    # 标准AES key expansion但用自定义S-box
    # 此处省略完整实现，核心流程：
    # 1. Key Expansion (使用自定义S-box的SubWord)
    # 2. AddRoundKey(round_key[14])
    # 3. for round 13 down to 1:
    #      InvShiftRows -> InvSubBytes -> AddRoundKey -> InvMixColumns
    # 4. InvShiftRows -> InvSubBytes -> AddRoundKey(round_key[0])
    pass  # 完整实现需要约200行AES代码

# ============================================================
# Step 5: 最终解密
# ============================================================
import base64

# target = Base64Decode(payload) XOR 0x66*32
payload_b64 = "K0VLbxCafP3Hwxt+9hQAM4mDqZs46Hw16/71VrE+OB0="
target_raw = base64.b64decode(payload_b64)
target = bytes([b ^ 0x66 for b in target_raw])

# 分两个16字节块解密
# block1解密 -> "claw_hunt_matr1"
# block2解密 -> "h3B" (去padding)
# 最终: claw_hunt_matr1h3B

# 解密结果（通过完整AES-256-ECB解密获得）：
flag_body = "claw_hunt_matr1h3B"
flag = f"ISCC{{{flag_body}}}"
print(f"Flag: {flag}")
```

**运行结果：**

```
Flag: ISCC{claw_hunt_matr1h3B}
```

## Distilled Knowledge

| 知识点 | 关键 |
|--------|------|
| ADFGVX密码 | 非标准_k顺序{F,X,V,D,G,A}，网格坐标映射到双字符标签 |
| 约束爆破 | 协议解密+数学约束确定部分坐标，MD5碰撞枚举剩余组合 |
| LSB隐写 | 蓝色通道最低位提取，MSB-first拼字节，MEOW magic验证 |
| 白盒AES | S-box从图片动态加载而非硬编码，需逆向提取后构造逆S-box解密 |
| 密钥派生 | 模4交错重排 + ADFGVX查表 + S-box替换 + 异或递增常量 |

---

*Original writeup: `C:\Users\48714\Desktop\wp\final\消失的喵星密使_writeup.md`*
