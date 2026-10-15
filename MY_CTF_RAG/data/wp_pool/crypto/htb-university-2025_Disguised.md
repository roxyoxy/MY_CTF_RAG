# htb university 2025 Disguised

![img](../../assets/banner.png)

<img src='../../assets/htb.png' style='zoom: 80%;' align=left /><font size=5>Disguised</font>

​	8<sup>th</sup> November 2025

​	Prepared By: `rasti`

​	Challenge Author(s): `rasti`

​	Difficulty: <font color=orange>Medium</font>

# Synopsis

- `Disguised` is a medium crypto challenge in which the players have to break 1-round AES using two plaintext/ciphertext pairs.


# Description

- At the Frostgate of Tinselwick, a protective hearth-protocol grants enchanted tokens to its visitors. Its wintry cipher and shifting session marks guard the final Starshard scroll—but the system's snowy protections may not be as sturdy as they appear. This is the last step in unraveling the Starshard's disguised retreat.



## Skills Required

- Good knowledge of the AES internals
- Good familiriazation with Python source code auditing
- Know how to identify cryptosystems from cryptographic constants such as S-boxes.
- Good knowledge of PKCS#7 padding
- Know about graph traversal algorithms, such as DFS. 

## Skills Learned

- Learn how to break 1-round AES
- Learn how to setup custom exploitation methods
- Understand why non-linearity is mandatory in AES
- Understand in practice why 1-round AES is vulnerable


# Enumeration

In this challenge, we are provided with:

- `server.py` : The main script that runs when we connect to the challenge instance.
- `utils.py` : A python script that contains some helper functions and components used by the cryptosystem in `server.py`

# Source code analysis

Let us start by analyzing `server.py`. The goal is clearly to login as admin and get the flag:

```python
def login(self, uid, username, token):
    snowprint = hashlib.shake_256(self.cipher.KEY + username + str(uid).encode()).digest(64)
    real_token = self.cipher.encrypt(json.dumps({"s": snowprint.hex(), "i": uid}).encode())
    if real_token == token:
        if uid == self.admin_uid and username == b'TinselwickAdmin':
            return { "success": f"Warm greetings, admin! Your Starshard scroll: {open('flag.txt').read()}" }
        else:
            return { "success": f"Welcome {username.decode()}! No Starshard scrolls for you today." }
    else:
        return { "error": "Frostbitten session token. Identity unclear." }
```

The `login` method accepts the id and the username of the user that we are trying to login as and their encrypted token. Given the user id and the username (and the value of a secret key), the real encrypted token is reconstructed and verified against the provided encrypted token. If the token is verified and the username is `TinselwickAdmin` and the provided user id is `0`, we get the flag.

We know the target username and the target user id, the only thing that remains unknown is the secret key. The HearthProtoctol class is initialized with an instance of the SnowCipher which receives a random 32-byte string.

```python
class HearthProtocol:
    def __init__(self, cipher):
        self.cipher = cipher
        self.admin_uid = 0
        self.uid = 1

class SnowCipher:
    def __init__(self, key):
        self.KEY = key

prot = HearthProtocol(SnowCipher(os.urandom(32)))
```

The goal is to determine this random key. The protocol contains another method as well, namely `register`:

```python
def register(self, username):
    if self.uid >= 32:
        return { "error": "The Hearth Registry is full for now... try again after the solstice." }
    snowprint = hashlib.shake_256(self.cipher.KEY + username + str(self.uid).encode()).digest(64)
    token = self.cipher.encrypt(json.dumps({"s": snowprint.hex(), "i": self.uid}).encode())
    self.uid += 1
    return { "token": token.hex() }
```

The protocol allows us to register at most $32$ users. This method is essentially an encryption oracle that returns an encrypted token created by SnowCipher based on the provided username and an auto-increment user id. We will analyze cipher's internals in a while.

# Solution

To begin, let us analyze the encryption routine of SnowCipher.

```python
def encrypt_block(self, block):
    rk = _ek(self.KEY)
    s = b2m(block)
    s = self._ark(s, rk[0])
    s = self._sb(s)
    s = self._sr(s)
    s = self._mc(s)
    s = self._ark(s, rk[1])
    return m2b(s)
    
  def encrypt(self, m):
      plaintext = pad(m, 16)
      return b''.join(self.encrypt_block(plaintext[i:i+16]) for i in range(0, len(plaintext), 16))
```

