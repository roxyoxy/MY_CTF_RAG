# 盲相阵列

> **Category**: MISC | **Difficulty**: Hard | **Competition**: ISCC 2026 区域赛
> **Flag**: `ISCC{R7!q_Z@4m^T9?p$V%k&2*n~L#x}`

---

## Attack Chain

```
I/Q采样 → 数字下变频 → BPSK解调 → 去PN扰码 → 逆Column-DMA交织 → Hamming(7,4)纠错 → CRC32校验 → 提取flag
```

## Key Techniques

### 1. 题目信息分析

附件包含 `rx_note.txt` 和 `array_iq.csv`（25670行I/Q复数采样）。接收链路：

```
发送端: 原始数据 → Hamming(7,4)编码 → 16路Column-DMA交织 → PN加扰 → BPSK调制 → 上变频
接收端: I/Q采样 → 下变频 → 符号判决 → 去PN → 逆DMA → Hamming纠错 → 取正文 → CRC校验
```

### 2. 符号恢复 — 从采样到比特

采样率 48000 / 符号率 1200 = 每符号 40 个采样点。遍历 offset 0~39，下变频后检查前 64 bit 是否为 `1010...` 交替。**offset=17** 完美匹配 64/64。

### 3. BPSK 盲相位估计

用 MPSK squaring method：所有符号平方后求和消去符号极性，取相角的一半估计载波相位。

```python
sos = sum(z * z for z in symbols)  # 平方消去符号极性
phi = atan2(sos.imag, sos.real) / 2  # 估计载波相位
```

### 4. 训练块分析

| 区间 | 比特数 | 用途 |
|:---|:---|:---|
| bit[0:64] | 64 | 交替同步序列 `101010...`，确定符号边界和极性 |
| bit[64:80] | 16 | PN 扰码器的 LFSR 初始种子 |

从 offset=17 读出种子：`bit[64:80] = 0xE5A2`

### 5. 去扰 + 去交织 + 纠错

| 步骤 | 操作 |
|------|------|
| 去PN | LFSR(种子0xE5A2, 多项式0x1D00) XOR |
| 逆DMA | 列优先→行优先重排 (16列) |
| 纠错 | Hamming(7,4)最近邻解码，码字 `[p1,p2,d1,p4,d2,d3,d4]` |

### 6. 暴力搜索 PN 对齐

PN skip 值未知，遍历 0~511，当 **skip=8** 时 CRC32 验证通过。

帧格式：`b"BP1 " + 正文 + 4字节CRC32(大端)`，75 个码字无错，5 个纠正 1 bit。

## Exploit

```python
import csv, io, math, zlib

def load_samples(csv_data):
    rows = csv.DictReader(io.StringIO(csv_data))
    return [complex(float(r["i"]), float(r["q"])) for r in rows]

class Scrambler:
    def __init__(self, seed, poly=0x1D00):
        self.poly, self.state = poly, seed & 0xFFFF
    def sequence(self, n):
        bits = []
        for _ in range(n):
            bits.append(self.state & 1)
            parity = bin(self.state & self.poly).count("1") & 1
            self.state = (self.state >> 1) | (parity << 15)
        return bits

def hamming_table():
    table = {}
    for nib in range(16):
        d1,d2,d3,d4 = (nib>>3)&1,(nib>>2)&1,(nib>>1)&1,nib&1
        table[nib] = (d1^d2^d4, d1^d3^d4, d1, d2^d3^d4, d2, d3, d4)
    return table

H74 = hamming_table()

def fec_correct(bits):
    nibbles, stats = [], {}
    for k in range(0, len(bits)-6, 7):
        rx = bits[k:k+7]
        best_nib, best_dist = 0, 8
        for nib_val, cw in H74.items():
            d = sum(a^b for a,b in zip(rx,cw))
            if d < best_dist: best_dist, best_nib = d, nib_val
        stats[best_dist] = stats.get(best_dist,0) + 1
        nibbles.append(best_nib)
    buf = bytearray()
    for j in range(0, len(nibbles)-1, 2):
        buf.append((nibbles[j]<<4) | nibbles[j+1])
    return bytes(buf), stats

def unscramble_dma(data, width):
    depth = len(data) // width
    out = [0] * len(data)
    for row in range(depth):
        for col in range(width):
            out[row*width+col] = data[col*depth+row]
    return out

# 主流程：遍历offset → 下变频 → 盲相位 → 硬判决 → 暴力PN skip → CRC验证
Fs, Rs, df, W = 48000, 1200, 1700, 16
osr = round(Fs / Rs)
# ... (下变频+解调+搜索过程)
# skip=8 时命中: CRC32=0xA60A993C, FEC: {0:75, 1:5}
```

**运行结果：**

```
[*] cfg: Fs=48000 Rs=1200 df=1700 osr=40 preamble=80 dma_w=16
[*] 25670 I/Q samples loaded
[+] symbol_offset=17  sync_corr=64/64
[+] pn_seed=0xE5A2  pn_shift=8
[+] FEC: {0: 75, 1: 5}
[+] CRC32=0xA60A993C verified
[+] flag: ISCC{R7!q_Z@4m^T9?p$V%k&2*n~L#x}
```

## Distilled Knowledge

| 知识点 | 关键 |
|--------|------|
| BPSK解调 | 下变频+盲相位估计+硬判决 |
| LFSR扰码 | PN序列XOR去扰 |
| Column-DMA | 列优先写→行优先读=逆交织 |
| Hamming(7,4) | 4bit数据→7bit码字, 纠1bit错 |
| CRC32 | 帧格式校验, 暴力搜索终止条件 |

---

*Original writeup: `C:\Users\48714\Desktop\wp\区域赛\盲相阵列_WriteUp.md`*
