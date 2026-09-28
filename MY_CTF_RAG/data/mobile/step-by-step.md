# 按步取匙

> **Category**: Mobile | **Difficulty**: Hard | **Competition**: ISCC 2026 区域赛
> **Flag**: `ISCC{9c3848064812451c4ec56a760a4318728d635565034c40c4}`

---

## Attack Chain

```
APK解包 → XOR解密凭据 → HKDF派生会话密钥 → LFSR+Feistel逆向Challenge1 → 哈希链Challenge2 → Token+SHA256 Challenge3 → 拼接生成flag
```

## Key Techniques

### 1. 提取登录凭据 — XOR 解密 + 字符串拼接

IDA 分析 `libzkpcore.so` 的 `.rodata` 段：

- **XOR 密钥**（偏移 `0x13D4`）：`k3y_j4va_k3y!!!!`
- **加密用户名**（偏移 `0x13E4`）：逐字节 XOR 解密得 `zkp_master`
- 密码由三个字符串片段拼接：`"Zkp_" + "M_as" + "t3er"` = `"Zkp_M_ast3er"`

### 2. HKDF 会话密钥派生

```
hash2 = SHA256("Zkp_M_ast3er")
hash3 = SHA256("CTF_ZKP_apk-8_SALT_V3")
PRK   = HMAC_SHA256(key=hash3, msg=hash2)
session_key = HMAC_SHA256(key=PRK, msg=b"AUTH_OK\x01")
```

派生出 32 字节会话密钥：`9d96a1a61528bb6c12ed11a4ae37e32f6f973d24330f627b35396424b71aaeea`

### 3. Challenge 1 — LFSR + Feistel 密码逆向

16-bit Galois LFSR（多项式 `0xB400`，种子 `0x974B`）生成 8 个伪随机值，打包为 4 个轮密钥，执行 4 轮 Feistel 加密，比较结果是否等于 `0xF3857EB2`。

Feistel 网络天然可逆，从目标值反向推导 4 轮即可得到输入：

- 轮密钥：`[0x94D8D810, 0x95385A75, 0x2E6D8EAE, 0x3F729D2C]`
- 轮函数：8 个 nibble 的 S-Box 置换
- 逆向求解 → **Challenge 1 答案：`C5BD08CA`**

### 4. Challenge 2 — 确定性哈希链

答案不依赖用户输入，由固化数据决定：`.rodata` 固定字节 → XOR → LCG 迭代 7 次 → 字节重排 → SHA-256。

**Challenge 2 答案：`c6c822afedae2ef59e21a92f9c4e11b1`**

中间令牌：`SHA256(c2_answer + "CTF_ZKP_INTERMEDIATE_SEED_V2")[:16].hex()` = `ccb1422317622cf651f9cbc1c8bcb714`

### 5. Challenge 3 — Token + SHA-256 验证

读取 token 前 8 字符按大端序组装为 `uint64`，与 `0x48B11DC893881847` XOR，SHA-256 取前 16 hex。

**Challenge 3 答案：`4ade28a174b4b568`**

### 6. Flag 生成

```python
c1_hash = SHA256("t1_static_C5BD08CA")[:8]
c2_hash = SHA256("t2_dynamic_c6c822afedae2ef59e21a92f9c4e11b1")[:8]
c3_hash = SHA256("t3_android_4ade28a174b4b568" + token)[:8]
flag_data = b"CTF_ZKP_FLAG_V2" + session_key + c1_hash + c2_hash + c3_hash
flag = "ISCC{" + SHA256(flag_data)[:24].hex() + "}"
```

## Exploit

