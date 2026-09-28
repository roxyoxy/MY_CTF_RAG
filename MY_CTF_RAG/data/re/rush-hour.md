# 手忙脚乱

> **Category**: Reverse | **Difficulty**: Hard | **Competition**: ISCC 2026 决赛
> **Flag**: `ISCC{R_0ElnXnl5d(u9q@(48MN`d`}`

---

## Attack Chain

```
Unicorn模拟器解密6组内置常量 → 还原加密流水线(列转置+替换表+比特流驱动) → 第一轮XOR+memcmp逆向求解 → 第二轮HDB3编码约束分析(全零等价) → 两轮输入送exe验证
```

## Key Techniques

### 1. Unicorn模拟器解密内置常量

程序内嵌6组加密数据(K6、K3、MASK1、TARGET、MASK2、TARGET_T)，使用TEA风格自定义流密码函数`decrypt_blob`解密。直接Python重现会遭遇32位精度问题，因此从二进制中提取`decrypt_blob`原始机器码，在Unicorn CPU模拟器中直接执行。

### 2. 加密流水线还原

```
key_derived = columnarTransposeEncrypt(K6, K3)
加密结果 = encryptPart2(columnarTransposeEncrypt(用户输入, key_derived), K6)
```

三个核心算法：
- **列转置密码**：将明文按密钥长度分列，按密钥字符排序后逐列读出
- **替换表生成(makeTable)**：Fisher-Yates洗牌生成printable ASCII排列表，LCG伪随机驱动
- **复合加密(encryptPart2)**：密钥展开为比特流，逐位驱动替换（根据bit选择两个不同替换表），每轮结束执行列转置

### 3. 第一轮求解 — XOR+memcmp逆向

验证逻辑：`encryptPart2_output XOR MASK1 == TARGET`
- 逆推：`TARGET XOR MASK1` → 逆encryptPart2（逆替换+逆转置）→ 逆列转置 → 明文
- 关键：逆encryptPart2时，每个以转置结尾的段都需要先逆转置再逆替换

### 4. 第二轮求解 — HDB3编码约束

第二轮对加密结果做HDB3编码，要求B/V统计与24个空字节的HDB3编码一致。
- 192个零比特：每连续4个零产生一组B00V替换，共48组 → B=48, V=48
- 任何1-bit都会打断零的连续性导致替换数减少
- 因此约束等价于：**比特序列必须全零**，即`output = MASK2 XOR TARGET_T`

## Exploit

