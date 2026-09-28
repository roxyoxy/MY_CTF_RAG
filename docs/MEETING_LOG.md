# MEETING_LOG — 会议纪要

> 全组共同记忆。格式与追加规则见 `PRINCIPLES.md` 第 4 节。
> 查询：`py scripts/meeting_query.py --help`
>
> 条目格式（严格）：
> ```
> ## [YYYY-MM-DD HH:MM] 角色代号
> 正文（可多行）
> ```
> 只追加不删除；写错追加更正条目；重大决策正文首行写【决策】。
> 标注（补录）= 事后根据讨论记录整理，非实时写入。

---

## [2026-09-22 09:00] A（补录）

立项起点：我的错题本软件全局搜索很烂——经常找不到想要的错题，
或者算法太简单。问 AI-A：搜索引擎是怎么一步步进化成今天这样的？

## [2026-09-22 09:20] AI-A（补录）

讲解了检索技术的进化谱系，每一步都是在解决上一步的缺陷：

1. 朴素字符串匹配 → 慢、无排序、中文整串匹配
2. 倒排索引 + 布尔检索 → 解决"快"，但只有有没有、没有好不好
3. TF-IDF → BM25（Okapi，1980s IR 集大成）→ 解决"排序准"：
   TF 饱和（k1）+ 文档长度归一化（b）+ 平滑 IDF
4. 语义向量检索 → 解决"懂意思"：搜"抛物线"能找到"二次函数图像"
5. 混合检索（BM25 + 向量 + RRF 融合）→ 现代标准答案

错题本搜索的病根在第 3 步之前——连相关性排序都没有。

## [2026-09-22 09:40] A（补录）

聊到了 RAG。我三个疑问：① 为什么大家都用 RAG，比微调好在哪？
② embedding 是什么，需要机器学习吗，我们得自己训吗？
③ 系统写好后换大模型还能用吗？

## [2026-09-22 10:00] AI-A（补录）

三个结论（后续设计反复引用，成为本项目公理）：

① RAG = 让 LLM 开卷考试，改文档即生效、答案可溯源；微调是把知识
焊进参数，更新要重训。业界共识：**风格用微调，事实用 RAG**。
② embedding = 预训练模型把文本映射成语义向量；我们"用"不"训"
（下载 bge-m3，C++ 发 HTTP 拿 float 数组，和解析 JSON 无本质区别）。
③ 检索层与生成层解耦，换任何 LLM 都行；**唯一耦合点：建库和查询
必须用同一个 embedding 模型**（向量空间是模型私有的，换模型 = 换坐标系）。

## [2026-09-22 14:00] A（补录）

选定题目：离线 CTF RAG 系统。理由：禁联网比赛需要全本地推理；
算法课要求核心算法自研（BM25/向量/RRF 都在检索层）；网安专业做
RAG 安全是差异化亮点。评审了 GPT 蓝图 + AI-A 四补丁（自研 HNSW、
中文 bigram、语料导入器、实验三组对比），细节存
`project_RAG/docs/AI-BRIEFING.md`。

## [2026-09-22 16:00] A（补录）

【决策】AI-A 在 `project_RAG` 一口气建成了完整骨架（约 1700 行，
58 项测试全绿，VS F5 可跑）。今天重新裁定其定位：
**project_RAG 降级为参考实现**——只许看它的设计思路，代码只允许
转成伪代码供手写参考，正式代码全部在本仓库（MY_RAG）由人重写。

## [2026-09-22 20:00] A

【决策】工作模式重置，本仓库（MY_RAG）立项。核心决议：

1. 我自己花时间做第一部分，哪怕复制粘贴也要搞明白为什么这么设计
   文件夹。自己创建 Visual Studio 项目、自己往里添加 .h 和 .cpp。
   AI 一口气全部创建好 = 失去意义。
2. 今天只完成第一部分，且**只做英文切片**（中文后置到 M2）。
   必须逐步迭代，先从简单的做起。
3. M1 施工顺序：先完成所有 .h 的设计 → 再写 main → 再写 .cpp →
   最后不断优化函数，这才构成第一阶段雏形。
4. 之后一步步修改文件夹格式、优化算法；把参考代码删除变成伪代码，
   进行手写分工发给 B、C。
5. B、C 手写期间，我用已完成的阶段一推进阶段 2、3 的设计。
6. 最终我开一次会议讲我的经验——让全组学会"从零写项目"的思维，
   而不是"去和 AI 说给我写一个"。
7. 动机：我打算面试大厂实习。这次机会不珍惜就白白浪费了，
   面试时总不能说"设计经验去问我的 AI"。

## [2026-09-22 20:05] AI-A

【决策】接受工作模式重置。角色从"代工"切换为"教练"：
多解释少动手，人不点名要某段代码就不给代码（宪法条款落进
PRINCIPLES.md 第 3 节）。M0 交付四件：

1. `README.md` — 项目门面 + 目录设计理由（面试常问）
2. `PRINCIPLES.md` — 协作宪法：AI 禁止事项（减速设计）、
   会议制度、给队友 AI 的可复制指令块
3. `docs/MEETING_LOG.md` — 本纪要（对话式，脚本可查）
4. `scripts/meeting_query.py` — 纪要查询工具（纯标准库）

刻意没建的目录：include/ src/ tests/ data/ .sln ——全部留给 A
亲手创建，这是本次决议的第一次执行。

## [2026-09-22 20:10] A