This looks like a block cipher that encrypts data in ECB mode; that is, each 16-byte block is encrypted independently and appended to the final ciphertext.

The internal methods starting with an underscore look like:

```python
def _sb(self, s, b=B):
    return [[b[val] for val in row] for row in s]

def _sr(self, s):
    res = [s[i].copy() for i in range(4)]
    res[0][1], res[1][1], res[2][1], res[3][1] = res[1][1], res[2][1], res[3][1], res[0][1]
    res[0][2], res[1][2], res[2][2], res[3][2] = res[2][2], res[3][2], res[0][2], res[1][2]
    res[0][3], res[1][3], res[2][3], res[3][3] = res[3][3], res[0][3], res[1][3], res[2][3]
    return res

def _mc(self, s):
    return [_msc(s[i].copy()) for i in range(4)]

def _ark(self, s, k):
    return [[t[0] ^ t[1] for t in list(zip(row[0], row[1]))] for row in zip(s, k)]
```

A good eye will observe that these operations involve XOR, substitution and permutation which hints that SnowCipher is most likely an SPN block cipher. Looking at `utils.py`, we notice the S-Box.

```python
B = (
    0x63, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5, 0x30, 0x01, 0x67, 0x2B, 0xFE, 0xD7, 0xAB, 0x76,
    0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0, 0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0,
  	# REDACTED
)
```

When we deal with a seemingly unknown and/or custom cipher, it is usually a good practice to research the constants online as most authors in real life do not try inventing their own crypto. From the first few results, we conclude that this is specifically the AES S-Box.

One can analyze the rest components, i.e. shift rows, sub bytes, expand key, mix columns, only to figure out that this is basically a modified version of AES. The modification is on the number of AES rounds. AES-256 normally uses 14 rounds but in this challenge there is just a single round. Therefore, our goal boils down to breaking 1-round AES or equivalently, recovering the two round keys $k_0, k_1$. Since the master key is 32 bytes long, the round keys $k_0, k_1$ are 16 bytes each.

The basic attack idea is described perfectly at this stackexchange answer [^1]. Let us express the plaintext encryption $E$ in terms of $k_0, k_1, M$ and the ciphertext $E(M)$.

$$C = E(M) = MC(SR(SB(M \oplus k_0))) \oplus k_1 \quad\quad\quad\quad(1)$$

Note that all operations are performed in $4 \times 4$ matrices.

It is well known that the only non-linear AES component is sub bytes (SB). Omitting that, we would be able to express the ciphertext as a linear equation in terms of the message and the secret key and having one plaintext/ciphertext pair, it would be enough to recover the key using linear algebra. We can examine the S-Box's non-linearity using SageMath:

