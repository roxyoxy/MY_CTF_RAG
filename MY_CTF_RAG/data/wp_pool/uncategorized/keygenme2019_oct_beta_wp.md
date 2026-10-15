# keygenme2019_oct__beta_wp

# keygenme2019_oct__beta_wp
---

<!-- page 1 -->

keygenme2019_oct_beta 
 
题目难度：中等 
题目说明：离散对数(DLP) 综合题 
Flag： 
flag{C54-13A2FCFDDC8366059410A6A48D45A9BA-7E3} 
解题过程： 
分析: 
flag 格式如下flag{A-B-C} 
A，B，C 都是base16 的整数。 
 
题目有2 步计算： 
第1 步：  
是一个除模运算(DivMod) , 每次除以4 模N，迭代了256 次，结果就是除以4^256。 
记作  X = B/4^256  （mod N） ,   
N =  md5(“crackme_2019_oct_beta”) = 17678322bacd53942b76aa11e47518af 
这里要注意N 是一个4k+3 类型的奇数。 
取余有4 种可能： 
case (b%4): 0, 1, 2, 3 
b/4 mod n 分别等价于计算： x=b/4;  x =(b+n)/4; x=(b+2n)/4; x=(b+3*n)/4 
 
第2 步：是一个模64 bit 素数P 的离散对数， 验证： 
(2^A)^X = Y mod P,   
其中： 
P= 2^63 + 0x63  = 0x8000000000000063 = 9223372036854775907 
Y =  0x7E3 = 2019  
 
也就是：  
题目固定Y = C = 2019， 且验证： 
powmod ((2^A)^X, P)  = 2019  , 那么{A-B-C}正确。 
同时，题目里判定了X 是一个小素数，大于32bit， bits(A)+ bits(X)<= 64 ， A 是一个比较
小的数,  2019 <= A <= 9102 ， （AX）最大为64 bit，通过X 和 AX 的限制条件，保证了
解的唯一性。 
可以总体求出 d = AX ， 利用幂函数的性质： (2^A)^X  = 2^(A*X) 
2^(AX)= 2019 mod P 
然后因子分解d 的一个素因子（大于32bit）就是X， 余下部分A = d/X 
绿盟科技，禁止外传
技，禁止外传
科技，禁止外传
传
绿盟科技，禁止外传
绿盟科技，禁止外传
绿
绿盟科技，禁止
绿盟科技，禁


---

<!-- page 2 -->

解法： 
首先做第1 步：解离散对数求出A，X 
 
求离散对数(Discrete logarithm problem)，方法很多。 
可以用sageMath，也可以用看雪论坛的DLPTool 工具（readyu 编写）： 
也可以在线的java 计算dlog 网页， 这里64bit 的模，规模比较小，在10 余秒即可计算。 
 
因为这里P-1 可以分解出4 个素数因子(0x8000000000000062)：  
P- 2 
P- 71 
P- 95 
P(48)- F91CC07D877D  
最大的Order 只有48bits， 那么期望计算步数只有O(24bits), 用Pohlig–Hellman algorithm 
计算起来会很快。 
 
方法一： 离散对数在线计算版： 
https://www.alpertron.com.ar/DILOG.HTM 
输入  
Base = 2 
Power = 2019 
Modulus = 9223372036854775907 
计算结果如下： 
Find exp such that 2^exp ≡ 2019 (mod 9223372036854775907) 
 
exp = 9157099761843089388 + 9223372036854775906k 
 
求出 dlog ：exp = A*X，分解为小因子即可得到X， 从而 A = exp/X。 
AX= exp = 7F148DB78D78AFEC = 9157099761843089388 
因子分解AX： 
P-2 ^(2) 
P-3 
P-107 
P(52)-A4EE3963076CF 
 
根据题目条件，X 是一个素数，大于32bits，那这里面只有唯一的选择： 
取 ： 
X=P(52) = 2901489151407823 = 0xA4EE3963076CF 
于是： 
A= exp/X = 3156 = 0xC54 
 
 
绿盟科技，禁止外传
技，禁止外传
科技，禁止外传
传
绿盟科技，禁止外传
绿盟科技，禁止外传
绿
绿盟科技，禁止
绿盟科技，禁


---

<!-- page 3 -->

方法二： DLPTool 工具计算版： 
 
用如下配置保存为txt， 然后运行命令行bat 文件： 
dlp.exe  dlp_64.txt   
 
即可求解： 
  
[START OF CONFIG FILE] 
// must keep this line 
  
[VERSION] 
  
[SEED] 
20191021 
  
[NUMBER BASE] 
16 
  
[BITS] 
64 
  
[P/MODULUS] 
8000000000000063 
  
[PHI/ORDER] 
8000000000000062 
  
[G/BASE] 
2 
  
[Y/PUB] 
7E3 
  
[X/PRI] 
  
  
[PHI PRIME FACTORS NUMBERS] 
4 
  
[PHI PRIME FACTORS LISTS] 
2^(1) 
71^(1) 
95^(1) 
F91CC07D877D^(1) 
绿盟科技，禁止外传
技，禁止外传
科技，禁止外传
传
绿盟科技，禁止外传
绿盟科技，禁止外传
绿
绿盟科技，禁止
绿盟科技，禁


---

<!-- page 4 -->

 
[END OF CONFIG FILE] 
 
方法二截图如下： 
 
 
绿盟科技，禁止外传
技，禁止外传
科技，禁止外传
传
绿盟科技，禁止外传
绿盟科技，禁止外传
绿
绿盟科技，禁止
绿盟科技，禁


---

<!-- page 5 -->

然后做第二步，计算出B： 
 
根据   X = B/4^256  （mod N） ,   
反推： B = (4^256 * X)  mod N = 
 
X = 0x A4EE3963076CF  
N =  md5(“crackme_2019_oct_beta”) = 0x 17678322bacd53942b76aa11e47518af 
K = 4^256 mod N = 0xBC35D6A6E6114C72B0413E91856FCF9 
 
=>计算： 
B = (K*X) mod N =  
7941A1C1459FB4DAF62FC269AFCEF16142071BA15357  mod 
17678322bacd53942b76aa11e47518af 
=13A2FCFDDC8366059410A6A48D45A9BA 
 
综合整理： 
A=0xC54 
B=0x13A2FCFDDC8366059410A6A48D45A9BA 
C=0x7E3 
Flag{A-B-C} 
 
得到Flag： 
flag{C54-13A2FCFDDC8366059410A6A48D45A9BA-7E3} 
 
绿盟科技，禁止外传
技，禁止外传
科技，禁止外传
传
绿盟科技，禁止外传
绿盟科技，禁止外传
绿
绿盟科技，禁止
绿盟科技，禁