M1 第一步由我执行：打开 Visual Studio，亲手创建空项目 MY_RAG.sln，
亲手添加第一个 .h。添加之前先和 AI-A 聊透：第一个 .h 应该是哪个、
里面放什么、为什么。

## [2026-09-22 20:15] AI-A

待命中。A 动手创建项目文件时，随时可以开始第一个 .h 的设计讨论
（只讨论，不写码）。建议议题顺序：数据结构先行——先有 Document
和 Chunk 的形状，才有切分器和索引器的接口。

## [2026-09-22 20:30] A

【决策】定义「刷新」指令（全组 AI 通用）：任何 AI 听到主人说
「刷新」，必须先重读 README.md、PRINCIPLES.md、docs/MEETING_LOG.md
（及 docs/design/ 新文档）再继续对话，不凭旧记忆回答。原因：
本项目人手写 + 多 AI 协作，文档是唯一真相源，纪要会被随时追加。
该指令已写入 PRINCIPLES.md §5 指令块，此后我只说一次，不再解释。

## [2026-09-22 21:00] A（补录）

M1 开工：A 亲手完成 VS2022 环境搭建——空项目 MY_RAG（勾选"解决方案
与项目同一目录"）、src/main.cpp 占位、三项编译器设置（/utf-8 命令行
"其他选项"手输、ISO C++17、/W4，均作用于所有配置×所有平台）。
坑位记录：中文版 VS 属性页无源文件时不显示 C/C++ 节点；
"附加选项"译作"其他选项(D)"且为纯文本框。include/ data/ 目录已建，
tests/ 按 YAGNI 推迟到第四轮。下一步：第一个 .h 设计讨论。

## [2026-09-22 21:30] A

【决策】types.h 设计定稿（AI-A 教练对话产出，A 逐项拍板）：

- 单文件 types.h 装全部 POD 结构（不拆 document.h/chunk.h）
- Document = { id, path, content }。category/title/总长度全部不存：
  派生数据不存第二份（一致性风险）；BM25 的长度归一化量的是
  chunk 的 token 数，与文档字节长度无关（无消费者）
- Chunk = { id, document_id, text, begin, end }。text 反例成立：
  派生数据存否的判据 = 推导成本 × 使用频率；begin/end 是调试期的
  眼睛（切块错了跳回原文定位），左闭右开区间 [begin, end)
- 编号注册表模式确立：id = vector 下标，O(1) 找回；Document 由
  加载器编号，Chunk 由切块器编号（谁生产谁编号）。倒排表/分数表/
  top-k 全部以 chunk id 为硬通货
- 下一步：A 亲手敲 types.h，敲完贴出 review；钩子：#pragma once

## [2026-09-22 22:00] A

【决策】环境折腾叫停 + 目录终态定型。A 明确表态"别在意这些细节"，
AI-A 认错：为"勾选同一目录"让 A 三次清场重建，过度纠结结构、
消耗主线时间。最终结构（不再动）：

```
MY_RAG\               ← 文档区（README/PRINCIPLES/docs/scripts）
└── MY_CTF_RAG\       ← 代码区（.sln + .vcxproj + include/src/data）
```

文件名以 A 实际所建为准：type.h（非 types.h）。教训（AI-A 记）：
环境类问题应在第一次歧义时就用 ls 看现场，而不是基于猜测出指令；
结构能跑就不折腾，精力留给主线。

## [2026-09-22 22:10] AI-A

【决策】type.h review 通过（A 亲手所写，本项目第一份代码产物）：

- 字段全部符合规格：Document{id,path,content} + Chunk{id,document_id,
  text,begin,end}，类型正确（int/size_t/std::string），无多余字段
- 自选动作 SearchResult{chunk_id,score} 加分：规格书没有、设计讨论
  埋过伏笔（top-k 输出载体），A 自己长出来了
- 注释质量高：消化而非照抄（"倒排索引和分数表的主键""检索的最小
  单位""左闭右开""回查 vector<Document>"均为设计讨论的自主复述）
- ⚠️ 编码事故（教学案例）：文件为 GBK——/utf-8 开关只管编译器怎么
  读，不管 VS 怎么存；VS 保存默认跟系统代码页（中文 Windows=GBK）
  走。修复：VS 内"高级保存选项/另存为-编码保存"转 Unicode (UTF-8
  带签名)；注释语言（保留中文 vs 翻英文）由 A 拍板
- 遗留：include\test.cpp 错位（.cpp 应在 src\），待 A 处理

## [2026-09-22 22:20] A

type.h 编码转 UTF-8(BOM) 与 test.cpp 错位两事，A 决定**先挂账，
M1 收尾时统一处理**。AI-A 维护待办清单：
① type.h 转 Unicode (UTF-8 带签名) ② test.cpp 删除或挪 src\
③ 注释语言（中文/英文）拍板——转码前定。下一议题：第二个 .h
（加载器：data\ 目录 → vector<Document>）。

## [2026-09-22 22:40] AI-A

loader 三问讨论纪要：① 独立 loader.h（type.h 只装名词）② path 存
相对路径、用时再拼全 ③ id = vector 下标，批量/增量都是 push_back。
A 自主追问链表方案并当场自我否决（正确：链表失去 O(1) 下标直通，
cache 不友好）。删除策略摆四选项（压缩重排/墓碑/free list/全量重建）。

## [2026-09-22 22:50] A