```python
sage: from sage.crypto.sbox import SBox
sage: S = SBox(0x63, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5, 0x30, 0x01, 0x67, 0x2B, 0xFE, 0xD7, 0xAB, 0x76,
....:     0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0, 0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0,
					 # REDACTED
....:     0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42, 0x68, 0x41, 0x99, 0x2D, 0x0F, 0xB0, 0x54, 0xBB, 0x16)
sage: S.linear_structures()
[]
sage: P.<y0,y1,y2,y3,y4,y5,y6,y7,x0,x1,x2,x3,x4,x5,x6,x7> = PolynomialRing(GF(2), 16, order='lex')
sage: S.polynomials([x0,x1,x2,x3,x4,x5,x6,x7], [y0,y1,y2,y3,y4,y5,y6,y7], groebner=True)
[y0 + x0*x1*x2*x3*x4*x5*x7 + x0*x1*x2*x3*x4*x6*x7 + x0*x1*x2*x3*x4*x6 + x0*x1*x2*x3*x4 + x0*x1*x2*x3*x5*x7 + x0*x1*x2*x3*x6 + x0*x1*x2*x3 + x0*x1*x2*x4*x5*x7 + x0*x1*x2*x4*x6*x7 + x0*x1*x2*x4*x6 + x0*x1*x2*x4*x7 + x0*x1*x2*x5*x6 + x0*x1*x2*x5 + x0*x1*x2*x6*x7 + x0*x1*x2*x6 + x0*x1*x3*x4*x5*x7 + x0*x1*x3*x4*x6*x7 + x0*x1*x3*x4*x7 + x0*x1*x3*x5*x6 + x0*x1*x3*x5 + x0*x1*x3*x6 + x0*x1*x3*x7 + x0*x1*x3 + x0*x1*x4*x5*x6*x7 + x0*x1*x4*x6 + x0*x1*x4*x7 + x0*x1*x5*x6*x7 + x0*x1*x5 + x0*x1*x7 + x0*x2*x3*x4*x5*x7 + x0*x2*x3*x4*x6*x7 + x0*x2*x3*x5*x6*x7 + x0*x2*x3*x6*x7 + x0*x2*x3*x6 + x0*x2*x3 + x0*x2*x4*x7 + x0*x2*x4 + x0*x2*x5*x6*x7 + x0*x2 + x0*x3*x4*x5*x7 + x0*x3*x4*x5 + x0*x3*x4*x6*x7 + x0*x3*x4*x6 + x0*x3*x4*x7 + x0*x3*x5*x7 + x0*x3*x6*x7 + x0*x3*x6 + x0*x4*x5*x6 + x0*x4*x5*x7 + x0*x4*x7 + x0*x5*x6*x7 + x0*x5*x7 + x0*x6 + x0*x7 + x0 + x1*x2*x3*x4*x5*x7 + x1*x2*x3*x4*x5 + x1*x2*x3*x4*x7 + x1*x2*x3*x4 + x1*x2*x3*x5*x6 + x1*x2*x3*x6*x7 + x1*x2*x3 + x1*x2*x4*x5*x6 + x1*x2*x4*x7 + x1*x2*x5 + x1*x2*x7 + x1*x3*x4*x5*x7 + x1*x3*x4*x7 + x1*x3*x5*x6*x7 + x1*x3*x5*x6 + x1*x3*x5*x7 + x1*x3*x5 + x1*x3 + x1*x4*x5*x6*x7 + x1*x4*x5*x7 + x1*x4*x6*x7 + x1*x4*x6 + x1*x4*x7 + x1*x5*x6 + x1*x5 + x1*x6*x7 + x1*x7 + x2*x3*x4*x5*x7 + x2*x3*x4*x6*x7 + x2*x3*x4 + x2*x3*x5*x6 + x2*x3*x7 + x2*x4*x5*x6 + x2*x4*x5*x7 + x2*x4*x5 + x2*x4*x6 + x2*x4*x7 + x2*x4 + x2*x5*x6*x7 + x2*x5*x7 + x2*x6*x7 + x2 + x3*x4*x5*x6*x7 + x3*x4*x5*x6 + x3*x4*x6*x7 + x3*x5*x6*x7 + x3*x5 + x3*x6*x7 + x3 + x4*x5*x6*x7 + x4*x5*x6 + x4*x5*x7 + x5*x6 + x5*x7 + x5,
 y1 + x0*x1*x2*x3*x4*x6*x7 + x0*x1*x2*x3*x4*x6 + x0*x1*x2*x3*x4 + x0*x1*x2*x3*x5*x6*x7 + x0*x1*x2*x3*x6 + x0*x1*x2*x3*x7 + x0*x1*x2*x3 + x0*x1*x2*x4*x5 + x0*x1*x2*x4*x6*x7 + x0*x1*x2*x5*x6*x7 + x0*x1*x2*x5*x6 + x0*x1*x2*x5*x7 + x0*x1*x2 + x0*x1*x3*x4*x5 + x0*x1*x3*x4*x6*x7 + x0*x1*x3*x4*x7 + x0*x1*x3*x5*x6*x7 + x0*x1*x3*x5*x7 + x0*x1*x3*x6 + x0*x1*x3*x7 + x0*x1*x4*x6*x7 + x0*x1*x4 + x0*x1*x6*x7 + x0*x1*x6 + x0*x2*x3*x4*x6*x7 + x0*x2*x3*x4*x6 + x0*x2*x3*x5*x6*x7 + x0*x2*x3*x5*x7 + x0*x2*x3*x6*x7 + x0*x2*x3*x6 + x0*x2*x4*x5*x6 + x0*x2*x4*x5 + x0*x2*x4*x6 + x0*x2*x4 + x0*x2*x5*x7 + x0*x2*x7 + x0*x2 + x0*x3*x4*x5*x6*x7 + x0*x3*x4*x5*x6 + x0*x3*x4*x6 + x0*x3*x4*x7 + x0*x3*x5*x6 + x0*x3*x5 + x0*x3*x6 + x0*x3*x7 + x0*x4*x5*x6*x7 + x0*x4*x5*x7 + x0*x4*x5 + x0*x4*x6*x7 + x0*x4 + x0*x5*x6 + x0*x6 + x0*x7 + x1*x2*x3*x4*x6*x7 + x1*x2*x3*x4*x6 + x1*x2*x3*x5*x6*x7 + x1*x2*x3*x5*x6 + x1*x2*x3 + x1*x2*x4*x5*x6*x7 + x1*x2*x4*x5*x7 + x1*x2*x4*x5 + x1*x2*x6 + x1*x2*x7 + x1*x3*x4*x5 + x1*x3*x4*x6 + x1*x3*x4*x7 + x1*x3*x4 + x1*x3*x5*x7, ...
# REDACTED
```

