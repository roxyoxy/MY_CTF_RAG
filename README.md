# MY_RAG — 离线 CTF RAG 系统（从零手写版）

> 算法设计大作业 · 3 人小组 · C++ · 完全离线
>
> **本仓库的灵魂不是代码，是写代码的人。**
> 协作规则见 `PRINCIPLES.md`（必读，尤其是 AI 助手）。

## 1. 项目是什么

一个不联网的 CTF 解题辅助系统：

```
输入：一道 CTF 题的描述（如 "ret2libc, NX on, libc 2.31"）
  ↓
检索：在自己的 writeup 知识库里找出最相似的题型片段
  ↓
生成：把检索结果作为上下文，交给本地大模型
  ↓
输出：带引用来源的解题思路
```

为什么值得做（三个理由）：

1. **比赛实用**：禁联网 CTF 比赛不能用云端 AI，需要全本地推理
2. **课程对口**：算法设计课——核心算法（BM25 / 向量检索 / RRF / HNSW）
   全部自己写，大模型只是被调用的组件
3. **专业特色**：网安专业做 RAG，检索投毒与提示注入的防御是天然亮点

## 2. 当前状态与迭代路线

**现在是 M2 阶段（中文检索 + 持久化）**。整体路线：

| 阶段 | 内容 | 状态 |
|---|---|---|
| M0 | 文档体系 + 会议纪要 + 查询脚本 | ✅ 2026-09-22 |
| M1 | 纯英文检索：接口设计(.h) → main → 实现(.cpp) → 优化 | ✅ 2026-09-28 四轮全部竣工 |
| M2 | 中文 bigram + 持久化 + 增量/删除 + Qt 管理界面 | 自家四件全 ✅（① 09-28 · ② ④ ③ 09-29）；剩 Qt 界面（外包施工中） |
| M3 | 向量检索（Flat → 自研 HNSW）+ RRF 混合 | 待开始 |
| M4 | 本地 LLM 后端（Ollama 原型 → llama.cpp 交付）| 待开始 |
| M5 | 对比实验（BM25/Dense/Hybrid 三组）+ 报告答辩 | 待开始 |

M1 内部进度（施工顺序见 PRINCIPLES.md §2）：

- 第一轮：**五份契约 .h 全部竣工** ✅ 2026-09-23
  （type / loader / chunker / tokenizer / indexer，全部亲手署名）
- 第二轮：**main.cpp 总装竣工** ✅ 2026-09-23
  （曾处于"链接红"预期中间态：4 个 LNK2019 = 四个待实现函数清单）
- 第三轮：**四个 .cpp 实现全部竣工** ✅ 2026-09-28
  （B = chunker + indexer，C = loader + tokenizer，见 docs/TASKS.md）
  T1 PR #1（09-27）· T2 PR #2（09-28）· T3/T4 B 离线 zip 交接、
  A 代提交（09-28）；**LNK2019 清零，端到端检索已跑通**
- 第四轮：**优化 + 测试竣工** ✅ 2026-09-28，**M1 收官**
  （main catch / tests 37 条全绿 + run_tests.bat / top_k 守卫 /
  控制台 UTF-8 / 优化遍历：六函数全"不动"，search 两处优化提案挂 M5）
- data\ 正式语料已落位（2026-09-28）：12 篇本人真实比赛 writeup
  （npusec 小赛），pwn/re/web/misc/ai_security/crypto 六类目

迭代纪律：**每个阶段先跑通再优化，先英文后中文，先暴力再高级数据结构**。
Flat 暴力检索不是偷懒——它是 HNSW 的正确性基线，没有基线的优化是玄学。

## 3. 目录设计（以及为什么）

实际布局（2026-09-22 定稿）：**文档区 / 代码区两层结构**。

