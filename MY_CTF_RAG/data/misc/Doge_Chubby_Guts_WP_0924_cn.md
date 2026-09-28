# 肥嘟嘟（你的胆子真是肥嘟嘟的）· Writeup（中文版）

- **题目**：你的胆子真是肥嘟嘟的（MISC · 肥嘟嘟）
- **分类**：MISC
- **附件**：`你的胆子真是肥嘟嘟的.zip` → 内含 `read.md` + `receipt.png` + `voice.wav` + `final.zip`
- **关键词**：二维码解码 · WAV DTMF 音频分析 · ZIP 真加密 · EXIF 隐写 · JPEG 尾部附加 ZIP · Base64
- **最终 flag**：`npusec{KFC_cr4zy_thursday_v1v0_50}`

---

## 题干（read.md）

> # 疯狂星期四：鸡腿失踪案
>
> 2026年9月10日，星期四。我的肯德基疯狂星期四外卖被偷了。
>
> 骑手甩给我一张小票 `receipt.png`，店家让我听录音 `voice.wav`，现场还有一个加密压缩包 `final.zip`。
>
> 侦探，帮帮我。外卖可以不要，但 flag 必须找回来。

---

## 解题过程

### 第一步：小票是张二维码 —— B 站发疯视频（诱饵）

`receipt.png`（660×660）画面的根本不是小票，而是一张**纯二维码**。cv2 解码：

```python
import cv2
det = cv2.QRCodeDetector()
data, pts, _ = det.detectAndDecode(cv2.imread('receipt.png'))
print(data)   # https://b23.tv/qxtV6hU
```

得到 B 站短链 `https://b23.tv/qxtV6hU`（疯狂星期四发疯视频，纯娱乐）。但别急——**PNG 的 IEND 之后还有 94 字节尾注**：

```python
data = open('receipt.png','rb').read()
i = data.find(b'IEND')
print(data[i+8:].decode())   # 侦探，视频好看吗？真正的线索在店家的语音中，快去发动你的鬼脑吧
```

尾注官方指路：真线索在 `voice.wav`。"鬼脑"也在暗示 B 站鬼畜文化。

### 第二步：录音是 4 声"电话按键音" —— DTMF 解码出密码 0721

`voice.wav` = 3 秒、44100 Hz、16bit 单声道 PCM。能量包络检测出 **4 个音调段**（每段 0.36 秒，等间隔分布），每段 FFT 出**两个强峰**——双音多频（DTMF）的标准指纹：

| 段 | 低频 (行) | 高频 (列) | 按键 |
|:---:|:---:|:---:|:---:|
| 1 | 941.7 Hz | 1336.1 Hz | **0** |
| 2 | 852.8 Hz | 1208.3 Hz | **7** |
| 3 | 696.4 Hz | 1337.0 Hz | **2** |
| 4 | 697.2 Hz | 1208.3 Hz | **1** |

```python
import wave, numpy as np
w = wave.open('voice.wav')
sig = np.frombuffer(w.readframes(w.getnframes()), dtype=np.int16).astype(float)
# 能量包络找音调段 → 每段加窗 FFT → 取双峰 → 查 DTMF 频率表
```

得密码 **`0721`**——B 站鬼畜名梗数字，与尾注"鬼脑"呼应，闭环。

### 第三步：解 final.zip → doge.jpg，密码藏在 EXIF 里

```python
import zipfile
zipfile.ZipFile('final.zip').extractall(pwd=b'0721')   # → doge.jpg
```

`doge.jpg`（1080×1132）查 EXIF，`ImageDescription`（tag 270）直接送密码：

```python
from PIL import Image
print(dict(Image.open('doge.jpg').getexif()))
# 305: 'Doge Meme Generator'
# 270: 'My secret is behind me. Password: chicken_thursday_50'
```

"My secret is **behind** me"（我的秘密在我身后）是双关：密码给你了，而秘密本体在** JPEG 数据流的身后**。

### 第四步：JPEG 尾部附加 ZIP —— 剥出加密的 flag.txt

`FFD9`（JPEG 结束符）之后还挂着 242 字节，正是第二个 ZIP：

```python
d = open('doge.jpg','rb').read()
i = d.find(b'\xff\xd9')
open('inner.zip','wb').write(d[i+2:])   # 剥出附加 ZIP
# 内含 flag.txt（48 字节，真加密：csize 60 = 48 + 12 字节加密头）
```

