# QuorumGlyph

> **Category**: Reverse | **Difficulty**: Hard | **Competition**: ISCC 2026 决赛
> **Flag**: `ISCC{QRM_FE6V6_3_w1tness_no_single_path!!}`

---

## Attack Chain

```
IDA逆向提取三层校验结构 → 第一层(奇偶见证)确认一致性约束 → 第二层(分组状态变换37轮)逆推主体明文 → 第三层(5字符令牌)用Unicorn模拟暴力搜索 → 三层拼合完整flag
```

## Key Techniques

### 1. 三层校验架构（对应题目提示"One witness lies. Two are incomplete. Three agree."）

| 层 | 处理对象 | 能给出什么 | 缺什么 |
|----|----------|-----------|--------|
| 第一层（奇偶见证） | 主体(中间5位被抹) | 一致性约束，无法反推明文 | 全部明文 |
| 第二层（分组状态变换） | 主体(中间5位被抹) | 主体明文 | 中间5位 |
| 第三层（令牌哈希） | 仅中间5位 | 中间5位 | 主体明文 |

前两层都清零了flag[9:14]，第三层专门处理这5个字符。单独哪层都不够，三层信息互不重叠，拼合才是完整flag。

### 2. Flag骨架提取

格式校验：长度42，前缀`ISCC{QRM_`，下标14为下划线，末尾为`}`。
```
ISCC{QRM_??????????_??????????????????????????????}
           [9:14]    [15:41]
```

### 3. 第二层逆向 — 37轮分组状态变换

- 初始种子`0x51554F52554D474C`（小端读出"LGMRQUQ"，反转="QUORUMGL"彩蛋）
- 37轮迭代：dword与轮常量XOR后ROL → 左向右链式XOR → 右向左链式XOR → 按固定模式swap
- 每步可逆（ROR/逆XOR传播/逆swap），从构造器生成的64字节目标状态反推主体明文
- 结果：`[15:42] = _3_w1tness_no_single_path!!`

### 4. 第三层 — 5字符令牌暴力搜索

- 字母表`ABCDEFGHJKLMNPQRSTUVWXYZ23456789`（32字符，5-bit编码）
- 5字符 x 5-bit = 25-bit编码值，约束`<= 0xFFFFFF`
- 牛顿迭代求模逆元：`a=0xD1E995`，5轮收敛到`0x989B19BD`
- 最终公式：`sub_401CA0(((编码值 - 0xB3EF33) × 0x989B19BD) & 0xFFFFFF) == 0xE04CF5339F776142`
- 用Unicorn加载原始二进制模拟`sub_401CA0`执行，遍历16M候选空间
- 命中令牌`FE6V6`，中间值`0xC0DE5`（"CODE"彩蛋）

## Exploit

