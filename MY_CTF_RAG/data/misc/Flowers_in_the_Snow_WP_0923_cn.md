# [校赛 MISC] 雪里看花 — Writeup（完整版）

> **分值**：400 pts ✅ 已提交通过
> **题型**：MISC / 多层隐写链（PNG尾部 + 双ZIP套娃 + 盲水印 + ZIP密码 + 视频隐写）
> **附件**：`challenge.png`（11.7 MB，1280×720）+ `challenge (3).zip`（与 png 同 md5，双份保险）
> **题干**：有好多好多东西，可惜都被当棍木了。

## Flag

```
npusec{sTr3am-f1ow3r-f0r-u-2333}
```

（stream-flower-for-u：码流之花送给你——点题"雪里看花"）

---

## 〇、题目全景：五层洋葱

```
challenge.png
├── Layer 1  PNG IEND 之后附加 968 万字节
├── Layer 2  双层 ZIP 套娃：外层=加密 noise.mp4（滚木）｜内层=明文 flag.txt（附在外层 EOCD 后）
├── Layer 3  陷阱：flag.txt 里是【假 flag】npulie{...}（npu+LIE=谎言）
├── Layer 4  外层 EOCD 注释 bwm_shape=143 → PNG 盲水印 → 密码 motion_over_pixels
└── Layer 5  密码解开 noise.mp4 → 黑白雪花视频帧里叠加弱图案 → 真 flag
```

题干双关全兑现：**"雪"** = 噪声图 + 雪花视频；**"被当棍木了"** = 全是滚木障眼法（noise.mp4 滚木、假 flag 滚木、双层 ZIP 滚木）。

## 一、侦察

### 1. 体积直觉

1280×720 的 PNG 正常至多 2MB，这张 11.7MB —— 图像数据接近纯随机（不可压缩）= 满屏雪花噪声，题目名字面意思。

### 1.2 尾部检查

```bash
python -c "
data = open('challenge.png','rb').read()
idx = data.find(b'IEND')
print('IEND at', idx, '/ file size', len(data))
print('trailing:', len(data)-idx-8)   # 9688151 字节！
"
```

`file` 识别尾部是 ZIP；文件最末尾有明文注释 **`the flag is in flag.txt`**。

> 📸 **[截图占位 01]** 存 `图片/01_IEND尾部发现.png`
> 内容：终端跑上述 IEND 检查脚本的输出（显示 trailing: 9688151）

## 二、第一陷阱：假 flag

`zipfile` 对前置数据自动修正偏移（SFX 容错），PNG 原文件直接当 ZIP 打开：

```bash
py -c "import zipfile; print(zipfile.ZipFile('challenge.png').read('flag.txt').decode())"
# npulie{n3v3r_g0nna_l00k_at_fr4mes}
```

**这不是真 flag。** 三处自曝：

1. 前缀是 `npulie{` 而非校赛正牌 `npusec{` —— **npu + LIE = NPU 在撒谎**
2. 注释 "the flag is in flag.txt" 本身就是谎言的一部分
3. flag 内容 `never gonna look at frames`（永远不用看帧）——**反话**，真 flag 恰恰藏在视频帧里

> 实测提交平台判错，确认假货。教训：**flag 前缀不符 / 内容劝退式（never/just give up）= 高度怀疑陷阱**。

> 📸 **[截图占位 02]** 存 `图片/02_假flag判错.png`
> 内容：提交平台判错提示（npulie{n3v3r...} Wrong 的瞬间——最有叙事价值的一张）

## 三、ZIP 结构解剖（滚木识别）

把 IEND 后数据切出（`tail.bin`，9688151 字节），双 EOCD 结构：

```
tail.bin
├── 外层 ZIP 壳
│   ├── LFH  noise.mp4   flag_bits=0x0001（ZipCrypto 加密），store，声明 9.7MB
│   ├── CDH  noise.mp4   @9687851
│   └── EOCD             @9687942，注释只有 13 字节：bwm_shape=143  ← 钥匙线索！
└── 内层 ZIP（174 字节，完整独立，贴在外层 EOCD 之后）
    ├── LFH/CDH  flag.txt（deflate 明文）← 假 flag 的老家
    └── EOCD + 注释 "the flag is in flag.txt"
```

工具从文件**末尾**找 EOCD 解析 → 只见内层 flag.txt；那个 9.7MB "加密视频" 是骗爆破的滚木。但滚木不白给——**外层 EOCD 注释 `bwm_shape=143` 是下一层的钥匙**：

- `bwm` = **B**lind **W**ater**M**ark（盲水印）
- `143` = 水印比特长度（wm_shape）

## 四、盲水印提取 → 拿到 ZIP 密码

```bash
py -m pip install blind-watermark
py -c "
from blind_watermark import WaterMark
bwm = WaterMark(password_wm=1, password_img=1)   # 默认密码即可
print(bwm.extract('img.png', wm_shape=143, mode='str'))
# motion_over_pixels
"
```

**`motion_over_pixels`**（在像素之上的运动）= noise.mp4 的 ZIP 密码：

