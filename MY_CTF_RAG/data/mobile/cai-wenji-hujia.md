# 蔡文姬·胡笳

> **Category**: Mobile | **Difficulty**: Hard | **Competition**: ISCC 2026 练习/附加题
> **Flag**: `ISCC{GuiHanQu8s9Dp68*G-ZN_#9Z3zLIbet6}`

---

## Attack Chain

```
APK解包 → 种子(包名hash^MP4大小^MAGIC) → TEA密钥 → 旋律bit-swap→FNV-1a-64→TEA→CW令牌 → CRC32流密码解密SQLite → RC6动态密钥解密 → Base58解码 → Flag
```

## Key Techniques

### 1. 种子计算

```python
seed = hashCode("com.example.mobile04") ^ 77055908 ^ 0x5B7BFF1C
# = 0x63597A4F
```

### 2. 四层加密链

| 层 | 算法 | 输入→输出 |
|---|------|----------|
| 1 | TEA-32 | 种子→CW令牌 |
| 2 | CRC32流密码 | CW令牌→解密SQLite DB |
| 3 | DB参数 | val_a, val_b → RC6 P/Q |
| 4 | RC6-32/20/4 | Base58密文→Flag |

### 3. 旋律序列

宫商角徵羽弹奏序列 `CWJXMASFLOWSTARROSEGOALDANCEMAGI`（32字符）。

### 4. 反调试保护

- `Debug.waitingForDebugger()` → 杀进程
- 后台线程设 `C1.T = 0xDEAD` → TEA偏移错误
- 必须走纯静态分析路线

## Distilled Knowledge

| 知识点 | 关键 |
|--------|------|
| 资源文件参与密钥 | MP4文件大小作为种子输入 |
| Java hashCode | 31倍累加哈希 |
| TEA | delta=0x9E3779B9, 32轮 |
| SQLCipher | 加密SQLite, 需密钥打开 |
| RC6 | 非标准P/Q, 动态密钥调度 |
| Base58 | 比特币地址编码 |

---

*Original writeup: `C:\Users\48714\Desktop\wp\其他\Mobile+蔡文姬·胡笳_Writeup.md`*
