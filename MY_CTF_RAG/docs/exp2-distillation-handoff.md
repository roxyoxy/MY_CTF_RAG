# 实验2 蒸馏交接文档（给接手 AI）

> 接手前提：本文档自包含，你不需要任何历史对话上下文，读完就能动手蒸。
> 范围：只做实验2 的"基础分类打分"语料蒸馏。**不涉及实验3 的参数问题。**

---

## 一、我们到底在做什么（30 秒读懂）

- **项目**：MY_CTF_RAG —— 一个本地离线的 CTF RAG 系统（C++ 检索 + 本地 LLM 生成）。
- **实验2**：专家 RAG × TraceRAG 内置小模型。要证明的结论一句话：
  > **本地小模型 qwen3 + "专家知识卡 bundle" ≈ 云端大模型，在 CTF pwn 域内的分类打分能力，零微调、全离线。**
- **蒸馏在实验2 里的角色**：给"专家 bundle"提供原料——把 CTF 题沉淀成"书架可检索的结构化知识卡"。
- **蒸馏的产出**：一张张"卡"，每张卡对应一道题 / 一个攻击链，带技术标签（命中书架）。

**蒸馏为什么重要**：实验2 的核心验证是"分类打分"——把一道题路由到正确的书架（技术类目）。卡上的 `techniques` 标签就是分类的依据，书架是分类的靶子。卡不够、或标签不准，分类打分就撑不住结论。

---

## 二、规模目标（关键：不是多多益善）

- 蒸馏目的 = 让 RAG 增量到**"足够证明结论"**，不是堆数量。
- **百级（517）→ 千级（~1500）足够**。万级边际效益递减、重复性作业、白烧 token，**不做**。
- 结论由**数量级**支撑（百 → 千跨一个数量级），不硬凑具体数字（5000 只是个概念）。
- 一句话：**蒸到能画出"卡数 × 分类质量"的上升曲线、够答辩说清"规模足够支撑结论"为止。**

---

## 三、卡 schema（蒸馏产物的形状）

每张卡是一个 JSON 文件，结构如下（`card` 是主对象，`chunks` 是引用原文块）：

```json
{
  "card": {
    "attack_chain_short": "一句话攻击链",
    "one_sentence_insight": "一句话洞见",
    "attack_steps": [
      {"step": "1", "title": "步骤标题", "detail": "步骤细节", "src": "wp 第3段 / chunk 指针"}
    ],
    "knowledge": [
      {"topic": "知识点主题", "key": "关键结论", "src": "wp 第5段 / chunk 指针"}
    ],
    "techniques": ["UAF", "heap_leak"],
    "pitfalls": [
      {"note": "坑点描述", "src": "wp 第2段 / chunk 指针"}
    ],
    "challenge_id": "16位hex",
    "card_id": "卡id",
    "contest": "google-ctf",
    "year": "2023",
    "stage": "final",
    "challenge_name": "某题名",
    "category": "pwn",
    "source_type": "wp",
    "source_ids": ["wp 文件路径/url"],
    "distill_source": "wp"
  },
  "chunks": ["原文 chunk 1", "原文 chunk 2"]
}
```

### 字段硬规则

1. **`challenge_id`（唯一键）**：`sha256("{contest}|{year}|{stage}|{challenge_name}|{category}")[:16]`。
   例：`sha256("google-ctf|2023|final|某题名|pwn")` 取前 16 位 hex。同一道题无论蒸几次，id 必须一致。
2. **`distill_source`**：只有两个合法值——`"wp"`（从官方 wp 蒸）或 `"code"`（从源码反推）。
3. **`source_type` / `source_ids`**：卡来自哪个原始文件/url，可溯源。
4. **`category`**：`pwn` / `web` / `re` / `crypto` / `misc` / `forensics` / `blockchain` / `ai` 之一。
5. **source 指针（保真度硬要求，见第六节）**：`attack_steps[].src`、`knowledge[].src`、`pitfalls[].src` 必须指向原文位置（wp 第几段 / 哪个 chunk）。**这条不能省**——它是"卡忠实于原文"能否被机械验证的前提。

---

## 四、蒸馏对象（原料优先级）

- **主路线**：有**官方 wp** 的题 → 直接用 wp 蒸（权威、准确、省）。
- **兜底**：没有 wp、只有**源码**的题 → `distill_source:"code"` 从源码反推。
- **❌ 禁止**：把"有 solution 才蒸"当成门槛。这是之前踩过的坑——错误 brief 写成"只蒸有官方 solution 的题"，导致 dicectf 跑完只蒸出 5 张（2022 全年仓库没放 solve 文件）。**正确门槛是"有可读原料（wp 或源码）就蒸"**，不是"必须有 wp"。

---

## 五、蒸馏动作（LLM 是提炼器，不是解题器）

- 读 wp（或源码），**提炼**成结构化卡，**不是重新解一遍题**。
- 从 wp 里抽：
  - `attack_chain_short` / `one_sentence_insight`
  - `attack_steps[]`（分步攻击步骤）
  - `knowledge[]`（知识点）
  - `techniques[]`（技术标签，**命中书架 canonical**，见第九节）
  - `pitfalls[]`（坑点）
- **每个 claim（step / technique / pitfall / knowledge）都要带 `src` 指针**，指向它出自原文的哪一段。没有指针的 claim 一律视为脑补，直接打回。

---

## 六、合格线（什么叫达标）

一张卡全部满足才算达标入库：