> 📸 **[截图占位 03]** 存 `图片/03_盲水印提取.png`
> 内容：盲水印提取命令输出 `motion_over_pixels`（可连上 bwm_shape=143 的 EOCD 注释一起截）

```bash
py -c "
import zipfile
z = zipfile.ZipFile('outer.zip')          # 外层壳单独切出
data = z.read('noise.mp4', pwd=b'motion_over_pixels')
open('noise.mp4','wb').write(data)        # 头部 ftypisom = 真 MP4
"
```

> ⚠️ 坑：OpenCV `imread` 读不了中文路径（`C:\...\题目\MISC\雪里看花\`），先复制到纯 ASCII 路径再处理，否则报 `can't open/read file`。

## 五、终层：雪花视频里的花

noise.mp4：1120 帧 / 60fps / 18.7s / 1280×720，每帧都是黑白 TV 雪花（像素双峰 0/255，std≈126）。

**flag 以极弱强度叠加在帧上**——单帧肉眼不可见，但：

- **统计证据**：全 1120 帧累加平均后，均值图 std=7.04，是纯随机期望（126/√1120≈3.8）的**两倍** → 帧间存在系统性明暗偏置 = 叠加了固定/运动图案
- 平均图对比度拉伸后：中央 x≈280-452 竖带出现**间隔严格 4px** 的暗列（文字/图形笔画指纹）

> 📸 **[截图占位 04]** 存 `图片/04_累加平均增强图.png`
> 内容：avg_stretched.png（1120 帧平均+对比度拉伸，雪花下的暗纹浮现）——现成产物，直接复制 `C:\Users\48714\Desktop\tmp_work_xueli\avg_stretched.png`
- **人眼播放直接可见**：播放时视觉暂留自动做"时间积分"（活体累加平均器），弱图案显形 —— 肉眼读出 `npusec{sTr3am-f1ow3r-f0r-u-2333}`，提交通过 ✅

> 📸 **[截图占位 05]** 存 `图片/05_播放器flag瞬间.png`
> 内容：播放器暂停在 flag 可见的那一帧（你肉眼收官的历史性画面，WP 的灵魂图）

> 解题实况：自动化逐帧统计（mean/std 3σ 异常帧、平坦块扫描）均未直接锁定文字帧——因为图案不是"少数帧闪现"而是"每帧弱叠加"，帧级指标变化极小。最后人眼播放一波带走。**视频隐写 baseline 检查项：先正常播一遍**，再上统计分析。

## 六、完整攻击链回放

```
challenge.png (11.7MB)
  │ 体积直觉 + IEND 尾部检查
  ▼ 968 万字节尾部 = 双层 ZIP
  │ zipfile 直开（SFX 容错）
  ▼ flag.txt → npulie{...} 假 flag（提交失败 ✗）
  │ 回头翻 ZIP 注释
  ▼ 外层 EOCD: bwm_shape=143 → 盲水印参数
  │ blind-watermark 默认密码提取
  ▼ motion_over_pixels → 外层 ZIP 密码
  │ 解开 noise.mp4（雪花视频）
  ▼ 正常播放 + 人眼时间积分
  ▼ npusec{sTr3am-f1ow3r-f0r-u-2333} ✅
```

## 七、知识点沉淀

1. **PNG 尾部附加**：解码器只认 IHDR…IEND，之后塞啥都照显——文件尾隐写经典位。检查：`find(b'IEND')` vs 文件总长。
2. **双 EOCD ZIP 套娃**：外层壳放"加密大文件"滚木，真 ZIP 附在外层 EOCD 后；解析器从末尾找 EOCD 天然只见内层。反过来 EOCD 注释（≤65535 字节）是绝佳的"藏提示"位。
3. **假 flag 认脸**：前缀不符（npulie vs npusec）+ 内容劝退（never gonna...）+ 自称在 flag.txt = 三重自曝。npuLIE 双关。
4. **盲水印**：`blind-watermark` 库（频域 DCT 分块 + Arnold 打乱），抗压缩抗缩放；默认密码 1/1 最常见，出题人常把 wm_shape 藏在别处。**水印内容常被用作下一层的密码/key chain 设计**。
5. **ZipCrypto 加密 ZIP 不一定要爆破**：先全文件找注释/相邻层找钥匙。
6. **视频弱叠加隐写**：图案每帧叠加 1/n 强度，单帧不可见；检测 = 全帧平均（std 显著大于 σ/√n）或**直接人眼播放**（时间积分）。"motion" 类提示词 = 图案可能在动，全片平均会糊，需分段平均。
7. **Windows 坑**：OpenCV/很多库 `imread` 不吃中文路径，先挪 ASCII 路径。

---

*Writeup by Claude Code + 老板肉眼终杀 · 2026-09-23 · 校赛以赛代练 第 1 题（400pts）*
*弯路存档：假 flag 半场开香槟一次；逐帧统计三连（3σ/平坦块/单像素位）全空手——弱叠加型隐写帧级指标不敏感，人眼+播放器才是第一检测器。*
