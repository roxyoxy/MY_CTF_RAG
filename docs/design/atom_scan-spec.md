# 施工单 — include/atom_scan.h（M2 第六契约）

> 依据：纪要 [2026-09-28 13:32] A（甲″裁决 + 八条细则）+ [2026-09-28 13:58] A（设计定稿四条）。
> 状态：**已竣工关账**（[2026-09-28 15:11] AI-A）。施工方式中途变更——
> A 拍板"AI-A 直接写出来"：A 创建文件壳，AI-A 执笔契约全文，A 复审
> （宪法 §3 一次性授权特例，同测试代码先例，不构成惯例）。
> 本文件转为设计存档 + review 底稿：条款规格/预答/硬性自由分界
> 仍是后续四件契约改写的参照母本。
> 本文件是 docs/design/ 第一份设计文档，「刷新」指令自本日起覆盖本目录。
> 本份是 M2① ⓪ 步（契约五件套）的第一件；关账后依次过其余四件。

---

## 0. 第 0 步（M1 制度：动笔前自测）

三问能脱口而出才动笔（答案都在纪要 13:32 / 13:58 里）：

1. `scan_atoms` 的输入是什么、输出是什么？
2. 它在管线里的位置——现在谁调它、M2⑤ 之后谁还会调它？
3. 它为什么是第六份**独立**契约，而不是 tokenizer.h 里加一个函数？
   （第 3 问考点：消费者是两个不是一个；include/ = 契约层的项目定义；
   GUI 高亮是预见的第三消费者）

---

## 1. 必读清单（按顺序）

1. `docs/MEETING_LOG.md` 条目 [2026-09-28 13:32] A —— 裁决全文
2. `docs/MEETING_LOG.md` 条目 [2026-09-28 13:58] A —— 本文件设计定稿
3. `include/tokenizer.h` 条款 2 —— ASCII_WORD 定义的移植母本
4. `include/type.h` —— Chunk::begin/end 的类型与语义（对齐对象）
5. 选读：`Desktop\MY_RAG_M2_分词架构征询.md` §4-5（atom 概念的来龙去脉）

---

## 2. 文件骨架（六段，对齐 M1 契约排版风格）

```text
#pragma once
// atom_scan.h
// <一句话定位 —— 你自己组织，提示：lexical units / byte spans>

<includes —— 自推敲，见 §5 预答 6>

enum class <AtomKind?> { ASCII_WORD, CJK_CHAR };

struct Atom {
    <三字段 + 注释>
};

std::vector<Atom> scan_atoms(const std::string& text);
// Contract: <七条，见 §3>
```

---

## 3. 契约条款规格（七条逐条：语义 / 判例 / 禁区）

### 条款 1 — 顺序与完备性

- 语义：atoms 按文本出现顺序排列，互不重叠；空输入或纯分隔符输入
  返回空 vector
- 判例：`""` -> `{}`；`" ... !!! "` -> `{}`
- 禁区：不要写"atoms 覆盖全部字节"——分隔符不属于任何 atom，这是
  刻意的（tokenize 条款 4 同款风格）

### 条款 2 — ASCII_WORD

- 语义：[A-Za-z0-9] 的**极大连续段**（maximal run）为一个
  ASCII_WORD atom；`-` 无条件词内；`.` 与 `_` 当且仅当左右均为
  [A-Za-z0-9] 时词内（三明治规则）
- 移植说明：从 tokenizer.h 条款 2 移植，**只改两处**：
  ① "letters, digits" 改为显式区间（从此与 locale 无关，
  MSVC/g++ 逐字节一致）；② 删除一切大小写字样——scanner 产的是
  字节区间不是词，**大小写概念不存在**
- 判例（全 ASCII，可直接进注释）：
  `libc-2.31` / `127.0.0.1` / `exploit.py` / `buf_size` -> 各一整个 atom；
  `done.` -> [done]；`wait...` -> [wait]；`__libc_csu_init` ->
  [libc_csu_init]；`e.g.` -> [e.g]
- 禁区：不要出现 lowercase / normalize / token 字样

### 条款 3 — CJK_CHAR

- 语义：单个码点落在 U+3400..U+4DBF 或 U+4E00..U+9FFF 时，该码点
  的完整 UTF-8 字节序列（3 字节）构成一个 CJK_CHAR atom
- 判例（注释里用码点记法，见 §5 预答 1）：
  U+5929 U+4E0B（tian xia）-> 两个 CJK_CHAR atoms
- 禁区：**不写解码算法**（lead byte / continuation byte 是 .cpp 的
  事）；**不写 bigram**——scanner 对 bigram 一无所知，那是
  tokenizer 的政策

### 条款 4 — 偏移语义

- 语义：begin / end 是**输入 text** 的字节偏移，左闭右开 [begin, end)
- 为什么：与 type.h 的 Chunk::begin/end 同型（size_t）同义——
  chunker 拿 atom 边界下刀、GUI 拿它高亮，都指回原文
- 禁区：不要引入"字符偏移"概念，字节偏移是唯一语言

### 条款 5 — 其余皆分隔符

- 语义：不属于 ASCII_WORD 也不属于 CJK_CHAR 的一切 = 分隔符，不产
  atom——含空格、半角/全角标点、emoji、其他文字系统（假名/谚文/
  西里尔……）
- 判例：全角逗号 U+FF0C -> no atom（不在 Han 范围，解码后分类
  自然拒绝）；全角字母 U+FF21 -> no atom（不在 [A-Za-z0-9]——
  显式区间的红利）；emoji -> no atom