```python
#!/usr/bin/env python3
"""
QuorumGlyph token cracker
通过 Unicorn 模拟原二进制中的混合函数进行暴力搜索
Usage: python solve.py [binary_path]
"""

import struct, sys, time

M32 = 0xFFFFFFFF

def newton_reciprocal(raw_val, iters=5):
    """用牛顿迭代求 raw_val 在 mod 2^32 下的乘法逆"""
    x = 1
    for _ in range(iters):
        x = (x * (2 - raw_val * x)) & M32
    return x


def build_emulator(img_data):
    """把 ELF 映射进 Unicorn，返回模拟器实例"""
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_64
    from unicorn.x86_const import UC_X86_REG_RSP, UC_X86_REG_RDI, UC_X86_REG_RAX

    IMG   = 0x400000
    MEMSZ = 0x200000
    STK   = 0x7FFF0000

    uc = Uc(UC_ARCH_X86, UC_MODE_64)
    uc.mem_map(IMG, MEMSZ)
    uc.mem_write(IMG, img_data)
    uc.mem_map(STK, MEMSZ)

    # 先跑一遍构造函数 sub_401080 让 .bss 就位
    trap = 0xDEADCAFE  # 假返回地址，跑到这里就停
    stk_top = STK + MEMSZ - 0x1000
    uc.reg_write(UC_X86_REG_RSP, stk_top)
    uc.mem_write(stk_top, struct.pack('<Q', trap))
    uc.emu_start(0x401080, trap, timeout=5000)

    # 读出构造结果
    cookie  = struct.unpack('<I', uc.mem_read(0x405060, 4))[0]
    expect  = struct.unpack('<Q', uc.mem_read(0x405080, 8))[0]
    charset = bytes(uc.mem_read(0x4050A0, 32)).decode()

    assert cookie == 0x51574D21, ".bss 未正确初始化"

    return uc, stk_top, trap, expect, charset, IMG, STK


def invoke_mixer(uc, arg, stk_top, trap):
    """模拟执行 sub_401CA0(edi=arg)，返回 rax"""
    from unicorn.x86_const import UC_X86_REG_RSP, UC_X86_REG_RDI, UC_X86_REG_RAX

    uc.reg_write(UC_X86_REG_RSP, stk_top)
    uc.mem_write(stk_top, struct.pack('<Q', trap))
    uc.reg_write(UC_X86_REG_RDI, arg & M32)
    try:
        uc.emu_start(0x401CA0, trap, timeout=10000)
    except Exception:
        return None
    return uc.reg_read(UC_X86_REG_RAX)


def decode_token(code, charset):
    """把 25-bit 编码值还原成 5 个字符"""
    buf = []
    rem = code
    for _ in range(5):
        buf.append(charset[rem & 0x1F])
        rem >>= 5
    buf.reverse()
    return ''.join(buf)


def crack(img_path):
    with open(img_path, 'rb') as f:
        raw = f.read()

    uc, stk_top, trap, expect, charset, *_ = build_emulator(raw)

    print("[*] charset = %s" % charset)
    print("[*] expect  = %#x" % expect)

    # 计算乘法逆（对应伪代码里 v124 的 Newton 5 轮迭代）
    COEFF = 13756821          # 0xD1E995
    BASE_OFFSET = 0xB3EF33    # 11792179
    MASK24 = 0xFFFFFF

    recip = newton_reciprocal(COEFF)
    assert (COEFF * recip) & M32 == 1
    print("[*] recip(0x%x) = %#x" % (COEFF, recip))

    print("[*] scanning code space [0, 0x%x] ..." % MASK24)
    t0 = time.time()

    for code in range(MASK24 + 1):
        mid = ((code - BASE_OFFSET) * recip) & MASK24
        out = invoke_mixer(uc, mid, stk_top, trap)

        if out == expect:
            tok = decode_token(code, charset)
            sec = time.time() - t0
            print("\n[+] hit!")
            print("    token = %s" % tok)
            print("    code  = %#x" % code)
            print("    mid   = %#x" % mid)
            print("    out   = %#x" % out)
            print("    time  = %.1fs" % sec)
            return tok

        if code and code % 500000 == 0:
            el = time.time() - t0
            spd = code / el if el else 1
            rem = (MASK24 - code) / spd if spd else 0
            print("    %d/%d  %.1f%%  %.0f/s  ETA~%.0fs" % (
                code, MASK24, 100.0*code/MASK24, spd, rem))

    print("[-] no match")
    return None


if __name__ == "__main__":
    path = sys.argv[1] if len(sys.argv) > 1 else "QuorumGlyph"
    tok = crack(path)
    if tok:
        print("\n[*] FLAG: ISCC{QRM_%s_3_w1tness_no_single_path!!}" % tok)
```

**运行结果：**

```
[*] charset = ABCDEFGHJKLMNPQRSTUVWXYZ23456789
[*] expect  = 0xe04cf5339f776142
[*] recip(0xd1e995) = 0x989b19bd
[*] scanning code space [0, 0xffffff] ...

[+] hit!
    token = FE6V6
    code  = 0x52727c
    mid   = 0xc0de5
    out   = 0xe04cf5339f776142
    time  = xxxs

[*] FLAG: ISCC{QRM_FE6V6_3_w1tness_no_single_path!!}
```

## Distilled Knowledge

| 知识点 | 关键 |
|--------|------|
| 三层校验架构 | 每层只验证部分信息，需要综合三层拼合flag |
| 分组状态变换 | 37轮XOR+ROL+链式XOR+swap，每步可逆 |
| 5-bit字符编码 | 32字符字母表(去掉I/O/0/1)，5字符=25-bit |
| 牛顿迭代求模逆 | `x_{n+1}=x*(2-a*x)` mod 2^32，5轮收敛 |
| Unicorn模拟执行 | 加载ELF模拟自定义混合函数，避免手工逆向1024轮 |
| splitmix参数 | `0xBF58476D1CE4E5B9`/`0x94D049BB133111EB`为标志性常数 |

---

*Original writeup: `C:\Users\48714\Desktop\wp\final\QuorumGlyph_WriteUp.md`*
