# 数字奥秘

> **Category**: MISC | **Difficulty**: Hard | **Competition**: ISCC 2026 决赛
> **Flag**: `ISCC{OxCjAYY2ub71VeqapwUAJYmzSDWQEMBm}`

---

## Attack Chain

```
PNG隐写提取tEXt块 → BF代码审计提取cell值 → 修复VM取整缺陷 → 广义CRT合并余数 → SHA-256密钥派生+XOR解密
```

## Key Techniques

### 1. PNG隐写识别
`challenge.bin`实际为PNG文件，文件体积(644KB)远超1376x768图像正常体积。遍历PNG chunk发现5个tEXt元数据块和IEND后附加的Python VM源码。

### 2. BF代码审计
SYNC_CODE约5万字符，几乎是清一色的`+`，精确定位8处`*#>`序列。统计每组`+`的数量得到cell终值：`[9766, 13086, 3108, 5927, 8767, 1447, 1156, 5926]`。

### 3. VM取整缺陷修复
VM的`#`指令使用`int()`截断浮点数，对`cell*0.1`有损。改用`math.ceil()`向上取整修正读数。截断结果`[976,...]`，修正后`[977,...]`，每格差1。

### 4. 广义中国剩余定理
8个模数不两两互素（如gcd(1024,1616)=16），需用扩展欧几里得求模逆的广义CRT两两合并策略，合并结果为`3367889`。

### 5. SHA-256密钥派生+XOR解密
将CRT结果与四层Base64解码得到的标签拼接：`"3367889:SYNC_SENSOR_2025"`，取SHA-256哈希作为32字节循环XOR密钥解密payload。

## Exploit

```python
import struct, math, hashlib, base64, sys

# ---------- 第一部分：从 PNG 的 tEXt 块中抽取隐藏字段 ----------
def read_png_text_chunks(bin_path):
    """解析 PNG 文件，返回所有 tEXt 块的 key-value 字典"""
    raw = open(bin_path, 'rb').read()
    meta = {}
    cursor = 8
    while cursor + 12 <= len(raw):
        block_len = struct.unpack('>I', raw[cursor:cursor+4])[0]
        block_tag = raw[cursor+4:cursor+8]
        block_dat = raw[cursor+8:cursor+8+block_len]
        if block_tag == b'tEXt':
            sep = block_dat.index(0)
            meta[block_dat[:sep].decode()] = block_dat[sep+1:]
        cursor += 12 + block_len
        if block_tag == b'IEND':
            break
    return meta

chunks = read_png_text_chunks("challenge.bin")
bf_program = chunks['SYNC_CODE'].decode('latin-1')
mod_list   = eval(chunks['PERIODS'].decode())
cipher_b64 = chunks['SYNC_PAYLOAD'].decode()

# ---------- 第二部分：还原 BF 程序中各格的真实数值 ----------
# BF 代码由连续的 '+' 和分散的 '*#' 组成
# 每遇到 '*' 时，此前累计的 '+' 数量就是当前格的终值
increments = []
accumulator = 0
for idx, token in enumerate(bf_program):
    if token == '+':
        accumulator += 1
    elif token == '*':
        increments.append(accumulator)
        accumulator = 0
    # 其它字符 (>, <, #, 等) 不影响计数器

print(f">> BF 格值: {increments}")

# ---------- 第三部分：修正 VM 缺陷 ----------
# VM 原始 # 指令用 int() 截断浮点，应改为 ceil 向上取整
readings = [math.ceil(n * 0.1) for n in increments]
print(f">> 修正读数: {readings}")

# ---------- 第四部分：广义 CRT -----------
def xgcd(a, b):
    """扩展欧几里得算法，返回 (gcd, x, y) 满足 a*x + b*y = gcd"""
    if a == 0:
        return b, 0, 1
    g, x1, y1 = xgcd(b % a, a)
    return g, y1 - (b // a) * x1, x1

def crt_merge(residues, modulus):
    """
    广义中国剩余定理（支持非互素模数）
    输入两组等长数组：residues (余数), modulus (模数)
    返回最小非负整数解
    """
    ans, M = residues[0], modulus[0]
    for i in range(1, len(modulus)):
        ai, ni = residues[i], modulus[i]
        g = math.gcd(M, ni)
        if (ai - ans) % g != 0:
            raise ValueError(f"余数不一致: x ≡ {ans} (mod {M}) 与 x ≡ {ai} (mod {ni})")
        Mi = M // g
        Ni = ni // g
        delta = (ai - ans) // g
        # 求模逆
        _, inv_mi, _ = xgcd(Mi % Ni, Ni)
        t = (delta * inv_mi) % Ni
        ans = ans + M * t
        M = M * Ni
        ans %= M
    return ans

answer = crt_merge(readings, mod_list)
print(f">> CRT 合并结果: {answer}")

# ---------- 第五部分：派生密钥 + XOR 解密 ----------
# 将 CRT 结果与 Base64 线索中提取的标签拼接后哈希
key_material = f"{answer}:SYNC_SENSOR_2025"
stream_key = hashlib.sha256(key_material.encode()).digest()   # 32 字节

ciphertext = base64.b64decode(cipher_b64)
plaintext = bytearray(len(ciphertext))
for j in range(len(ciphertext)):
    plaintext[j] = ciphertext[j] ^ stream_key[j % 32]

flag_str = plaintext.decode()
print(f">> 密钥种子: {key_material}")
print(f">> Flag: {flag_str}")

# ---------- 校验 ----------
if flag_str.startswith("ISCC{") and flag_str.endswith("}"):
    print(">> 校验通过！")
else:
    print(">> 格式异常，请检查中间步骤。")
```

**运行结果：**

```
>> BF 格值: [9766, 13086, 3108, 5927, 8767, 1447, 1156, 5926]
>> 修正读数: [977, 1309, 311, 593, 877, 145, 116, 593]
>> CRT 合并结果: 3367889
>> 密钥种子: 3367889:SYNC_SENSOR_2025
>> Flag: ISCC{OxCjAYY2ub71VeqapwUAJYmzSDWQEMBm}
>> 校验通过！
```

## Distilled Knowledge

| 知识点 | 关键 |
|--------|------|
| PNG隐写 | tEXt块可嵌入自定义key-value数据，IEND后可附加任意内容 |
| BF代码审计 | 海量`+`中定位稀疏的`*#>`输出指令，统计区间获得cell值 |
| VM缺陷 | `int()`截断vs`ceil()`向上取整，浮点×0.1导致有损量化 |
| 广义CRT | 模数不互素时用扩展欧几里得求模逆，两两合并 |
| SHA-256密钥派生 | 拼接CRT结果+标签后哈希，生成循环XOR密钥流 |

---

*Original writeup: `C:\Users\48714\Desktop\wp\final\数字奥秘_WriteUp_final.md`*
