# htb-university-2025_Optimistic

![img](../../assets/banner.png)

<img src='../../assets/htb.png' style='zoom: 80%;' align=left /><font size='5'>Optimistic</font>

​	4<sup>th</sup> November 2025

​	Prepared By: `rasti`

​	Challenge Author: `rasti`

​	Difficulty: <font color=lightgreen>Very Easy</font>

​	Classification: Official

# Synopsis

- `Optimistic` is a very easy crypto challenge in which the players have to break Nihilist cipher with known keyword and known key length using frequency analysis techniques.

# Description

- In Tinselwick's first magical mishap, Lottie Thimblewhisk discovers a strange peppermint-coded message whose enchanted structure hides something far more important. This challenge explores a festive ciphering mechanism and a seasonally wrapped secret. Your first step in uncovering what happened to the wandering Starshard.



## Skills Required

- Basic knowledge of the Polybius Square cipher
- Familiar with frequency analysis techniques

## Skills Learned

- Learn how to break the Nihilist cipher with known key length and known keyword
- Learn practical intermediate-level difficulty frequency analysis

# Enumeration

In this challenge we are provided with two files:

- `source.py` : The main script that contains the challenge's logic and produces `output.txt`
- `output.txt` : The output file containing the data required to solve the challenge

## Analyzing the source code

Let us take a look at the source script. Starting from the end of the script, we can see that our goal is to decrypt the flag:

```python
from secret import STARSHARD_SCROLL, FESTIVE_WHISPER_CLEAN, PEPPERMINT_KEYWORD
# REDACTED
COCOA_AES_KEY = hashlib.sha256(FESTIVE_WHISPER_CLEAN.encode()).digest()
WRAPPED_STARSHARD = AES.new(COCOA_AES_KEY, AES.MODE_ECB).encrypt(pad(STARSHARD_SCROLL, 16)).hex()
open('output.txt', 'w').write(f'{PEPPERMINT_KEYWORD = }\n{PEPPERMINT_CIPHERTEXT = }\n{WRAPPED_STARSHARD = }')
```

Decrypting `WRAPPED_STARSHARD` is equivalent to recovering the AES key `COCOA_AES_KEY` which is equivalent to recovering `FESTIVE_WHISPER_CLEAN`.

`FESTIVE_WHISPER_CLEAN` is the plaintext encrypted with `STARSTREAM_KEY` that produced `PEPPERMINT_CIPHERTEXT`:

```python 
CANDYCANE_ALPHABET = string.ascii_uppercase + string.digits
SZ = 6
L = SZ**2
STARSTREAM_KEY = ''.join(random.sample(CANDYCANE_ALPHABET, k=L))
PEPPERMINT_CIPHERTEXT = swirl_encrypt(STARSTREAM_KEY, FESTIVE_WHISPER_CLEAN)
```

Therefore, we can obtain `FESTIVE_WHISPER_CLEAN` by recovering the encryption key `STARSTREAM_KEY` and then decrypting `PEPPERMINT_CIPHERTEXT` with it.

The encryption key is a random 36-character long string consisting only of uppercase letters and the digits 0-9.

Let us first analyze `weave_peppermint_square`:

```python
def weave_peppermint_square():
    peppermint_square_flat = CANDYCANE_ALPHABET
    for c in PEPPERMINT_KEYWORD:
        peppermint_square_flat = peppermint_square_flat.replace(c, '')
    peppermint_square_flat = PEPPERMINT_KEYWORD + peppermint_square_flat
    return [list(peppermint_square_flat[i:i+SZ]) for i in range(0, len(peppermint_square_flat), SZ)]

peppermint_square = weave_peppermint_square()

BAUBLE_COORDS = {
    peppermint_square[i][j]: f'{i+1}{j+1}'
    for j in range(SZ)
    for i in range(SZ)
}
```

This basically constructs a Polybius square using the known keyword `AR4ND0MK3Y`, as follows:

<table style='text-align:center'><tr><td>A</td><td>R</td><td>4</td><td>N</td><td>D</td><td>0</td></tr><tr><td>M</td><td>K</td><td>3</td><td>Y</td><td>B</td><td>C</td></tr><tr><td>E</td><td>F</td><td>G</td><td>H</td><td>I</td><td>J</td></tr><tr><td>L</td><td>O</td><td>P</td><td>Q</td><td>S</td><td>T</td></tr><tr><td>U</td><td>V</td><td>W</td><td>X</td><td>Z</td><td>1</td></tr><tr><td>2</td><td>5</td><td>6</td><td>7</td><td>8</td><td>9</td></tr></table>

and `BAUBLE_COORDS` is a map of each letter with its corresponding row and column. More specifically, it looks like:

```python
{'A': '11', 'M': '21', 'E': '31', 'L': '41', 'U': '51', '2': '61', 'R': '12', 'K': '22', 'F': '32', 'O': '42', 'V': '52', '5': '62', '4': '13', '3': '23', 'G': '33', 'P': '43', 'W': '53', '6': '63', 'N': '14', 'Y': '24', 'H': '34', 'Q': '44', 'X': '54', '7': '64', 'D': '15', 'B': '25', 'I': '35', 'S': '45', 'Z': '55', '8': '65', '0': '16', 'C': '26', 'J': '36', 'T': '46', '1': '56', '9': '66'}
```

 Now let us analyze how the plaintext is actually encrypted:

```python
FESTIVE_WHISPER_CLEAN = re.sub(r'[^a-zA-Z0-9]', '', FESTIVE_WHISPER_CLEAN).upper()

def swirl_encrypt(starstream_key, starlit_plaintext):
    twinkling_ct = []
    for i in range(len(starlit_plaintext)):
        key_off = int(BAUBLE_COORDS[starstream_key[i % len(starstream_key)]])
        pt_off = int(BAUBLE_COORDS[starlit_plaintext[i]])
        twinkling_ct.append(key_off + pt_off)
    return twinkling_ct
```

The plaintext is initially converted to uppercase and sanitized to contain only ascii letters and digits.

Then, the coordinates of each plaintext character and the corresponding key character are added and appended to the ciphertext array. Notice that the key repeats after $i$ reaches $36$. Algebraically, the encryption can be represented as the integer addition:

$C_i = P_i + K_{i \pmod {36}}$

We need a way to recover the encryption key, or even the plaintext directly, from the ciphertext.

# Solution

## Finding the vulnerability

The vulnerability lies on the following observations:

1. The key is short and it repeats every $36$ characters.
2. The plaintext is most likely English text and based on the ciphertext length, it is quite large.
3. English texts are susceptible to frequency analysis techniques as some letters appear significantly more often than others.

In fact, the solution idea is similar to how the Vigenere cipher with known key length is broken. Since the key length is known, we know that the same key character is used to encrypt every 36<sup>th</sup> character. Thus:

$$K_1 &\rightarrow& P_{1,37,73,109,\ \dots}\\K_2 &\rightarrow& P_{2,38,74,110,\ \dots}\\ &\vdots& \\ K_{36} &\rightarrow& P_{36,72,108,144,\ \dots}$$

Now we need to think of a way to remove candidates for each key character. We know for sure that plaintext characters are in the map `BAUBLE_COORDS`. Let us create a reverse mapping for convenience:

```python
OFFSETS = BAUBLE_COORDS
REV_OFFSETS = {int(v): k for k, v in OFFSETS.items()}
```

Let us start with the first encrypted character and find all the candidate keys that produce a plaintext that is a valid element of the 6x6 square.

```python
for k in ALPH:
		key_off = OFFSETS[k]
		p = PEPPERMINT_CIPHERTEXT[0] - int(key_off)
		if p in REV_OFFSETS:
				print(REV_OFFSETS[p])
```

The following candidates are printed: `H, S, C, B, Y, T, 0, D, N, J, Q`. These are still too many to be able to bruteforce them for each key position so we need to eliminate more candidates. This is where frequency analysis comes in.

We need to work with the entire group of characters encrypted with the first key character and decrypt it using all these possible key characters. Based on the fact that English texts follow a specific distribution in their letter frequency, we can score the plaintext that contains the most common letters, with a high value. In the end, hopefully, the $K_i$ that produces a plaintext with a big score, should be the correct $K_i$.

This can be achieved with the following code:

```python
def score(text):
    return sum(text.count(ch) for ch in 'ETAOINSHRDLU')

def obtain_key_character(g):
    encrypted_group = PEPPERMINT_CIPHERTEXT[g::L]
    results = {}
    for k in ALPH:
        key_candidate = OFFSETS[k]
        candidate_plaintext = []
        for c in encrypted_group:
            p = c - key_candidate
            if p in REV_OFFSETS:
                candidate_plaintext.append(REV_OFFSETS[p])
        results[k] = score(''.join(candidate_plaintext))
    # sort the dictionary in descending order so that the first element is the letter with the biggest score
    results = {k: v for k, v in sorted(results.items(), key=lambda item: item[1], reverse=True)}
    return list(results.keys())[0]
```

Then the entire key can be obtained by:

```python
KEY = ''.join(obtain_key_character(i) for i in range(L))
```

Finally, we can construct a method that decrypts a ciphertext with a provided key and decrypt `PEPPERMINT_CIPHERTEXT` with the recovered key.

```python
def decrypt(key, ciphertext):
    pt = ''
    for i, c in enumerate(ciphertext):
        key_off = int(OFFSETS[key[i % len(key)]])
        pt += REV_OFFSETS[abs(c - key_off)]
    return pt

PLAINTEXT = decrypt(KEY, PEPPERMINT_CIPHERTEXT)
```

Now, we can get the flag by following the exact same operations as the challenge script.

```python
import hashlib
from Crypto.Cipher import AES
from Crypto.Util.Padding import unpad

AES_KEY = hashlib.sha256(PLAINTEXT.encode()).digest()
flag = AES.new(AES_KEY, AES.MODE_ECB).decrypt(bytes.fromhex(WRAPPED_STARSHARD))
print(unpad(flag, 16).decode())
```



## Getting the flag

A final summary of all that was said above:

1. We notice that we are provided with a Polybius square cipher and we know the key length and the keyword used to construct the square.
2. Notice that the key is short which makes the cipher vulnerable.
3. Apply frequency analysis techniques to recover the encryption key and decrypt the ciphertext.
4. Knowing the plaintext, construct the aes key and decrypt the encrypted flag.
