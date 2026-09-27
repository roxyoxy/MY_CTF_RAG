# [Campus CTF Crypto] 宇宙广播 (Cosmic Broadcast) — Writeup

> **Category**: CRYPTO / RSA (broadcast attack + FactorDB factorization + second decryption)
> **Attachment**: `youareawallfacer` (ASCII text: five integers n0/c0, n1/c1, n2/c2, nw)
> **Description**: "The star-chart coordinates ripple; the Bunker Era begins" (a Three-Body-Problem-themed line)
> **Theme**: In Liu Cixin's *The Dark Forest*, Wallfacer Luo Ji broadcasts the coordinates of star 187J3X1 ("the curse") to the cosmos to prove the Dark Forest theory; after the broadcast, Earth enters the Bunker Era. The filename "you are a wallfacer" casts the solver as Luo Ji.

## Flag

```
npusec{gOOdBYe187J3X1}
```

(goodbye 187J3X1 — the star whose coordinates were broadcast; the punchline of the whole puzzle)

---

## 1. Attachment Structure

```
n0 = 39370573027756593388734188422634...  (130 digits)
c0 = 20988249585280955924141496354682...
n1 = 25966112577544650951811614709295...
c1 = 39614339117341945820099120518392...
n2 = 37153493332423964681612162220814...
c2 = 27194368376011180544944338342322...
nw = 8718057580099298913498554193866676608085550852030166913524450865138089339848121631378052449058690339  (100 digits, the odd one out — w = wallfacer)
```

Three (n, c) pairs plus a lone nw. No e given — foreshadowing.

## 2. Layer 1: Håstad Broadcast Attack (e=3)

**The title says it literally**: the same plaintext m broadcast with e=3 to three moduli (n0, n1, n2).

Preconditions:
- `gcd(n_i, n_j)` pairwise coprime (all = 1, ruling out common-modulus/shared-factor routes)
- e=3 with 3 moduli → m³ < n0·n1·n2 holds, no CRT overflow

```python
from sympy.ntheory.modular import crt
import gmpy2
x, _ = crt([n0, n1, n2], [c0, c1, c2])   # CRT merge
m, exact = gmpy2.iroot(gmpy2.mpz(x), 3)  # integer cube root
print(exact)   # True — the attack is mathematically confirmed
```

This yields m = 42 bytes of **binary garbage** (332 bits). Not the flag — it's the next layer's ciphertext (note: m and nw are both 332-bit and m < nw; that size match is no coincidence).

> 📸 **[Screenshot slot 01]** save to `WP_cn\密码学\图片\01_广播攻击.png` (shared with the CN version)
> Content: broadcast-attack script output (make the `exact = True` line prominent; including the garbage bytes helps)

> Pitfall log: exact=True but garbage output ≠ failed attack. The "broadcast plaintext" is itself an intermediate key — a multi-layer RSA nesting design.

## 3. Layer 2: Factoring nw (The Wallfacer's Lock)

nw = 100 digits (332 bits), composite. Local three-strike all whiffed:

- `sympy.isprime(nw)` = False (composite confirmed)
- Fermat factorization: no hit in 200k rounds (factors unbalanced)
- No small factors under 100k (short-run Pollard rho hopeless)

**Escalate to FactorDB** (`factordb.com`, the CTF factorization first stop) — instant hit, status FF (fully factored):

```
nw = 6620414351<10> × 1316844704...89<91>
```

One small factor (6620414351) + one 91-digit prime. A 332-bit RSA modulus with a 34-bit factor = the classic "queryable/ECM-able" design; checking the online DB beats local grinding.

> 📸 **[Screenshot slot 02]** save to `WP_cn\密码学\图片\02_FactorDB分解.png` (shared)
> Content: the FactorDB result page (FF status + the `6620414351<10> · 1316844704...89<91>` factorization)

## 4. Layer 3: Decrypt m with nw's Private Key

```python
from math import gcd
p = 6620414351
q = nw // p
phi = (p - 1) * (q - 1)

for e in [3, 65537]:          # e unknown → try common values
    if gcd(e, phi) != 1: continue
    d = pow(e, -1, phi)
    plain = pow(m, d, nw)     # m = from the broadcast attack
    print(e, plain.to_bytes(29, 'big'))
```

- e=3: garbage (invertible since gcd=1, but output isn't text)
- **e=65537: `npusec{gOOdBYe187J3X1}`** ✅

> 📸 **[Screenshot slot 03]** save to `WP_cn\密码学\图片\03_解密出flag.png` (shared)
> Content: the terminal printing `npusec{gOOdBYe187J3X1}` on the e=65537 run (with the e=3 garbage for contrast if possible)

Full chain: three-way broadcast (e=3) → ciphertext m → nw's private key (e=65537) → flag.

## 5. Attack Chain Replay

```
(n0,c0)(n1,c1)(n2,c2)  +  nw
   │ CRT merge + integer cube root (Håstad e=3 broadcast attack)
   ▼ m (332-bit binary,恰好 < nw)
   │ FactorDB: nw = 6620414351 × 91-digit prime
   │ e unknown → try 3 ✗ / 65537 ✓
   ▼ pow(m, d, nw)
   ▼ npusec{gOOdBYe187J3X1}
```

## 6. Takeaways

1. **Håstad broadcast attack**: same plaintext, small exponent e, ≥e distinct coprime moduli → CRT-merge then take the integer e-th root. A title containing "broadcast" is the hint. `gmpy2.iroot`'s exact flag is a built-in validity check for the attack.
2. **Multi-layer nesting recognition**: a "garbage" plaintext whose bit-length matches another given modulus and is smaller than it → it's ciphertext under that modulus, not the finish line.
3. **FactorDB** is the CTF first stop for big-number factorization (`factordb.com`, URL query `?query=N`); when local Fermat/rho/small-factor sweeps come up empty, go online immediately. An RSA modulus carrying a small (<40-bit) factor is almost always a lookup/ECM-intended design.
4. **e not given**: try 3, then 65537, filtered by gcd(e, φ)=1 invertibility; the printable ratio of the decrypted bytes is a fast heuristic.
5. **Title intelligence**: variable suffix w=wallfacer, "Bunker Era" in the description, Three-Body memes throughout — CTF titles and variable names are free intelligence.

---

*Writeup by Claude Code · 2026-09-23 · Campus CTF practice, challenge #2 (Crypto)*