1. **content gate**：`attack_chain_short` / `attack_steps` / `knowledge` 三者**至少一个非空**（拦空壳）。
2. **字段完备**：`challenge_name` / `contest` / `year` / `category` 四个必填全有。
3. **技术标签命中书架**：`techniques` 里的每个标签要能对上 `pwn_taxonomy_v1.yaml` 的 48 架 canonical（拦跑偏标签）。
4. **忠实度（保真）**：
   - **机械验证**：抽 k 张，逐条检查每个 claim 的 `src` 指针是否真能撑住那句话（指向的原文里有没有这个 claim 的依据）。
   - **独立裁判**：脑补率判定必须由**与蒸馏模型隔离的另一方**（另一个模型或人）对照原文逐条审。**蒸馏 LLM 自己判卷 = 用幻觉查幻觉，禁止。**

---

## 七、成本闸门（动手前必做，铁律）

- **顺序**：先小样蒸 **20 张** wp 卡 → 独立裁判量出**真实脑补率** → 再定阈值 → 再批量。
- **阈值 5% 只是占位**，真实阈值以 pilot 实测为准（旧数据里 code 反推的脑补率是 7.7%，wp 蒸馏预期更低，但没实测前别写死）。
- **动手前必须报预算**：蒸 N 张卡 ≈ 多少成本、上限多少。绝不无上限并行开 agent。

---

## 八、去重规则（新旧卡撞车）

- 去重唯一键 = `challenge_id`。
- 撞了（同一道题老 code 卡 + 新 wp 卡）→ **新 wp 卡覆盖老 code 卡**（wp 保真度更高）。
- 老 code 卡**不删**，降级存档（手写资料不删的红线），只在索引里让位给新卡。

---

## 九、书架体系（分类的靶子）

- 文件：`D:\ctf-rag-injection\eval\pwn_taxonomy_v1.yaml`
- 结构：**11 家族 / 48 技术架**，其中 3 个是 `context`（不算书架），**有效书架 = 45**。
- 卡的 `techniques` 标签要命中这 48 架的 `canonical` 词表。
- 注意：目前**只有 pwn 有书架体系**，其余 7 家族（web/re/crypto/misc/...）是平铺、无 taxonomy。实验2 当前只做 pwn 方向。

---

## 十、数据现状（你接手时的家底）

### 关键文件路径

| 内容 | 路径 |
|---|---|
| 生产 staging（卡存储） | `D:\ctf-rag-injection\staging\`（结构 `{contest}\{year}\{name}.json`，三层） |
| 书架 taxonomy | `D:\ctf-rag-injection\eval\pwn_taxonomy_v1.yaml` |
| 书架 enrich（summary/cues/generic_chain） | `D:\ctf-rag-injection\eval\node_enrich_preview.jsonl` |
| 路由词表（B′ 词法路由） | `D:\ctf-rag-injection\eval\routing_terms_v1.json` |
| 角色标注 | `D:\ctf-rag-injection\eval\card_role_preview.jsonl` |
| 已导出资产（**已过期，需重导**） | `C:\Users\48714\Desktop\MY_RAG\MY_CTF_RAG\assets\pwn_cards.json` |

### 卡数现状（2026-10-15 实测）

| 口径 | 数量 |
|---|---|
| staging 全量 | **628 卡** |
| 其中 pwn | **124 卡**（113 旧 + 11 脏） |
| misc / crypto / web / re / forensics / blockchain / ai | 173 / 124 / 90 / 82 / 23 / 10 / 2 |

### ⚠️ 11 张脏卡（接手第一件事要处理）

这 11 张是上一次蒸馏 agent **绕过 reducer 直接写进生产目录**的，**没过 content gate、没查 challenge_id 冲突**，属于未质检数据：

```
google-ctf\2017\2017-pwn-cfi, 2017-pwn-wiki
google-ctf\2018\pwn-env, pwn-execve-sandbox, pwn-poetry
google-ctf\2019\pwn-secureboot, pwn-solver, sandbox-sbox
dicectf\2023\dicer-visor, disc-pwn, term-zelda
```

处理方式（接手时定夺）：先隔离到 `staging_new`（不要留在生产目录），再逐张过第六节合格线——达标才回 staging，不达标回炉或丢弃。

---

## 十一、你接手的第一步（建议顺序）

1. **隔离 11 张脏卡**：移出 `D:\ctf-rag-injection\staging\`，放到临时目录。
2. **重导资产**：等 staging 干净后，重跑 `py -X utf8 scripts/exp2_export_assets.py`，确认 `pwn_cards.json` = 干净的 pwn 卡数（不含脏卡）。
3. **小样蒸 20 张 wp 卡**：选有官方 wp 的 pwn 题，按第五节动作 + 第六节合格线蒸，强制带 `src` 指针。
4. **独立裁判量脑补率**：用与蒸馏模型隔离的模型/人，对照 wp 原文逐条审这 20 张。
5. **定阈值 → 批量**：以 pilot 实测脑补率定阈值，再报预算、批量蒸到千级（~1500）。
6. **每批过 reducer**：去重（challenge_id）+ gate + 保真，达标才入库，保留 git tag 做 checkpoint。

---

## 附：实验2 全貌（蒸馏只是其中一环，供理解定位）

```
实验2 = 专家 RAG × TraceRAG 内置小模型

前级（专家 RAG）：
  C1  qwen3 自路由（本地小模型把题路由到书架）——已探针达标 router@1=0.636
  C2/B′ 词法路由（BM25-r 置信度兜底，处理 qwen3 超纲不弃权的问题）
  bundle 组装（按路由结果取对应书架的专家卡）

后级（TraceRAG）：
  context builder（bundle 前置到检索材料之前）

验证：五臂评测 + 盲考卷分类打分（33 计分 + 4 超纲）

蒸馏的定位：给"bundle 组装"供卡——卡越准、越够，分类打分越稳。
```

**蒸馏是语料准备环节，不是全部实验。** 你只负责把卡蒸好、蒸准、蒸够到千级，其余（C++ 集成、五臂评测）不归你管。
