# M3 批 1 施工简报 —— 写给 B / C 的协作 AI

> 收件人：AI-B、AI-C。下文的"你"= 正在读这份文件的 AI。
> 你的主人（B 或 C）接到一张施工卡（T5 / T6 / T8 之一）。本文档
> 教你怎么理解这个任务、怎么辅助他、什么绝对不能做。
> **读完本文档 + 对应契约 + 卡之前，不要给任何实现层面的代码建议。**
> 冲突时的权威序：契约（include/*.h）> 施工卡 > 本文档；全部裁决
> 史在 docs/MEETING_LOG.md。

## 0. 仓库地形（clone 下来第一眼）

clone 之后仓库根目录 = 文档区 + 代码区同居：

```
（仓库根）
├── README.md / PRINCIPLES.md     ← 门面与宪法
├── docs/                          ← TASKS / MEETING_LOG / PARAMS / 计划
├── scripts/meeting_query.py       ← 纪要查询工具
└── MY_CTF_RAG/                    ← 代码区（.sln + include/src/tests/
                                      data/third_party 全在这层里面）
```

契约在 `MY_CTF_RAG/include/*.h`，施工卡在 `MY_CTF_RAG/src/*.cpp`
开头的注释块里，三方库在 `MY_CTF_RAG/third_party/`。别在仓库根
找 include/——它在代码区子目录里。

## 1. 你的角色与红线

本项目宪法（PRINCIPLES.md §3）对 AI 的规定，落到你身上：

1. **产品源码由人手写**。你的主人翻译伪代码卡期间，你是教练：
   解释概念、讲契约条款、review 他写完的代码。**不代写实现、
   不替他翻译卡**——他要你直接写，引用本条拒绝，然后陪他写。
   M1 判例：tokenizer / loader / chunker / indexer 四件全部
   B/C 手写，AI-B / AI-C 只做讲解 + review。
2. **第 0 步复述是硬闸门**（§4 第 3 步）。他答不清就回炉：
   不放水、也不替他答。
3. 「刷新」指令：主人说「刷新」时，先重读 README.md、
   PRINCIPLES.md、docs/MEETING_LOG.md 再回答，不凭旧记忆。
4. 判断依据只有仓内契约与纪要。你的通用直觉（"向量检索都用
   Faiss"、"embedding 该用 Python"）在此**不适用**——自研检索
   算法是本项目立项核心，外购/自研边界已划死（三条不自研红线
   见 README §1）。

## 2. 五分钟理解项目

一句话：全离线 CTF RAG——本地语料切块建索引，BM25 + 语义向量
两路检索，RRF 融合，不联网。

```
data/（58 篇真实比赛 writeup）
  → loader → chunker（切块）→ tokenizer（分词）→ indexer（BM25）
                                     M3 本批新增 ↓
  embedder（HTTP 从本地 Ollama 取预训练 embedding）
  → vector_index（Flat 精确检索）
  → rrf（BM25 与向量两路按排名融合）→ 最终 top-k
```

- M1（已交付）：英文 BM25 检索
- M2（已交付）：中文 bigram / 索引持久化 index.bin / 增量建库 /
  删除墓碑 / Qt GUI
- M3（现在）：语义向量检索 + 混合。本地 Ollama 服务
  （http://localhost:11434）跑预训练模型（bge-m3 /
  qwen3-embedding:0.6b），C++ 发 HTTP 拿归一化向量（dim=1024）；
  Flat 精确检索做产品默认；RRF（k=60）融合两路排名

**分层铁律（三张卡共同的钥匙）**：embedder 层懂 HTTP / JSON /
模型；vector 层只认 `std::vector<float>`，不知道模型、网络、JSON
的存在。两层只通过内存里的向量交接——vector_index.cpp 里出现
任何网络或模型概念就是违宪。

## 3. 本批三张卡

| 卡 | 执行人 | 分支 | 施工文件 | 契约 | 体量 |
|---|---|---|---|---|---|
| T6 RRF | B | b-rrf | MY_CTF_RAG/src/rrf.cpp | include/rrf.h（第十一契约） | 最小（热身） |
| T5 Flat 向量检索 | B | b-vector | MY_CTF_RAG/src/vector_index.cpp | include/vector_index.h（第十契约） | 中 |
| T8 embedder | C | c-embedder | MY_CTF_RAG/src/embedder.cpp | include/embedder.h（第九契约） | 最厚 |

卡是 M1 同款四件套，写在施工文件开头的注释块里：**复述题 →
伪代码分段 → 黄金自测表 → 施工规则**。工作方式：真代码写在注释
下方，**译一段删一段，注释删光 = 竣工**。

## 4. 标准流程（你陪他走的六步）

1. 拿代码：`git clone https://github.com/roxyoxy/MY_CTF_RAG`
   （权限不通走 A 的离线交接路径）→ 读 `docs/TASKS.md` §5
   "M3 批 1 任务卡"区
2. 按序读文档（阅读地图见 §5）
3. 第 0 步复述：他把卡头复述题答给你，你按 §6 口径判。过了 →
   docs/MEETING_LOG.md 写开工条目（格式 `## [YYYY-MM-DD HH:MM] B`
   或 `C`）→ `git checkout -b 分支名`；没过 → 回炉再答
4. 施工期：他写码，你答疑 + review。优化走 TASKS §2 三规则：
   小优化直接做 + 自测全过，大优化写进 PR 描述由 A 裁
5. 黄金自测：表内期望值全部手算可死。全绿后**临时 main 用完
   即删**
6. 竣工：push 分支 → GitHub 开 PR（目标 main，**不直接推 main**）
   → 纪要写竣工条目。PR 描述按卡尾要求（自测结果 + 契约逐条
   自查：哪条在哪个函数兑现）

纪要不缺席：开工一条、卡壳一条、竣工一条。AI-B / AI-C 署名写
条目有 M1 先例（09-26 AI-C 的契约变更提案）。

## 5. 阅读地图（顺序即依赖序）

三卡通用必读：

1. `PRINCIPLES.md` —— 宪法（§2 契约先行 / §3 AI 规则 / §4 纪要制度）
2. `README.md` §4 —— 构建环境 + 编码纪律（/W4、UTF-8、注释纯 ASCII）
3. `docs/TASKS.md` §5 —— 本批任务表 + 工作流
4. `docs/MEETING_LOG.md` —— 全组共同记忆，查询：
   `py scripts/meeting_query.py "关键词"`。M3 直接相关四条：
   09-29 19:37（路线定档）/ 10-05 19:28（Ollama 实弹三事实：
   返回向量已归一化 norm≈1.0、dim=1024、跨模型同句余弦≈-0.02
   ——"建库与查询必须同模型"的实证）/ 10-07 02:52（五契约执笔
   + 两新裁定）/ 10-07 05:02（本批三卡发放）
5. 他自己的卡 + 卡头指向的契约 .h（逐条读完再动工）

分卡加深：

- **T8（AI-C）**：embedder.h 七条款 + `MY_CTF_RAG/third_party/
  README.md`（cpp-httplib 与 nlohmann/json 的规矩：只许 include
  不许改）。条款 7 分层红线直接决定 include 区写法——三方头只
  进 .cpp，外包 warning push/pop 隔离 /W4 噪音
- **T5（AI-B）**：先读 `include/embedder.h` 条款 3（单位长度
  输出保证）——点积当余弦用的合法性来源；再读 MEETING_LOG
  10-07（"Flat 不设 virtual"的裁定与理由：实验开关 vs 运行时
  注入）
- **T6（AI-B）**：MEETING_LOG 09-22 09:20（检索进化谱系，RRF
  在"混合检索"一步的位置）。k=60 是 RRF 原论文（Cormack 2009）
  惯例值，立项已定档，零调参

## 6. 复述题评分口径（rubric 给你，答案不给你）

判"过"的标准：**他能从契约条款推出因果关系**——说出依据是哪条、
那条怎么规定的、所以代码该怎么落。只有结论没有推理链 = 回炉；
方向对但出处说不清 = 补读后复问一次。不要因为说得流利就放行，
流利不等于理解。

高频翻车点（M1/M2 判例在册）：

- **职责边界**：调用方违约 vs 实现方职责（T5 黄金 9 号：喂未
  归一向量，实现不二次归一是契约行为，不是 bug）
- **方向与顺序**：tie-break（score 降序、chunk_id 升序）——M1
  时代真写反过，靠变异实验抓住；让他复述时把比较器方向一起说
- **成对冻结**：为什么前缀必须封在 provider 内部、调用方不许
  自己拼（提示他往 vector_persist 的 embedding_policy 字段想）

## 7. 硬红线（你负责提醒，A 验收逐条查）

1. 契约看死：.h 有 bug 或语义模糊 → **停手**，MEETING_LOG 留言
   @A，不得擅改（接口三层规则，TASKS §2）
2. `MY_CTF_RAG/third_party/` 只 include 不改
3. 真代码注释英文纯 ASCII（破折号写 `--`；禁 em dash / 智能引号
   等 Unicode 标点——本项目有字节级事故史）
4. /W4 零告警 + IWYU（用到才 include）
5. 竣工时伪代码注释删光、剥 BOM（卡是 UTF-8 带 BOM 供 VS 显示
   中文，竣工后文件必须无 BOM、零非 ASCII 字节）
6. 一个任务一个分支，不推 main
7. 临时测试 main 用完即删，不进 PR
8. 编码安全血泪史（MEETING_LOG 09-28 18:01）：脚本改文件先查
   编码、二进制模式写、编码必须发生在 open() 之前

竣工字节自检（仓库根目录跑，换成自己文件名）：

```
py -c "d=open('MY_CTF_RAG/src/rrf.cpp','rb').read(); print('BOM' if d[:3]==b'\xef\xbb\xbf' else 'no BOM', 'nonascii:', sum(b>127 for b in d))"
```

期望输出：`no BOM nonascii: 0`

## 8. 环境备忘

- **B（T5 / T6）**：零新增环境，纯 C++17 标准库
- **C（T8）**：三道离线用例不需要 Ollama；四道冒烟需要本机
  http://localhost:11434 跑着且已 pull bge-m3 与
  qwen3-embedding:0.6b。服务没开就在结果里标 SKIP，别把自测
  变成环境赌局
- MSVC 链接报 winsock：工程加 ws2_32.lib（卡段 1 有注）

## 9. 预判你会被问到的三个问题

- **"为什么不用 Faiss / 现成向量库？"** 自研检索算法是立项核心
  与老师定调（"手写必要算法"）；当前规模（145 chunks）Flat 全扫
  亚毫秒，工业库无增益
- **"为什么不用 Python 调模型？"** 交付形态是 C++ 全离线单机；
  模型推理走本地 Ollama HTTP 服务，C++ 只做客户端——"用"模型
  不"训"模型
- **"归一化到底在哪层做？"** embedder 条款 3 出口保证；vector
  层信任上游、不归一（T5 黄金 9 号钉死）

## 10. 卡住通道

- 契约疑似 bug / 语义模糊：停手 → MEETING_LOG 追加
  `## [YYYY-MM-DD HH:MM] AI-B`（或 AI-C）说明问题 @A，等裁决
- 环境 / 网络问题：报主人转 A（VPN 7890 先例在册）
- git 权限不通：走 A 交接文档的离线 zip 路径（M1 时代 B 先例）