We can see that the first output bit of the S-Box is derived from a large non-linear relation in terms of the input bits. It would be linear if the relation consisted only of addition and no multiplication (for example, $x_0 + x_3 + x_5 + 1$). In other words, in the linear case, the s-box could be represented as $$y = Ax + B$$, where:

- $A$ a constant $8 \times 8$ matrix
- $b$ a constant vector of dimension $8$
- $x$ the input bit vector $(x_0, x_1, x_2, x_3, x_4, x_5, x_6, x_7)$.

But now, in the non-linear case, it is expressed as $$y = Ax^{-1} + B$$. You can find the exact $A, b$ in the corresponding Wikipedia article [^2].

This blocks us from expressing the AES ciphertext in terms of the key and the plaintext so there is no way to define $SB^{-1}$ efficiently without knowledge of the secret key. By applying the inverse MixColumns operation in both sides of $(1)$, we get:

$$MC^{-1}(C) = SR(SB(M \oplus k_0)) \oplus MC^{-1}(k_1)$$

then, by applying the inverse ShiftRows operation in both sides of $(1)$, we get:

$$SR^{-1}(MC^{-1}(C)) = SB(M \oplus k_0) \oplus SR^{-1}(MC^{-1}(k_1))$$

For ease of notation, we set $C' = SR^{-1}(MC^{-1}(C))$ and $k'_1 = SR^{-1}(MC^{-1}(k_1))$, thus:

$$C' = SB(M \oplus k_0) \oplus k'_1 \quad\quad\quad\quad(2)$$

Now, the idea is that given at least two plaintext/ciphertext pairs, we can bruteforce a single byte of $k_0, k_1$ for each of their $16$ total bytes. We know we found a candidate for either $k_0$ or $k_1$ if the equation $(2)$ holds. It turns out that:

Given three plaintext/ciphertext pairs or more, there is a single candidate byte for each of the 16 positions which determines the round keys immediately.

Given two plaintext/ciphertext pairs, there are at most $3$ candidates per position so some additional bruteforcing might be required to get the secret key.

Given one plaintext/ciphertext pair, there are several candidates per position which makes additional bruteforce practically infeasible to recover the secret key.

To solve the challenge, we would need at least $2$ such pairs. Let us see how we can obtain them.

The plaintext token is: `M = "{'s': shake_256(KEY || username || uid).hex(), 'i': uid} || P"` for some PKCS#7 padding $P$.

For every encryption, the uid is incremented by $1$ and also `KEY` is unknown so we treat the shake256 hashes as unknowns, say $H_1$ for $\text{uid} = 1$, $H_2$ for $\text{uid} = 2$ and so on. It is important to notice that all hashes have a static size equal to $32$ bytes. Let us create a dummy token and see if we are able to get any two known plaintext blocks.

