# [Campus CTF MISC] 雪里看花 (Flowers in the Snow) — Writeup (Full Version)

> **Points**: 400 pts ✅ Accepted
> **Category**: MISC / multi-layer steganography chain (PNG trailing data + nested double ZIP + blind watermark + ZIP password + video stego)
> **Attachment**: `challenge.png` (11.7 MB, 1280×720) + `challenge (3).zip` (same md5 as the png, duplicate)
> **Description**: "So many things here — sadly all of them were turned into rolling logs." (decoys)

## Flag

```
npusec{sTr3am-f1ow3r-f0r-u-2333}
```

(stream-flower-for-u — the flower in the stream, matching the title "Flowers in the Snow")

---

## 0. The Big Picture: Five Layers of Onion

```
challenge.png
├── Layer 1  9.6M bytes appended after PNG IEND
├── Layer 2  Nested double ZIP: outer = encrypted noise.mp4 (rolling log) | inner = plaintext flag.txt (glued after outer EOCD)
├── Layer 3  TRAP: flag.txt contains a FAKE flag npulie{...} (npu+LIE = a lie)
├── Layer 4  Outer EOCD comment bwm_shape=143 → blind watermark on the PNG → password motion_over_pixels
└── Layer 5  Password unlocks noise.mp4 → weak pattern superimposed on snow frames → real flag
```

Every pun pays off: **"snow"** = noise image + snow video; **"rolling logs"** = decoys everywhere (noise.mp4, the fake flag, the double ZIP shell).

## 1. Reconnaissance

### 1.1 Size intuition

A 1280×720 PNG should top out around 2 MB — this one is 11.7 MB: image data is essentially incompressible random noise. The title is literal.

### 1.2 Trailing-data check

```bash
python -c "
data = open('challenge.png','rb').read()
idx = data.find(b'IEND')
print('IEND at', idx, '/ file size', len(data))
print('trailing:', len(data)-idx-8)   # 9688151 bytes!
"
```

`file` smells ZIP at the tail; the very end carries a plaintext comment: **`the flag is in flag.txt`**.

> 📸 **[Screenshot slot 01]** save to `WP_cn\MISC\图片\01_IEND尾部发现.png` (shared with the CN version)
> Content: terminal output of the IEND check script above (showing trailing: 9688151)

## 2. First Trap: The Fake Flag

`zipfile` auto-corrects for prefix data (SFX tolerance), so the raw PNG opens as a ZIP:

```bash
python -c "import zipfile; print(zipfile.ZipFile('challenge.png').read('flag.txt').decode())"
# npulie{n3v3r_g0nna_l00k_at_fr4mes}
```

**Not the real flag.** Three self-betrayals:

1. Prefix is `npulie{`, not the contest's standard `npusec{` — **npu + LIE = NPU is lying**
2. The comment "the flag is in flag.txt" is itself part of the lie
3. The content `never gonna look at frames` — **reverse psychology**: the real flag IS in the video frames

> Confirmed wrong by the judge. Lesson: **prefix mismatch / defeatist content (never gonna...) = strong trap indicators**.

> 📸 **[Screenshot slot 02]** save to `WP_cn\MISC\图片\02_假flag判错.png` (shared)
> Content: the judge rejecting `npulie{n3v3r...}` — the most narrative shot of the whole writeup

## 3. ZIP Structure Dissection (Spotting the Rolling Log)

Carve the tail into `tail.bin` (9688151 bytes) — a double-EOCD structure:

```
tail.bin
├── outer ZIP shell
│   ├── LFH  noise.mp4   flag_bits=0x0001 (ZipCrypto), stored, claims 9.7MB
│   ├── CDH  noise.mp4   @9687851
│   └── EOCD             @9687942, 13-byte comment: bwm_shape=143  ← the key hint!
└── inner ZIP (174 bytes, complete & standalone, glued AFTER the outer EOCD)
    ├── LFH/CDH  flag.txt (deflate, plaintext) ← home of the fake flag
    └── EOCD + comment "the flag is in flag.txt"
```

Parsers scan **backwards from EOF** for the EOCD → they only see the inner flag.txt; the 9.7MB "encrypted video" is a rolling log baiting brute-force. But the log isn't wasted — **the outer EOCD comment `bwm_shape=143` is the next layer's key**:

- `bwm` = **B**lind **W**ater**M**ark
- `143` = watermark bit length (wm_shape)

## 4. Blind Watermark Extraction → ZIP Password

```bash
pip install blind-watermark
python -c "
from blind_watermark import WaterMark
bwm = WaterMark(password_wm=1, password_img=1)   # defaults work
print(bwm.extract('img.png', wm_shape=143, mode='str'))
# motion_over_pixels
"
```

**`motion_over_pixels`** is the password for noise.mp4:

