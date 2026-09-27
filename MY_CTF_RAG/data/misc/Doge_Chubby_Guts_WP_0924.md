# Your Guts Are So Chubby (肥嘟嘟) · Writeup (English)

- **Challenge**: 你的胆子真是肥嘟嘟的 ("Your Guts Are So Chubby")
- **Category**: MISC
- **Attachment**: `你的胆子真是肥嘟嘟的.zip` → `read.md` + `receipt.png` + `voice.wav` + `final.zip`
- **Keywords**: QR decoding · WAV DTMF analysis · password-protected ZIP · EXIF steganography · ZIP appended after JPEG EOI · Base64
- **Final flag**: `npusec{KFC_cr4zy_thursday_v1v0_50}`

---

## Challenge Text (read.md, translated)

> # Crazy Thursday: The Case of the Missing Drumstick
>
> Sept 10, 2026, Thursday. My KFC Crazy Thursday delivery was stolen.
>
> The rider left me a receipt `receipt.png`, the shop told me to listen to a recording `voice.wav`, and there's an encrypted archive `final.zip` at the scene.
>
> Detective, help me. Keep the chicken, but I must get the flag back.

(Crazy Thursday / "V me 50" is a famous Chinese KFC meme — it matters later.)

---

## Solution

### Step 1: The "Receipt" Is a QR Code — A Bilibili Meme Video (Decoy)

`receipt.png` (660×660) contains no receipt at all — it's a plain **QR code**:

```python
import cv2
det = cv2.QRCodeDetector()
data, pts, _ = det.detectAndDecode(cv2.imread('receipt.png'))
print(data)   # https://b23.tv/qxtV6hU
```

That's a Bilibili short link (a Crazy Thursday meme video — pure entertainment). But before chasing it, check **what trails behind PNG's IEND** (94 extra bytes):

```python
data = open('receipt.png','rb').read()
i = data.find(b'IEND')
print(data[i+8:].decode())
# "Detective, enjoyed the video? The real clue is in the shop's voice recording.
#  Go fire up your ghost brain."
```

The challenge author literally points the way: the real clue is in `voice.wav` ("ghost brain" = a hint at Bilibili meme culture).

### Step 2: The Recording Is 4 "Keypad Tones" — DTMF Decodes to 0721

`voice.wav` = 3 s, 44100 Hz, 16-bit mono PCM. An energy envelope reveals **exactly 4 tone bursts** (0.36 s each, evenly spaced), and each burst's FFT shows **two strong peaks** — the classic Dual-Tone Multi-Frequency fingerprint:

| Burst | Low freq (row) | High freq (col) | Key |
|:---:|:---:|:---:|:---:|
| 1 | 941.7 Hz | 1336.1 Hz | **0** |
| 2 | 852.8 Hz | 1208.3 Hz | **7** |
| 3 | 696.4 Hz | 1337.0 Hz | **2** |
| 4 | 697.2 Hz | 1208.3 Hz | **1** |

```python
import wave, numpy as np
w = wave.open('voice.wav')
sig = np.frombuffer(w.readframes(w.getnframes()), dtype=np.int16).astype(float)
# energy envelope -> segment the bursts -> windowed FFT per burst
# -> pick the two peaks -> look up the DTMF grid
```

Password: **`0721`** — a famous Chinese ACG/meme number, closing the loop with the "ghost brain" hint.

### Step 3: Open final.zip → doge.jpg; the Password Hides in EXIF

```python
import zipfile
zipfile.ZipFile('final.zip').extractall(pwd=b'0721')   # → doge.jpg
```

`doge.jpg` (1080×1132) carries a gift in its EXIF `ImageDescription` (tag 270):

```python
from PIL import Image
print(dict(Image.open('doge.jpg').getexif()))
# 305: 'Doge Meme Generator'
# 270: 'My secret is behind me. Password: chicken_thursday_50'
```

"My secret is **behind** me" is a pun: here's the password, but the secret itself sits **behind the JPEG data stream**.

### Step 4: A ZIP Appended After FFD9 — Carve Out the Encrypted flag.txt

242 bytes trail after `FFD9` (the JPEG end-of-image marker) — a second ZIP:

