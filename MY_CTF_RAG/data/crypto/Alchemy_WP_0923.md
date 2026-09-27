# [Campus CTF Crypto] Alchemy (炼丹) — Writeup

> **Category**: CRYPTO / classical cipher chain (Morse + ZipCrypto + homophone-hanzi obfuscation + Base64 + Caesar)
> **Attachments**: `key.png` (2560×1440 screenshot) + `Pandora's_box.zip` (encrypted, contains 60-byte `不可名状之物.txt` / "The Unnameable Thing")
> **Statement** (400 pts): "Crypto alchemist — thousand-year ginseng, wild royal jelly ... all melted into one furnace. Caesar, base64, obfuscation, braille — my classical cipher is complete."

## Flag

```
npusec{base64&decrypt_is_good!}
```

---

## 1. Recon

- `Pandora's_box.zip` is an **encrypted zip** (ZipCrypto, flag bit 0). One file inside: a 60-byte txt.
- `key.png` is a Notepad screenshot containing dots and dashes:

![Fig 1: Morse code in the key.png notepad](images/1_morse_screenshot.png)
<!-- screenshot slot 1: original key.png (notepad morse window) -->

```
-- --- .-. ... . .. ... .-- --- -. -.. . .-. ..-. ..- .-..
```

## 2. Morse opens the box (password: morseiswonderful)

```
-- --- .-. ... .  .. ...  .-- --- -. -.. . .-. ..-. ..- .-..
M  O    R    S   E   I  S    W   O   N   D  E   R  F   U  L
```

Uppercase fails; **lowercase `morseiswonderful`** unlocks the zip (always try both cases on classical-crypto passwords).

## 3. The Unnameable Thing — poisoned base64

The txt (44 chars, valid length 11×4 with `==` padding, but illegal symbols inside):

![Fig 2: zip unlocked and the unnameable txt](images/2_unlock_and_cipher.png)
<!-- screenshot slot 2: successful extraction + raw txt (note the rare hanzi) -->

```
@GpvbXl叄e叄Z懿bXk迩NCZ柶eXdsc迩puX迩NtX迩Fp@XghfQ==
```

Four rare hanzi (×8 total) plus `@` (×2) — the "obfuscation" layer from the statement.

### Homophone hanzi → digits

| Char | Pinyin | Sounds like | Replaced by |
|---|---|---|---|
| 懿 | yì | 一 (one) | `1` |
| 迩 | ěr | 二 (two) | `2` |
| 叄 | sān | 三 (three) | `3` |
| 柶 | sì | 四 (four) | `4` |

`@` → `a` (same keyboard key, shifted — a classic glyph confusion).

Clean base64:

```
aGpvbXl3e3Z1bXk2NCZ4eXdsc2puX2NtX2FpaXghfQ==
```

## 4. Base64 + Caesar finale

Decode:

```
hjomyw{vumy64&xywlsjn_cm_aiix!}
```

Looks like a flag but scrambled — final Caesar shift, **ROT-6** hits:

```
h jomyw { vumy64 & xywlsjn _ cm _ aii x ! }
  ↓ +6
npusec{base64&decrypt_is_good!}
```

(Sanity anchors: `vumy64`+6 = `base64`, `xywlsjn`+6 = `decrypt` — the plaintext names its own ciphers.)

![Fig 3: solve script output (substitute → b64 → ROT6)](images/3_solve_output.png)
<!-- screenshot slot 3: end-to-end Python verification, VERIFIED + flag -->

## 5. Full attack chain

```
key.png Morse → MORSEISWONDERFUL → lowercase = zip password
  → the Unnameable txt
  → de-obfuscate: 懿→1 迩→2 叄→3 柶→4, @→a
  → base64 decode
  → Caesar ROT-6
  → npusec{base64&decrypt_is_good!}
```

The statement's ingredient list (caesar, base64, obfuscation, braille) is the recipe; Morse is not on the list — it's the furnace door. No braille layer appeared in this solve path (likely the author lumped Morse in with dot-writing, or it decorates another path).

## 6. Tools & pitfalls

- Python `zipfile` with `pwd=b'morseiswonderful'` — no cracking needed
- Pitfall 1: password case (`MORSEISWONDERFUL` ✗, `morseiswonderful` ✓)
- Pitfall 2: read the txt as UTF-8 (60 bytes = 44 chars); a byte-level view hides the hanzi structure
- Pitfall 3: recovering `@`→`a` can be done by inverse verification — `@` leads two b64 groups; the decoded bytes are consistently off by 0x68, which back-solves the substitution