### 第五步：第二把钥匙开箱 → Base64 → flag

```python
import zipfile, base64
ct = zipfile.ZipFile('inner.zip').read('flag.txt', pwd=b'chicken_thursday_50')
print(base64.b64decode(ct).decode())
# npusec{KFC_cr4zy_thursday_v1v0_50}
```

`v1v0` = V我50，疯狂星期四梗宇宙终极闭环。

---

## 完整攻击链

```
你的胆子真是肥嘟嘟的.zip
  ├─ read.md ──── 疯狂星期四案情介绍（三件套指路）
  ├─ receipt.png ─┬─ ① QR 解码 → B 站视频（诱饵，"视频好看吗"）
  │               └─ ② IEND 尾注 → "真线索在语音中"（官方指路）
  ├─ voice.wav ──── ③ DTMF 双音 ×4 → 0721（final.zip 密码）
  └─ final.zip ──── ④ 0721 解压 → doge.jpg
       └─ ⑤ EXIF ImageDescription → chicken_thursday_50（第二把钥匙）
            └─ ⑥ FFD9 后附加 ZIP → flag.txt（真加密）
                 └─ ⑦ chicken_thursday_50 解密 → Base64 → npusec{KFC_cr4zy_thursday_v1v0_50}
```

---

## 知识点沉淀

1. **DTMF 电话双音多频**：每键 = 低频组（697/770/852/941 Hz）× 高频组（1209/1336/1477/1633 Hz）各取一；解法 = 能量包络切段 → 每段 FFT 取双峰 → 查表
2. **音调段检测**：`√(滑动能量)` 包络 + 阈值 + `diff` 找边沿，比直接 FFT 全段更能定位多个音
3. **EXIF 隐写**：JPEG 的 `ImageDescription` / `Software` 等文本字段是藏密码/提示的常见位置，`PIL getexif()` 一把出
4. **JPEG 尾部附加**：`FFD9` 之后的内容被看图软件无视，是 ZIP/数据附加的经典位；`find(b'\xff\xd9')` 即可剥离
5. **真伪加密判别**：真加密的 csize = 原文 + 12 字节加密头（60 = 48 + 12 ✓）；伪加密只是 flag bit0 撒谎（对照法棍题）
6. **梗即钥匙**：0721 / V我50 / 疯狂星期四——出题人文化梗与密码强关联，解出数字先过一遍"是不是名梗"

---

## 踩坑记录

- 二维码扫出的视频链接是娱乐诱饵，不是线索本体——**附件里的"多余引导文本"（IEND 尾注）才是官方路标**
- 内层 ZIP 与外层不同：final.zip 是伪加密与否要验，内层是真加密（错的密码会 RuntimeError，别盲猜——先找 EXIF）

---

## 📸 复现与截图位（待补，复现后回填）

> 截图统一存 `WP_cn\MISC\图片\`，命名 `序号_内容.png`；复现流程按本文五步走。中间产物（inner.zip / 解出的 doge.jpg）在 `xiaosai\_work\fdd\` 可直接取用。

| # | 截图内容 | 存放路径 | 状态 |
|:---:|:---|:---|:---:|
| 1 | receipt.png 画面（二维码特写）+ cv2 解码输出 B 站短链 | `图片/01_二维码解码.png` | ⬜ |
| 2 | IEND 后尾注 hex/文本视图（"真正的线索在店家的语音中"） | `图片/02_尾注指路.png` | ⬜ |
| 3 | 能量包络图：4 个音调段 + 各段 FFT 双峰频率标注 | `图片/03_DTMF双峰.png` | ⬜ |
| 4 | DTMF 查表得 0721 + final.zip 解压成功（doge.jpg 出现） | `图片/04_密码0721开箱.png` | ⬜ |
| 5 | doge.jpg 画面 + EXIF 输出（Password: chicken_thursday_50） | `图片/05_EXIF密码.png` | ⬜ |
| 6 | FFD9 后附加 ZIP 剥离 + flag.txt 解密 + Base64 出 flag | `图片/06_终flag.png` | ⬜ |

回填时把 `![01](图片/01_二维码解码.png)` 贴到对应步骤。
