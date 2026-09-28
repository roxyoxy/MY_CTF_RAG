# 迷雾验证

> **Category**: Mobile | **Difficulty**: Medium | **Competition**: ISCC 2026 School
> **Flag**: `ISCC{A9f#QxT7vL2@pR4!}`

---

## Attack Chain

```
jadx分析Java层 → 识别干扰项 → 定位native verify → 分析SO库 → 三段分别逆推
```

## Key Techniques

### 1. Java层干扰项识别

- `FlagFormatChecker.Check()` → 纯计算, 结果未使用
- `FlagCore.HeavyCheck()` → 空循环, 返回值被忽略
- **真实验证路径**: `dispatchCheck()` → `LocalExecutor.verify()` → **native**

### 2. SO库密钥还原

| 密钥 | 还原方法 |
|------|---------|
| RC4: `"mysecretkey"` | 反转字符串碎片: `"yek"+"terc"+"esym"` → 反转 |
| XOR: `"xor-seed-42"` | 偏移字符串: `"yps."+"tffe"+".53"` → 每字节-1 |
| B64: `"b64-key-123"` | Java层AES-CBC解密 `assets/bin.data` |
| AES key: `"1234567890abcdef"` | 双重XOR抵消 (`^1 ^1 = 不变`) |

### 3. 自定义Base64

标准表按 `sum(key_chars) % 64 = 5` 左旋5位, 用 `str.maketrans` 映射回标准表。

### 4. 三段解密

| 段 | 长度 | 加密 | 密文 |
|----|------|------|------|
| Part1 | 5字节 | 自定义Base64 | `VYqrN6J` |
| Part2 | 6字节 | RC4 → hex | `92874fce8c7b` |
| Part3 | 5字节 | XOR → hex | `381f201952` |

## Distilled Knowledge

| 知识点 | 关键 |
|--------|------|
| APK逆向流程 | jadx找native → IDA分析so |
| 干扰项识别 | 空循环/未使用返回值=干扰 |
| 密钥混淆 | 反转拼接, 字节偏移, 双重XOR |
| 自定义Base64 | 标准表旋转N位, maketrans映射 |

---

*Original writeup: `C:\Users\48714\Desktop\wp\校赛\迷雾验证_writeup.md`*
