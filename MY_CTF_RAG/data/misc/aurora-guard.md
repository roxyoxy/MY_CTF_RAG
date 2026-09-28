# 长虹守卫

> **Category**: MISC | **Difficulty**: Hard | **Competition**: ISCC 2026 区域赛(第二场)
> **Flag**: `ISCC{1eebca67baab5c99623c5dba0251dfbe}`

---

## Attack Chain

```
飞行日志Base64隐写指令 → 跨IDL时间修正 → 三个事件UTC时间戳 → SHA256-CTR解密
```

## Key Techniques

### 1. 飞行日志隐写指令

日志CSV末尾嵌入Base64编码的解密规则，解码得完整KEYMAT格式。

### 2. IDL时间修正

```
idx 26: 经度+180→-180, 向东跨越IDL
FDR BUG: date_corr=-1day 计算了但未写入
修正: UTC = local + date_corr - timezone
```

| 事件 | 时区 | date_corr |
|------|------|-----------|
| EVT_A | UTC-11 | -1day |
| EVT_B | UTC+14 | 不修正 |
| EVT_C | UTC-11 | -1day |

### 3. 陷阱："于无声际听惊雷"

三个EVT_C标记(X9/noise, A2, Z1/decoy)，A2看似正确但实际是诱饵，真正的答案是 X9(noise)。

### 4. SHA256-CTR 解密

```python
stream = SHA256(seed + struct.pack(">I", counter))
flag = cipher XOR stream
```

## Distilled Knowledge

| 知识点 | 关键 |
|--------|------|
| Base64隐写 | 非CSV内容嵌入数据末尾 |
| IDL时区 | 跨越国际日期变更线±1天 |
| FDR BUG | 时间修正未生效，需手动应用 |
| SHA256-CTR | SHA256做密钥流生成器 |
| 陷阱识别 | "noise"才是答案，非"A2" |

---

*Original writeup: `C:\Users\48714\Desktop\wp\区域赛-2\长虹守卫_WriteUp_final.md`*
