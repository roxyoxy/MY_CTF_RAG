# TASKS — 任务分发与验收

> M1 第三轮实现分工的正式任务卡。规则依据 `PRINCIPLES.md`。
> 伪代码已写在各 `src/*.cpp` 的注释块内（中文，施工时翻译完一段删一段，
> 注释删光 = 竣工）。

## 0. 工作流（git + PR）

1. 仓库：`https://github.com/roxyoxy/MY_CTF_RAG`（私有，A 管 main），
   A 拉 B / C 进 collaborator
2. B / C：`git clone` → `git checkout -b 分支名`（如 `b-indexer`、`c-loader`）
3. 手写代码：在伪代码注释块下方写真代码；真代码的注释用**英文纯 ASCII**
4. 自测用例全过后：`git push` 分支 → GitHub 上开 PR（目标 `main`）
5. A 拿 PR 分支到本地编译 + 联调（兼容性测试），**过了才 merge**
6. 会议纪要不缺席：开工写一条、卡壳写一条、竣工写一条

分支纪律：不直接推 `main`；一个任务一个分支；PR 描述里写清
自测结果 + 做了什么优化 + 为什么。

## 1. 任务总表

| 任务 | 文件 | 契约 | 执行人 | 状态 |
|---|---|---|---|---|
| T1 | `src/tokenizer.cpp` | `include/tokenizer.h` | C | 待开工 |
| T2 | `src/loader.cpp` | `include/loader.h` | C | 待开工 |
| T3 | `src/chunker.cpp` | `include/chunker.h` | B | 待开工 |
| T4 | `src/indexer.cpp` | `include/indexer.h` | B | 待开工 |
| T0 | data/ 语料 + 集成验收 + M2/M3 设计推进 | — | A | 进行中 |

## 2. 铁规则（对所有人）

### 接口三层规则

| 层 | 规则 |
|---|---|
| 函数签名 / 结构体字段 / 常量语义 | **绝对冻结**（契约管 WHAT） |
| 函数内部怎么写 | **完全自由**（实现管 HOW） |
| 发现契约有 bug 或模糊 | **停手**，写进会议纪要，全组讨论，不得擅改 |

红线：`InvertedIndex` 的**账本格式**（postings 按 chunk_id 升序、
chunk_lengths 的下标就是 chunk_id）不许"优化"掉——search 查账、
M2 持久化、M3 RRF 混合全都吃这个格式。**内部怎么建账自由，
账本长什么样不自由。**

### 优化三规则

1. 第一版先按伪代码跑通 + 自测全过——**正确性是入场券**，
   没跑通就优化是玄学
2. 小优化直接做（做完自测仍全过）；大优化（换数据结构 / 换算法）
   写成注释 TODO 或纪要提案，A 集成时统一裁
3. 每处优化必须答得出"为什么比基础版好"——量级分析或推理，
   不接受"感觉快"。M5 实验会回来验证的

### 第 0 步（动工前，必做）

读完必读清单后，先向你的 AI **复述**：这个模块的输入 / 输出 /
在整条管线里的位置。讲得清才动工（宪法：讲不出为什么就退回去重聊）。

## 3. C 的任务卡（T1 + T2，数据入口侧）

### 必读清单（按顺序）

1. `PRINCIPLES.md`（宪法，尤其第 3 节 AI 规则；§5 是给你 AI 的指令块）
2. `README.md` §4（构建环境既成事实 + 编码纪律）
3. `docs/MEETING_LOG.md` 关键决策（`py scripts/meeting_query.py "关键词"`）
4. `include/type.h` → `include/tokenizer.h` → `include/loader.h`
5. `src/main.cpp`（你的代码最终被谁调用——这就是验收标准）
6. 你自己 .cpp 文件里的伪代码卡

### T1 tokenizer.cpp 要点

- 最薄的一份，热身用
- 词内字符 = 字母 + 数字 + 连字符，其余全是分隔符
- 两端同源铁律：建索引和查询用**同一个** tokenize()（公理③的词项版）
- 连字符保留是 CTF 领域定制：use-after-free / format-string 不可断

### T2 loader.cpp 要点

- 递归遍历 → 字典序排序 → 再编号（谁生产谁编号）
- 已预答的问题：空目录返回空 vector 不 throw；目录不存在才 throw；
  M1 不处理任何删除
- `std::filesystem` 是 C++17 标配，直接用

### C 的验收标准

- 编译 0 error 0 warning（项目开 /W4）
- 伪代码卡里的自测用例全过；临时验证文件跑完即删，不进 PR
  （第四轮 tests/ 建立后这些用例转正）
- PR 描述：自测结果 + 优化清单及理由
- 真代码注释英文纯 ASCII，伪代码注释删光

## 4. B 的任务卡（T3 + T4，检索算法侧）

### 必读清单

同 C（第 4 条换成 `include/chunker.h` → `include/indexer.h`，
另读 `docs/PARAMS.md`——你调的参数都在册）。

### T3 chunker.cpp 要点

- **数词、记字节**：一次扫描产出词位置表（start / end 都是字节偏移）
- 不能调 tokenize()（它只给词不给字节位置），必须自己扫，
  但词内字符规则必须与 tokenizer.h 完全一致
- 已预答的问题：`deleted=true` 的文档 M1 不过滤（M1 没有删除入口），
  直接切；空文档产 0 块
- CHUNK_SIZE=500 / CHUNK_OVERLAP=50 已在 PARAMS.md 登记，
  不许在 .cpp 里写死新数字

### T4 indexer.cpp 要点

- 两个函数：建账 build_index + 查账 search
- IWYU：本文件要调 tokenize()，想想还要 include 谁
- BM25 公式在契约里逐字实现；整数除法陷阱（total / 块数先转 double）
- 这是全项目最核心的算法文件：M5 实验的被测对象就是它

### B 的验收标准

同 C，另加两条：

- search 排序：score 降序、**同分 chunk_id 升序**——这是契约条款，
  比较器方向写反是高频事故
- tie-break 用自测用例钉死（构造两个同分 chunk 验证顺序）

## 5. 全局待办总账（唯一权威清单）

### M1 尾巴

- [ ] data/ 语料：3-5 篇迷你英文 writeup，分类子目录（A，联调硬前置）
- [ ] 四个 .cpp 实现合流，main 链接从 LNK2019×4 清零（PR 流程）
- [ ] main.cpp catch loader 的 throw（第四轮；现在 data 缺失会直接崩）
- [ ] tests/ 目录建立，伪代码卡自测用例转正（第四轮，YAGNI 到期）
- [ ] 契约留白记录：墓碑字段 M1 无消费者（M2 删除路径落地时
      chunker / indexer 加过滤——不是 bug，是排期）

### M2 排期（有意后置，非遗忘）

- [ ] 中文 bigram 分词
- [ ] 索引持久化（现在每次启动重建）
- [ ] 增量建库
- [ ] 删除路径落地（墓碑真正消费：删除命令 + 定期全链路重建）

### M3-M5

- [ ] M3：向量检索 Flat → 自研 HNSW + RRF 混合
- [ ] M4：本地 LLM 后端（Ollama 原型 → llama.cpp 交付）
- [ ] M5：对比实验（BM25 / Dense / Hybrid）+ 调参（PARAMS.md 是总账）

## 6. 版本

- v1.0 · 2026-09-23 · A 拍板分工方案，AI-A 执笔
