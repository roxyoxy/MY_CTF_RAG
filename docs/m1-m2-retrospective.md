# M1-M2 阶段总结（复习专用）

> 2026-09-29 · 项目进度过半（M0-M2 全部收官，M3 定档）。
> 本文档为 A 的复习材料：项目走到哪、交付了什么、每块的核心
> 设计与答辩点、沉淀了哪些可迁移的方法论。
> 复习路线建议：先 §1 速览 → §3 逐卡片过（对着契约原文）→
> §4 方法论（M3 直接复用）→ §6 挂账（发卡前要清）。

## 1. 一页速览

**项目**：完全离线的 CTF RAG 系统——检索层核心算法自研
（BM25 / bigram 分词 / 切块 / 持久化 / 增量 / 删除 / 向量 /
RRF），模型推理采用成熟开源组件。3 人组，C++17 + VS2022。

**一句话管线**：

```
data/ 语料 → loader 递归加载 → chunker 切块(500/50) →
tokenizer/atom_scan 分词(中 bigram) → indexer 建倒排账本 +
BM25 查账 → persist 快照缓存(index.bin) →
console / Qt GUI 双前端查询
```

**关键数字仪表盘**：

| 指标 | 演化 | 终值 |
|---|---|---|
| 契约（include/*.h） | 5 → 6 → 7 → 8 | **8 份**（type/loader/chunker/tokenizer/indexer + atom_scan/persist/corpus_diff） |
| 回归测试 | 37 → 50 → 75 → 97 → 100 → 114 | **114 条全绿**（/W4 零告警） |
| 语料 | 12 → 24 → 58 docs | **58 篇 / 145 chunks**（六类目含 mobile） |
| 前端 | console | console + **Qt GUI**（F5 直达，VS 原生集成） |
| 里程碑 | M0(09-22) → M1(09-28) → M2(09-29) | **M0-M2 全部收官**，M3 定档 09-29 |

## 2. 时间线大事记

| 日期 | 事件 |
|---|---|
| 09-22 | M0：文档体系立项（README / PRINCIPLES / MEETING_LOG / TASKS），蓝图评审 AI-BRIEFING.md |
| 09-23 | M1 第一轮：五份契约 .h 全竣工；第二轮：main.cpp 总装（预留 LNK2019×4 中间态） |
| 09-26 | tokenizer 词内规则升级三明治规则（'.' 与 '_' 左右均为字母数字时入词） |
| 09-27 | T1 tokenizer.cpp · PR #1 合并（B/C 分工 + PR 流首跑通） |
| 09-28 | **大日**：T2 PR #2；T3/T4 B 离线 zip 交接 A 代提交，LNK2019 清零端到端跑通；第四轮 tests 37 条 + 优化遍历，**M1 收官**；当天连打 M2① 中文 bigram 四步（⓪契约五件套→①scanner/tokenizer/chunker→②三层测试→③语料重切验收）全竣工；语料 12→24（CN 同源互译） |
| 09-29 凌晨-上午 | M2-② persist（第七契约，测试 97）；M2-③ corpus_diff（第八契约，114 条）+ ctf-wiki 34 篇实弹入库（58/145 定格）；M2-④ 墓碑消费（删除路径）；Qt 外包回流拼装 + VS 原生集成；A F5 验收通过 + push（132b42b），**M2 全线收官** |
| 09-29 午后 | 老师反馈（性能优先/实用为主/引用成熟模型）→ **M3 路线初步定档（丁案）**，计划见 m3-plan.md |

## 3. 核心模块复习卡片

每张卡：干什么 / 关键设计 / 答辩一句话。

### 3.1 atom_scan（第六契约，甲″单一事实源）

- **干什么**：一次扫描 UTF-8 字节流，产出 Atom 表
  （{begin, end, kind: ASCII_WORD | CJK_CHAR}，begin/end 均为
  字节偏移）
- **关键设计**：tokenizer 和 chunker 的分词规则**唯一事实源**——
  两边都调 scan_atoms，规则想不一致都做不到。三单位拆分：
  **atom ≠ token ≠ byte**（一个汉字 3 字节 = 1 atom = bigram 的
  一半）。汉字判定 U+3400-4DBF + U+4E00-9FFF **先解码后分类，
  禁 lead-byte 猜**（猜错会把畸形字节序列吞邻）；ASCII 词内
  字符显式 [A-Za-z0-9-]
- **答辩点**："为什么单独一份 scanner 契约？"——因为分词规则
  出现过两处消费者（tokenizer 出 token，chunker 出字节位置），
  规则重复 = 未来漂移的 bug 工厂；单一事实源让"规则"本身成为
  契约

### 3.2 tokenizer（bigram 组合层）

- **干什么**：atom 序列 → token 序列。ASCII 原子直通；相邻
  CJK 原子两两组合 bigram
- **关键设计**：两端同源铁律（建索引与查询同一 tokenize，公理③
  词项版）；bigram 两段式构造不依赖"3 字节一汉字"不变量（畸形
  输入下 atom 已由 scanner 保证边界）；孤字丢弃、不跨 ASCII、
  标点断 run、计数不加权
- **答辩点**："为什么 bigram 不用 jieba？"——bigram 零词典零
  训练、O(n) 确定性、对 CTF 术语（函数名/payload 混中文）反而
  更稳；分词精度损失由 M3 向量检索补——**这句直接衔接 M3 叙事**

### 3.3 loader（谁生产谁编号）

- **干什么**：递归遍历 data/ → 字典序排序 → 再编号 id
- **关键设计**：**先排序后编号**保证同语料两次加载 id 恒定
  （chunk_id = chunk_lengths 下标这条红线的前提）；空目录返空
  不 throw，目录不存在才 throw；std::filesystem C++17 标配
- **答辩点**：id 稳定性是整个账本体系的地址系统——中间插入
  文件会让字典序下标连锁位移，这正是 M2-③ 全量重建路线的
  根因（见 3.6）

### 3.4 chunker（字节滑窗 + 墓碑闸门）

- **干什么**：Document → Chunk，500 atom 窗口 / 50 atom 重叠
- **关键设计**：**数 atom 不是数字符**（字节偏移才是 UTF-8 安全
  的定位单位）；块边界 ≠ 下块起点（重叠 50，chunk.end 恒为
  atoms[e-1].end，Q4 裁决）；**条款 6 墓碑闸门**：deleted=true
  的文档在源头就不产块（下游零过滤，删除语义单点收口）；
  空文档产 0 块
- **答辩点**：为什么重叠？——500/50 让跨块边界的关键词对
  （如函数名被切断）在重叠区至少完整出现一次；为什么 atom
  计数？——中英文等权，一个汉字块长膨胀 3 倍的 bug 不会发生

### 3.5 indexer（账房 + 查账台，M5 被测对象）

- **干什么**：build_index 建倒排账本（postings 按 chunk_id
  升序、chunk_lengths 下标即 chunk_id——**账本格式红线**，
  search/持久化/M3 RRF 三方消费）+ search BM25
- **关键设计**：BM25 公式契约逐字实现（k1/b 登记 PARAMS.md）；
  整数除法陷阱（total/块数先转 double）；排序 score 降序、
  **同分 chunk_id 升序**（tie-break 契约条款）；top_k<=0 守卫
  返空；partial_sort O(m log k) 与 scores 稠密向量化两提案
  成册挂 M5（量级关过不了的不动，规则 3）
- **答辩点**："你这个 BM25 和标准实现差在哪？"——dl 以 token
  数计（bigram 口径）、中文不补偿（M5 实验项）；优化克制本身
  是答案：六函数遍历全"不动"，因为 N=145 时量级关全过不了

### 3.6 corpus_diff（双指针对账 + 墓碑继承）

- **干什么**：两代语料 diff（added/removed/edited）+ 旧墓碑
  向新一代继承
- **关键设计**：输入双有序（loader 序）→ 双指针 O(n+m) 无内部
  排序；身份键 = **path**（不是 id——id 会因中间插入连锁位移，
  path 是文件的真名）；**按内容失明继承**（old.deleted &&
  !new.deleted 则继承）——"编辑被删文档不复活"是这个语义的
  直接推论（复活通道 = 物理删文件重加，两层删除语义入册）
- **答辩点**：为什么不做真增量（乙/丙/乙′三案死因）？——真增量
  与打包 id 互斥（chunk_lengths 红线 + 重编号级联 O(账本) ≈
  重建成本），全量重建在 145 chunks 是毫秒级，缓议书
  docs/design/incremental-deferral.md 留 M5 复议（语料×1000）

### 3.7 persist（快照 = 缓存，不是数据库）

- **干什么**：index.bin 的 save / validate / load，哑三件
- **关键设计**：24B 头（magic + FORMAT_VERSION + PIPELINE_VERSION
  + 统计量）+ FNV-1a 载荷指纹；**八态**（OK / IO_ERROR / 坏魔数 /
  坏版本 / 坏指纹 / 截断 / 尺寸不符 / ……）；**损坏 = cache
  miss**（降级重建，绝不 crash——快照是派生数据，命丢了才是
  事故）；原子替换（tmp + rename，断电不留半具尸体）；
  FORMAT（格式怎么变）与 PIPELINE（语义怎么变）**双版本维度**——
  "embedding 模型变化也要 bump" 的伏笔 M3 兑现
- **答辩点**："为什么不存 SQLite？"——我们的需求是"一个可校验
  可整体作废的字节缓存"，自定义二进制逐字段编解码 60 行写完，
  还白得一个字节级确定性（同输入必同输出，测试可写死）

### 3.8 删除语义（M2-④，两层设计）

- **逻辑删除** = 墓碑文件留 data/ + 立即重建 + save（快照运行时
  永不两态，"删除是事实不是约定"）
- **物理清除** = 删文件重启自动完成（diff 报 removed）
- "编辑即复活"是已知边界（A 签字，path 继承的内容失明推论）

### 3.9 Qt GUI（八契约的纯消费者）

- **干什么**：语料树（类目分组/墓碑删除线）/ 查询框 / 结果表
  （Rank/Score/Snippet）/ 预览（字节→QChar 高亮）/ 日志 /
  状态栏
- **关键设计**：GUI 不新增任何算法——同一套七份核心 .cpp 链接
  进 console 与 GUI 两个工程（双入口无 LNK2005）；VS 原生
  QtVS_v302 工程（QT6 注册 + QtDeploy 自动部署 + cwd 内置
  ../），F5 直达；无 .ui/Q_OBJECT → 零 moc/uic
- **关键实现课**：byte→QChar 换算（QString::fromUtf8 分段解码
  [0,begin) 与 [begin,end)，.size() 即 UTF-16 单位 = 光标位置
  严丝合缝）；snippet 左截 5 步（left(k-3) 留 "..." 三位）

## 4. 方法论沉淀（M3 直接复用的资产）

1. **老三步**：契约 .h（WHAT 冻结）→ 伪代码卡（HOW 草案）→
   真代码（履约）。M3 恢复 B/C 分工，此流程原样重启
2. **契约三层规则**：签名/字段/常量语义绝对冻结；函数内部完全
   自由；发现契约 bug 停手入纪要全组议——M3 五份新契约
   （第九至十三）沿用
3. **测试红先审测试**（三例入册）：测试红时先审测试自身缺陷
   再动实现（persist 七坏一好矩阵首红即测试自伤）
4. **派生数据保存判据 = 推导成本 × 使用频率**：BM25 不存 →
   索引存 → M3 向量存（重嵌分钟级）→ hnsw.bin 后议——同一条
   判据用了三次
5. **缓议书模式**：规模不支持的技术挂缓议线 + 触发条件入册
   （incremental-deferral 先例；M3 的 HNSW Go/No-Go 同款思想）
6. **优化三规则**：先跑通再优化（正确性是入场券）；小优化直接
   做、大优化（换结构/换算法）写提案 A 裁；每处优化答得出
   "为什么比基础版好"（量级分析，不接受感觉快）——M5 兑现
7. **PR 流 + 降级预案**：分支 → PR 描述（自测结果/优化清单/
   为什么）→ A 本地验收合并；B 梯子故障时离线 zip 交接 A
   代提交（先例 09-28）
8. **AI 协作减速带**：AI 不主动写码、被点名才给那一段、修改
   留痕、commit/push 逐次批准——M2 两例赶时间模式（persist.cpp
   /corpus_diff.cpp AI-A 执笔）均为 A 明示授权 + 挂学习债

## 5. 语料资产

| 批次 | 规模 | 说明 |
|---|---|---|
| npusec 12 篇（EN） | 12 docs / 29→30 chunks | 本人真实比赛 writeup，六类目 |
| +12 篇 CN 同源互译 | 24 docs / 77 chunks | 中文全文检索开通（甲案） |
| +ctf-wiki 34 篇 | **58 docs / 145 chunks** | 单题 WP（总 WP 重复排除），mobile 新类目，走 CORPUS_CHANGED 实弹入库 |

M5 复用：5 条主题查询（将扩至 ~20 条，T10）+ 145 chunks 基线。

## 6. 挂账清单（M3 发卡前清算，03:27 裁定）

| # | 挂账 | 来源 |
|---|---|---|
| ① | persist.cpp 实现层走读 | M2-② AI-A 执笔赶时间，A 亲手学习债 |
| ② | 卡缝五处 | persist 施工留痕 02:19 |
| ③ | 黄金测试 3/8/12 构造思路 | 同上 |
| ④ | corpus_diff 双指针走读 | M2-③ 同款执笔，A 亲手学习债 |

## 7. 下半场展望（一句话版）

M3（定档 09-29，计划 docs/m3-plan.md）：Ollama 本地 embedding
（bge-m3 vs qwen3-embedding A/B 实测定板）→ Flat 精确向量检索
（自研，当前规模即性能最优）→ RRF 混合（自研 k=60）→ HNSW
自研（算法核心 + M5 实验对象，Go/No-Go 可控）→ M4 本地 LLM
（Ollama 原型 → llama.cpp 交付）→ M5 四路对比实验 + 答辩。
