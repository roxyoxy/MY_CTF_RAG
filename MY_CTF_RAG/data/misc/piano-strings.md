# 黑白琴弦

> **Category**: MISC | **Difficulty**: Medium | **Competition**: ISCC 2026 区域赛
> **Flag**: `ISCC{gorb20tDTUJZ8qz4151515}`

---

## Attack Chain

```
21张条形码PNG三层提取(扫描+LSB+tEXt) → 索引排序 → 构建21×21 QR码 → 解码得RAR密码 → PDF隐写提示 → 识谱得flag
```

## Key Techniques

### 1. 三层数据提取

21张 PNG 图片（barcode_00.png ~ barcode_20.png），每张进行三层提取：

| 层 | 方法 | 内容 |
|---|------|------|
| L1 | pyzbar 扫描条形码 | 8 字符字符串 |
| L2 | R 通道第 0 行 LSB 隐写 | 8 字符字符串 |
| L3 | PNG tEXt Chunk（Comment 字段） | 8 字符字符串 |

### 2. 识别索引层 — 字母+6位hex+相同字母

在三层中筛选"索引型"数据：格式为 `字母+6位hex+相同字母`（首尾字母相同），字母范围 A-U。例如 `A1fc37fA` 对应字母 A。

```
A→barcode_02  B→barcode_05  C→barcode_12  D→barcode_07  E→barcode_04
F→barcode_06  G→barcode_14  H→barcode_11  I→barcode_09  J→barcode_13
K→barcode_10  L→barcode_01  M→barcode_20  N→barcode_00  O→barcode_15
P→barcode_03  Q→barcode_18  R→barcode_19  S→barcode_17  T→barcode_08
U→barcode_16
```

### 3. 21×21 QR 码构建

每个 hex 值 24 位，取后 21 位构成 QR 码一行，21 行 = Version 1 QR 码：

```python
hex_data = {
    'A': '1fc37f', 'B': '104541', 'C': '17445d', 'D': '17545d',
    'E': '175a5d', 'F': '104b41', 'G': '1fd57f', 'H': '000300',
    'I': '18e118', 'J': '178ed8', 'K': '00670a', 'L': '1e358f',
    'M': '0b7e51', 'N': '0016b5', 'O': '1fdd02', 'P': '1054de',
    'Q': '1744a3', 'R': '174fb4', 'S': '174d7b', 'T': '105094',
    'U': '1fd02a',
}

for letter in 'ABCDEFGHIJKLMNOPQRSTU':
    val = int(hex_data[letter], 16)
    bits21 = format(val, '024b')[3:]  # 丢弃前3位
    row = [int(b) for b in bits21]
```

矩阵放大后用 OpenCV QRCodeDetector 解码得：`gorb20tDTUJZ8qz`（RAR 密码）

### 4. PDF 隐写

用密码解压 `score.rar` 得致爱丽丝钢琴谱。PyMuPDF 提取隐藏文本层：

```
KEY+bnVtYmVyYWJvdmVsaW5l3
```

Base64 解码 `bnVtYmVyYWJvdmVsaW5l` → `numberaboveline`，完整含义：**KEY + number above line 3**

### 5. 识别第三行谱表上方指法数字

PDF 渲染为图片后切分第三行谱表区域，五线谱上方的指法标记数字（1-5 小数字）从左到右依次为：**4 1 5 1 5 1 5**

## Exploit

```python
import cv2, numpy as np, struct, re
from PIL import Image
from pyzbar.pyzbar import decode

def extract_png_text_chunk(filepath):
    with open(filepath, 'rb') as f:
        f.read(8)  # PNG signature
        while True:
            header = f.read(8)
            if len(header) < 8: break
            length = struct.unpack('>I', header[:4])[0]
            chunk_type = header[4:8]
            data = f.read(length)
            f.read(4)  # CRC
            if chunk_type == b'tEXt':
                _, _, value = data.partition(b'\x00')
                return value.decode('ascii', errors='replace')
    return ''

def extract_lsb_row0(filepath):
    img = Image.open(filepath)
    row0_pixels = [img.getpixel((x, 0))[0] for x in range(img.width)]
    bits = ''.join(str(p & 1) for p in row0_pixels)
    return ''.join(chr(int(bits[i:i+8], 2)) for i in range(0, 64, 8))

# Step 1-2: 提取三层 + 识别索引
index_pattern = re.compile(r'^([A-U])([0-9a-f]{6})\1$')
hex_data = {}
for i in range(21):
    for layer_fn in [lambda: decode(Image.open(f'barcode_{i:02d}.png'))[0].data.decode(),
                     lambda: extract_lsb_row0(f'barcode_{i:02d}.png'),
                     lambda: extract_png_text_chunk(f'barcode_{i:02d}.png')]:
        text = layer_fn()
        m = index_pattern.match(text)
        if m:
            hex_data[m.group(1)] = m.group(2)

# Step 3: 构建 QR 码
matrix = []
for letter in 'ABCDEFGHIJKLMNOPQRSTU':
    bits21 = format(int(hex_data[letter], 16), '024b')[3:]
    matrix.append([int(b) for b in bits21])

arr = np.array(matrix, dtype=np.uint8)
scale, border = 20, 4
qr_img = 255 - np.kron(np.pad(arr, border), np.ones((scale,scale))).astype(np.uint8) * 255
result, _, _ = cv2.QRCodeDetector().detectAndDecode(qr_img)
print(f"QR解码: {result}")  # gorb20tDTUJZ8qz

# Step 4: 解压 RAR + PDF 提取
# 7z x score.rar -pgorb20tDTUJZ8qz

# Step 5-6: PDF 隐写 + 识谱
key = "gorb20tDTUJZ8qz"
numbers = "4151515"
print(f"Flag: ISCC{{{key}{numbers}}}")
```

**运行结果：**

```
QR解码结果: gorb20tDTUJZ8qz
PDF隐藏文本: KEY+bnVtYmVyYWJvdmVsaW5l3
Base64解码: numberaboveline
第三行指法数字: 4 1 5 1 5 1 5
Flag: ISCC{gorb20tDTUJZ8qz4151515}
```

## Distilled Knowledge

| 知识点 | 关键 |
|--------|------|
| 条形码多层隐写 | 文本+LSB+元数据三通道 |
| QR码手工构建 | hex→二进制→矩阵→放大→解码 |
| PNG tEXt chunk | 可携带自定义文本元数据 |
| PDF隐写 | 不可见字符层隐藏文本 |
| 索引识别 | 首尾字母相同=索引标记 |

---

*Original writeup: `C:\Users\48714\Desktop\wp\区域赛\黑白琴弦_WriteUp_final.md`*