```
MY_RAG/                      ← 仓库根 = 文档区（人和 AI 先读这里）
├── README.md                ← 你在这里
├── PRINCIPLES.md            ← 宪法：AI 协作规则 + 会议制度
├── docs/
│   ├── MEETING_LOG.md       ← 会议纪要：全组共同记忆（scripts 可查）
│   ├── TASKS.md             ← 任务分发与验收（B/C 开工入口）
│   └── PARAMS.md            ← 可调参数登记表（M5 调参总账）
├── scripts/
│   └── meeting_query.py     ← 纪要查询工具
└── MY_CTF_RAG/              ← 代码区（VS 解决方案）
    ├── MY_CTF_RAG.sln
    ├── MY_CTF_RAG.vcxproj
    ├── include/             ← 头文件：模块对外的"契约"（八份，M2 +atom_scan/persist/corpus_diff）
    │   ├── type.h           ← 基础数据结构（Document/Chunk/SearchResult）
    │   ├── loader.h         ← 搬进来：data 目录 → vector<Document>
    │   ├── corpus_diff.h    ← 对账本：两代语料的 added/removed/edited + 墓碑继承（M2③ 新增）
    │   ├── chunker.h        ← 切开来：Document → Chunk（500 词/50 词重叠）
    │   ├── tokenizer.h      ← 磨成粉：text → tokens（词内字符集规则）
    │   ├── indexer.h        ← 账房+查账台：倒排索引 + BM25
    │   ├── atom_scan.h      ← 词法单一事实源：Atom 扫描（M2① 新增）
    │   └── persist.h        ← 索引快照：save/validate/load（M2② 新增）
    ├── src/                 ← 实现：对契约的"履约"
    │   └── main.cpp         ← 总装车间（M1 第二轮产物）
    ├── data/                ← 语料，按 category 分目录（58 篇：npusec 24 + ctf-wiki 34，含 mobile 类目）
    └── (tests/ 第四轮再建，YAGNI)
```

为什么这样分（面试常问）：

- **文档区 / 代码区分层**：文档先于代码出生、晚于代码死亡，生命周期
  不同；人和队友的 AI 进仓库先读文档再进代码，互不干扰。
- **include / src 分离**：头文件是契约，源文件是履约。别人只读 include/
  就知道怎么用你的模块，不需要看实现。这是 C++ 工程的物理边界。
- **scripts 独立**：辅助工具不是产品。产品要交付，工具只要好用。
- **data 独立**：内容与逻辑分离。语料可整体替换（英文库换中文库），
  程序一行不改。

## 4. 快速开始

**当前状态**：**M2-③ 增量建库竣工（2026-09-29），M2 自家四件套
全清**。八份契约 + 114 条回归测试全绿（/W4）。启动流程：validate
快照 -> 命中即 load（`index.bin` 缓存，损坏自动降级重建）-> 未命中
fresh build + save；**语料变动（CORPUS_CHANGED）时收获旧快照：
打一行 diff 报告（added/removed/edited）+ 按 path 继承墓碑 ->
重建**——"编辑被删文档"不再复活（复活通道 = 物理删文件重加）。
查询环支持 `list` / `del <id>`：删除 = 墓碑 + 立即重建 + save。
M2① 中文 bigram、M2-② 持久化、M2-④ 墓碑消费此前竣工。
`data\` **58 篇 writeup / 145 chunks**（npusec 小赛 24 EN+CN 同源
互译 + ctf-wiki 34 新知识含 mobile 类目，09-29 经 CORPUS_CHANGED
添加流程实弹入库），`query>` 交互查询（中英文均可）。测试一键跑：
`tests\run_tests.bat`。
构建环境既成事实（B/C 必读）：

- VS2022 项目设置：`/utf-8` + ISO C++17 + `/W4`，作用域=所有配置×所有平台
- 附加包含目录：`$(ProjectDir)include`（src/ 引用契约的路径来源）
- **源码注释纪律：纯 ASCII 英文**（破折号用 `--`，禁用 `—`、弯引号、
  省略号；VS 默认按系统代码页 GBK 存盘，非 ASCII 字符会埋雷）
- 可调参数一律 `constexpr` 集中放模块 .h，并登记 `docs/PARAMS.md`

会议纪要查询：

```
py scripts/meeting_query.py --recent 5          # 最近 5 条发言
py scripts/meeting_query.py -s A                # A 说过的话
py scripts/meeting_query.py "HNSW"              # 全文搜关键词
py scripts/meeting_query.py -s A -n 3 --compact # A 最近 3 条（只看标题）
```

## 5. 给队友

1. 先读 `PRINCIPLES.md`（10 分钟，尤其第 3 节 AI 规则）
2. 把 `README.md` + `PRINCIPLES.md` + `docs/MEETING_LOG.md` 发给你的
   AI 助手，让它读完再跟你对话（指令模板见 PRINCIPLES.md 第 5 节）
3. 你的发言和 AI 的发言都会记进会议纪要——这是设计，不是监控

## 6. 相关资源

- 参考实现（只许看伪代码，规则见 PRINCIPLES）：`Desktop\project_RAG\`
- 项目蓝图评审记录：`project_RAG\docs\AI-BRIEFING.md`