- 禁区：不要枚举"全部分隔符"——写补集定义（不属于前两者即是），
  一句话挡住整个 Unicode

### 条款 6 — 畸形 UTF-8 防御

- 语义：解码失败（非法序列/断尾序列/surrogate/超 U+10FFFF）时
  **跳过一个字节继续**；本函数对**任意**字节输入不抛异常、不崩溃、
  必正常返回
- 论据（已入纪要 13:58，双链）：契约链（search 不抛承诺）+ 现实链
  （投喂的 writeup 不保证干净，一个脏文件不能瘫痪索引启动）
- 禁区：不要 throw；不要"假设输入合法 UTF-8"这种免责条款

### 条款 7 — 单一事实源（宪法条款，本文件的灵魂）

- 语义：本函数是词法单位判定的**唯一权威**；tokenizer 与 chunker
  必须经由它获取词法单位，任何模块不得私实现字符分类规则
- 措辞提示：英文可用 "single source of truth"
- 定位：前六条说它**做什么**，这条说它**是谁**——09-28 甲″裁决的
  制度化。写的时候想想这句话怎么用英文说出分量

---

## 4. 已预答的问题（施工中卡壳先查这里）

1. **CJK 判例怎么写进纯 ASCII 源码？**
   注释用码点记法（U+5929）；将来测试代码用 `"天"` 通用字符名
   （源码保持纯 ASCII，编译期生成真字符）。**不要**在 .h 里贴汉字
   原文——纯 ASCII 纪律优先于判例直观性
2. **大写词怎么办？** ROP 原样成 atom。小写化是 tokenizer.h 条款 1
   的政策，与本契约无关
3. **CRLF？** \r 与 \n 都不在词内字符集，分隔符
4. **合法但非 Han 的多字节序列（如 emoji）？** 解码成功、范围判定
   失败 -> 分隔符。跳整段还是逐字节是实现自由——分隔器产不出
   atom，两种跳法输出完全相同，契约不规定
5. **汉字和 ASCII 相邻？** U+0052U+004FU+0050 + U+94FE（"ROP链"）
   -> 两个 atoms：一个 ASCII_WORD（扫到 CJK 首字节停）+ 一个
   CJK_CHAR。跨界合并不存在
6. **include 候选：** `<string>` `<vector>` 必带；`<cstddef>`
   （size_t 的正规出处）要不要较真——A 拍板，review 时对答案
   （M1 先例：main.cpp 的 std::vector 走传递 include，A 裁不补；
   本次是否同裁，你定）

---

## 5. 硬性与自由（接口冻结 / 措辞自由的分界）

**硬性（review 红线）：**

- 文件名 atom_scan.h；第一行 `#pragma once`
- 枚举两值命名：`ASCII_WORD` / `CJK_CHAR`，用 `enum class`
- `struct Atom` 恰好三字段 `begin` / `end` / `kind`，
  类型 `size_t` / `size_t` / 枚举类型
- 签名一字不差：`std::vector<Atom> scan_atoms(const std::string& text);`
- 七条条款语义一个不少（措辞可自由，语义不可缺）

**自由（你的地盘）：**

- 全部英文措辞、注释详略、判例摆法、示例挑哪些
- 顶部一句话定位怎么写
- 待拍板小项：枚举名 `Kind` vs `AtomKind`（AI-A 推荐 `AtomKind`：
  顶级 `Kind` 太泛，`AtomKind` 自带语境；你拍板）

---

## 6. 施工流程

1. VS 解决方案资源管理器 → MY_CTF_RAG 项目 → 右键**头文件**筛选器
   → 添加 → 新建项 → 头文件，命名 atom_scan.h
   （头文件不参与链接，无编译设置要动；添加进项目只为出现在
   解决方案资源管理器视图）
2. 按骨架六段写；英文注释纯 ASCII——破折号用 `--`，禁用
   em dash / 弯引号 / 省略号（M1 血泪条款）
3. 每写完一条条款，回头对照 §3 该条的"禁区"自检一遍
4. 写完跑字节自检（见 §7），全绿再贴出
5. 全文贴给 AI-A review

---

## 7. 验收标准

A 自查清单（贴出前）：

- [ ] 骨架五件齐：pragma once / 文件头注释 / enum class / struct /
      签名 + 契约
- [ ] 七条款全覆盖，判例双栏：ASCII 侧（条款 2）+ 码点记法 CJK 侧
      （条款 3/5）
- [ ] 第 7 条"single source of truth"到位
- [ ] 与 tokenizer.h 条款 2 对照：语义移植无走样，且无小写化残留
- [ ] 字节自检 0 个非 ASCII：

```bash
py -c "d=open(r'C:\Users\48714\Desktop\MY_RAG\MY_CTF_RAG\include\atom_scan.h','rb').read(); bad=[i for i,b in enumerate(d) if b>=128]; print('PURE ASCII' if not bad else 'NON-ASCII at '+str(bad[:10]))"
```

AI-A review 检查点（贴出后）：

- [ ] 逐条款对 §3 语义核验
- [ ] IWYU：include 集合最小且字母序
- [ ] 判例正确性（码点记法逐个核算）
- [ ] 语法烟测：临时翻译单元 include 本头文件编译一次（不落仓库）
- [ ] 通过后：纪要关账条 + 字节复检 + 连同积压 docs 一次 commit