```python
import sys, struct, subprocess
sys.stdout.reconfigure(encoding='utf-8')
from unicorn import Uc, UC_ARCH_X86, UC_MODE_64, UC_HOOK_BLOCK
from unicorn.x86_const import *

# ============================================================
#  Part 1: 用 Unicorn 模拟器执行二进制中的 decrypt_blob 函数
# ============================================================
EXE_PATH = "attachment-62.exe"

with open(EXE_PATH, "rb") as fh:
    raw = fh.read()

BASE_ADDR = 0x140000000
emu = Uc(UC_ARCH_X86, UC_MODE_64)

# 映射代码段和数据段
emu.mem_map(BASE_ADDR + 0x1000, 0x6000)
emu.mem_write(BASE_ADDR + 0x1000, raw[0x600 : 0x600 + 0x5800])
emu.mem_map(BASE_ADDR + 0x8000, 0x2000)
emu.mem_write(BASE_ADDR + 0x8000, raw[0x6000 : 0x6000 + 0x1000])

# 映射栈和工作缓冲区
STACK_TOP = 0x7FFE0000
emu.mem_map(STACK_TOP - 0x100000, 0x100000)
emu.reg_write(UC_X86_REG_RSP, STACK_TOP - 0x800)
WORK_AREA = 0x200000000
emu.mem_map(WORK_AREA, 0x10000)

LOOP_ENTRY = BASE_ADDR + 0x1510   # decrypt_blob 循环入口
LOOP_EXIT  = BASE_ADDR + 0x1560

def call_decrypt_blob(cipher, seed_val):
    """通过 Unicorn 模拟执行 decrypt_blob，返回解密后的明文"""
    sz = len(cipher)
    src_buf  = WORK_AREA + 0x1000
    dst_buf  = WORK_AREA + 0x3000
    str_ptr  = WORK_AREA + 0x5000

    emu.mem_write(src_buf, cipher)
    emu.mem_write(str_ptr, struct.pack("<Q", dst_buf))
    emu.reg_write(UC_X86_REG_RDI, src_buf)
    emu.reg_write(UC_X86_REG_RSI, str_ptr)
    emu.reg_write(UC_X86_REG_R11, dst_buf)
    emu.reg_write(UC_X86_REG_RBP, sz)
    emu.reg_write(UC_X86_REG_R10, 0)
    emu.reg_write(UC_X86_REG_R8,  0)
    emu.reg_write(UC_X86_REG_R9,  (seed_val ^ 0xA3B1C2D3) & 0xFFFFFFFF)
    emu.reg_write(UC_X86_REG_RDX, 0)

    out = [None]
    def on_block(uc, addr, _, __):
        if addr == LOOP_ENTRY and uc.reg_read(UC_X86_REG_RDX) >= sz:
            out[0] = bytes(uc.mem_read(dst_buf, sz))
            uc.emu_stop()

    hook = emu.hook_add(UC_HOOK_BLOCK, on_block)
    emu.emu_start(LOOP_ENTRY, LOOP_EXIT, count=sz * 200)
    emu.hook_del(hook)
    return out[0] if out[0] else bytes(emu.mem_read(dst_buf, sz))


# ============================================================
#  Part 2: 解密全部 6 组内置常量
# ============================================================
BLOBS = {
    "k6":       ("1855a378c86c",                                                           0x5555),
    "k3":       ("67cedd",                                                                  0x6666),
    "mask_a":   ("4b5d1c5e410f506bed0c079a18b04c7c09c8d74957090771",                        0x3333),
    "target_a": ("8cad03aa6a32b926158f7c95eacadd159400412da55643ac",                        0x1111),
    "mask_b":   ("cce7e28169d8ffcd64d27a929fbdb6105cb0137a67e78359",                        0x4444),
    "target_b": ("cce1a2150c88288ca52d9302724bd6b687512c9b7c4f4faa",                        0x2222),
}

CONST = {}
for tag, (hexdata, sd) in BLOBS.items():
    CONST[tag] = call_decrypt_blob(bytes.fromhex(hexdata), sd)

k6       = CONST["k6"]
k3       = CONST["k3"]
mask_a   = CONST["mask_a"]
target_a = CONST["target_a"]
mask_b   = CONST["mask_b"]
target_b = CONST["target_b"]


# ============================================================
#  Part 3: 密码学原语实现
# ============================================================
def _wrap32(v):
    return v & 0xFFFFFFFF

def build_permutation_table(key_bytes, init_seed):
    """生成 printable ASCII 的排列替换表"""
    charset = list(range(32, 127))
    state = init_seed
    for ch in key_bytes:
        state = _wrap32(ch + 131 * state)
    # Fisher-Yates 洗牌 (LCG 伪随机)
    for idx in range(94, 0, -1):
        state = _wrap32(1103515245 * state + 12345)
        pick = state % (idx + 1)
        charset[idx], charset[pick] = charset[pick], charset[idx]
    return charset


def columnar_encrypt(pt, col_key):
    """列转置加密"""
    width = len(col_key)
    height = len(pt) // width
    matrix = [list(col_key)]
    off = 0
    for _ in range(height):
        matrix.append(list(pt[off : off + width]))
        off += width
    col_order = sorted(range(width), key=lambda c: matrix[0][c])
    out = []
    for c in col_order:
        for r in range(1, height + 1):
            out.append(matrix[r][c])
    return bytes(out)


def columnar_decrypt(ct, col_key):
    """列转置解密"""
    width = len(col_key)
    height = len(ct) // width
    col_order = sorted(range(width), key=lambda c: col_key[c])
    grid = [bytearray(width) for _ in range(height + 1)]
    grid[0] = list(col_key)
    off = 0
    for c in col_order:
        for r in range(1, height + 1):
            grid[r][c] = ct[off]
            off += 1
    out = []
    for r in range(1, height + 1):
        for c in range(width):
            out.append(grid[r][c])
    return bytes(out)


def expand_key_to_bits(k):
    """将密钥字节展开为比特流"""
    stream = []
    for byte in k:
        for shift in range(7, -1, -1):
            stream.append((byte >> shift) & 1)
    return stream


def encrypt_stage(data, enc_key):
    """正向加密：替换 + 转置复合"""
    tbl_a = build_permutation_table(enc_key, 0x1234)
    tbl_b = build_permutation_table(enc_key, 0x8888)
    kbits = expand_key_to_bits(enc_key)
    buf = bytearray(data)
    cursor = 0
    for bi in range(len(kbits)):
        idx_in_charset = buf[cursor] - 32
        buf[cursor] = tbl_b[idx_in_charset] if kbits[bi] else tbl_a[idx_in_charset]
        cursor += 1
        if cursor == len(buf):
            buf = bytearray(columnar_encrypt(bytes(buf), enc_key))
            cursor = 0
    return bytes(buf)


def decrypt_stage(data, enc_key):
    """逆向加密：逆替换 + 逆转置"""
    tbl_a = build_permutation_table(enc_key, 0x1234)
    tbl_b = build_permutation_table(enc_key, 0x8888)
    # 构建逆替换表
    inv_a = [0] * 95
    inv_b = [0] * 95
    for i in range(95):
        inv_a[tbl_a[i] - 32] = i + 32
        inv_b[tbl_b[i] - 32] = i + 32

    kbits = expand_key_to_bits(enc_key)
    text_len = len(data)
    buf = bytearray(data)

    # 定位每个转置点
    trans_points = []
    p = 0
    for bi in range(len(kbits)):
        p += 1
        if p == text_len:
            trans_points.append(bi)
            p = 0

    # 将比特流按转置点分段
    segs = []
    head = 0
    for tp in trans_points:
        segs.append((head, tp))
        head = tp + 1
    if head <= len(kbits) - 1:
        segs.append((head, len(kbits) - 1))

    # 从后向前逆向处理每一段
    for si in range(len(segs) - 1, -1, -1):
        s_lo, s_hi = segs[si]
        # 段尾是转置点 → 先逆转置
        if s_hi in trans_points:
            buf = bytearray(columnar_decrypt(bytes(buf), enc_key))
        # 逆替换
        for bi in range(s_hi, s_lo - 1, -1):
            local_off = bi - s_lo
            lookup = inv_b if kbits[bi] else inv_a
            buf[local_off] = lookup[buf[local_off] - 32]
    return bytes(buf)


def xor_bytes(*arrays):
    """对多组 bytes 做 XOR"""
    result = bytearray(arrays[0])
    for arr in arrays[1:]:
        for i in range(len(result)):
            result[i] ^= arr[i]
    return bytes(result)


def to_bitstream(raw):
    """字节序列转比特流"""
    return [((b >> s) & 1) for b in raw for s in range(7, -1, -1)]


def hdb3_encode(bits):
    """HDB3 编码，返回符号序列"""
    symbols = []
    polarity = -1
    n_pulses = 0
    n_zeros  = 0
    for b in bits:
        if b == 1:
            symbols.append("+" if polarity == -1 else "-")
            polarity = -polarity
            n_pulses += 1
            n_zeros = 0
        else:
            symbols.append("0")
            n_zeros += 1
            if n_zeros == 4:
                if n_pulses % 2 == 0:
                    symbols[-4] = "B"
                symbols[-1] = "V"
                n_zeros = 0
                n_pulses = 0
    return symbols


def count_bv(seq):
    """统计 HDB3 序列的 B/V 数量和奇偶校验"""
    nb = nv = pv = 0
    for i, ch in enumerate(seq):
        if ch == "B":
            nb += 1
        elif ch == "V":
            nv += 1
            pv ^= (i & 1)
    return nb, nv, pv


# ============================================================
#  Part 4: 第一轮求解
# ============================================================
print("=" * 50)
print("  第一轮求解")
print("=" * 50)

derived_key = columnar_encrypt(k6, k3)
goal_r1 = xor_bytes(target_a, mask_a)   # 目标 encrypt_stage 输出

mid_r1 = decrypt_stage(goal_r1, k6)
answer_r1 = columnar_decrypt(mid_r1, derived_key)
print(f"[Round 1] 明文: {answer_r1}")

# 前向验证
verify_ct = columnar_encrypt(answer_r1, derived_key)
verify_enc = encrypt_stage(verify_ct, k6)
assert xor_bytes(verify_enc, mask_a) == target_a, "Round 1 验证失败"
print("[Round 1] 前向验证通过")


# ============================================================
#  Part 5: 第二轮求解
# ============================================================
print()
print("=" * 50)
print("  第二轮求解")
print("=" * 50)

# HDB3 约束分析：24 个空字节 → 192 零比特 → 48 组 B00V → (B=48, V=48, pv=0)
# 要达到 V=48，输入比特必须全零 → output XOR mask_b XOR target_b = 0
ref_bits = to_bitstream(b"\x00" * 24)
ref_b, ref_v, ref_p = count_bv(hdb3_encode(ref_bits))
print(f"[Round 2] 参考 HDB3 统计: B={ref_b}, V={ref_v}, parity={ref_p}")

goal_r2 = xor_bytes(mask_b, target_b)   # 目标 encrypt_stage 输出
mid_r2 = decrypt_stage(goal_r2, k6)
answer_r2 = columnar_decrypt(mid_r2, derived_key)
print(f"[Round 2] 明文: {answer_r2}")

# 前向验证
verify_ct2 = columnar_encrypt(answer_r2, derived_key)
verify_enc2 = encrypt_stage(verify_ct2, k6)
test_data = xor_bytes(verify_enc2, mask_b, target_b)
got_b, got_v, got_p = count_bv(hdb3_encode(to_bitstream(test_data)))
assert (got_b, got_v, got_p) == (ref_b, ref_v, ref_p), "Round 2 验证失败"
print("[Round 2] 前向验证通过")


# ============================================================
#  Part 6: 提交 exe 验证
# ============================================================
print()
print("=" * 50)
print("  EXE 验证")
print("=" * 50)

payload = answer_r1 + b"\n" + answer_r2 + b"\n"
proc = subprocess.run(
    ["./attachment-62.exe"],
    input=payload, capture_output=True, timeout=5
)
print(f"stdout: {proc.stdout.decode('utf-8', errors='replace')}")
if b"PASS" in proc.stdout:
    print("\n>>> 两轮验证均通过 <<<")
    print(f"\nFlag: ISCC{{{answer_r2.decode('ascii')}}}")
else:
    print("验证未通过，请检查")
```

