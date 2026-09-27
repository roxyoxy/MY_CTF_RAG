# Yaho · Writeup (English)

- **Challenge**: 吼呀？呀吼！(Yaho)
- **Category**: Crypto · **Points**: 172 (dynamic)
- **Attachment**: `yaho.txt` (a ciphertext made of the characters "ya" and "hou" plus spaces)
- **Tags**: Morse Code · Substitution · Caesar Shift (dual-ring: letters + digits)
- **Flag**: `npusec{9e3d33c9-yaho-yaho-yaho-2ad9db0f101d}`

---

## Solution

### Step 1: Identify the cipher from the story

The description names the adventurer **"Morse"** — a direct hint at **Morse Code**.
The ciphertext uses exactly two Chinese characters, matching Morse's two symbols:

- `呀` (ya) → `.` (dot)
- `吼` (hou) → `-` (dash)
- single space = letter separator, double space = word separator

### Step 2: Mechanical substitution + lookup

Replace every `呀` with `.` and every `吼` with `-`, decode with a Morse table (46 groups, triple-verified):

```
3S7R77Q3_MOVC_MOVC_MOVC_6OR3RP4T545R MOVE LEFT 14
```

The second half is plaintext English: **MOVE LEFT 14**.

### Step 3: MOVE LEFT 14 = Caesar left 14 — letters AND digits, each in its own ring

The biggest trap of this challenge: the instruction applies to **every character**, not the classic letters-only Caesar:

- **Letters**: shift -14 mod 26 → `MOVC → YAHO` (matches the challenge title, direction confirmed)
- **Digits**: shift -14 mod 10 ≡ -4 → `3→9, 7→3, 6→2, 4→0, 5→1`

| Cipher | 3 | S | 7 | R | 7 | 7 | Q | 3 | | M | O | V | C | | 6 | O | R | 3 | R | P | 4 | T | 5 | 4 | 5 | R |
|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| Plain  | 9 | E | 3 | D | 3 | 3 | C | 9 | | Y | A | H | O | | 2 | A | D | 9 | D | B | 0 | F | 1 | 0 | 1 | D |

### Step 4: Final formatting per the flag hint

Platform format hint: **digits, lowercase letters, and `-`** —
- everything to **lowercase**
- separators are **dashes `-`** (the decoded Morse `..--.-` underscore means the author mis-encoded the `-` in their flag)

### Solve Script (full reproduction)

```python
morse = {'.-':'A','-...':'B','-.-.':'C','-..':'D','.':'E','..-.':'F','--.':'G',
         '....':'H','..':'I','.---':'J','-.-':'K','.-..':'L','--':'M','-.':'N',
         '---':'O','.--.':'P','--.-':'Q','.-.':'R','...':'S','-':'T','..-':'U',
         '...-':'V','.--':'W','-..-':'X','-.--':'Y','--..':'Z',
         '-----':'0','.----':'1','..---':'2','...--':'3','....-':'4','.....':'5',
         '-....':'6','--...':'7','---..':'8','----.':'9','..--.-':'_'}

raw = open('yaho.txt', encoding='utf-8').read().strip()
t = raw.replace('呀', '.').replace('吼', '-')
msg = ' '.join(''.join(morse.get(c, c) for c in w.split()) for w in t.split('  '))
print(msg)   # 3S7R77Q3_MOVC_MOVC_MOVC_6OR3RP4T545R MOVE LEFT 14

ct = msg.split(' MOVE')[0]
out = ''
for c in ct:
    if c.isalpha():                       # letter ring mod 26
        out += chr((ord(c) - 65 - 14) % 26 + 65)
    elif c.isdigit():                     # digit ring mod 10
        out += str((int(c) - 14) % 10)
    else:
        out += c
print('npusec{' + out.lower().replace('_', '-') + '}')
# npusec{9e3d33c9-yaho-yaho-yaho-2ad9db0f101d}
```

---

## Flag

```
npusec{9e3d33c9-yaho-yaho-yaho-2ad9db0f101d}
```

---

## Recap: Three Traps, One Field Manual

Two-layer nesting: **Chinese-character Morse (substitution) → self-instructed Caesar shift (dual-ring)**. Entry point: the character name "Morse"; the self-check for layer two is `MOVC→YAHO` matching the title. Field-tested traps:

1. **Hand-transcription error**: I hand-copied the decode output into the next step and duplicated one MOVC block — first submission rejected. Lesson: **pipe one step's output straight into the next; never hand-copy**.
2. **Experience bias**: assuming "classic Caesar shifts letters only" and submitting digits unchanged — here digits shift too (mod 10). Lesson: **follow the instruction literally**; "MOVE LEFT 14" means every character.
3. **Hidden format traps**: Morse decodes to `_`, but the flag uses `-` (author's encoding slip) + all lowercase — pure reasoning can't resolve this; the platform's format hint (digits/lowercase/`-`) closes it. Lesson: **when a flag won't take, suspect the format trio (case/separator/wrapper) and hunt for an official format hint**.

## 📸 Reproduction & Screenshot Slots

| # | Content spec | Path | Status |
|:---:|:---|:---|:---:|
| 01 | Platform challenge page (description + 172pts + format hint) | images/01_challenge_page.png | ⬜ |
| 02 | Solve script output (Morse decode + dual-ring shift) | images/02_script_output.png | ⬜ |
| 03 | Platform acceptance page (flag accepted) | images/03_accepted.png | ⬜ |
