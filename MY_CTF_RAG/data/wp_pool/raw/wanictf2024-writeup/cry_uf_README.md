# cry uf README

---
title: "uf"
level: 5
flag: "FLAG{hope_this_chal_is_not_automatically_solved_by_AI_c14ef1732e87a6c}"
writer: "Laika"
---

# uf

## 問題文
🙄

## 解法
まずは、どのようにフラグが暗号化されているかを追ってみます。
encrypt()は $i$ が256以上・未満で $X$ に対する操作が変わります。
簡単のために一度 `if i >= n // 2` の中の操作を無視します。
すると、このfor文は `m * randbits(n)` と同等の操作になります。

次に、if文の中も考慮してみます。
ここまでであれば単にGCDをとれば $m$ が求まりましたが、ここに `x ^= randbits(1)` が加わると、 $X$ の下位256bitは完全にランダムになります。

一度ここまでの操作を整理して数式にすると、与えられた $X_i$ は512 bitの整数 $r_i$ と256 bitの整数 $s_i$ を用いて、

$$
X_i = mr_i + s_i 
$$

と表せます。

このような $X_i$ が与えられているときに $m$ を求める問題はApproximate GCD Problem [^agcd] と呼ばれます。 
[^agcd]: Approximate GCDとかApproximate Common Divisor Problem(ACDP)とかの名称を見かけます。オリジナルはACDPっぽい。
N. Howgrave-Graham. Approximate integer common divisors. in J. Silverman (ed), Cryptography and Lattices, Springer LNCS2146 (2001) 51−66.

これは $s_i$ が比較的小さい場合には、格子基底簡約を用いて $m$ が求まる可能性があります。
具体的には以下のような格子を構成します。　

$$
\begin{pmatrix}
K &  X_1 &  X_2 &  X_3 \\\\
0 & -X_0 &    0 &    0 \\\\
0 &    0 & -X_0 &    0 \\\\ 
0 &    0 &    0 & -X_0 
\end{pmatrix}
$$

今回は $K=1$ でもOKです。
これをLLLなどで基底簡約すると出現するベクトルの中に

$$
(r_0 K, r_0 s_1 - r_1 s_0, \cdots, r_0 s_3 - r_3 s_0)
$$

のようなベクトルが出現します。
$r_0 K$ の値から $r_0$ を求め、さらに $s_0$, $m$ を求めることでフラグが入手できます。

### 補足
格子基底簡約を用いた暗号システムへの攻撃は高難易度のCTFでは頻繁に出題されます。
今回はApproximate GCD Problemという典型パターンのうちの1つをそのまま適用できる形で出題し、初めて格子基底簡約を用いた攻撃に取り組んだ人でも解きやすい問題を目指しました。
他にもHidden Number Problemをはじめとした格子基底簡約を用いた攻撃の典型パターンがあるので、解いてみてください。


## Solution
First, let's follow how the flag is encrypted. 
The `encrypt()` function changes its operation on $X$ depending on whether $i$ is greater than or less than 256. 
For simplicity, let's ignore the operation inside the `if i >= n // 2`. 
Thus, this for loop performs the same operation as `m * randbits(n)`.

Next, let's consider the if statement. Up to this point, simply taking the GCD would give us $m$, but with the addition of `x ^= randbits(1)`, the lower 256 bits of $X$ become completely random.

If we organize the operations so far into an equation, the given $X_i$ can be expressed as:

$$
x_i = mr_i + s_i 
$$

using a 512-bit integer $r_i$ and a 256-bit integer $s_i$.

The problem of finding $m$ given such $X_i$ is called the Approximate GCD Problem. [^agcd_en]
[^agcd_en]: Also known as Approximate Common Divisor Problem (ACDP). The original seems to be ACDP.
N. Howgrave-Graham. Approximate integer common divisors. in J. Silverman (ed), Cryptography and Lattices, Springer LNCS2146 (2001) 51−66.

When $s_i$ is relatively small, it is possible to find $m$ using lattice basis reduction. Specifically, we construct a lattice as follows:

$$
\begin{pmatrix}
K &  X_1 &  X_2 &  X_3 \\\\
0 & -X_0 &    0 &    0 \\\\
0 &    0 & -X_0 &    0 \\\\ 
0 &    0 &    0 & -X_0 
\end{pmatrix}
$$

In this case, $K=1$ is fine. By performing lattice basis reduction using LLL or similar algorithms, a vector such as

$$
(r_0 K, r_0 s_1 - r_1 s_0, \cdots, r_0 s_3 - r_3 s_0)
$$

will appear. 
By finding $r_0$ from the value of $r_0 K$, and subsequently finding $s_0$ and $m$, the flag can be obtained.

Attacks on cryptosystems using lattice basis reduction frequently appear in high-level CTF challenges. 
This problem aims to be approachable for beginners tackling lattice basis reduction attacks for the first time by directly applying a typical pattern of the Approximate GCD Problem. 
There are other typical patterns of lattice basis reduction attacks, such as the Hidden Number Problem, so please try solving those as well.

### Note
Attacks on cryptographic systems using lattice basis reduction are frequently featured in high-difficulty CTFs. 
This problem was presented in a form where the typical pattern of the Approximate GCD Problem could be directly applied, making it easier for those tackling lattice basis reduction attacks for the first time. 
There are other typical patterns of attacks using lattice basis reduction, such as the Hidden Number Problem, so please try solving them as well.

### Solver
```sage
import ast

from Crypto.Util.number import long_to_bytes

with open("output.txt") as f:
    X = ast.literal_eval(f.readline())

n = len(X)
A = matrix(ZZ, n, n)

K = 2^256
A[0, 0] = K
for i in range(1, n):
    A[0, i] = X[i]
    A[i, i] = -X[0]

A = A.LLL()
a = A[0]
r0 = a[0] // K
s0 = X[0] % r0
m = (X[0]-s0) // r0
m = abs(m)
print(long_to_bytes(m))
```





