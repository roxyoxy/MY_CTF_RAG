# ZA WARUDO (咋瓦鲁多) · Writeup (English)

- **Challenge**: 砸瓦鲁多 (MISC · ZA WARUDO, 500 pts)
- **Category**: MISC · Audio Steganography
- **Attachment**: `challenge.flac` (3'33")
- **Keywords**: Bilibili cache forensics · original-track diff · complex-bin cancellation · BFSK · 0xAA55 preamble · Paradox Engine
- **Final flag**: `npusec{wHat3v3r_1t_t4kes}`

---

## Challenge Text

> ZA WARUDO
> 500 pts
> Time will tell us the answer.
>
> The original track seems to come from a video on Bilibili.

---

## Solution

This was a two-phase campaign: **Phase 1 "spot the difference"** (locate the original track + pin down the injected band) and **Phase 2 "separate and decode"** (complex-domain cancellation + preamble cracking). The wall sat exactly at the phase boundary — which is also the most instructive part of this challenge.

### Step 0: The Meme Is the Signpost — ZA WARUDO = Time Stop

"ZA WARUDO" is DIO's time-stop incantation from JoJo; the tagline "time will tell us the answer" puns on the same theme. Following the Bilibili hint, the original track was identified: **Stranded In Space** (Bilibili avid `435860352`, from the Red Alert 2: Mental Omega "Music Omega" series) — the BGM associated with the **Paradox Engine**, whose signature ability is literally *Time Freeze*. The theme loops perfectly — and plants the first trap: **it primes you to suspect time-stretching / reversal first**.

### Step 1: Bilibili Cache Forensics — Digging the Original from Local Cache

No re-download needed: the Bilibili PC client cache lives locally with a clean layout:

```
C:\Users\<user>\Videos\bilibili\435860352\
├── videoInfo.json              ← metadata (title confirms Stranded In Space)
├── 435860352-1-30280.m4s       ← audio stream (30280 = Bilibili audio ID)
└── 435860352-1-30016.m4s       ← video stream
```

An .m4s is just a standard fMP4 with a few junk bytes prepended — slice at the `ftyp` box and you have a valid m4a:

```python
data = open('435860352-1-30280.m4s','rb').read()
i = data.find(b'ftyp')            # i = 13 here
open('orig.m4a','wb').write(data[i-4:])
```

Converted to 48 kHz PCM (`orig.wav`, 3'33.85") and compared with `challenge.flac` (3'33.48") — durations nearly identical, so the "time-stretch" theory is already shaky, but hard evidence is still required.

### Step 2: Ruling Out Timeline Tampering — "Time" Told No Lies

Cross-correlating the low-frequency music body (100–3000 Hz bandpassed): the offset measured at the beginning / middle / end is **identical everywhere (4055 samples ≈ 84.5 ms, constant)**. Time-stretching would show linear drift; splices would show jumps. Neither — the timeline is clean apart from a constant 84.5 ms lead.

> Reverse playback (`reversed.flac`) and the FLAC-embedded cover art (`cover.png`, rotations/upscales) were also checked — dead ends. By elimination, the difference must live in the frequency domain.

### Step 3: Spot the Difference — Pinning the 4.4–7.6 kHz Injection Band (End of Phase 1)

Per-beat (beat ≈ 1.6678 s) FFT comparison: the low-frequency body matches bin-for-bin, but **a set of narrowband tones absent from the original pops out in the high band**, candidates in 4.4–7.6 kHz:

```
Injection candidates (8): 4719, 4907, 5040, 5491, 6047, 6199, 7003, 7200 Hz
```

The gross structure also emerged — later proven to be the most valuable half-way product:

- **Phase 1 (t ≈ 0.3–53.4 s)**: sparse; group onsets **strictly every 3.3333 s, 16 groups total**
- **Phase 2 (t ≥ 53.4 s to end)**: dense data section, all 8 candidates active

### Step 4: The Wall and the Breakthrough — Amplitude-Domain Trap, Complex-Domain Exit

**Post-mortem of the wall**: applying the dual condition ("challenge envelope > 5× noise-floor median AND > 3× original envelope") to capture challenge-only events on all 8 candidates, the Phase 1 event distribution came out as —

```
6199 Hz × 7    7003 Hz × 24    7200 Hz × 7
```

**A real preamble should show two tones appearing 8 times each, regularly** — the loudest candidate, 7003 Hz (24 events, randomly clumped), is in fact an artifact of the AAC↔FLAC codec difference. In the amplitude domain, codec artifacts and injected signals are indistinguishable: `orig` was decoded from AAC 317 kbps, `challenge` is FLAC — quantization noise and phase offsets of the two chains all masquerade as signal under magnitude comparison.

**The breakthrough**: move to **complex-bin cancellation**. For each candidate frequency `f`, compute sliding complex DFT coefficients (100 ms Hann window, 5 ms hop) for both the challenge `C_f(k)` and the aligned original `O_f(k)`; since the two codec chains impose a fixed amplitude/phase difference, estimate a **complex transfer coefficient** `H_f`:

```
C_f(k) ≈ H_f · O_f(k)        H_f = ⟨O, C⟩ / ⟨O, O⟩
Residual R_f(k) = |C_f(k) − H_f · O_f(k)|
```

To keep genuine hidden pulses from contaminating the fit, each iteration keeps only the lowest-residual 65% of frames, for 8 rounds. Here: `H_6199 ≈ 0.917+0.169j`, `H_7200 ≈ 0.885+0.189j` — the music body collapses in the residual, while 50–60 ms short pulses at 6199/7200 Hz stand out cleanly. **Only these 2 of the 8 candidates survive in the residual; the other 6 are eliminated on the spot.**

### Step 5: The 0xAA55 Preamble — the Preamble Gifts You the Mapping

In Phase 1 (t < 53 s) only 16 pulses survive on the two surviving tones (one every 3.333 s); in time order:

```
7200 6199 7200 6199 7200 6199 7200 6199 | 6199 7200 6199 7200 6199 7200 6199 7200
```

Let `6199 Hz = 0`, `7200 Hz = 1`:

```
10101010 01010101  =  0xAA 0x55
```

`0xAA55` — the most classic frame-sync word in digital communications (alternating pattern aids clock recovery; the AA55 pair brackets byte alignment). The preamble not only marks where data begins, it **hands over the tone→bit mapping for free** — no guessing required.

### Step 6: 200 bits → 25 bytes → flag

Phase 2 (t ≥ 53 s; first pulse 53.660 s, last 168.275 s) yields **exactly 200 data pulses** (200 = 8 × 25 — a clean multiple is itself evidence of correct decoding). Sorted by time, MSB-first, 8 bits per byte:

```
01101110 01110000 01110101 01110011 01100101  →  n p u s e c
01100011 01111011 ...                          →  c { ...
```

```text
npusec{wHat3v3r_1t_t4kes}
```

"Whatever it takes" — an Avengers closer; the flag is 25 characters = 25 bytes, matching the pulse count exactly.

---

## Flow Diagram

```
challenge.flac ──┐
                 ├─ ① cross-correlation alignment (shift=4055, constant) → no time-stretch/splice
Bilibili m4s ────┤      ↓
  └─ orig.m4a/wav├─ ② per-beat FFT diff → 8 injection candidates in 4.4–7.6 kHz
                 │      ↓
                 ├─ ③ complex-bin cancellation (iterative H fit) → artifacts die, 6199/7200 survive
                 │      ↓
                 ├─ ④ Phase1 16 pulses → 10101010 01010101 = 0xAA55 preamble (free mapping)
                 │      ↓
                 └─ ⑤ Phase2 200 pulses → BFSK bitstream → 25 ASCII bytes
                        → npusec{wHat3v3r_1t_t4kes}
```

---

## Knowledge Notes

1. **Bilibili cache forensics**: `Videos\bilibili\<avid>\` holds the audio as `<avid>-1-30280.m4s`; an m4s = junk header + standard fMP4, slice before `ftyp` to get an m4a. No need to hunt for re-downloads
2. **Time-stretch detection**: segmented cross-correlation — constant offset = clean, linear drift = time-stretch, jumps = splices. For "time will tell" style prompts, run a timeline health check before anything else
3. **Complex-bin cancellation**: the correct way to diff across codecs (AAC vs FLAC) — sample-level alignment + per-bin complex DFT + iterative complex transfer-coefficient fit + residual detection. Amplitude-domain diffs are guaranteed to be polluted by codec artifacts
4. **0xAA55 preamble**: a classic frame header in communications protocols. Treat any regular sparse pulse section as a preamble first — it solves "where does data start" and "how do tones map to bits" in one shot
5. **BFSK**: two single tones encoding 0/1 is the simplest and most common audio digital modulation; decoding = per-bin energy/residual pulse stream → sort by time → bits
6. **Self-consistency checks**: 200 pulses = 8×25 bytes, flag length 25 chars, prefix `npusec{` with closing `}` — decoding correctness needs no external oracle, the structure speaks for itself

---

## Methodology Retrospective: How to Attack Audio-Diff Challenges Next Time

Phase 1 went smoothly (locate the original → rule out time-stretching → pin the injection band), then everything **stalled at "candidate validation"** — 8 candidate tones were detected, an "8-bit parallel byte stream" assumption got anchored immediately, and a lot of time burned on grid fitting and codeword dictionaries. The retrospective condenses into a standard operating procedure for audio diff challenges:

1. **Align first, diff second** — sample-level cross-correlation for the exact shift is the precondition for everything
2. **Cross-codec diffs must live in the complex domain** — in the amplitude domain, codec artifacts and injected signals are inseparable; one complex coefficient H absorbs the whole amplitude/phase difference, leaving only true signal in the residual
3. **Validate candidate regularity before any coding inference** — true signals are regular in time (equally spaced, clean event counts); artifacts clump randomly. Count events per candidate separately; 30 seconds prunes 8 candidates down to 2
4. **Regular sparse section = preamble first** — don't hunt for coding patterns in intra-group intervals (those are often intervals between artifacts, i.e. patterns in noise); a preamble (AA55/55AA etc.) hands you the mapping table for free
5. **Prefer the plainest explanation** — "N candidate tones ≠ N parallel bits"; serial 2-FSK is far more common than an 8-bit chord; symbol width is a hypothesis, not a fact — validate before refining

In one line: **a detector's output is a list of suspects, not convicts — every candidate must pass the regularity check before entering decoding inference.**

---

## Pitfall Log

- **The time-stop meme is a smokescreen**: the narrative (ZA WARUDO / time freeze) steers attention toward time-stretch and reversal, but the timeline is perfectly clean — the meme confirms the original track's identity (Paradox Engine), it is not a hint about the technique
- **Amplitude-domain false positives**: the AAC↔FLAC codec difference fabricated stable "challenge-has, original-hasn't" illusions on 6 tones (7003 Hz: 24 fake events vs 8 real ones per true tone); even dual thresholds (noise floor ×5 + original envelope ×3) were not enough — complex-domain cancellation is the only reliable cure
- **Artifact intervals are not a code**: Phase 1 intra-group intervals showed a tidy pattern {a, b, a+b, a+2b} that consumed significant time as a "rhythm code" — in hindsight those were intervals between false-positive events, pure noise patterns

---

## 📸 Reproduction & Screenshot Slots (to fill after replay)

> Save screenshots to `WP_EN\MISC\images\`, named `NN_description.png`. One-shot replay: from the challenge folder, `python solve.py challenge.flac _work\orig.wav -o out` (requires `numpy scipy soundfile`); ready-made artifacts `events_overview.png` (BFSK pulse timeline), `run.log`, `flag.txt` live in `xiaosai\题目\MISC\咋瓦鲁多\`.

| # | Content | Path | Status |
|:---:|:---|:---|:---:|
| 1 | Bilibili cache folder + videoInfo.json title + m4s→m4a extraction code & output | `images/01_cache_extract.png` | ⬜ |
| 2 | Cross-correlation alignment output: shift = 4055 samples (84.479 ms), constant | `images/02_alignment.png` | ⬜ |
| 3 | Original vs challenge spectrograms side by side, 4.4–7.6 kHz (injected tones visible) | `images/03_hiband_diff.png` | ⬜ |
| 4 | Complex-cancellation residual: artifact bins vanish, 6199/7200 pulses survive | `images/04_complex_cancel.png` | ⬜ |
| 5 | solve.py terminal output (preamble 0xaa55 + 200 bits + flag line) | `images/05_solve_run.png` | ⬜ |
| 6 | events_overview.png full pulse timeline + flag.txt close-up | `images/06_final_flag.png` | ⬜ |

When filling in, embed `![01](images/01_cache_extract.png)` at the matching step.