【决策】删除策略定稿：**墓碑 + 定期全链路重建**（Lucene 段合并
同款模式）。日常删除只打标记 id 不动；维护时 Document 重载 +
Chunk 重切 + 索引重建一条龙（Chunk/索引是派生数据，可重算，全链路
重建 = 重跑几秒）。A 拍板 **M1 即在 type.h 加墓碑字段**（一步到位，
M2 不回头改 type.h）。挂账项③（注释语言）因新文件将写而提前到期，
随 loader.h 规格书一并拍。

## [2026-09-22 23:05] A

【决策】注释语言统一 **英文**（挂账③落槌）。A 的判断：编码乱码
只困扰 AI 的读取视角，人在 VS 里看 GBK 中文正常——成立，但选英文
可根除问题。技术依据：英文注释 = 纯 ASCII，在 GBK 与 UTF-8 下
字节完全等价，编码二义性消失，/utf-8 下也不会触发 C4819 告警。
挂账清单简化：原①（type.h 转 UTF-8）与③合并为"type.h 中文注释
翻译为英文"（译完即纯 ASCII，无需再动编码）；②（test.cpp）不变。
type.h 墓碑字段已按规格落地并 review 通过（A 自主优化了 begin/end
注释排版）。下一步：A 亲手写 loader.h（英文注释，契约六条自行
组织英文表述）。

## [2026-09-23 00:30] AI-A

【决策】loader.h review 通过，第二份契约文件关账（A 亲手所写）。
第一轮 review 抓出三件事：① include 写成 "types.h" 而磁盘实为
type.h——编译级错误，教训：契约必须与现场对齐，记忆不算数；
② 中文注释与 09-22 23:05 英文决策冲突，A 改英文执行；③ 契约⑤
（content 为 UTF-8 全文）两轮才补齐。A 主动升级一处：把契约⑥
的 fail fast 手段提前定为 throw（.cpp 轮照此履约，main 未来可
catch）。第二轮：签名/pragma/最小暴露/六条契约全过。
下一议题：chunker.h（切块器，vector<Document> → vector<Chunk>）。

## [2026-09-23 00:50] AI-A

【决策】chunker 设计讨论定案：一维全局编号（二维/邻接表方案经
"chunk_id 回查 text 的查询路径"推演否决——注册表模式保直通）；
按词计数下刀（单位选对则单词不被斩，中文 M2 再战）；chunk_size=
500 词 / overlap=50 词，constexpr 常量集中放 .h，调参属 M5。
A 三问作答亮点：自主提出二维数组方案（选型意识）、记得 overlap。

## [2026-09-23 01:05] AI-A

【决策】chunker.h 关账（第三份契约文件，A 亲手所写）。第一轮
review：骨架满分（签名&、两个 include 无 string、constexpr 全大写）；
契约三处超规格进化（示例 doc0->0..4、overlap 写到 why 级、实现
边界标注）。抓出一处矛盾：第5条"短文档恰好一块"与空文档打架
（0 词也短于 500，按字面产 1 块空 Chunk 污染索引）——契约自洽性
教训：条款打架时实现者随机选边，比没有契约更危险。另补 text=
substr 一致性条款（约束唯一生产者）。**A 授权 AI-A 本次直接改文件**
（两处），已修，不构成惯例。
下一步：M1 剩余 .h 全景盘点 → 索引/检索（BM25 的家）。

## [2026-09-23 01:20] AI-A

【决策】tokenizer.h 关账（第四份契约，最薄的一份：签名一行）。
三问 review：小写化裁决正确（但"合并会变多"账算反了——合并使
词表变小、TF 变准，双赢）；抓出自相矛盾案："逗号不是词的一部分"
vs"按空格切"打架（空格切逗号仍粘在词上）→ 修正为词内字符集规则：
字母+数字+连字符为词内字符，其余全分隔符（连字符保留是 CTF 领域
定制：use-after-free/format-string 不可断）；两端同源铁律 = 09:40
公理③的词项版。停用词表不做（BM25 的 IDF 天然压制）。
**em dash 事故（字节级）**：契约第3条混入 Unicode 破折号，VS 按
GBK 存为 A1AA——源头是 AI-A 规格书参考句，"英文=纯ASCII"论断
存在漏洞（智能标点：—“”‘’…均为非 ASCII）。守则入库：①源码注释
只写直 ASCII 标点 ②每份 .h 交付前跑字节检查（py 一行命令）。
A 已修复，复检 PURE ASCII。进度 4/6，下一议题：indexer（主菜）。

## [2026-09-23 02:40] AI-A

【决策】indexer.h 关账（第五份契约，最厚的一份）。review 内容
满分：include 字母序、常量注释自带 "defaults for M1, tuned in M5"
（PARAMS 制度入代码）、注册表模式自主复述（index is the registry）、
多行签名可读性进化、BM25 公式纯 ASCII 入契约。选型：倒排表用
unordered_map（单词点查场景，哈希 O(1)，工业惯例；接口不变 M5 可换）。
em dash 二进宫（build/search 契约第 1 条各一处 A1AA，源头仍是
AI-A 规格书参考句）——AI-A 守则升级：**发给 A 的任何参考句先自过
ASCII 自检**。A 授权字节级修复，已完成。

【决策】type.h 中文注释全部英文化（挂账①③合并项清算，A 授权）：
排版结构原样保留（行尾/独立行/空行/对齐），↑ 箭头因非 ASCII 清除。
五份 .h 字节总复检全部 PURE ASCII。