```python
import hashlib, hmac, struct

# === XOR 解密用户名 ===
xor_key = b"k3y_j4va_k3y!!!!"
encrypted = bytes([0x11,0x58,0x09,0x00,0x07,0x55,0x05,0x15,
                   0x3a,0x19,0x62,0x2f,0xb2,0x52,0xb4,0x1c])
username = bytes([e ^ k for e, k in zip(encrypted, xor_key)])[:10].decode()
password = "Zkp_" + "M_as" + "t3er"

# === HKDF 会话密钥 ===
hash2 = hashlib.sha256(password.encode()).digest()
hash3 = hashlib.sha256(b"CTF_ZKP_apk-8_SALT_V3").digest()
prk = hmac.new(hash3, hash2, hashlib.sha256).digest()
session_key = hmac.new(prk, b"AUTH_OK\x01", hashlib.sha256).digest()

# === Challenge 1: LFSR + Feistel 逆向 ===
def lfsr_step(state):
    v = state >> 1
    if state & 1: v ^= 0xB400
    return v & 0xFFFF

state = 0x974B
v21 = []
for _ in range(8):
    state = lfsr_step(state)
    v21.append(state)

v20 = [
    (v21[1] | (v21[0] << 16)) ^ 0x6B7D13C2,
    (v21[3] | (v21[2] << 16)) ^ 0xF0D1DC81,
    (v21[5] | (v21[4] << 16)) ^ 0x6D17AF13,
    (v21[7] | (v21[6] << 16)) ^ 0x9BACCF43,
]

perm = [0x0e,0x04,0x0d,0x01,0x02,0x0f,0x0b,0x08,
        0x03,0x0a,0x06,0x0c,0x05,0x09,0x00,0x07]

def feistel_f(a1, perm):
    result = 0
    for i in range(8):
        nibble = (a1 >> (4 * i)) & 0xF
        result |= perm[nibble] << (4 * i)
    return result & 0xFFFFFFFF

target = 0xF3857EB2
v8 = (target >> 16) & 0xFFFF
v9 = target & 0xFFFF
for k in range(3, -1, -1):
    v8_old = v9
    dup = v8_old | (v8_old << 16)
    f_input = (dup ^ v20[k]) & 0xFFFFFFFF
    v5 = feistel_f(f_input, perm)
    F = ((v5 & 0xFFFF) ^ ((v5 >> 16) & 0xFFFF)) & 0xFFFF
    v9 = (v8 ^ F) & 0xFFFF
    v8 = v8_old
c1_answer = f"{((v9 << 16) | v8):08X}"

# === Challenge 2/3 ===
c2_answer = "c6c822afedae2ef59e21a92f9c4e11b1"
token = hashlib.sha256((c2_answer + "CTF_ZKP_INTERMEDIATE_SEED_V2").encode()).digest()[:16].hex()

v11 = 0
for i in range(8):
    v11 = (ord(token[i]) | (v11 << 8)) & 0xFFFFFFFFFFFFFFFF
v18 = v11 ^ 0x48B11DC893881847
c3_answer = hashlib.sha256(struct.pack('<Q', v18)).hexdigest()[:16]

# === Flag ===
c1_hash = hashlib.sha256(("t1_static_" + c1_answer).encode()).digest()[:8]
c2_hash = hashlib.sha256(("t2_dynamic_" + c2_answer).encode()).digest()[:8]
c3_hash = hashlib.sha256(("t3_android_" + c3_answer + token).encode()).digest()[:8]
flag_data = b"CTF_ZKP_FLAG_V2" + session_key + c1_hash + c2_hash + c3_hash
flag_hex = hashlib.sha256(flag_data).digest()[:24].hex()
print(f"Flag: ISCC{{{flag_hex}}}")
```

**运行结果：**

```
Username: zkp_master  Password: Zkp_M_ast3er
Session Key: 9d96a1a61528bb6c12ed11a4ae37e32f6f973d24330f627b35396424b71aaeea
Challenge1: C5BD08CA
Challenge2: c6c822afedae2ef59e21a92f9c4e11b1
Token: ccb1422317622cf651f9cbc1c8bcb714
Challenge3: 4ade28a174b4b568
Flag: ISCC{9c3848064812451c4ec56a760a4318728d635565034c40c4}
```

## Distilled Knowledge

| 知识点 | 关键 |
|--------|------|
| XOR解密 | 逐字节异或还原明文 |
| HKDF | HMAC-based密钥派生 |
| LFSR | Galois型，多项式+种子 |
| Feistel网络 | 天然可逆，逆向轮函数 |
| ZKP概念 | 零知识证明框架 |

---

*Original writeup: `C:\Users\48714\Desktop\wp\区域赛\按步取匙_WriteUp_final.md`*
