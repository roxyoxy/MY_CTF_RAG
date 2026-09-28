# 双校区来信

> **Category**: MISC | **Difficulty**: Easy | **Competition**: ISCC 2026 School
> **Flag**: `ISCC{wE3rT5yU7iO9pL0kJ2hG4fD6sA8qQ}`

---

## Attack Chain

```
音频频谱图 → 获取拼音hdbxqsdx → JPEG尾部提取RAR → 密码解压 → 按顺序拼接
```

## Key Techniques

### 1. 频谱图分析

用Audacity打开 `campus_broadcast.wav`，查看频谱图（STFT），可见文字：`hdbxqsdx`
= "厚德博学求是笃行" 拼音首字母（上海师范大学校训）

### 2. JPEG附加RAR

```
JPEG结束标记 FFD9 之后附加 1349 字节
→ "TheFollowingIsARarFile" + RAR5压缩包
```

提取: 找到 `FFD9` → 之后找 `Rar!` → 提取RAR

### 3. 解压与拼接

RAR密码: `hdbxqsdx`（频谱图获得）
解压得到8个文件（对应8个拼音字），按音频顺序拼接得到flag。

## Distilled Knowledge

| 知识点 | 关键 |
|--------|------|
| 频谱图隐写 | Audacity查看WAV频谱, 可见文字 |
| JPEG附加数据 | `FFD9`后数据被图像查看器忽略 |
| RAR隐写 | 嵌入图片末尾, 需手动提取 |
| 多媒体联合 | 音频给顺序, 图片藏碎片 |

---

*Original writeup: `C:\Users\48714\Desktop\wp\校赛\双校区来信_wp.md`*
