# ez_re · Writeup（中文版）

- **题目**：easy_re（ez——re）
- **分类**：RE · **分值**：125 pts（动态分值，9.23 解题时平台显示 500 pts）
- **附件**：`encode.exe`（PE32+ x86-64，212KB，MinGW-w64/GCC 编译，16 区段）、`encrypted.txt`（32 字节密文）、`encrypted.zip`（前两者打包，未加密）
- **关键词**：IDA F5 · TEA 变体 · 异或自逆 · OpenGL 诱饵
- **最终 flag**：`npusec{cyb3r$3cur1ty_2O26_f1@g!}`

---

## 题干

> easy_re / 125 pts
>
> 第一次去卢浮宫时，并没有什么特别的感觉
> 因为独属于我的蒙娜丽莎，我早已遇见
>
> 尝试了解ida吧，开始逆向之旅。
> 或许正向难以看懂，可以试着去逆向思考
> 青蛙的眼睛对动态的事物更清晰
> 不要乱运行文件，不然还得重新下

---

## 解题过程

### 第一步：静态侦察——OpenGL 是烟雾弹

Python 过一遍二进制指纹：

- 编译器：MinGW-w64（GCC），16 区段是 MinGW 调试区段的正常现象，不是加壳
- 字符串：**大量 OpenGL 符号**（`glad_` 加载器全家桶、`glReadPixels`/`glStencilMask`/stencil 系列、`OpenGL ES` 版本串）——看似 GPU 加密
- 关键文件名：`encrypted.txt`、` encrypted.bin`、CRT 的 `fopen/fread/fwrite`

题干"青蛙的眼睛对动态的事物更清晰"在怂恿你上动态调试追 GPU 渲染——**诱饵**。真正干活的是纯整数运算，IDA 里 main 一目了然。教训：字符串面板里最唬人的 API 群不一定是主角，先看 main 再决定动不动态。

### 第二步：IDA F5——算法本体（TEA 变体 + 异或）

IDA 打开 `encode.exe` → F5 反编译 `main`（本题唯一核心函数）：

```c
__int64 __fastcall main()
{
  unsigned int a1_8; char *plainword; FILE *fp_out; FILE *fp_in;
  int i_0; int i; uint32_t v3;

  _main();
  fp_in  = fopen("plainword.txt", "rb");
  fp_out = fopen("encrypted.txt", "wb");
  plainword = (char *)calloc(0x32, 1);
  v3 = 0;
  a1_12 = 405228090;                       // 状态字 1（TEA 里的 v0 位）
  a1_8  = 620287023;                       // 状态字 2
  fread(plainword, 1, 0x20, fp_in);        // 读 32 字节明文
  for ( i = 0; i <= 31; ++i )              // 32 轮状态机（不碰明文）
  {
    a1_12 += (16 * a1_8 - 1668836417) ^ (v3 + a1_8) ^ ((a1_8 >> 5) - 1151594428);
    v3 -= 559038737;                       // delta（TEA 里是 sum += delta，这里倒着减）
    a1_8  -= (16 * a1_12 + 803884916) ^ (v3 + a1_12) ^ ((a1_12 >> 5) - 1589449333);
  }
  for ( i_0 = 0; i_0 <= 7; ++i_0 )
    *(_DWORD *)&plainword[4 * i_0] ^= a1_8;   // 终态 a1_8 当密钥流，8 个 dword 全异或它
  fwrite(plainword, 1, 0x20, fp_out);
  fclose(fp_in); fclose(fp_out); free(plainword);
  puts(&Buffer);
  return 0;
}
```

（F5 原文照录，仅砍掉重复的变量声明区并加注释；`puts(&Buffer)` 是收尾打印，与加密无关。）

识别要点：

1. `(16*x + K1) ^ (sum + x) ^ ((x>>5) + K2)` 的形状是 **TEA 轮函数**的变体（标准 TEA 是 `<<4`/`>>5` + 加法拼 key；这里把 key 常量换成了减法魔数，v3 用减 delta）
2. **循环体完全不碰明文**——32 轮只更新内部状态，明文只在最后一步出场
3. 加密本体 = **用终态 `a1_8`（单个 32 位字）把 32 字节明文按 dword 异或 8 遍**——同一个密钥流重复 8 次

### 第三步：逆向思考——异或自逆，模拟即解

题干"或许正向难以看懂，可以试着去逆向思考"：不用找"decrypt"，因为 **XOR 的逆就是 XOR**。密钥流与明文无关，直接用同样的常数把 32 轮状态机空跑一遍拿到 `a1_8`，再对密文异或回去：

