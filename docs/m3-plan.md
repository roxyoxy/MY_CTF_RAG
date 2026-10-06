# M3 施工策略计划（初步定档版）

> 2026-09-29 定档。三源合一：老师线上反馈（性能优先导向）+
> GPT 3 意见（A 全文采纳）+ AI-A 补强三点。
> **正式计划待 2026-09-30 A 与老师线下确认后修订**（本文件届时
> 升级为正式版，或追加修订记录）。

## 0. 定位宣言

> **成熟模型负责"理解语义"，我们自己负责"向量怎么存、怎么找、
> 两路怎么融合"。**

老师 2026-09-29 线上反馈原文要点：

- 到目前为止就已经还不错了
- 引用别人训练好的模型也不是不可以
- **性能优先；重心从以自己手搓为主，转向以实用好用为主；
  手写必要算法为辅**（不影响效果、不涉及机器学习与训练）

这不是降低要求，是一次**成熟化**：真正的软件工程本来就不是
"凡是存在的轮子都重新造"，而是知道**什么是研究对象、什么是依赖**。

## 1. 新自研边界（正式目标）

| 层 | 怎么处理 | 状态 |
|---|---|---|
| tokenizer / BM25 / 切块 / 持久化 / 增量 / 删除 | 自研 | ✅ M1/M2 已交付 |
| embedding 神经网络模型 | **直接使用预训练模型**（公理②"用不训"） | M3 |
| 推理运行时 | **成熟组件**（Ollama → llama.cpp，M4 边界提前复用） | M3/M4 |
| HTTP/JSON glue | 工程接入，不做研究对象 | M3 |
| 精确向量检索（Flat） | **自研**（exact 基线 + 当前规模实用默认） | M3 |
| HNSW | **自研**（课程算法核心 + M5 实验对象，降位不放弃） | M3 后期 |
| RRF 融合 | **自研** | M3 |
| LLM | M4 直接集成成熟本地模型 | M4 |
| 评测 | 自己做 | M5 |

项目叙事从"从零手写版 RAG"升级为：

> **核心检索算法可解释、自研；模型推理采用成熟开源组件的
> 全离线 RAG。**

三条"不自研"红线（对全员）：不自研神经网络；不自研 Transformer
推理；不自己解析模型权重。这些写出来只会得到更差的版本。

## 2. 路线定档：丁案

| 路线 | 判断 |
|---|---|
| 甲：C++ 进程内集成模型推理（ONNX Runtime） | 弃。模型运行时/tokenizer 对齐/模型格式全提前拖进 M3，工程收益低（且 C++ 复刻 bge tokenizer 是著名深坑） |
| 乙：mock embedding | 不作产品路线；保留为测试用途（见 §5 T8） |
| 丙：Python sentence-transformers HTTP | 弃。给最终产品留下 Python 常驻服务依赖 |
| **丁：真实预训练模型 + 本地 Ollama HTTP + C++ 客户端** | **定档**。M4 本就规划 Ollama 原型 → llama.cpp 交付，M3 提前复用这条工程边界 |

```
C++ 检索程序 → localhost HTTP → 本地 embedding 服务 → float[]
```

保留的核心工程抽象不是某个模型，而是 **provider 可替换**：C++
不关心模型跑在 Ollama、llama.cpp 还是别的 runtime，只认一个
embedding API（Ollama `/api/embed`；llama.cpp server 有
OpenAI 兼容 `/v1/embeddings`，后期替换现实可行）。

与 AI-A 09-29 晨间方案（乙路线：HNSW 挂 M5 缓议）的差异及
统一：GPT 方案将 HNSW **降位保留**——从"产品必需"降为
"算法核心 + M5 实验对象"，施工排第 5 步（产品主线 1-4 步
先成型）。AI-A 附议并加一条**中止开关**（见 §8）：若 M4
工期挤压，HNSW 可整体顺延而不阻塞任何已交付能力——缓议思想
作为风险控制融入，而非砍掉。

## 3. 架构与数据流

