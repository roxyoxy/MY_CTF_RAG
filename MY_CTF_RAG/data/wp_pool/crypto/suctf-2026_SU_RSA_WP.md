# suctf-2026_SU_RSA__WP

# SUCTF2026 - SU_RSA - WriteUp

## WriteUp

+ 本题参考的是这篇论文，考点其实是三元Coppersmith与连分数，但是界没卡好导致二元Coppersmith就能做。
+ 参考的论文：[2025-1281.pdf](https://eprint.iacr.org/2025/1281.pdf)

+ 本题的题型是参考论文中的如下定理，这里就简单描述一下定理以及思路：

>**定理6**：
>
>+ 设$N=pq$，是一个RSA模数，并满足$q<p<2q$，设公钥指数会$e=N^{\alpha}$，并满足RSA密钥等式$ed-k(p-1)(q-1)=1$。
>+ 设$\frac{p_{r-1}}{q_{r-1}},\frac{p_r}{q_r}$为$\frac{e}{N}$的两个连分数逼近，并且是满足以下条件的最大分母$q_{r-1}<q_r<\frac{N^{\frac{3}{4}}}{\sqrt{e}}$，同时满足$gcd(p_r,e)=1$，假设存在整数$|u|,|v|<N^{\delta}$，使得$d=uq_r+vq_{r-1}$，并且$k=up_r+vp_{r-1}$，此外，设$S$是$p+q$的一个近似值，并满足$|p+q-S|<N^{\gamma}$
>+ 则只要满足$\delta<\frac{1}{6}\alpha -\frac{1}{2}\gamma+\frac{1}{4}$，就可以在多项式时间内被分解。

+ 攻击的构造如下：

  + 设$\frac{p_{r-1}}{q_{r-1}}$和$\frac{p_r}{q_r}$是$\frac{e}{N}$的两个相邻的渐进连分数，满足$q_{r-1}<q_r<\frac{N^{\frac{3}{4}}}{\sqrt{e}}$，并且满足$gcd(e,p_r)=1$
  + 并且我们假设：$ed-k(p-1)(q-1)=1$，其中$d=uq_r+vq_{r-1}$以及$k=up_r+vp_{r-1}$，以及$p+q=S+w$，其中$S$已知，$w$未知
  + 此时`RSA`的密钥方程就可以重新写成如下形式：

  $$
  \begin{array}{l}
  ed-k(p-1)(q-1)=1\\
  \Leftrightarrow \\
  e(uq_r+vq_{r-1})-(up_r+vp_{r-1})(N+1-S-w)=1
  \end{array}
  $$

  + 将这个等式的右边乘法展开，并且通过适当的移项就可以写成如下形式：

  $$
  p_rwu+p_{r-1}wv-(N+1-S)p_ru-(N+1-S)p_{r-1}-eq_{r-1}v-1\equiv 0~mod(~eq_r)
  $$

  + 此时将同余式的两边同时乘以$p_r^{-1}~mod(~eq_r)$，这就可以推导出如下的一个多项式：

  $$
  wu+a_1wv+a_2u+a_3v+a_4\equiv 0~mod(~eq_r)\\
  
  \begin{array}{l}
  其中:\\
  a_1\equiv p_{r-1}p_r^{-1}~mod(~eq_r)\\
  a_2\equiv -(N+1-S)~mod(~eq_r)\\
  a_3\equiv -((N+1-S)p_{r-1}-eq_{r-1})p^{-1}_r~mod(~eq_r)\\
  a_4\equiv -p_r^{-1}~mod(~eq_r)
  \end{array}
  $$

  + 此时多项式：$f(x,y,z)=xy+a_1xz+a_2y+a_3z+a_4$，其三元组$(x,y,z)=(w,u,v)$是同余式$f(x,y,z)\equiv 0~mod(~eq_r)$
  + 这样就可以构造出一个模多项式，使用三元Coppersmith攻击求解小根。
  + 接下来就可以构造多项式系数矩阵，通过格基规约的方法求解出小系数的模多项式，通过求解这个小系数的多项式从而得到$(w,u,v)$

+ 这里同时给出`RSA加密`小私钥攻击`d`的一些方法与比较：

| 攻击方法               | 私钥的上界$d<N^{\delta}$   | 额外泄露信息              |
| ---------------------- | -------------------------- | ------------------------- |
| 经典维纳攻击           | $\delta=0.25-\epsilon$     | 无额外信息                |
| 拓展维纳攻击           | $\delta=0.25-\epsilon$     | 无额外信息                |
| Boneh & Durfee攻击     | $\delta=0.2847-\epsilon$   | 无额外信息                |
| Boneh & Durfee子格攻击 | $\delta = 0.2929-\epsilon$ | 无额外信息                |
| Blömer & May 攻击      | $\delta = 0.2899-\epsilon$ | 无额外信息                |
| CFL攻击                | $\delta >0.3$              | 需要泄露$S=p+q$或其高低位 |

+ exp如下，可以在这个github仓库中看到`https://github.com/MengceZheng/RSA_CFL/tree/main`。但是其实用这个脚本的攻击效率还没有使用二元copper的效率快（应该是界没把握好的原因吧），这里就贴个二元copper的代码。
+ 当时生成数据用github仓库攻击可能大概需要3分钟，而二元copper试了一下，大概可以秒出`QAQ`。

```python
from sage.all import *
from Crypto.Util.number import long_to_bytes
N = 92365041570462372694496496651667282908316053786471083312533551094859358939662811192309357413068144836081960414672809769129814451275108424713386238306177182140825824252259184919841474891970355752207481543452578432953022195722010812705782306205731767157651271014273754883051030386962308159187190936437331002989
e = 11633089755359155730032854124284730740460545725089199775211869030086463048569466235700655506823303064222805939489197357035944885122664953614035988089509444102297006881388753631007277010431324677648173190960390699105090653811124088765949042560547808833065231166764686483281256406724066581962151811900972309623
S = 19240297841264250428793286039359194954582584333143975177275208231751442091402057804865382456405620130960721382582620473853285822817245042321797974264381440
c = 49076508879433623834318443639845805924702010367241415781597554940403049101497178045621761451552507006243991929325463399667338925714447188113564536460416310188762062899293650186455723696904179965363708611266517356567118662976228548528309585295570466538477670197066337800061504038617109642090869630694149973251

A = N - S + 1
X = 2**338
Y = 2**399
U = X * Y + 1
M = 3
T = 1

PR = PolynomialRing(ZZ, names=("u", "x", "y"))
u, x, y = PR.gens()
Q = PR.quotient(x * y + 1 - u)
f = Q(1 + x * (A + y)).lift()

PR2 = PolynomialRing(ZZ, names=("w", "z"))
w, z = PR2.gens()

shifts = sorted((x**i) * (e ** (M - k)) * (f**k) for k in range(M + 1) for i in range(M - k + 1))
monomials = sorted({mon for poly in shifts for mon in poly.monomials()})


for j in range(1, T + 1):
    for k in range(floor(M / T) * j, M + 1):
        shifts.append(Q((y**j) * (f**k) * (e ** (M - k))).lift())
        monomials.append((u**k) * (y**j))


basis = Matrix(ZZ, len(monomials))
for i, poly in enumerate(shifts):
    basis[i, 0] = poly(0, 0, 0)
    for j in range(1, i + 1):
        mon = monomials[j]
        if mon in poly.monomials():
            basis[i, j] = poly.monomial_coefficient(mon) * mon(U, X, Y)

basis = basis.LLL()
polys = []
for row in basis.rows()[:12]:
    poly = PR(0)
    for coeff, mon in zip(row, monomials):
        if coeff:
            poly += ZZ(coeff // mon(U, X, Y)) * mon
    if poly:
        polys.append(poly)

flag = None
sub_map = {u: w * z + 1, x: w, y: z}


for i in range(len(polys)):
    if flag is not None:
        break
    for j in range(i + 1, len(polys)):
        g1 = PR2(polys[i].subs(sub_map))
        g2 = PR2(polys[j].subs(sub_map))
        if g1.is_constant() or g2.is_constant():
            continue
        res = g1.resultant(g2, z)
        if res == 0 or res.is_constant():
            continue

        roots_w = [ZZ(root) for root, _ in res.univariate_polynomial().change_ring(QQ).roots() if root in ZZ]
        for kw in roots_w:
            if abs(kw) >= X:
                continue
            roots_z = [ZZ(root) for root, _ in g1(w=kw).univariate_polynomial().change_ring(QQ).roots() if root in ZZ]

            for yz in roots_z:
                if abs(yz) >= Y or (1 + kw * (A + yz)) % e != 0:
                    continue
                phi = A + yz
                d = (1 + kw * phi) // e
                s = N - phi + 1
                delta = s * s - 4 * N
                sq = isqrt(delta)
                if sq * sq != delta:
                    continue
                p = (s + sq) // 2
                q = (s - sq) // 2
                print(f"p = {p}")
                print(f"q = {q}")
                if p * q == N:
                    flag = long_to_bytes(pow(c, d, N))
                    break

            if flag is not None:
                break

    if flag is not None:
        break

if flag is None:
    raise ValueError("failed to recover the flag")
print(flag.decode())
# SUCTF{congratulation_you_know_small_d_with_hint_factor}
```