【里程碑】M1 第一轮（设计全部 .h）竣工：type / loader / chunker /
tokenizer / indexer 五份契约齐装满员，.h 进度 5/5。挂账清单仅剩
② test.cpp 错位。下一轮施工：main.cpp——用五个假想接口调用把
整条管线串起来（验收标准先行）。

## [2026-09-23 03:45] AI-A

【决策】main.cpp 关账，M1 第二轮（main 先行）竣工。review 满分
+ 三处自主进化（建账三行对齐赋值 / while(true)+EOF break 结构 /
chunk 全文打印超规格）。环境课：src 与 include 分居的代价首次
显形——附加包含目录 $(ProjectDir)include 首次配置（所有配置×
所有平台）；.h 互引不红（同目录）而 src 引用红（跨目录）的机理
已讲。LNK2019 ×4 = 设计好的中间态，四个符号恰为 main 消费的
四个契约函数（tokenize 缺席 = IWYU 的反向验证），即第三轮工作
清单。挂账②清算：test.cpp 由 A 亲手删除，**挂账清零**。
argv 补课：命令行出生行李（argc/argv）≠ 进程持续（循环的事）；
查询输入定方案 b 交互循环（建账一次查账 N 次，架构论据）。
下一轮：第三轮逐个补 .cpp，建议顺序由薄到厚：tokenizer →
loader → chunker → indexer。data\ 迄今为空，A 需备 3-5 篇迷你
英文 writeup（pwn/web 分类目录）。

## [2026-09-23 03:35] AI-A

【决策】第三轮开工方式定型：A 已在 VS 亲手创建四个空 .cpp
（src\ 下 tokenizer/loader/chunker/indexer），AI-A 获授权写入
**伪代码占位卡**（注释用中文，A 指定）。卡结构四件套：伪代码主体 +
提示（逐条对回契约条款）+ 自测用例 + 施工规则（真代码写在注释块
下方，翻译完一段删一段，注释删光 = 竣工）。编码方案：UTF-8 带 BOM
（与 type.h 一致）。**中文仅限伪代码脚手架**——真代码注释仍守英文
纯 ASCII 纪律，注释删光后文件自然回归纯 ASCII，宪法条款不被侵蚀。
工程细节入卡：chunker 不能调 tokenize()（需字节位置须自扫）；
indexer 补 IWYU 提示（需 include tokenizer.h）；BM25 整数除法陷阱。
待拍板：分工方案甲（A 出 tokenizer+indexer / C 出 loader / B 出
chunker）与代码合流方式（git 或文件拷贝）。data\ 语料仍待 A 备。

## [2026-09-23 03:50] A

【决策】M1 第三轮分工定稿：**四个 .cpp 全部下发**，A 不写实现，
专职 data 语料 + 集成验收 + M2/M3 设计推进（原决议第 5 条落地）。
分法按管线两侧：**B = chunker + indexer**（检索算法侧，M5 评测的
被测对象谁评测谁写到骨头里）、**C = loader + tokenizer**（数据
入口侧，工程训练对口 M4）。伪代码卡定位为"及格线"，鼓励 B/C 在
跑通 + 自测全过后做优化（优化三规则见 TASKS.md §2）。
A 对"不写实现"的风险自知：通过深度 review + 第四轮主导优化补
实现层理解，review 是 A 的补课主战场。
合流方式：**git + GitHub 私有仓库 + PR 工作流**——B/C 开分支推 PR，
A 拉 PR 分支本地编译联调（兼容性测试），过了才 merge（手动版 CI）。

## [2026-09-23 03:55] AI-A

