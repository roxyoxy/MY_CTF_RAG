# MY_RAG — 离线 CTF RAG 系统（核心算法自研版）

> 算法设计大作业 · 3 人小组 · C++ · 完全离线
>
> **本仓库的灵魂不是代码，是写代码的人。**
> 协作规则见 `PRINCIPLES.md`（必读，尤其是 AI 助手）。
>
> **English README: [`../README.md`](../README.md)**（本文件为完整中文版）。

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
2. **课程对口**：算法设计课——检索层核心算法（BM25 / bigram
   分词 / Flat 向量检索 / 自研 HNSW / RRF）自己写、可解释；
   embedding 与 LLM 引用成熟开源模型与组件（2026-09-29 老师定调：
   性能优先、实用好用为主、手写必要算法为辅，不涉 ML 训练）。
   什么是研究对象、什么是依赖，边界清晰——见下表

| 层 | 处理 |
|---|---|
| tokenizer / BM25 / 切块 / 持久化 / 增量 / 删除 | 自研（M1/M2 已交付） |
| embedding 模型 / 推理运行时（Ollama→llama.cpp） | 成熟开源组件 |
| Flat 精确向量检索 / 自研 HNSW / RRF | 自研（M3） |
| LLM | M4 集成成熟本地模型 |
| 评测 | 自己做（M5） |
3. **专业特色**：网安专业做 RAG，检索投毒与提示注入的防御是天然亮点

## 2. 当前状态与迭代路线

**现在是 M5 阶段（对比实验 + 报告答辩）**，M0-M4 已全部收官。整体路线：

| 阶段 | 内容 | 状态 |
|---|---|---|
| M0 | 文档体系 + 会议纪要 + 查询脚本 | ✅ 2026-09-22 |
| M1 | 纯英文检索：接口设计(.h) → main → 实现(.cpp) → 优化 | ✅ 2026-09-28 四轮全部竣工 |
| M2 | 中文 bigram + 持久化 + 增量/删除 + Qt 管理界面 | ✅ 2026-09-29 全线收官（F5 验收 + push） |
| M3 | 向量检索（Ollama embedding + Flat 基线 + 自研 HNSW 双后端）+ RRF 混合 | ✅ 六步全竣工（10-12：⑥ HNSW 自研竣工，Recall@10 对 Flat 1.0；Flat 留产品默认，HNSW 为 M5 实验对象） |
| M4 | 本地 LLM 后端（Ollama 原型 → llama.cpp 交付）| ✅ 2026-10-12 收官：⓪-④ 竣工 + ⑤ llama.cpp 真机 smoke 9/9 PASS（embedding dim=1024 归一化对齐；chat 接地引用 3.5-4.2s，同机同模型较 Ollama 快约 2 倍）；⑥ 注入金弹并入 M5 |
| M5 | 对比实验（BM25/Dense-Flat/Dense-HNSW/Hybrid 四路，两层评测框架）+ 报告答辩 | 待开始 |

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
├── README.md                ← 英文主文档（English README）
├── LICENSE                  ← MIT
├── CONTRIBUTING.md          ← 参与指南（环境 / 回归门 / 硬规则）
├── PRINCIPLES.md            ← 宪法：AI 协作规则 + 会议制度
├── .github/workflows/       ← CI：Windows runner 构建 + 243 测试回归
├── docs/
│   ├── README.md            ← 文档索引（外人阅读地图）
│   ├── README.zh.md         ← 完整中文版 README（你在这里）
│   ├── MEETING_LOG.md       ← 会议纪要：全组共同记忆（scripts 可查）
│   ├── TASKS.md             ← 任务分发与验收（B/C 开工入口）
│   ├── PARAMS.md            ← 可调参数登记表（M5 调参总账）
│   ├── m3-plan.md           ← M3 施工策略计划（09-29 初步定档）
│   ├── m1-m2-retrospective.md ← M1-M2 阶段总结（复习专用）
│   ├── m4-generation-ab.md  ← M4 生成模型 A/B 实验档
│   ├── assets/              ← 演示截图与 GIF
│   └── design/              ← 设计文档（缓议书等）
├── scripts/
│   ├── meeting_query.py     ← 纪要查询工具
│   ├── dump_chunks.cpp      ← T10：真 chunker 导出 JSONL（评测口径与产品一致）
│   └── ab_eval.py           ← T10：A/B 评测脚本（23 题集内嵌，M5 复用资产）
└── MY_CTF_RAG/              ← 代码区（VS 解决方案）
    ├── MY_CTF_RAG.sln
    ├── MY_CTF_RAG.vcxproj
    ├── include/             ← 头文件：模块对外的"契约"（十五份：M1 五 + M2 三 + M3 五 + M4 二）
    │   ├── type.h           ← 基础数据结构（Document/Chunk/SearchResult）
    │   ├── loader.h         ← 搬进来：data 目录 → vector<Document>
    │   ├── corpus_diff.h    ← 对账本：两代语料的 added/removed/edited + 墓碑继承（M2③ 新增）
    │   ├── chunker.h        ← 切开来：Document → Chunk（500 词/50 词重叠）
    │   ├── tokenizer.h      ← 磨成粉：text → tokens（词内字符集规则）
    │   ├── indexer.h        ← 账房+查账台：倒排索引 + BM25
    │   ├── atom_scan.h      ← 词法单一事实源：Atom 扫描（M2① 新增）
    │   └── persist.h        ← 索引快照：save/validate/load（M2② 新增）
    ├── src/                 ← 实现：对契约的"履约"（十四份已实现 + main.cpp 总装，M3 混合查询链 + vector.bin 已接线；hnsw 与 llama.cpp 工厂待 M5 flag 接线）
    ├── data/                ← 语料，按 category 分目录（58 篇：npusec 校内赛 22 + ISCC 2026 赛后题解 36，含 mobile 类目）
    ├── tests/               ← 回归测试（243 条全绿，15 单元）+ run_tests.bat 一键跑
    └── gui/                 ← Qt GUI 源码四件（MY_CTF_RAG_GUI 工程编译，F5 直达；含 M4 查询 worker）
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