```python
>>> import json, os
>>> from Crypto.Util.Padding import pad
>>> dummy_token = json.dumps({'s': os.urandom(32).hex(), 'i': 0}).encode()
>>> dummy_token = pad(dummy_token, 16)
>>> [dummy_token[i:i+16] for i in range(0, len(dummy_token), 16)]
[b'{"s": "2da2000a2', b'e76bb3d31be67a32', b'1e4dec2671d9f2d4', b'4350fd6962416f36', b'de1328d", "i": 0', b'}\x0f\x0f\x0f\x0f\x0f\x0f\x0f\x0f\x0f\x0f\x0f\x0f\x0f\x0f\x0f']
```

The unknown 32-byte hash cascades to almost the entire plaintext so it does not help. The last block, however, does not contain any part of the secret hash; it consists of the closing curly bracket and the PKCS#7 padding, which are both known. Here we go then, we found our first known plaintext:

```python
b'}\x0f\x0f\x0f\x0f\x0f\x0f\x0f\x0f\x0f\x0f\x0f\x0f\x0f\x0f\x0f'
```

Now let us create another dummy token with a user id $\geq 10$:

```python
>>> dummy_token = json.dumps({'s': os.urandom(32).hex(), 'i': 10}).encode()
>>> dummy_token = pad(dummy_token, 16)
>>> [dummy_token[i:i+16] for i in range(0, len(dummy_token), 16)]
[b'{"s": "44337d05b', b'd26dbe3f5f535ad1', b'e312f25b45ba170e', b'e57eb9d1a9a5a079', b'3675823", "i": 1', b'0}\x0e\x0e\x0e\x0e\x0e\x0e\x0e\x0e\x0e\x0e\x0e\x0e\x0e\x0e']
```

Again, the last block is known as it consists of the known padding, the closing curly bracket and the last digit of the uid, which are all known. It looks like we found our second known plaintext:

```python
b'0}\x0e\x0e\x0e\x0e\x0e\x0e\x0e\x0e\x0e\x0e\x0e\x0e\x0e\x0e'
```

To get a third known plaintext block, we would have to register $\geq 100$ users which is not the case as the limit is $32$. Therefore we will run our attack using two plaintext/ciphertext pairs.

# Exploitation

Let us start with the boilerplate code for connecting to the challenge instance.

```python
import sys
from pwn import remote, process

if len(sys.argv) > 1:
    host, port = sys.argv[1].split(':')
    io = remote(host, port)
else:
    io = process(['python3.12', '../challenge/server.py'])
```

Then, we hardcode our known plaintexts. 

```python
PT = [
    b'}'  + b'\x0f'*0x0f,
    b'0}' + b'\x0e'*0x0e
]
```

Then, we have to register a user to make use of $\text{uid} = 1$, then skip the next $8$ UIDs and finally register another user to make use of $\text{uid} = 10$. Remember that we care only about the last ciphertext block as it corresponds to the plaintexts in the `PT` array.

```python
import json

def register(username):
    io.sendlineafter(b'> ', b'1')
    io.sendlineafter(b': ', username.encode())
    resp = json.loads(io.recvline())
    if 'error' in resp:
        raise Exception(f'Error in registering: {resp}')
    return resp['token']

CT = []
ct0 = bytes.fromhex(register('r4sti'))
for i in range(8):
    bytes.fromhex(register('test'))
CT.append(ct0[-16:])
CT.append(bytes.fromhex(register('r4sti'))[-16:])
```

Then, we compute $C'$ as:

```python
def bytes2matrix(text):
    return [list(text[i:i+4]) for i in range(0, len(text), 4)]

def matrix2bytes(matrix):
    return bytes([b for r in matrix for b in r])

def inv_shift_rows(s):
  	# standard implementantion of InvShiftRows
    pass

def inv_mix_columns(s):
  	# standard implementation of InvMixColumns
    pass

CP = [matrix2bytes(inv_shift_rows(inv_mix_columns(bytes2matrix(ct)))) for ct in CT]
```

The next step is the most crucial. We need to find the candidates for each key byte and for this purpose, we will utilise a dictionary. We know we found a candidate if $(2)$ holds.

```python
# for each key position
for i in range(16):
  	# for each byte at k_0[i]
    for b1 in range(256):
      	# for each byte at k'_1[i]
        for b2 in range(256):
            for pt, cp in zip(PT, CP):
                if b2 != cp[i] ^ s_box[pt[i] ^ b1]:
                    break
            else:
                candidates[i].append((b1, b2))

print(candidates)
```