**运行结果：**

```
==================================================
  第一轮求解
==================================================
[Round 1] 明文: b'@"9w3Cv).e,\'7>khG-[Vg-0L'
[Round 1] 前向验证通过

==================================================
  第二轮求解
==================================================
[Round 2] 参考 HDB3 统计: B=48, V=48, parity=0
[Round 2] 明文: b'R_0ElnXnl5d(u9q@(48MN`d`'
[Round 2] 前向验证通过

==================================================
  EXE 验证
==================================================
stdout: input 24 char plaintext:
input 24 char plaintext:
PASS

>>> 两轮验证均通过 <<<

Flag: ISCC{R_0ElnXnl5d(u9q@(48MN`d`}
```

## Distilled Knowledge

| 知识点 | 关键 |
|--------|------|
| Unicorn模拟解密 | 提取原始机器码模拟执行，避免32位精度问题 |
| 列转置密码 | 按密钥排序分列读出，密钥字符顺序决定列顺序 |
| 比特流驱动替换 | 密钥展开为比特流，每位选择不同替换表 |
| 逆向分段处理 | 从后向前，每段先逆转置再逆替换，不能遗漏 |
| HDB3编码约束 | 全零比特=B00V x48组，任何1-bit都会打断替换 |
| 两轮校验结构 | 同一加密流水线，不同密钥常量，第二轮输入为flag |

---

*Original writeup: `C:\Users\48714\Desktop\wp\final\手忙脚乱_WriteUp.md`*