【决策】`docs/TASKS.md` 建卡（v1.0）：工作流（PR 六步 + 分支纪律）、
任务总表（T0-T4）、铁规则（接口三层规则 + InvertedIndex 账本格式
红线 + 优化三规则 + 第 0 步复述）、B/C 双卡（必读清单 / 要点 /
验收标准，indexer 加排序方向专项）、全局待办总账（M1 尾巴 / M2
排期 / M3-M5，含"墓碑字段 M1 无消费者"留白记录）。
配套 `.gitignore` 就位（x64/.vs/*.user 等不进仓库）。
B/C 开工前置链：clone → 读 TASKS.md → 第 0 步复述 → 开分支动工。

## [2026-09-26 13:27] C

开工 T1 tokenizer，创建分支 c-tokenizer。

## [2026-09-26 19:22] C

【发现】tokenizer.cpp 自测时发现伪代码卡用例 4 与头文件矛盾：按 tokenizer.h“词内字符=字母+数字+连字符，其余为分隔符”，"libc-2.31" 应切为 [libc-2] [31]，但卡的期望 [libc-2.31] 要求点号入词，当前代码按头文件格式实现。

## [2026-09-26 19:42] C

【竣工】完成T1 tokenizer.cpp，分支 c-tokenizer。全程：第 0 步复述通过后按伪代码卡
手写实现（扫描窗口双游标模式），自测卡内 5 条用例，4 条直接通过，用例 4 暴露点号在分词问题上的矛盾。
AI 参与：教练讲解 + review；经我授权AI补写注释，属一次性授权，不构成惯例。收尾自检：真代码注释英文纯 ASCII、无 BOM、伪代码删光、严格告警档编译零告警（VS /W4 以 A联调为准）、临时 test main 用完即删不进 PR。遗留：缩进 Tab/空格混用待我统一后提交。

## [2026-09-26 20:11] AI-C

【决策】tokenizer 词内字符规则升级：`.` 与 `_`以"三明治规则"入词——当且仅当左右邻居均为字母/数字时视为词内字符；连字符维持原契约无条件入词；不新增预审/标记模块，规则直接在 tokenize() 内实现。
背景：T1 自测中 "libc-2.31" 的切分暴露点号歧义。
暂不新增预审层：① 预审改写文本会破坏 type.h 不变量（content 为磁盘原字节、Chunk::begin/end 指向原文偏移）；② 查询端没有预审，等于两套逻辑必须永远同步，与"两端同源"精神相悖；③ 三明治规则改动面极小（仅字符判定升级为看位置，扫描窗口结构不动）。
判例表：
- "libc-2.31" -> [libc-2.31]            版本号整体保留（原伪代码卡期望就此转正）
- "done." / "wait..." -> [done] / [wait]  句号、省略号不入词
- "127.0.0.1" / "exploit.py" / "example.com" -> 整体入词
- "buf_size" -> [buf_size]；"__libc_csu_init" -> [libc_csu_init]
  （前导下划线无左侧邻居故剥离；两端同源保证查询同样切法、照样命中）
- "_emphasis_" -> [emphasis]；"e.g." -> [e.g]（已知无害瑕疵，IDF 极低）
已评估、暂缓（观望清单，M5 实验后复盘）：/ （bin/sh 切散由两端同源兜底）、
' （don't 的 [t] 被 IDF 压制）、+ （C++ 低频，特判过拟合）、, （1,024 同 /）、
% ! # （纯符号场景少）。
影响与后续：① tokenizer.h 条款 2 需随之改写，本条即契约变更记录；@B：T3 chunker
的词内字符规则请以新契约为准，趁未合流改动成本最低。② 自测用例更新：libc-2.31
转正，另补 done. / 127.0.0.1 / exploit.py / wait... / buf_size。③ 改动极小，
不开新分支，续在 c-tokenizer 完成，PR 描述分列两步变更。

## [2026-09-26 20:28] C

【竣工】完成T1 tokenizer.cpp的升级。依据上一条新的约定修改代码，增加了对.和_的判定。并修改了tokenizer.h中的约定2。由AI-C根据新约定生成13条测试用例全部通过。

## [2026-09-26 21:07] C
【发现】 push时权限不通，改走fork绕道。

## [2026-09-26 21:47] C

开工 T2 loader，创建分支 c-loader。添加loader.cpp到解决方案。

## [2026-09-27 1:12] C

【竣工】T2 loader.cpp（AI-C 参与开工前契约讲解与代码 review）。
实现对照契约逐条落地：
- recursive_directory_iterator 递归收集 .md/.txt，扩展名忽略大小写
- path 经 lexically_relative + generic_string，存相对 dir 的正斜杠路径
- 先按相对路径字典序排序再编号（id = 下标）——同一语料处处产出同一
  id 序列，M2 增量建库的地基
- ifstream 二进制模式整文件读入，content 与磁盘字节一致
- is_directory(error_code) 双分支：目录不存在/访问失败分别抛错（fail fast）
- 空目录返回空 vector，不抛
AI-C review 确认两处易错点方向正确：空文件会令 `stream << rdbuf()`
置 failbit，故读后查 bad() 而非 fail()；tolower 前转 unsigned char
（UB 规避纪律自 tokenizer 延续）。

## [2026-09-27 01:20] A

【决策】批准 09-26 20:11 AI-C 的 tokenizer 契约变更（三明治规则）：
'.' 与 '_' 左右邻居均为字母/数字时入词，其余仍为分隔符；连字符
维持无条件入词。理由认可：CTF 语料中 IP / 文件名 / 符号名不应
斩断；判例表与观望清单完备；e.g. 瑕疵由 IDF 压制可接受。

【决策】PR #1（T1 tokenizer，c-tokenizer 分支）验收通过并合并。
A 侧验收记录：编译 0 错 0 警（/W4）；LNK2019 保持 4 个符合预期
（tokenize 不在 main 的直接消费清单，是 indexer 的依赖）；字节
复检无 BOM、0 个非 ASCII 字节；13 条自测用例由 A 侧独立复跑
全过。合并时 vcxproj 冲突取 main 超集版本（四个 .cpp 全注册）。
附则：C 代改 chunker 注释两处收下，立规矩——改不属于自己的文件，
内容对之外还必须纪要 @ 对方留痕。遗留不阻塞项：tokenizer.cpp
缩进 Tab/空格混用，C 下次提交 loader 前顺手统一。

## [2026-09-27 3:26] C

修复tokenizer.cpp缩进不统一的问题,因改动极小没有单独创建任务分支，在loader中附带完成。
## [2026-09-27 20:30] B（补录）

开工 T3 chunker。因梯子不通，改走 A 交接文档 §6 的备用路径：
本地 git 存档 + 文件回传 A 代提交。分支 b-chunker。

## [2026-09-27 22:30] B（补录）

【竣工】T3 chunker.cpp 完成。按伪代码卡手写：第一遍扫描产出词位置表
（start/end 为字节偏移；三明治判定位置感知、自己扫，不调 tokenize——
它只给词不给字节位置），第二遍滑动窗口按 CHUNK_SIZE 下刀、回退
CHUNK_OVERLAP 形成重叠。自测 10 条用例全过（g++ / MSVC 双编译器，
0 error 0 warning）；字节检查无 BOM、0 个非 ASCII 字节。

踩坑记录（值得全组看）：Document 用聚合初始化按位置填值
（{0, generateWords(10)}），把文本填进了 path，而 content 为空。
用例 1、2 因 content 恰好也是空而"假通过"，用例 3 才炸出来。
教训：位置初始化会静默出错，改用逐字段赋值。

测试有效性自证（变异实验）：把点号/下划线规则分别改坏成"少算词"与
"多算词"两个方向，用例 7 / 用例 9 分别抓住；随后补用例 9、10 填上
"多算词"方向的盲区。

AI 参与：AI-B 讲解 + review（含独立编译复核与变异实验），代码全部我手写。

## [2026-09-27 23:40] B（补录）

【竣工】T4 indexer.cpp 完成（build_index + BM25 search）。
build_index：逐块 tokenize 统计 tf 建倒排表；chunk_lengths 用 push_back
（依赖 id 连续升序，注释已注明该前提）；avgdl 用 double 规避整数除法。
search：逐查询词累加 BM25（df 由 posting 表长免费得到），结果按 score
降序、同分 chunk_id 升序，top_k 用 min 保护。自测 9 条用例全过
（g++ / MSVC 双编译器）；字节检查无 BOM、0 个非 ASCII 字节。

变异实验：tie-break 方向写反 → 用例 6a 抓住；avgdl 退化成整数除法 →
首轮 8 条全过（盲区），补用例 9（3 块词数 1/1/2，断言 avgdl 落在
1.3~1.4）后抓住。结论：测试绿 ≠ 实现对，变异实验是照妖镜。

IWYU 踩坑：本文件要调 tokenize()，但 indexer.h 不带 tokenizer.h，
漏 include 报 'tokenize' was not declared；另有一次文件名拼错
（tokensizer.h）——两次都靠编译发现，不是靠眼睛。

交付方式说明：T3 / T4 两个提交在本地 b-chunker 分支
（deeef59、14f3ba9），因梯子不通无法 push，按 A 交接文档 §6 由
B 回传文件、A 代为提交。include/ 契约未做任何改动，.vcxproj 未动。

## [2026-09-28 01:10] A

【决策】PR #2（T2 loader，c-loader 分支）验收通过并合并。
A 侧验收记录：编译 0 错 0 警（/W4）；LNK2019 由 4 降为 3，符合预期
（load_documents 落地，剩 chunk_documents / build_index / search 恰为
T3/T4 工作面）；字节复检 loader.cpp 无 BOM、0 个非 ASCII 字节；
A 侧独立复跑自测 7 条全过（目录不存在抛异常 / 空目录返空不抛 /
递归收集 .md 与 .txt 且扩展名忽略大小写 / 相对路径字典序排序后
连续编号且正斜杠分隔 / content 与磁盘字节一致含 CRLF / 墓碑字段
默认 false）。合并无冲突。
附则：tokenizer.cpp 缩进统一（Tab 改 4 空格）随本分支落账，
A 以 git diff -w 复核为纯空白改动零逻辑变更，PR #1 遗留项关闭。

## [2026-09-28 01:40] A

【决策】T3 chunker + T4 indexer（B，离线 zip 交接路径，A 代提交合并）
验收通过并合并入 main。A 侧验收记录：

- 编译 0 错 0 警（/W4），**LNK2019 清零**——四个实现全部落地，
  main 首次全链接成功，MY_CTF_RAG.exe 产出，第三轮竣工标志
- 字节复检两文件均无 BOM、0 个非 ASCII 字节，伪代码卡删光
- A 侧独立复跑自测 17 条全过：三明治规则同构（chunker 扫出的词
  与 tokenize() 逐词一致）、begin/end 整词字节偏移、600 词文档
  2 块且第二块自 w450 起（重叠 50 词）、空文档/纯分隔符 0 块、
  孤立连字符按契约成词、全局连续 id、avgdl 双精度（4/3 而非 1）、
  postings 按 chunk_id 升序、同分按 chunk_id 升序、tf 饱和排序、
  未知词/纯分隔查询返空、top_k 截断、端到端检索命中
- 验收插曲（自省）：A 侧测试首轮 3 条 FAIL 经查全为 A 测试用例
  自身错误（end 期望值算错、重叠回退口算错、把 --- 误当分隔符），
  B 实现无误。修正用例后 17/17。印证 B 纪要的教训：
  测试红先审测试，测试绿也未必实现对
- 附则：top_k 传负数的病态 resize 为边角遗留，记入第四轮待办
  （M1 无此调用方）；B 双编译器（g++/MSVC）自测与变异实验
  方法论全组通报表扬

## [2026-09-28 15:30] A

【决策】data/ 正式语料落位（T0 清账）：以本人 09-23/24 npusec 小赛的
12 篇英文 writeup 为正式语料（来源 Desktop/xiaosai/WP_EN，拷贝入库
原件不动），按类目归位 pwn/re/web/misc/ai_security/crypto（密码学
目录改 ASCII 名 crypto，yaho 归 crypto）。flag 明文保留（本仓库
public，赛后公开 writeup 属常规做法，A 拍板不洗）。3 篇合成验收
种子删除——语料叙事统一为"全部为本人真实比赛 writeup"。
实测：12 文档切 29 chunks，5 条主题查询 top-1 全命中，块级排序
可见同文档多块上榜。第四轮优化/测试以本语料为靶场。

## [2026-09-28 02:17] A

【决策】第四轮开工，首项 main.cpp catch loader 的 throw 竣工。
此前 data/ 缺失时异常无人接，std::terminate 静默暴毙（退出码 3）。
修法：Stage 1（load/chunk/build）整体入 try；catch (const
std::exception&) 基类引用兜住 filesystem_error 全继承链且不切片；
错误走 cerr，return 1。三个变量提升到 try 外（try 内声明出块即
失效）；try 范围刻意只盖建库段（search 契约保证不抛，查询环不设防）。
AI 参与：AI-A 教练（异常传播链讲解 + 四决策表 + review），代码 A 手写。
验收：编译 0 错 0 警；无 data/ 目录运行 → 一行人话报错 + 退出码 1
（改前：无输出 + abort 3）；正常跑 → 12 docs / 29 chunks 查询照旧，
退出码 0；字节复检无 BOM、0 非 ASCII。附则：std::vector 靠 loader.h
传递 include，A 裁不补；README/PRINCIPLES 语料信息 4 处过期随本提交同步。
另拍板两项：第四轮 tests/ 转正 + top_k 保护由 A 自做（不走 B 卡）；
tests/ 三件套 = 每模块一个 test_*.cpp + 命令行编译不进 .sln +
共享 tests/check.h 手写断言（不引外部库）。
更正：上条（data/ 语料落位）时间戳应为 2026-09-28 01:36 前后，
"15:30" 系 AI-A 误记，按只追加规矩在此更正。

## [2026-09-28 02:32] A

【决策】第四轮主体竣工：tests/ 转正 + top_k 守卫，一次提交入账。
- tests/ 建立（A 自做，不走 B 卡）：check.h 共享断言（C++17 inline
  变量 + test_summary 退出码）；test_tokenizer 12 条（新设计，期望值
  逐条对契约条款手算）、test_loader 7 条（验收用例转正）、
  test_chunker 7 + test_indexer 11（原 17 条拆包 + 守卫新 1 条），
  共 37 条全绿，/W4 零警告，全文件无 BOM 零非 ASCII。run_tests.bat
  一键全编全跑，"cl 报错走 stdout"的教训烤入（失败才吐编译日志）。
- 工作模式变更（A 拍板）：测试代码非核心算法，AI-A 执笔、A 逐条
  审阅；产品代码人手写铁律不变。tests 不进 .sln——每个测试自带
  main，进主工程必 LNK2005 双 main 打架，走命令行编译。
- top_k 守卫 @B：indexer.cpp search 入口加 top_k<=0 提前返空 2 行
  （原实现 min(top_k,size) 后 resize 负数转 size_t 巨值会炸），
  test_indexer 第 9 条钉死（0 与 -3 均返空）。B 文件改动经 A 拍板，
  留痕备查。主工程重编 + 冒烟通过（12 docs / 29 chunks 照旧）。

## [2026-09-28 02:37] A

main.cpp 加 Windows 控制台 UTF-8 代码页切换（SetConsoleOutputCP /
SetConsoleCP 设 CP_UTF8，#ifdef _WIN32 守可移植，B 的 g++ 侧不受
影响），A 手写。动因：语料 writeup 含制表符树（└─ ├─）与中文题名
（救赎之道），默认 GBK 控制台输出乱码。AI-A review 通过 + 重编冒烟
（全输出 2079 字节合法 UTF-8，行为不变）；A 本机 F5 肉眼验收
（中文/制表符/箭头全部正常显示）。M2 中文分词的显示层铺垫就此就位。

## [2026-09-28 02:46] A

【决策】第四轮优化遍历竣工，**M1 正式收官**。
逐函数过堂（tokenize / load_documents / chunk_documents / build_index /
search / main）：六项全部判决"不动"——现有规模（12 docs / 29 chunks）
下所有候选优化均过不了优化三规则第 3 条的量级关（reserve 预估、
tf_counts hoist、ostringstream 双跳均为微秒级收益）；loader 的双跳
与 chunker 的重叠文本驻留记为 M2 重构时顺手处理项。
一条提案成册挂 M5：① search 排序 sort→partial_sort（O(m log m) →
O(m log k)，k=10；m=10^6 时排序段约 6 倍）；② scores 累加器由
unordered_map<int,double> 改稠密 vector<double>(N) 下标直寻——契约
保证 chunk_id 稠密连续 0..N-1，**账本格式反向给优化留门**（契约
设计的红利）。两处待 M5 有真数据时兑现并实验验证。
语义备案：查询词重复（libc libc）逐词累加两次 = 查询词加权，系
契约"逐查询词累加"的直译行为，非 bug，测试按此钉死。
分词规则单一事实源提案 A 已裁：M2 动工前再议（M2 必改分词规则，
现在抽公共判定件可能白抽）。
M1 四轮全部竣工：契约 → main → 实现 → 优化+测试。总产出：5 份
契约、4 份队友手写实现、37 条回归测试全绿、12 篇真实语料切 29 块、
LNK2019 4→0、端到端 BM25 检索可用。下一步 M2：中文 bigram +
索引持久化 + 增量建库 + 删除路径落地（墓碑消费）。

## [2026-09-28 02:53] A

【决策】M2 范围扩容：核心四件套（bigram / 持久化 / 增量 / 删除）
竣工后，加做 **Qt 图形化管理界面**（M2 收尾件，第五项）。
动机：命令行调试吃力——语料浏览、查询调试、结果查看、删除操作
都需要可视化。架构定位：五份契约先行，GUI 只是核心检索层的新
消费者（load/chunk/build/search 均为自由函数，无需为 GUI 改核心）；
持久化就位后 GUI 启动即用。风险预告：Qt + 中文文本是 GBK 编码
事故高危区，施工时改 .ui/.cpp 前必查编码（套用既有教训档案）。

## [2026-09-28 12:50] A

M2 开张，第一议题 = 分词规则单一事实源（09-28 02:46 拍板"M2 动工
前再议"，现在就是动工前）。AI-A 摆出三案：甲（抽公共原子扫描件，
bigram 只改一处）/ 乙（维持两份实现，契约+测试盯防）/ 丙（先 bigram
双改再抽），并给出新论据：bigram 使 tokenizer 的 token 与 chunker 的
计数单位分裂（重叠 bigram vs 单汉字计数），两端同源需从"词"下沉到
"原子"（字节区间 + 类型），英文路径同构测试必重写。
【决策】裁决暂缓，先外部征询：AI-A 执笔架构征询书
（Desktop\MY_RAG_M2_分词架构征询.md，自包含：规则判例表/三案对垒/
七个次级难点/编号提问/项目约束），A 拿去与其他 AI 交流后再裁。
次级难点一并入册：原子粒度 / 扫描件归属 / chunker 计数口径失衡 /
汉字判定范围与全角标点排除 / bigram 边界三连 / BM25 TF 膨胀 /
同构测试中文版设计。

## [2026-09-28 13:32] A

【决策】M2 单一事实源裁决落地（外部 AI 会诊回流后终裁）。
外部意见（GPT，Desktop\GPT2.md）经 AI-A 逐条对账——引用契约原文
核验为真，非幻觉——A 拍板如下：

1. **路线 = 甲″**：抽公共 Atom Scanner，tokenizer 与 chunker 双依赖
   它（非 chunker 依赖 tokenizer）；放置为 **include/atom_scan.h，
   第六份契约**（对 GPT"内部头文件"案的修正：本项目 include/ 即
   契约层，且 M2⑤ GUI 命中词高亮是预见的第三消费者，藏内部自欺）。
2. **GPT 八条细则全盘采纳**：孤字丢弃（单字查询返回空，unigram
   混合索引留 M5 复议）/ bigram 不跨 ASCII / 中文标点断 run /
   切块计数不加权（1 atom = 1 unit）/ BM25 不补偿（M5 实验）/
   三层测试（scanner 黄金 + tokenizer 组合 + chunker 边界 + E2E）/
   汉字判定先解 code point 再分类（禁 lead-byte 粗判：E4-E9 会漏
   Ext-A 段、误收易经卦符 U+4DC0-4DFF）/ ASCII 词显式 [A-Za-z0-9]
   不走 isalnum（消灭 locale 变量）。
3. **概念框架入册**：atom（切块单位）≠ token（索引单位）≠ byte
   （定位单位）。M1 中"一个英文词三职合一"是偶然重合，bigram 起
   正式拆分。BM25 的 dl 永远 = tokenize() 输出 token 数（indexer.h
   "word count"注释随之改 token count）。
4. **汉字范围冻结**：U+3400-4DBF（Ext-A）+ U+4E00-9FFF（基本区）；
   Ext-B+/兼容区不管，扩范围只加 code point 判断不推翻 scanner。
5. **本次属契约语义变更**（chunker.h 条款 2 "A word is a token as
   defined by tokenizer.h" 在 bigram 后为假），按宪法先改契约后动
   实现。施工程序四步：⓪ 契约变更五件套（新增 atom_scan.h +
   tokenizer.h 条款 2 + chunker.h 条款 2 + indexer.h 注释 + PARAMS.md
   口径）→ ① scanner 落地 + 两消费者改造（验收 = 37 条英文测试
   原样全绿）→ ② bigram 实装 + 三层中文测试 → ③ 语料重切验收
   （29 chunks 预期变化：中文开始计数）。
AI 参与：AI-A 摆甲/乙/丙三案 + 原子下沉论据 + 对账核验 + 认领一处
自身推理错误（"TF ×2 均匀放大"说法被 GPT 纠正：bigram 是换 token
空间，不是均匀放大）；A 终裁。待决：atom_scan.h 及消费者改造谁写
（A 亲手 vs B/C 卡）。

## [2026-09-28 13:39] A

【决策】M2 工作模式拍板（上一条待决项清账）：
1. **M2 全程独立完成**（A + AI-A），不发 B/C 卡。流程照 M1 老样子：
   先动 .h（A 手写契约）→ AI-A 获授权填 .cpp 伪代码卡 → A 亲手
   补全真代码。M1 第三轮"A 不写实现"的策略到此为止，A 首次进入
   实现层（atom_scan / tokenizer / chunker 改造全部亲手）。
2. **M3 恢复分工下发**（B/C 任务卡 + PR 流照旧）。
3. **Qt GUI 全权 AI-A 执笔**：UI 设计不算核心算法，同测试代码
   先例（AI 执笔、A 审阅）；核心检索层零改动原则不变。
AI-A 附议：atom_scan 由 A 亲手写收益最大——M2 地基 + 实现层首秀。