```text
                    ┌── BM25 search ─────────────────┐
query ──────────────┤                                 │
                    │                                 ├─ RRF ─→ final top-k
                    │                                 │
                    └─ embed(query)                   │
                          ↓                           │
                    dense search                      │
                     ├─ Flat（exact 基线）             │
                     └─ HNSW（近似，后期）─────────────┘

build:
Chunk.text → embedding provider → vector<1024 float> → VectorIndex
             ├─ Flat registry
             └─ HNSW graph（从 vectors 重建，不先持久化）
```

**分层铁律**：embedding 与 vector search 彻底分层。HNSW/Flat
不知道什么叫 BGE、Qwen、HTTP、JSON——只认 `vector<float>`。
否则模型一换，索引代码跟着工程层一起炸（公理③的模块版）。

规模事实（为什么 Flat 是实用默认）：58 docs / 145 chunks，
1024 维 float32 原始向量 ≈ 0.57 MB；一次查询 Flat 全扫
145×1024 ≈ 14.8 万维乘加，现代 CPU 亚毫秒。**当前规模下真正
慢的是 query → embedding 模型推理，不是向量搜索本身。**
HNSW 价值门槛在 10^5~10^6 向量；Flat 同时是 exact ground
truth——HNSW 的 Recall 只能拿 Flat 对着量。

## 4. 持久化与快照设计

M2 的 `index.bin` 不动不膨胀。新增独立快照：

```
index.bin    BM25 / M2 快照（稳定，零改动）
vector.bin   dense vectors + 向量身份元数据（M3 新增）
hnsw.bin     HNSW 图（后议：145 点建图毫秒级，不值得缓存；
             语料扩到建图成本 × 使用频率 划算时再设计）
```

**vector.bin 身份四件套**（换模型 = 换坐标系，旧向量全部作废）：

| 字段 | 作用 |
|---|---|
| `model_id` | 模型身份（如 `bge-m3` / `qwen3-embedding:0.6b`） |
| `dimension` | 维度（两候选模型同为 1024，结构不膨胀） |
| `embedding_policy` | instruction/前缀策略成对冻结（Qwen3 是 instruction-aware：建库侧 passage 前缀 × 查询侧 instruction 前缀，必须成对入档，不许单边改） |
| `VECTOR_PIPELINE_VERSION` | 我们自己的版本旋钮 |

M2 的 `PIPELINE_VERSION` 设计（"embedding 模型变化也要 bump"
的伏笔）在这里正式兑现。

**向量增量复用（AI-A 补强，GPT 未展开）**：145 chunks 全量重嵌
= 数百次本地推理调用，不能每次语料变动都全量付。设计：

- vector.bin 按 **doc 组织**：`path → content_hash → vectors[]`
  （该文档所有 chunk 的向量，按 chunk 顺序）
- CORPUS_CHANGED 时与 corpus_diff 报告对齐（双指针，同
  inherit_tombstones 模式）：**未变 doc 直接复用全部向量；
  added / edited doc 全量重嵌**（edited doc 的 chunk 边界会漂移，
  doc 级复用简单正确；chunk 级复用是过度优化，不做）
- content_hash 复用 FNV-1a 先例

派生数据保存判据第三次应用：BM25 不存（重建毫秒级）→ 倒排索引
存（重建秒级）→ 向量存（重嵌分钟级 + 外部调用）。判据还是那一条：
**推导成本 × 使用频率**。

## 5. 施工六步（严格按序）

每步交付即可验收，前 4 步完成 = 产品能力成型（首次拥有真正的
语义检索 + 混合检索），第 5 步是纯增量。

| 步 | 内容 | 产出 | 验收 |
|---|---|---|---|
| 1 | embedding provider 契约 + Ollama 接通 | `include/embedder.h`（第九契约）+ 客户端 | 输入 UTF-8 文本输出定长 float[]；model_id/dimension/normalized 可查询；真模型冒烟 |
| 2 | 模型 A/B 实验拍板 | 实验数据 + 拍板记录 | bge-m3 vs qwen3-embedding:0.6b，用**自己的中英混合 CTF 查询集**跑 Recall，数据说话（判据 A 主导，见 T10） |
| 3 | Flat dense retrieval | `include/vector_index.h`（第十契约）+ Flat 实现 | exact 语义检索可试"意思一样、字面不同"的查询；黄金测试 |
| 4 | RRF 接入 + 混合查询链 | `include/rrf.h`（第十一契约）+ main/GUI 接线 | BM25 + Dense + RRF 全链，**人工效果验收** |
| 5 | 自研 HNSW（**Go/No-Go 评审点**） | `include/hnsw.h`（第十二契约）+ 实现 | Recall@10 vs Flat 真值 ≥ 阈值；三参数可讲清 |
| 6 | M5 决定生产默认 | 实验矩阵 | BM25 / Dense-Flat / Dense-HNSW / Hybrid 按 recall/nDCG/latency/memory 裁 |

