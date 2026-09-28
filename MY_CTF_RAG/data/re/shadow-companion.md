# 如影随形

> **Category**: Reverse | **Difficulty**: Medium | **Competition**: ISCC 2026 区域赛(第二场)
> **Flag**: `ISCC{?oj}tV]G1Ve/@El$}`

---

## Attack Chain

```
PE64逆向 → 提取4x4矩阵变换+替换表+Vigenere密钥 → 16次调用逆序反转
```

## Key Techniques

### 1. 双层加密：Vigenere + 替换密码

每次调用 `u8afi3(a1, row, col, a4, a5)`：
1. **Vigenere流密码**: 对4x4矩阵16字节做滚动密钥加密
2. **替换密码**: 对 `a1[row][col]` 做 `a5` 次迭代替换

### 2. 16次调度序列

由矩阵M中元素在M_target中的位置决定，共16次调用，参数不同。

### 3. 逆序反转

```python
for i in range(15, -1, -1):
    matrix[idx] = subst_reverse(matrix[idx], a5)
    matrix = vigenere_reverse(matrix, a4)
```

## Distilled Knowledge

| 知识点 | 关键 |
|--------|------|
| Vigenere密码 | 滚动密钥, 密文自身作为下一轮密钥 |
| 替换密码 | 95字节可打印ASCII空间内查表 |
| 逆序反转 | 先替换后Vigenere → 逆序先逆Vigenere再逆替换 |
| 干扰项 | "iscc_{this_is_not_flag}" 是假flag |

---

*Original writeup: `C:\Users\48714\Desktop\wp\区域赛-2\如影随形_WriteUp_final.md`*
