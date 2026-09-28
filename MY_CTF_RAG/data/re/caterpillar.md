# 毛毛虫的逆袭

> **Category**: Reverse | **Difficulty**: Easy | **Competition**: ISCC 2026 区域赛
> **Flag**: `ISCC{)S%kk=qb3jd$#(Hp&#1f}`

---

## Attack Chain

```
UPX脱壳 → IDA逆向 → 识别LCG循环移位加密 → 逆运算解密
```

## Key Techniques

### 1. 文件识别与 UPX 脱壳

`file` 命令确认为 64 位 Windows 控制台程序，`strings` 发现 `UPX0`、`UPX1` 特征字符串。

```bash
upx -d caterpillar46.exe -o caterpillar46_unpacked.exe
# 111KB → 266KB
```

### 2. IDA 反编译分析加密逻辑

脱壳后反编译 main 函数，发现程序根据 `difftime` 选择两条路径：

- 时间差 <= 5 秒 → 走**路径2（LCG加密验证）** — 正常输入走这条
- 时间差 > 5 秒 → 走路径1（Base42编码验证）

路径2核心加密逻辑：

```c
seed = 46;  // 初始种子 = 0x2E
for (i = 0; i < 20; i++) {
    seed = 1103515245 * seed + 12345;  // 经典LCG
    offset = (seed >> 24) % 95;        // 高8位模95
    encrypted[i] = ((input[i] - 32 + offset) % 95) + 32;  // 可打印ASCII循环移位
}
if (memcmp(encrypted, "<b$HpR4)b\"f'b-k>)~%y", 20) == 0)
    success!
```

IDA 中大段位运算（1491936009 相关）本质是编译器对 `% 95` 的优化。

### 3. 逆向解密

加密公式：`encrypted[i] = ((input[i] - 32 + offset[i]) % 95) + 32`

逆运算：`input[i] = ((encrypted[i] - 32 - offset[i]) % 95 + 95) % 95 + 32`

## Exploit

```python
# LCG 参数: a=1103515245, c=12345, m=2^32, seed=46
seed = 0x2E
cipher = '<b$HpR4)b"f' + chr(0x27) + 'b-k>)~%y'

flag = ''
for i, c in enumerate(cipher):
    seed = (1103515245 * seed + 12345) & 0xFFFFFFFF
    offset = ((seed >> 24) & 0xFF) % 95
    dec = ((ord(c) - 32 - offset) % 95 + 95) % 95 + 32
    flag += chr(dec)
    print(f"[{i:2d}] seed=0x{seed:08X}  offset={offset:2d}  "
          f"cipher='{c}'  plain='{chr(dec)}'")

print(f"\nFlag: ISCC{{{flag}}}")
```

**运行结果：**

```
[ 0] seed=0xD1A247CF  offset=19  cipher='<'  plain=')'
[ 1] seed=0xCD13D55C  offset=15  cipher='b'  plain='S'
[ 2] seed=0xBD9C1065  offset=94  cipher='$'  plain='%'
[ 3] seed=0x9B8FF13A  offset=60  cipher='H'  plain='k'
...
[19] seed=0xD1671CEA  offset=19  cipher='y'  plain='f'

Flag: ISCC{)S%kk=qb3jd$#(Hp&#1f}
验证: True
```

## Distilled Knowledge

| 知识点 | 关键 |
|--------|------|
| UPX脱壳 | `upx -d` 标准脱壳 |
| LCG识别 | a=1103515245, c=12345 经典参数 |
| 循环移位加密 | 可打印ASCII空间(32~126, 95字符)内移位 |
| 编译器优化 | `% 95` 被优化为位运算，需识别 |

---

*Original writeup: `C:\Users\48714\Desktop\wp\区域赛\毛毛虫的逆袭_WriteUp_final.md`*
