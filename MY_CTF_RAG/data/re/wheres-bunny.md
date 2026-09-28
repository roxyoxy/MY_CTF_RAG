# Where's bunny

> **Category**: Reverse | **Difficulty**: Medium | **Competition**: ISCC 2026 School
> **Flag**: `ISCC{iehucaaoIylrtleyolwudIs}`

---

## Attack Chain

```
洞穴模拟(约瑟夫环) → 选取4个密钥 → 四层加密链识别 → 逐层逆推解密
```

## Key Techniques

### 1. 约瑟夫环变体 — 洞穴模拟

10个洞穴，淘汰算法 `x_i = (x_{i-1} + i + 1) mod 10`，周期20。
存活洞穴: `{1, 3, 6, 8}` → 值 `{344, 21, 89, 233}`

### 2. 四层加密链

| Counter | 洞穴 | 值 | 加密 | 密钥 |
|---------|------|-----|------|------|
| 1 | 1 | 344 | RC4 | `"344"` |
| 2 | 3 | 21 | XOR | `"21"` |
| 3 | 6 | 89 | ADD | `"89"` |
| 4 | 8 | 233 | TEA | SHA-256(`"233"`)前16字节 |

完整链: `明文 → RC4 → XOR → ADD → PKCS7填充 → TEA → Hex编码 → 密文`

### 3. 算法识别特征

- **SHA-256**: 初始值 `0x6a09e667, 0xbb67ae85...`
- **TEA**: delta `0x9E3779B9`
- **RC4**: KSA + PRGA 标准结构

## Distilled Knowledge

| 知识点 | 关键 |
|--------|------|
| 算法常量识别 | SHA-256初始值, TEA delta, RC4 KSA |
| 约瑟夫环 | 淘汰选元素, 可能有周期性 |
| MSVC std::string | SSO 16字节 + length + capacity |
| __usercall | 寄存器传参, 需结合汇编分析 |

---

*Original writeup: `C:\Users\48714\Desktop\wp\校赛\Where's_bunny_wp.md`*