> 📸 **[Screenshot slot 03]** save to `WP_cn\MISC\图片\03_盲水印提取.png` (shared)
> Content: watermark extraction printing `motion_over_pixels` (ideally with the `bwm_shape=143` EOCD comment visible too)

```bash
python -c "
import zipfile
z = zipfile.ZipFile('outer.zip')          # carve the outer shell out first
data = z.read('noise.mp4', pwd=b'motion_over_pixels')
open('noise.mp4','wb').write(data)        # head ftypisom = real MP4
"
```

> ⚠️ Pitfall: OpenCV `imread` fails on non-ASCII (Chinese) paths — copy the file to an ASCII path first.

## 5. Final Layer: The Flower in the Snow

noise.mp4: 1120 frames / 60fps / 18.7s / 1280×720, every frame black-and-white TV static (bimodal 0/255 pixels, std≈126).

**The flag is superimposed on the frames at tiny strength** — invisible in a single frame, but:

- **Statistical evidence**: averaging all 1120 frames yields a mean image with std=7.04 — **double** the pure-random expectation (126/√1120≈3.8) → systematic per-pixel bias across frames = a superimposed pattern
- Contrast-stretched average: a central vertical band (x≈280-452) shows dark columns at a **strict 4px interval** (stroke fingerprint of text/graphics)

> 📸 **[Screenshot slot 04]** save to `WP_cn\MISC\图片\04_累加平均增强图.png` (shared)
> Content: `avg_stretched.png` — the 1120-frame average, contrast-stretched; dark pattern emerging from the snow. Ready-made file: `C:\Users\48714\Desktop\tmp_work_xueli\avg_stretched.png`
- **The naked eye wins**: during playback, persistence of vision performs temporal integration (a living moving-average filter), revealing the weak pattern — read out `npusec{sTr3am-f1ow3r-f0r-u-2333}`, accepted ✅

> 📸 **[Screenshot slot 05]** save to `WP_cn\MISC\图片\05_播放器flag瞬间.png` (shared)
> Content: the player paused on the frame where the flag is visible — the historic moment of the human-eye finish

> Field notes: automated per-frame statistics (mean/std 3σ outliers, flat-block scans) never isolated a "text frame" — the pattern is not "flashed on a few frames" but "weakly added to every frame", so per-frame metrics barely move. A human watching the video took it down. **Baseline check for video stego: PLAY IT FIRST**, then bring in statistics.

## 6. Full Attack Chain Replay

```
challenge.png (11.7MB)
  │ size intuition + IEND trailing check
  ▼ 9.6M-byte tail = double ZIP
  │ zipfile direct open (SFX tolerance)
  ▼ flag.txt → npulie{...} FAKE (rejected ✗)
  │ re-read ZIP comments
  ▼ outer EOCD: bwm_shape=143 → watermark params
  │ blind-watermark default-password extraction
  ▼ motion_over_pixels → outer ZIP password
  │ unlock noise.mp4 (snow video)
  ▼ normal playback + human temporal integration
  ▼ npusec{sTr3am-f1ow3r-f0r-u-2333} ✅
```

## 7. Takeaways

1. **PNG trailing data**: decoders stop at IEND; anything after it still renders — the classic hiding spot. Check `find(b'IEND')` vs file size.
2. **Double-EOCD ZIP nesting**: outer shell carries an "encrypted big file" rolling log while the real ZIP sits after the outer EOCD; parsers reading from EOF see only the inner one. EOCD comments (≤65535 bytes) are a prime spot to hide hints.
3. **Fake-flag tells**: prefix mismatch (npulie vs npusec) + defeatist content (never gonna...) + self-claiming "flag is here" = triple self-betrayal. The npuLIE pun.
4. **Blind watermarking**: `blind-watermark` lib (block-DCT + Arnold shuffle), compression- and scaling-resistant; default passwords 1/1 are most common; wm_shape is often hidden elsewhere. **Watermark content frequently serves as the next layer's password (key-chain design)**.
5. **Encrypted ZIP ≠ brute force**: scan file comments and adjacent layers for the key first.
6. **Weak-superimposition video stego**: pattern added at 1/n strength per frame, invisible per frame; detect via frame averaging (std ≫ σ/√n) or **just play it** (temporal integration). A "motion" hint word means the pattern moves — a global average will blur it; average in chunks instead.
7. **Windows pitfall**: OpenCV & friends choke on non-ASCII paths — relocate to ASCII first.

---

*Writeup by Claude Code + the boss's eyes for the final kill · 2026-09-23 · Campus CTF practice, challenge #1 (400pts)*
*Detour archive: celebrated early on the fake flag once; three per-frame statistical sweeps (3σ / flat-blocks / single-pixel bits) all came up empty — weak-superimposition stego barely moves frame-level metrics. Eye + player remains the #1 detector.*
