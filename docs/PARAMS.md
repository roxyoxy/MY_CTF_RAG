# PARAMS — 可调参数登记表

> 规矩（2026-09-23 A 定）：**凡新写代码时引入的可调参数，当场登记一行**。
> M1 只拍默认值保证能跑；真正的调参实验是 M5 的正式工作。
> 参数本体住各自模块的 .h（constexpr），本表是全项目总账。

## 参数总账

| 参数 | 默认值 | 住址 | 含义 | M5 调参方向 |
|---|---|---|---|---|
| CHUNK_SIZE | 500 atoms | chunker.h | 每块原子数上限（1 atom = 1 个 ASCII 词或 1 个汉字） | 大块上下文全但词频被稀释；小块反之；中文块字节更轻（汉字 3B/atom vs 英文词 ~6B/atom），M5 复核均衡 |
| CHUNK_OVERLAP | 50 atoms | chunker.h | 相邻块重叠原子数 | 惯例 10–20%，跨刀口句子完整性 vs 冗余存储 |
| BM25_K1 | 1.2 | indexer.h | TF 饱和速度 | 惯例 1.2–2.0；越大饱和越慢 |
| BM25_B | 0.75 | indexer.h | 长度归一化强度 | 0（不看长度）– 1（全额归一） |
| TOP_K | 10 | indexer.h | 检索返回条数 | 受 M4 LLM 上下文预算约束 |
| EMBEDDER_DEFAULT_ENDPOINT | http://localhost:11434 | embedder.h | 本地 embedding 服务地址（provider 可替换的物理落点） | M4 换 llama.cpp server 时更改 |
| EMBEDDER_DEFAULT_TIMEOUT_SECONDS | 180 | embedder.h | 单次 HTTP 调用超时（建库批量 CPU 推理慢，给足） | GPU 常驻可收紧；M5 latency 记账 |
| RRF_K | 60 | rrf.h | RRF 排名融合阻尼 | 立项定档零调参（AI-BRIEFING Q6）；M5 可扫 10-100 |
| HNSW_M | 16 | hnsw.h | 每层每节点最大连接数（图连接度） | 惯例 8-48；大 M 图肥建慢 recall 升 |
| HNSW_EF_CONSTRUCTION | 200 | hnsw.h | 建图候选宽度（建图质量） | 惯例 100-500 |
| HNSW_EF_SEARCH | 100 | hnsw.h | 查询束宽（recall-latency 旋钮） | M5 主旋钮：升 recall 升延迟 |
| HYBRID_ROUTE_K | 20 | 调用方策略（main.cpp / mainwindow.cpp） | 每路喂给 RRF 的候选条数（展示仍 TOP_K=10） | M5 可扫 10-50：列表越深，双路共识越受青睐 |
| EMBED_SLICE | 32 | 调用方策略（main.cpp / mainwindow.cpp） | 建库嵌入的每批块数（进度可见 + 单请求体上限） | M5 latency 记账 |

## 运行时配置默认值（不占参数行）

- dense 模型 `model_id` = `qwen3-embedding:0.6b`（**M3 ② A/B 实测拍板
  2026-10-11**：23 条中英混合查询 × 145 块纯 dense 对比，qwen 全指标
  胜出——top1 16/23 vs 15、hit@5 91% vs 78%、hit@10 96% vs 83%、
  MRR@10 0.777 vs 0.697、嵌入 27.8s vs 40.2s；题集与脚本 = T10 交付，
  **已转正 `scripts/`**（dump_chunks.cpp + ab_eval.py，随 T9 commit
  入仓 10-11），23 题集内嵌 ab_eval.py 为 M5 复用资产）。查询侧自动挂
  qwen3-instruct-v1 前缀（embedder.h 成对冻结策略）
- 向量快照 `vector.bin`（T9，2026-10-11）：身份四件套
  model_id/embedding_policy/dimension/VECTOR_PIPELINE_VERSION +
  per-doc path+content_hash 复用键；语料未变启动 0 网络秒上
  （145 向量 0.01s，对比冷嵌 20s），单文档编辑只重嵌该文档
  （0.5s 量级）

## 维护规则

- 新参数随写随登（谁写谁登，AI 可代录）
- 改默认值 = 改 .h 里的 constexpr + 同步本表
- M5 对比实验的实验变量从本表选取，实验结果回填"调参方向"列

## 版本

- v1.5 · 2026-10-11 · **T9 竣工**：vector.bin 快照入册（身份四件套 +
  per-doc 复用键，语义与行为见运行时配置节）；T10 工具转正
  scripts/（dump_chunks.cpp + ab_eval.py，23 题集内嵌）
- v1.4 · 2026-10-11 · **M3 ② A/B 拍板**：dense 默认模型 bge-m3 →
  qwen3-embedding:0.6b（23 题实测全指标胜出，数据见运行时配置节）
- v1.3 · 2026-10-11 · 步骤④ 接线：调用方常量两条入账（HYBRID_ROUTE_K
  / EMBED_SLICE，住址 = 双前端调用方策略）；dense 模型默认 bge-m3
  暂定值入备注。顺手补 v1.2 版本注（当日只改了表格漏了版本注）
- v1.2 · 2026-10-07 · M3 六参数入账（EMBEDDER 两项 / RRF_K / HNSW
  三旋钮）——版本注 10-11 补录
- v1.1 · 2026-09-28 · 500/50 口径 words→atoms（M2① 甲″契约变更，
  chunker 计数单位下沉到 atom）；dl 口径不变 = tokenize() 输出的
  token 数（atom ≠ token，见 indexer.h）；中文块字节更轻的观察入账
- v1.0 · 2026-09-23 · 随 indexer 设计讨论建立，首批 5 参数
