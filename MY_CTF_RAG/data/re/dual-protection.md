# Dual Protection

> **Category**: Reverse | **Difficulty**: Medium | **Competition**: ISCC 2026 School
> **Flag**: `ISCC{9mvKaM9ClapU!_VT1XYN>(eut_J7QU}`

---

## Attack Chain

```
PE32分析 → 三层字节变换识别 → LCG加密验证函数破解 → 反调试绕过 → 逆推解密
```

## Key Techniques

### 1. 三层字节变换

| 函数 | 正向变换 | 逆向 |
|------|---------|------|
| sub_401000 | `XOR 0x55 → ROL(byte,2) → +i` | `-i → ROR(byte,2) → XOR 0x55` |
| sub_401050 | `XOR key[i%8] → +0x7F` | `-0x7F → XOR key[i%8]` |
| sub_4010D0 | `XOR (i+32)` | `XOR (i+32)` (自逆) |

密钥表: `0x12 0x34 0x56 0x78 0x90 0xAB 0xCD 0xEF`

### 2. LCG自解密验证函数

280字节加密机器码，运行时用LCG解密后作为函数执行：

```python
seed = 0xDEADBEEF  # 正常; 0xBADF00D (调试时)
state = seed
for i in range(280):
    state = (state * 0x19660D + 0x3C6EF35F) & 0xFFFFFFFF
    random_byte = (state >> 24) & 0xFF
    decrypted[i] = encrypted[i] ^ random_byte
```

### 3. 反调试双重检测

- `CheckRemoteDebuggerPresent` 改变LCG种子
- 解密后代码内嵌 `PEB.BeingDebugged` (FS:[30h]+2) 检查

## Distilled Knowledge

| 知识点 | 模式 |
|--------|------|
| LCG自解密代码 | VirtualAlloc + XOR循环 + 函数指针调用 |
| XOR自逆 + ROL/ROR | XOR逆操作=自身, ROL逆=ROR |
| PE反调试 | PEB.BeingDebugged, CheckRemoteDebuggerPresent |
| 加密链逆推 | 从最后一步往前逆推, 注意运算顺序 |

## Tools

- IDA Pro 32-bit (反编译)
- Python (解密脚本)

---

*Original writeup: `C:\Users\48714\Desktop\wp\校赛\Dual_Protection_wp.md`*
