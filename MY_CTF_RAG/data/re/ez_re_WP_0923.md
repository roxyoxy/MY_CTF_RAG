# ez_re · Writeup (English)

- **Challenge**: easy_re (ez——re)
- **Category**: RE · **Points**: 500
- **Attachments**: `encode.exe` (PE32+ x86-64, 212KB, MinGW-w64/GCC, 16 sections), `encrypted.txt` (32-byte ciphertext), `encrypted.zip` (bundle of the two, unencrypted)
- **Keywords**: IDA F5 · TEA variant · XOR self-inverse · OpenGL decoy
- **Final flag**: `npusec{cyb3r$3cur1ty_2O26_f1@g!}`

---

## Challenge Text (translated)

> easy_re / 500 pts
>
> The first time I visited the Louvre, I felt nothing special —
> because MY Mona Lisa, I had already met.
>
> Try getting to know IDA — let the reversing journey begin.
> If reading it forward is hard, try thinking in reverse.
> A frog's eyes see moving things best.
> Don't run files carelessly, or you'll have to download again.

---

## Solution

### Step 1: Static Recon — the OpenGL Pile Is a Smoke Bomb

Quick binary fingerprinting in Python:

- Compiler: MinGW-w64 (GCC); 16 sections is normal for MinGW debug sections, **not** packing
- Strings: a **huge OpenGL symbol pile** (`glad_` loader family, `glReadPixels`/`glStencilMask`/stencil series, `OpenGL ES` version strings) — looks like GPU-based encryption
- Filenames: `encrypted.txt`, ` encrypted.bin`, plus CRT `fopen/fread/fwrite`

The line "a frog's eyes see moving things best" eggs you into dynamic GPU tracing — **that's the bait**. The real work is pure integer math, plainly visible in `main`. Lesson: the scariest-looking API cluster in the strings window isn't necessarily the protagonist; read `main` before going dynamic.

### Step 2: IDA F5 — the Algorithm (TEA Variant + XOR)

Open `encode.exe` in IDA → F5 on `main` (the only function that matters):

```c
fp_in  = fopen("plainword.txt", "rb");
fp_out = fopen("encrypted.txt", "wb");
plainword = calloc(0x32, 1);
v3 = 0;
a1_12 = 405228090;          // state word 1 (TEA's v0 slot)
a1_8  = 620287023;          // state word 2
fread(plainword, 1, 0x20, fp_in);          // read 32-byte plaintext
for (i = 0; i <= 31; ++i) {                 // 32 rounds
    a1_12 += (16*a1_8 - 1668836417) ^ (v3 + a1_8) ^ ((a1_8 >> 5) - 1151594428);
    v3    -= 559038737;                     // delta (TEA does sum += delta)
    a1_8  -= (16*a1_12 + 803884916) ^ (v3 + a1_12) ^ ((a1_12 >> 5) - 1589449333);
}
for (i_0 = 0; i_0 <= 7; ++i_0)
    *(uint32*)&plainword[4*i_0] ^= a1_8;    // final a1_8 = keystream, XORed over 8 dwords
fwrite(plainword, 1, 0x20, fp_out);
```

Recognition points:

1. The shape `(16*x + K1) ^ (sum + x) ^ ((x>>5) + K2)` is the **TEA round function** in variant clothing (standard TEA uses `<<4`/`>>5` plus additive keys; here the keys are negative magic constants and the delta is subtracted)
2. **The loop never touches the plaintext** — 32 rounds only evolve internal state
3. The actual encryption = **XOR the 32-byte plaintext (as 8 dwords) with the single final state word `a1_8`** — one 32-bit keystream repeated 8 times

### Step 3: Think in Reverse — XOR Undoes XOR, Simulate and Decrypt

"If reading it forward is hard, think in reverse": there is no `decrypt` to find, because **XOR is its own inverse**. The keystream doesn't depend on the plaintext, so just re-run the 32-round state machine with the same constants to obtain `a1_8`, then XOR the ciphertext:

```python
import struct
M = 0xFFFFFFFF
a1_12, a1_8, v3 = 405228090, 620287023, 0
for _ in range(32):
    a1_12 = (a1_12 + (((16*a1_8 - 1668836417) & M) ^ ((v3 + a1_8) & M) ^ (((a1_8 >> 5) - 1151594428) & M))) & M
    v3    = (v3 - 559038737) & M
    a1_8  = (a1_8 - (((16*a1_12 + 803884916) & M) ^ ((v3 + a1_12) & M) ^ (((a1_12 >> 5) - 1589449333) & M))) & M
K = a1_8                                  # = 0x7a84a720

ct = bytes.fromhex(
    '4ed7f10945c4ff1959c5b7080494e70f'
    '5296f0037f95cb4816f8e24b60c0a507')    # encrypted.txt
pt = b''.join(struct.pack('<I', struct.unpack('<I', ct[4*i:4*i+4])[0] ^ K)
              for i in range(8))
print(pt)   # b'npusec{cyb3r$3cur1ty_2O26_f1@g!}'
```

**Flag: `npusec{cyb3r$3cur1ty_2O26_f1@g!}`** (exactly 32 bytes, matching the `fread` size of 0x20; leetspeak for "cyber security 2026 flag")

---

## Full Attack Chain

```
encode.exe
  ├─ string recon: OpenGL pile = frog bait (skip main and you'd chase the GPU)
  └─ IDA F5 main:
       ├─ 32-round TEA-variant state machine (subtractive delta, negative key constants)
       └─ final a1_8 as a 32-bit keystream ⊕ 8 plaintext dwords
            └─ Python replays the state machine → K=0x7a84a720 → ciphertext ⊕ K = flag
```

---

## Takeaways

1. **Recognizing the TEA family**: the three-part shape `(x<<4 ± k) ^ (sum + x) ^ ((x>>5) ± k)` screams TEA/XTEA/XXTEA variant; cosmetic changes to delta/constants don't change the identification
2. **Plaintext-independent keystream + XOR = offline inversion**: if the keystream never sees the plaintext, just replay the algorithm to recover it — no inverse circuit needed
3. **F5 before dynamic analysis**: 16 MinGW sections ≠ packing; an intimidating API string cluster (OpenGL/graphics) can be a smoke bomb — ten lines in `main` were the whole truth
4. **Every line of the challenge text maps to something**: the Mona Lisa poem = flavor, "think in reverse" = invert the operation instead of hunting a decrypt function, "frog eyes" = dynamic-analysis bait, "don't run files" = don't execute unknown PEs on your host
5. **Unsigned right shift on Windows**: IDA's uint32 `>>` is a logical shift; when porting to Python, mask every intermediate with `& 0xFFFFFFFF`

---

## Post-mortem Notes

- Nearly got dragged into GPU-rendering dynamic-analysis rabbit hole by the OpenGL strings (the "frog" line was egging it on) — three minutes of F5 on `main` ended the fight; **static first, dynamic as fallback**
- When porting C's wrapping 32-bit arithmetic, every add/sub/shift result needs `& 0xFFFFFFFF`; miss one and the keystream is garbage

---

## 📸 Reproduction & Screenshot Slots (to fill after replay)

> Save screenshots to `WP_EN\RE\images\`, named `NN_description.png`; replay flow: open encode.exe in IDA → follow this WP step by step.

| # | Content | Path | Status |
|:---:|:---|:---|:---:|
| 1 | IDA with encode.exe loaded, main located in function list (full UI) | `images/01_ida_loaded.png` | ⬜ |
| 2 | Full F5 pseudocode of `main` (32-round loop + XOR loop visible) | `images/02_ida_f5_main.png` | ⬜ |
| 3 | Static recon script output (keyword string hits + 16-section table) | `images/03_static_recon.png` | ⬜ |
| 4 | Decryptor output (`keystream K = 0x7a84a720` + `flag: b'npusec{...}'`) | `images/04_decrypt_output.png` | ⬜ |
| 5 | Platform acceptance of the flag | `images/05_accepted.png` | ⬜ |

When filling in, embed `![01](images/01_ida_loaded.png)` at the matching step.