RRF 公式维持立项定档（AI-BRIEFING Q6）：

```
RRF(d) = Σ 1 / (k + rank_r(d))，k = 60
```

只融合**排名**不融合分数（BM25 8.73 与 cosine 0.81 不同尺度，
直接相加无数学意义）；零调参；模块极小测试极易写死。

模型 A/B 候选理由：qwen3-embedding:0.6b（639MB，1024 维，
100+ 语言，**官方强调 code retrieval**，32K ctx，
instruction-aware，Ollama 现成）vs bge-m3（567M 同量级，1024 维，
多语老牌）。CTF writeup 不是普通中文语料——中文 + 英文 + 汇编 +
函数名 + payload + 代码片段，code retrieval 权重天然高。GPT 押
Qwen3 先试、bge-m3 对照；**最终用自己查询集的数据拍，不用印象拍**。
答辩词就此升级为："我们在同一份中英混合 CTF 数据集上对两个同量级
本地模型做过检索实验，选了 Recall 更好的那个"——这句话同时就是
M5 的开场白。

## 6. B/C 任务卡（初步拟定，正式版待 09-30 后下发）

分工原则沿用历史裁定：**被 M5 评测的算法由 B 写到骨头里；
数据入口与外部系统的胶水由 C 承担**（对口其 M4 LLM 后端角色）。
老三步不变：契约 .h → 伪代码卡 → 真代码；分支 + PR 流恢复。

### B（检索算法侧，T5-T7）

**T5 · vector_index.h 第十契约 + Flat 实现**
- 契约要点：VectorIndex 抽象接口（build + search）；cosine
  相似度；top-k；tie-break 沿用 M1 惯例（同分 chunk_id 升序）
- 实现自由度：第一版朴素直扫跑通（归一化缓存等优化走提案制，
  M5 兑现）
- 测试：手算小向量集黄金表 + 边界（零向量 / 空索引 / k > N）

**T6 · rrf.h 第十一契约 + 实现**
- 公式逐字实现 k=60；输入两路（或多路）排名表，输出融合排名
- 测试：两路手算黄金 + 单路退化 + 空输入 + 未上榜文档处理

**T7 · hnsw.h 第十二契约 + 自研实现（M3 后期大件）**
- 核心件：分层图 / 随机层级 / 贪心入口 / best-first 搜索 /
  候选堆；`M` / `efConstruction` / `efSearch` 三参数登记
  PARAMS.md（分别影响图连接度 / 建图质量 / 查询宽度——efSearch
  是 recall-latency 的直接旋钮）
- 测试设计（本卡灵魂）：**Recall@10 对照 Flat 真值 ≥ 阈值**
  （如 0.95）——HNSW 的测试就该这么写，Flat 就是它的标准答案
- 验收附加条款：能对 A 复述三参数各自影响什么（答辩核心，讲不出
  = 没写到骨头里）

### C（数据入口侧，T8-T10）

**T8 · embedder.h 第九契约 + Ollama HTTP 客户端**
- 契约要点：`embed(texts) → vector<vector<float>>`（批量，
  Ollama /api/embed 支持多输入）；model_id / dimension /
  normalized 可查询；超时与失败重试策略
- 接口设计须允许**注入 fake provider**（乙案 mock 的唯一正当
 用途）：单元测试不依赖 Ollama 运行，真服务只做冒烟——离线
  测试原则
- HTTP/JSON glue 选型（任务卡设计时定）：倾向 vendor 单头库
  （cpp-httplib + nlohmann/json，MIT，零构建侵入）——按新叙事
  glue 不是研究对象；vendor 进 `third_party/` 并登记许可证