```python
import struct
M = 0xFFFFFFFF
a1_12, a1_8, v3 = 405228090, 620287023, 0
for _ in range(32):
    a1_12 = (a1_12 + (((16*a1_8 - 1668836417) & M) ^ ((v3 + a1_8) & M) ^ (((a1_8 >> 5) - 1151594428) & M))) & M
    v3    = (v3 - 559038737) & M
    a1_8  = (a1_8 - (((16*a1_12 + 803884916) & M) ^ ((v3 + a1_12) & M) ^ (((a1_12 >> 5) - 1589449333) & M))) & M
K = a1_8                                  # = 0x7a84a720

ct = bytes.fromhex(
    '4ed7f10945c4ff1959c5b7080494e70f'
    '5296f0037f95cb4816f8e24b60c0a507')    # encrypted.txt
pt = b''.join(struct.pack('<I', struct.unpack('<I', ct[4*i:4*i+4])[0] ^ K)
              for i in range(8))
print(pt)   # b'npusec{cyb3r$3cur1ty_2O26_f1@g!}'
```

**flag：`npusec{cyb3r$3cur1ty_2O26_f1@g!}`**（恰好 32 字节，与 `fread` 的 0x20 对齐；leetspeak "cyber security 2026 flag"）

---

## 完整攻击链

```
encode.exe
  ├─ 字符串侦察：OpenGL 全家桶 = 青蛙诱饵（不看 main 差点去追 GPU）
  └─ IDA F5 main：
       ├─ 32 轮 TEA 变体状态机（v3 减式 delta、减法魔数 key）
       └─ 终态 a1_8 作 32 位密钥流 ⊕ 明文 8 个 dword
            └─ Python 模拟状态机 → K=0x7a84a720 → 密文 ⊕ K = flag
```

---

## 知识点沉淀

1. **TEA 家族认脸**：`(x<<4 ± k) ^ (sum + x) ^ ((x>>5) ± k)` 三件套形状，见到就是 TEA/XTEA/XXTEA 变体；delta 变体（加/减、魔数不同）不影响识别
2. **密钥流与明文无关 + XOR = 可离线求逆**：只要 keystream 不依赖明文，空跑算法拿密钥流再异或即解，不需要写逆向循环
3. **F5 优先于动态调试**：16 区段 MinGW ≠ 加壳；唬人的 API 字符串群（OpenGL/图形库）可能是烟雾弹，main 里十行代码才是真相
4. **题干句句有指涉**：蒙娜丽莎诗=装饰、"逆向思考"=别找 decrypt、直接逆运算、"青蛙"=动态诱饵、"别乱运行"=防你在宿主机裸跑 exe
5. **Windows 无符号右移**：IDA 里 uint32 的 `>>` 是逻辑移位，Python 复现时要 `& 0xFFFFFFFF` 管住每步溢出

---

## 踩坑记录

- 差点被 OpenGL 字符串带进 GPU 渲染 + 动态调试的沟里（题干"青蛙"还在旁边煽风）——先 F5 main 三分钟结束战斗，**静态优先、动态兜底**
- 复现 C 的 32 位回绕运算时每一处加减/移位结果都要 `& 0xFFFFFFFF`，漏一处密钥流全错

---

## 📸 复现与截图位（待补，复现后回填）

> 截图统一存 `WP_cn\RE\图片\`（已建），命名 `序号_内容.png`。复现材料已备齐：
> - IDA 数据库 `题目/RE/ez——re/encode.exe.i64` 在位——IDA 直接开库即恢复分析，无需重新反编译
> - 侦察/解密脚本已落 `_work/recon_ezre.py`、`_work/solve_ezre.py`，均已验证可复现

| # | 截图内容 | 怎么拍 | 存放路径 | 状态 |
|:---:|:---|:---|:---|:---:|
| 1 | IDA 打开 encode.exe，左侧函数列表定位 main，界面全貌 | IDA 打开 `encode.exe.i64` → Functions 面板点 main | `图片/01_ida_打开.png` | ⬜ |
| 2 | main 函数 F5 伪代码**完整**（32 轮循环 + 异或 + fwrite 都可见） | main 里按 F5，截完整伪代码窗口 | `图片/02_ida_f5_main.png` | ⬜ |
| 3 | 静态侦察输出（16 区段 + 1407 条 gl 命中 + 关键文件名） | `_work` 下跑 `py recon_ezre.py` | `图片/03_静态侦察.png` | ⬜ |
| 4 | 解密输出（`keystream K = 0x7a84a720` + `flag: npusec{...}` 两行） | `_work` 下跑 `py solve_ezre.py` | `图片/04_解密输出.png` | ⬜ |
| 5 | 平台提交通过记录 | 平台题页/提交记录，含题名与通过标记 | `图片/05_提交通过.png` | ⬜ |

回填时把 `![01](图片/01_ida_打开.png)` 贴到对应步骤即可（第一步→侦察节、第二步→F5 节、第四步→第三步节）。
