# 喧宾夺主的信号

> **Category**: MISC | **Difficulty**: Medium | **Competition**: ISCC 2026 区域赛
> **Flag**: `ISCC{1dx3f1c4Ot10n_1Vs_th3_ki3y_t50_v1ct0Try}`

---

## Attack Chain

```
流量包过滤TCP → HTTP POST提取Base64 → 解码得加密RAR → UDP组播中找密码 → LSB隐写提取flag
```

## Key Techniques

### 1. 流量包整体分析

Wireshark 打开 `attachment-17.pcapng`，发现共 1509 个包：
- 1500 个 UDP 发往 `239.255.255.250:1900`（SSDP 组播，题干"摄像头垃圾流量"）
- 仅 9 个 TCP 包

题干提示"声东击西，三十六计第一套第六计"——大量 UDP 是障眼法，少量 TCP 才是真正线索。

### 2. TCP 流中提取 Base64

Wireshark 过滤 `tcp`，追踪 TCP 流发现 HTTP POST：

```
POST /command HTTP/1.1
Host: 45.78.1.1
Content-Type: application/json

{"instruction": "UEsDBBQAAQBjABJEdFxb4iukXQEA...", "note": "This is the real command."}
```

`instruction` 字段是一段很长的 Base64 编码数据。`note` 字段提示"This is the real command"。

### 3. Base64 解码得到加密 RAR

将 `instruction` 的值 Base64 解码，文件头为 `PK\x03\x04`，实际上是一个加密的压缩包。

### 4. 从 UDP 组播中找到密码

题干说"看似无意义的广播中，藏着打开真正宝藏的钥匙"。过滤 UDP 包逐个查看 payload：
- 绝大多数是 28 字节随机 Base64 字符串
- **4 个包特殊**——末尾有 `=` 填充符

解码得到：`ShengDongJiXi@36-1-6`（声东击西 · 三十六计第一套第六计）

### 5. 解压得到图片 + LSB 隐写提取 Flag

用密码解压得到 `image.png`（100x100，491 字节）。像素值仅在低位波动，是典型的 LSB 隐写特征。

## Exploit

```python
from PIL import Image

img = Image.open("image.png")
pixels = list(img.getdata())

# 提取 RGB 最低位
bits = ""
for p in pixels:
    bits += str(p[0] & 1) + str(p[1] & 1) + str(p[2] & 1)

# 每8位转为字节，遇到0xFF终止
result = bytearray()
for i in range(0, len(bits) - 7, 8):
    byte_val = int(bits[i:i+8], 2)
    if byte_val == 0xFF and len(result) > 10:
        break
    result.append(byte_val)

text = result.decode("latin-1")
print(text)
```

**运行结果：**

```
ISCC{1dx3f1c4Ot10n_1Vs_th3_ki3y_t50_v1ct0Try}
```

## Distilled Knowledge

| 知识点 | 关键 |
|--------|------|
| 流量过滤 | 大量噪音包中找少量有效TCP |
| Base64→ZIP | `PK\x03\x04` = ZIP文件头 |
| UDP隐写 | 在组播噪音中隐藏密码（`=`填充符区分） |
| LSB隐写 | RGB最低位拼接 → 字节流 |
| 题干提示 | "声东击西"暗示UDP是障眼法 |

---

*Original writeup: `C:\Users\48714\Desktop\wp\区域赛\喧宾夺主的信号_WriteUp_final.md`*