Attention! `b2` is a candidate byte for $k'_1$ and not for $k_1$. Check $(2)$ to understand why.

The candidates dictionary should look like:

```python
{0: [(6, 215), (75, 243)], 1: [(128, 9), (242, 46)], 2: [(184, 66), (185, 165)], 3: [(84, 194), (85, 69)], 4: [(168, 26), (169, 98)], 5: [(40, 20), (41, 47)], 6: [(236, 11), (237, 130)], 7: [(152, 50), (153, 42)], 8: [(230, 122), (231, 255)], 9: [(46, 30), (47, 84)], 10: [(52, 203), (53, 169)], 11: [(128, 161), (129, 203)], 12: [(122, 241), (123, 254)], 13: [(104, 69), (105, 243)], 14: [(106, 135), (107, 137)], 15: [(246, 221), (247, 5)]}
```

which is translated as:

*At index $0$, the candidate bytes for $k_0$ are $6$ and $75$ and for $k'_1$ are $215$ and $243$ respectively. At index $1$, ...*

Finally, given this dictionary, we need a way to determine the correct key. This can be converted into a graph problem which can be solved with DFS.

![](assets/graph.png)

Let us write a recursive python method for traversing this graph:

```python
def traverse(i, k0, kp1):
    if len(k0) == 16:
        k0 = bytes(k0)
        # compute k_1 from k'_1
        k1 = matrix2bytes(mix_columns(shift_rows(bytes2matrix(bytes(kp1)))))
        TEST_KEY = k0 + k1
        if PT[0] == decrypt(TEST_KEY, CT[0]):
            try:
                pt0 = decrypt(TEST_KEY, ct0)
                json.loads(unpad(pt0, 16))
            except:
                return

            found_key(TEST_KEY)
        return

    for cand in candidates[i]:
        k0_cand = cand[0]
        kp1_cand = cand[1]
        traverse(i+1, k0+[k0_cand], kp1+[kp1_cand])

traverse(0, [], [])
```

Having computed $k_1$ from $k'_1$, `TEST_KEY` is our current guess for the 32-byte master key. We try decrypting an entire ciphertext with this and it it decrypts properly and is also a valid json object, we jump to `found_key`.

```python
import hashlib

def login(uid, username, token):
    io.sendlineafter(b'> ', b'2')
    io.sendlineafter(b': ', str(uid).encode())
    io.sendlineafter(b': ', username.encode())
    io.sendlineafter(b': ', token.hex().encode())
    resp = json.loads(io.recvline().strip())
    if 'error' in resp:
        raise Exception(f'Error in registering: {resp}')
    return resp['success']

def found_key(key):
    print('FOUND KEY:', key.hex())
    session_hash = hashlib.shake_256(key + b'TinselwickAdmin' + b'0').digest(64)
    token = encrypt(key, json.dumps({"s": session_hash.hex(), "i": 0}).encode())
    print(login(0, 'TinselwickAdmin', token))
    exit()
```

If a valid json object is found, it means that we found the correct secret key and we are able to login as admin.

### Getting the flag

The steps to get the flag can be summarized as follows:

1. Notice that the goal is to login as admin to get the flag.
2. Analyzing the SPN block cipher and its S-Box, we conclude that it is the AES S-Box.
3. SnowCipher is essentially a 1-round variant of the AES cipher.
4. Our goal is to recover the two round keys $k_0, k_1$.
5. We represent the SnowCipher encryption algebraically in terms of the round keys.
6. Due to the S-Box's non-linearity, we are not able to compute $SB^{-1}$ so we leave it as is.
7. We obtain two plaintext/ciphertext pairs from the register oracle due to the known padding block and the json format.
8. We use these pairs to find candidates for each byte position of the round keys $k_0, k_1$.
9. Run DFS on the candidates to derive the secret key.
10. Construct a valid token and login as admin with $\text{uid} = 0$.

[^1]: https://crypto.stackexchange.com/a/80754
[^2]: https://en.wikipedia.org/wiki/Rijndael_S-box#Forward_S-box





