# [Campus CTF MISC] Blind to Mount Lu (不识庐山) — Writeup

> **Category**: MISC / multi-layer stego + **symbology perspective trap** (QR → Han Xin Code)
> **Attachments**: `challenge.png` (55KB, 2664×2664 RGBA) + `challenge.zip` (decoy, byte-identical)
> **Statement** (300 pts): "Blind to Mount Lu / seen from afar, high, low — each a different view" (Su Shi's poem)
> **Theme**: "I never knew the true face of Mount Lu, for I was standing inside the mountain" — you can't recognize this "mountain" because you're stuck inside the QR viewpoint. The real flag `always stay in here` pairs with the trap flag `never gonna give you up`.

## Flag

```
npusec{A1w4Ys_5tAy_1n_H3rE}
```

---

## 1. Layer 1: the surface QR (hint layer)

The visible QR scans identically from 8 orientations (rotations/mirrors/transposes):

```
这里似乎什么也没有呢... 4oCc5L6n4oCd55yL5oiQ5bKt4oCc5qiq4oCd5oiQ5bOw
```

base64 → **"viewed SIDEWAYS a ridge, a peak seen HORIZONTALLY"** — the poet's 横/侧 were **deliberately swapped**. Meaning: the whole challenge is about swapping your viewpoint (and it warns you the direction is inverted).

![Fig 1: surface QR scan result](images/1_surface_scan.png)
<!-- screenshot slot 1: scan result + the swapped-idiom decode -->

## 2. Layer 2: alpha-channel stego

R/G/B carry the surface code; the **alpha channel holds only values 254/255** (1/255 contrast, invisible).

- 255→1, 254→0; sample the center 6×6 of each 12×12 pixel block → **222×222 hidden bitmap**
- 222 = 6×37 → **6×6 grid of 37×37 modules**
- Checkerboard layout: **even blocks (r+c)%2==0 are real QR fragments** (18, each scannable); **odd blocks are something else** (18)

![Fig 2: alpha layer 6×6 overview](images/2_alpha_overview.png)
<!-- screenshot slot 2: alpha bitmap render, even=QR / odd=checker texture -->

## 3. Layer 3: even blocks = rickroll trap

18 even blocks scan to 2-3 chars each; **column order** yields 42 chars:

```
oaO1oTyyr04mqwAlKmxjoz40K0pkqxIsrGO1K3IjsD
```

ROT13 → base64:

```
npulie{N3v3r_90nn4_G1vE_y0u_up}    ← FAKE flag (npulie ≠ npusec)
```

Its real function: **weld the solver's mindset to QR + column order + ROT13 + b64.**

## 4. Layer 4: odd blocks = inverted Han Xin Code (the twist)

### 4.1 Where QR analysis dies

The odd blocks are checkerboard-textured with ~175 deviations each. As QR V5:
- 8 masks × 2 polarities × 4 ECC levels: every RS decode fails
- The 15-bit format info matches no valid BCH code
- Sparse inter-block diffs (156/1369) prove there is no QR-style EC structure

### 4.2 The break: change symbology

**37×37 is simultaneously QR Version 5 AND Han Xin Code Version 8:**

```
QR     : 17 + 4×5  = 37
Hanxin : 21 + 2×8  = 37
```

After bitwise inversion, all 18 odd blocks match the Han Xin V8 finder/alignment/fixed-area templates with **zero mismatch**:

| Parameter | Value |
|---|---:|
| Version | 8 (37×37) |
| ECC level | 1 |
| Mask | 1 |
| Polarity | inverted |
| Codewords | 117 = 99 data + 18 RS |
| RS field | GF(256), poly 0x163 |

Recognition cue: **finder patterns in all four corners** (QR has three), asymmetric for orientation.

![Fig 3: odd block vs Han Xin V8 template](images/3_hanxin_match.png)
<!-- screenshot slot 3: inverted odd block vs V8 fixed-area template overlay -->

### 4.3 Why the earlier "checkerboard XOR" looked like data

Han Xin mask 1 flips where (r+c) is even; the checkerboard baseline is 1 where (r+c) is **odd** — exact complement:

```
raw ⊕ checker_odd = (canonical ⊕ 1) ⊕ checker_odd = canonical ⊕ mask1
```

So the "flip template" was **already the unmasked Han Xin matrix** in the data area — only the symbology switch and Han Xin's own codeword interleaving (13-way picket-fence) were missing.

### 4.4 Decoding

Each block encodes exactly **2 characters in Text mode (2×6bit = 12 info bits)**:

- Unmask → row-major module read → 936 bits = 117 codewords
- Inverse picket-fence → 99 data + 18 RS bytes (all 18 blocks pass RS check)
- Text header `0010` + 6bit + 6bit char + `111111` terminator

Results per block: Iw 81 tm / oa qm Km / r0 qR px / O1 EM Sh / Rk S5 I9 / p2 p1 K0

**Same column order as the trap layer:**

```
oaO1p2Iwr0RkqmEMp181qRS5KmShK0tmpxI9
```

ROT13 → base64:

```
npusec{A1w4Ys_5tAy_1n_H3rE}
```

![Fig 4: solver output](images/4_solve_output.png)
<!-- screenshot slot 4: hanxin solver output with 18 char pairs + flag -->

## 5. Mathematical confirmation (the "156 private bits" illusion)

Earlier analysis treated raster diffs of 18 blocks as 156 private bits with "C-struct bitfield" artifacts. The truth:

- Each block's plaintext is only 12 bits (2 chars), diffused by RS(117,99) + picket-fence + module placement
- Measured: varying-module support = **156**, and after centering, GF(2) rank = **12** — exactly a 12-dimensional linear diffusion, not independent ciphertext
- Artifacts like byte19 always landing in ASCII 'H'–'O' and nine "random bytes" are coincidences of slicing a linear subspace along raster byte boundaries

## 6. Full chain

```
surface QR (swapped-idiom hint)
  → alpha{254,255} stego → 222×222 → 6×6 of 37×37
  → even: QR fragments → column order → ROT13+b64 → npulie{...} fake flag (mindset anchor)
  → odd: inverted Han Xin V8/ECC1/mask1 → 2 chars × 18 blocks
  → column order → ROT13+b64 (same pipeline as the trap)
  → npusec{A1w4Ys_5tAy_1n_H3rE}
```

## 7. Lessons

1. **The size collision is the soul of this challenge**: 37×37 is legal in two symbologies; the even blocks weld your eyes to QR
2. When a "QR" fails even format-info BCH, suspect the **symbology itself** before sweeping masks/polarities/ECC parameters
3. "Each a different view from far, high, low" doesn't mean rotating the image — it means **climbing a different mountain** (switching code systems)
4. Toolchain: Han Xin Code spec = ISO/IEC 20830; open-source reference = zint (`hanxin.c`)

## 8. Collaboration note

Solved jointly: boss + Claude (layer extraction / trap breaking / structural analysis / rank-12 cross-verification) and an external AI (Han Xin recognition breakthrough). Final solver `不识庐山_hanxin_solver.py` lives in the challenge folder; the GF(2) rank verification script is in `_work/不识庐山/`.