**当前状态**：**M0-M4 全收官（10-12），现在是 M5 阶段**。
M3 六步全竣工（⑥ HNSW 自研 10-12，Recall@10 对 Flat 1.0）；
M4 全竣工（⓪-③½ + ④ Qt chat 页 + worker——QueryWorker 包全链
后台线程（embed_query 与 chat 两段 HTTP 均不碰 UI 线程），单轮问答、
[n] 点击跳源高亮、越界引用标红计数（citation sanity check）、生成失败
降级检索列表；⑤ llama.cpp 交付运行时 10-12 真机 smoke 9/9 PASS：
embedding dim=1024 归一化对齐 0.15s/查，chat 接地引用 3.5-4.2s/答，
同机同模型较 Ollama（8.4s，实验档 docs/m4-generation-ab.md）快约
2 倍——"Ollama 原型 → llama.cpp 交付"闭环；产品默认 runtime 仍
Ollama，切换对比留 M5）。
十五份契约 + 243 条回归测试全绿（/W4），console + Qt GUI 双前端均已接入
**BM25 + 语义向量 + RRF 混合查询链**（qwen3-embedding:0.6b @ 本地
Ollama，A/B 实测拍板 10-11，服务不可达自动降级 BM25-only）；
**向量持久化 vector.bin（T9）落地**：per-doc path+content_hash
复用键——语料未变启动零网络秒上（145 向量 0.01s，冷嵌 20s），单文档
编辑只重嵌该文档，换模型自动识别全量重嵌（BAD_MODEL）。
M3 批 1 三卡全部落地
（T8 embedder · T5 Flat · T6 RRF，10-11）。
启动流程：validate
快照 -> 命中即 load（`index.bin` 缓存，损坏自动降级重建）-> 未命中
fresh build + save；**语料变动（CORPUS_CHANGED）时收获旧快照：
打一行 diff 报告（added/removed/edited）+ 按 path 继承墓碑 ->
重建**——"编辑被删文档"不再复活（复活通道 = 物理删文件重加）。
查询环支持 `list` / `del <id>`：删除 = 墓碑 + 立即重建 + save。
阶段复盘与复习材料：`docs/m1-m2-retrospective.md`。
`data\` **58 篇 writeup / 145 chunks**（npusec 校内赛 22 EN+CN 同源
互译 + ISCC 2026 赛后题解 36 含 mobile 类目，09-29 经 CORPUS_CHANGED
添加流程实弹入库；语料为赛后公开的真实题解，可整体替换为自己的
writeup 库），`query>` 交互查询（中英文均可）。测试一键跑：
`tests\run_tests.bat`。
构建环境既成事实（B/C 必读）：

- VS2022 项目设置：`/utf-8` + ISO C++17 + `/W4`，作用域=所有配置×所有平台
- 附加包含目录：`$(ProjectDir)include`（src/ 引用契约的路径来源）
- **源码注释纪律：纯 ASCII 英文**（破折号用 `--`，禁用 `—`、弯引号、
  省略号；VS 默认按系统代码页 GBK 存盘，非 ASCII 字符会埋雷）
- 可调参数一律 `constexpr` 集中放模块 .h，并登记 `docs/PARAMS.md`

**模型下载（本地推理，全程离线）**——先装 [Ollama](https://ollama.com/download)
（默认服务 `localhost:11434`），再拉：

```
ollama pull qwen3-embedding:0.6b   # 检索向量模型（639MB，M3 A/B 拍板）
ollama pull qwen3:4b-instruct      # 生成模型（2.5GB，M4 ③½ A/B 冻结）
                                   # ——不是 qwen3:4b！该 tag 现指向
                                   # thinking 变体（见 PARAMS.md）
ollama pull qwen3:1.7b             # 可选：fast mode 候选（1.4GB，A/B 落选方）
```

检索/生成服务不可达时自动降级（dense→BM25-only；生成→仅出检索列表），
模型不装程序照常跑，只是少了对应能力。

**llama.cpp 交付运行时（M4⑤ 真机验证过）**——资产在仓库外
`Desktop\llama.cpp\`（binary = GitHub llama.cpp b11564 win-cpu-x64；
GGUF = HF `Qwen/Qwen3-Embedding-0.6B-Q8_0` 609MB +
`bartowski/Qwen_Qwen3-4B-Instruct-2507-Q4_K_M` 2381MB）。起服务：

```
cd Desktop\llama.cpp
bin\llama-server.exe -m Qwen3-Embedding-0.6B-Q8_0.gguf --port 8101 --embedding --pooling last
bin\llama-server.exe -m Qwen_Qwen3-4B-Instruct-2507-Q4_K_M.gguf --port 8102 -c 8192
```

`smoke_llama.exe`（同目录，build_smoke.bat 可重建）链接产品真实
embedder/generator 源文件打活服务，9 项检查一键复验。

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