**T9 · vector.bin 持久化（vector_persist.h 第十三契约，或并入
persist.h 扩展——契约议题，发卡前裁）**
- 身份四件套（§4）+ 按 doc 组织（path + content_hash →
  vectors[]）+ diff 驱动增量复用 + 原子替换沿用 M2 模式
- 测试：身份失配（换 model_id / 改 policy / bump 版本）必须
  判废；复用命中 / 未变 doc 不重嵌的对照测试

**T10 · A/B 实验工具（C 支撑，A 主导判据与拍板）**
- 同一查询集 × 两模型 → Recall@k / nDCG 对比表
- 查询集建设：现有 5 条主题查询扩到 ~20 条中英混合（含代码
  类查询，如函数名 / payload 片段），登记入册成为 M5 复用资产

### A（本人）

契约设计主持（第九至十三契约逐份聊透）、A/B 判据定义与拍板、
各步验收、GUI 语义/混合检索页（尾部，GUI 是契约的新消费者）、
M5 实验矩阵预规划。

## 7. 排期量级（粗估，正式版明天定）

| 步 | 量级 |
|---|---|
| 1 embedder + 接通 | 2-3 天（含环境） |
| 2 A/B 拍板 | 1-2 天（依赖查询集） |
| 3 Flat | 2-3 天 |
| 4 RRF + 混合链 + 人工验收 | 2-3 天 |
| 5 HNSW | 2-3 周（大件） |
| 合计 | 4-6 周（与课业节奏合并考虑） |

前 4 步 ≈ 1.5 周，产品能力即成型；第 5 步独立可顺延。

## 8. 风险与开关

1. **HNSW Go/No-Go（第 5 步入口评审）**：若 M4 工期挤压或
   1-4 步返工超支，HNSW 整体顺延（M4 后或 M5 前），不阻塞
   任何已交付能力——产品默认 Flat 本来就是当前规模最优解。
   顺延 ≠ 取消：缓议书模式（incremental-deferral 同款），
   触发条件与回归时间入册。
2. **Ollama 离线交付**：`ollama pull` 需联网。比赛/交付预案：
   提前在有网机器 pull 后整目录拷贝，或本地 gguf + Modelfile
   导入，或 M4 的 llama.cpp server 直接顶上（provider 可替换
   的抽象红利）。T8 验收含一条离线场景冒烟。
3. **A/B 平局风险**：两模型差距不显著时选 bge-m3（更老牌、
   社区验证多、无 instruction 复杂度）；差距显著则数据说话。
4. **挂账清算（发卡前必须）**：① persist.cpp 走读 ② 卡缝五处
   ③ 黄金测试 3/8/12 构造思路 ④ corpus_diff 双指针走读——
   03:27 裁定，M3 发卡 = 清算窗口，A 的学习债不过夜至施工中段。

## 9. 明日老师沟通议题清单（A 线下弹药）

1. M3 范围：HNSW 自研保留（降位为算法核心 + M5 实验对象，
   产品默认 Flat）是否符合课程期望？4-6 周工期可否？
2. embedding 引用预训练模型、A/B 实测定板的路线是否认可？
3. M3 提前复用 M4 的 Ollama 边界（embedding 先走本地服务），
   老师有无异议？
4. 结题/答辩时间节点与 M5 实验报告的最低要求？
5. 项目定位表述："核心检索算法自研 + 模型推理采用成熟开源组件
   的全离线 RAG"——老师认可后落 PRINCIPLES 修订
6. （可选）比赛离线交付形态偏好：Ollama 常驻 vs llama.cpp
   server 单二进制

## 10. 待正式定档事项（09-30 录音回来后）

- [ ] 本计划升级正式版（老师意见合并）
- [ ] PRINCIPLES.md §1 叙事修订落盘（提案已备：自研边界表）
- [ ] README 标题与 §1 理由 2 复核（今日已按新叙事更新）
- [ ] T8-T10 契约编号与归属终裁（vector_persist 独立 vs 并入）
- [ ] B/C 任务卡正式下发（发卡前挂账清算 ①-④）
- [ ] M5 实验矩阵预注册（四路对比 + 参数网格）