```python
d = open('doge.jpg','rb').read()
i = d.find(b'\xff\xd9')
open('inner.zip','wb').write(d[i+2:])   # carve the appended archive
# contains flag.txt (48 bytes, GENUINELY encrypted: csize 60 = 48 + 12-byte crypto header)
```

### Step 5: Second Key Unlocks the Box → Base64 → Flag

```python
import zipfile, base64
ct = zipfile.ZipFile('inner.zip').read('flag.txt', pwd=b'chicken_thursday_50')
print(base64.b64decode(ct).decode())
# npusec{KFC_cr4zy_thursday_v1v0_50}
```

`v1v0` = "V me 50" — the Crazy Thursday meme universe reaches its final closure.

---

## Full Attack Chain

```
你的胆子真是肥嘟嘟的.zip
  ├─ read.md ──── the case briefing (introduces the three artifacts)
  ├─ receipt.png ─┬─ ① QR decode → Bilibili video (decoy, "enjoyed the video?")
  │               └─ ② trailing note after IEND → "real clue is in the voice"
  ├─ voice.wav ──── ③ DTMF dual tones ×4 → 0721 (final.zip password)
  └─ final.zip ──── ④ unlocked with 0721 → doge.jpg
       └─ ⑤ EXIF ImageDescription → chicken_thursday_50 (second key)
            └─ ⑥ ZIP appended after FFD9 → flag.txt (genuinely encrypted)
                 └─ ⑦ decrypt with chicken_thursday_50 → Base64 → npusec{KFC_cr4zy_thursday_v1v0_50}
```

---

## Takeaways

1. **DTMF**: each key = one low-group tone (697/770/852/941 Hz) × one high-group tone (1209/1336/1477/1633 Hz); recipe = energy envelope segmentation → per-burst FFT for two peaks → grid lookup
2. **Tone segmentation**: sliding RMS envelope + threshold + edge diff locates bursts better than one big FFT
3. **EXIF steganography**: `ImageDescription` / `Software` text fields are classic password/hint drops; `PIL getexif()` reads them in one call
4. **Data after JPEG EOI**: viewers ignore everything past `FFD9` — the classic append spot; `find(b'\xff\xd9')` carves it
5. **Real vs fake ZIP encryption**: real encryption shows csize = plaintext + 12-byte crypto header (60 = 48 + 12 ✓); fake encryption only lies in flag bit0 (contrast with the Baguette challenge)
6. **Memes are keys**: 0721 / "V me 50" / Crazy Thursday — when a decoded value is a famous meme number, it's probably the password, not a coincidence

---

## Post-mortem Notes

- The QR's video link is an entertainment decoy — the real signpost was the challenge author's trailing text after IEND
- The inner ZIP is genuinely encrypted (wrong passwords raise RuntimeError); don't brute-force before checking EXIF

---

## 📸 Reproduction & Screenshot Slots (to fill after replay)

> Save screenshots to `WP_EN\MISC\images\`, named `NN_description.png`; replay follows the five steps of this WP. Intermediate artifacts (inner.zip / extracted doge.jpg) remain in `xiaosai\_work\fdd\` for direct reuse.

| # | Content | Path | Status |
|:---:|:---|:---|:---:|
| 1 | receipt.png contents (QR close-up) + cv2 decode output with the Bilibili link | `images/01_qr_decode.png` | ⬜ |
| 2 | Hex/text view of the trailing note after IEND ("real clue is in the voice") | `images/02_iend_note.png` | ⬜ |
| 3 | Energy envelope plot: 4 tone bursts + per-burst FFT dual peaks annotated | `images/03_dtmf_peaks.png` | ⬜ |
| 4 | DTMF table lookup giving 0721 + final.zip extracted successfully (doge.jpg appears) | `images/04_password_0721.png` | ⬜ |
| 5 | doge.jpg contents + EXIF dump (Password: chicken_thursday_50) | `images/05_exif_password.png` | ⬜ |
| 6 | Carving the appended ZIP + decrypting flag.txt + Base64 revealing the flag | `images/06_final_flag.png` | ⬜ |

When filling in, embed `![01](images/01_qr_decode.png)` at the matching step.
