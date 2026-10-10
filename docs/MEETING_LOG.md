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

## [2026-09-27 01:12] C

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

## [2026-09-27 03:26] C

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

## [2026-09-28 13:58] A

【决策】atom_scan.h 设计讨论定稿（AI-A 教练对话产出，A 逐项拍板）：

1. **签名走全量式**：`vector<Atom> scan_atoms(const std::string&)`。
   两消费者检验胜出——tokenizer 组 bigram 需 lookahead 下一个原子、
   chunker 两遍法本就要全表；与 tokenize() 返回 vector 的项目风格
   一致。量级关：500 atoms x 24B = 12KB，无需拉取式省内存。
2. **Atom = { begin, end, kind }**：size_t 对齐 Chunk::begin/end
   （atom 边界直接喂切块下刀）；kind 用 enum class（堵隐式转 int
   的手滑比较）；**不存 text 子串**——"推导成本 x 使用频率"判据的
   反向应用：Chunk::text 跨 API 边界高频用所以存，Atom 是即用即弃
   中间产物（500/chunk 量级）所以不存。
3. **畸形 UTF-8 = 跳一个字节继续，不抛异常**。双链论证：契约链
   （search 不抛承诺 → scanner 链上不能 throw）+ 现实链（A 补充：
   投喂的 writeup 不保证干净，若抛异常一个脏文件就能让索引启动
   瘫痪；容忍降级优于全局崩溃）。GBK 文件混入 data/ = 该篇中文
   检索不到但程序不崩，可接受降级。
4. 契约七条草案成立（顺序不重叠 / ASCII_WORD 显式 [A-Za-z0-9]+三明治
   / CJK_CHAR 码点范围 U+3400-4DBF+U+4E00-9FFF / 字节偏移左闭右开 /
   其余皆分隔符 / 畸形防御跳字节 / 宪法条款：tokenizer 与 chunker
   禁止私实现字符规则）。
   下一步：A 亲手创建 include\atom_scan.h，英文注释自组织，贴出
   review + 字节自检。其余四件契约改写待本份关账后依次过。

## [2026-09-28 15:11] AI-A

【决策】atom_scan.h 关账（第六份契约，M2① ⓪ 步第一件竣工，1/5）。

- 施工方式特例（A 拍板"直接写出来不用文档"）：A 在 VS 亲手创建
  文件壳（#pragma once），AI-A 执笔契约全文，A 复审——宪法 §3 的
  一次性授权，同测试代码先例，不构成产品代码惯例
- 复审战果（AI 执笔 + A 审阅模式首杀）：A 抓出 struct Atom 尾注
  "never crossing an API boundary" 与签名自相矛盾（返回
  vector<Atom> 恰好跨 API 边界）。按 A 方案一并逐字修复：不存字节
  拷贝 / 即用即弃中间产物 / 同一调用链内被 tokenize() 与
  chunk_documents() 消费
- A 复审另两项执行：e.g. 判例补入（施工单 8 条判例漏写 1 条）+
  条款 2 三段式排版（规则—判例—禁区）。e.g. 处加半句澄清：
  第一个 '.' 入词、尾部的剥离——同一字符在同一词内两次判定相反，
  三明治规则最浓缩的展示窗
- 验收：字节自检 PURE ASCII（2818 字节）；cl /W4 临时翻译单元
  语法烟测零告警
- 施工单（docs/design/atom_scan-spec.md）转设计存档，头部状态
  已同步实际施工方式
- 下一步：其余四件契约改写依次过——tokenizer.h 条款 2 →
  chunker.h 条款 2 → indexer.h word count 注释 → PARAMS.md
  500/50 口径

## [2026-09-28 15:35] AI-A

【决策】tokenizer.h 条款 2 bigram 改写关账（M2① ⓪ 第二件，2/5）。

- 设计定稿（AI-A 教练对话，A 三项拍板）：P1 条款 1 精确化保留
  （"ASCII letters come out lowercase; Han characters have no case"，
  不并入条款 2）；P2 孤字丢弃的用户可见代价（单字查询返回空）
  不进契约——13:32 条已有案，unigram 混合挂 M5 复议；P3 判例全进
- 新契约结构：条款 2 重写为组合政策两段式——(a) ASCII_WORD atom
  逐个小写化输出；(b) 极大字节相邻 CJK_CHAR run 内滑窗 bigram，
  run 长度 1 不产 token；字符级规则零复述，权威引用 atom_scan.h
  （单一事实源条款在消费端落地）。条款 3/4/include 原样
- A 起草英文全文，AI-A review 抓出一处定义级矛盾：run 定义词
  "consecutive"（列表相邻读法）vs 补丁句 "separator terminates
  the run"——判例 天。下（U+3002 不产 atom，atom 表里两汉字恰相邻）
  按列表读法会产出跨标点 bigram 天下；修法 = 定义词换
  "byte-adjacent" + 括号半句（each atom starts exactly where the
  previous one ends），三个断 run 条件（ASCII atom / 分隔符 / 畸形
  字节）统一为同一定义的推论。与 atom_scan.h 复审的 API 边界矛盾
  同物种：定义位与补丁位打架，实现者随机选边
- 判例 +1：U+5929 U+0020 U+4E0B -> {}（真输入空格显式记 U+0020），
  兼治表头 notation 歧义（码点间空格是记法不是输入字节）
- 落盘方式：A 授权 AI-A 执笔（第二次实例授权，同 atom_scan.h
  先例；A 草稿原文 + AI-A 两处复审修，不构成惯例）
- 验收：字节自检 PURE ASCII（1989 字节）；run_tests.bat 37 条
  全绿（tokenizer 12 + loader 7 + chunker 7 + indexer 11，含
  test_tokenizer 编译烟测）。契约先行中间态安全兑现：现实现对
  英文路径的行为与新条款 (a) 一致，中文路径已定义未实现（归
  ① 步改造）
- 下一步：chunker.h 条款 2 改 atom 计数（⓪ 第三件，"A word is
  a token as defined by tokenizer.h" 作废换血）

## [2026-09-28 15:39] AI-A

【决策】⓪ 契约五件套竣工：chunker.h 条款 2 + indexer.h dl 注释 +
PARAMS.md 口径一次改完（3/5、4/5、5/5）。

- 落盘方式：A 批量授权（"剩下的几个 .h 你一起改了之后统一讲"，
  第三次实例授权，批量式；关键设计判断统一汇报由 A 事后审）
- chunker.h：计数单位 words→atoms（含 CHUNK_SIZE/CHUNK_OVERLAP
  常量注释、条款 3 "Atom-to-byte"、条款 4 同步）；权威引用
  tokenizer.h→atom_scan.h，chunker 对 tokenizer 的依赖归零
  （甲″双依赖落地）；点题句入契约："a Han run of n characters
  is n atoms, not n-1 bigrams -- the bigram pairing happens later,
  inside tokenize()"（钉死计数不加权 + 防"在 chunker 里数 bigram"
  的错误实现）；include 不动（scan_atoms 由 chunker.cpp 消费）
- indexer.h：chunk_lengths 注释 word count→token count（两处），
  契约条款 3 加防混句 "with bigrams this is at most the atom
  count"——atom ≠ token 拆分在 dl 侧的钉子（500 atoms 的纯中文
  块 dl ≈ 499 bigram，不等于 500）
- PARAMS.md v1.1：500/50 口径 atoms + 含义列注明 1 atom = 1 个
  ASCII 词或 1 个汉字；中文块字节更轻的观察入调参方向列（汉字
  3B/atom vs 英文词 ~6B/atom，500 atoms 中文块 ≈1.5KB 英文块
  ≈3KB），挂 M5 复核均衡——体感观察按 P2 先例进 PARAMS 不进契约
- 验收：chunker.h / indexer.h 双 PURE ASCII（1309 / 2341 字节）；
  run_tests.bat 37 条全绿（契约先行中间态持续安全）
- **⓪ 全部完成**：atom_scan.h + tokenizer.h + chunker.h +
  indexer.h + PARAMS.md 五件齐装，契约层 bigram-ready。下一步 =
  ① scanner 落地 + tokenizer/chunker 双改造（A 亲手实现，13:39
  拍板；AI-A 出伪代码卡，验收 = 37 条英文测试原样全绿）

## [2026-09-28 15:52] AI-A

① scanner 落地开工：A 在 VS 亲手创建 src\atom_scan.cpp，AI-A 获
授权写入伪代码施工卡（M2① 第 1/3 张，四件套：复述题 / 三段伪代码
/ 15 条黄金自测表 / 施工规则；UTF-8 带 BOM 供 VS 显示，竣工时卡
删光另存无 BOM）。姊妹卡 2/3（tokenizer.cpp 改造）、3/3（chunker.cpp
改造）待本卡竣工后依次发。

【契约发现】atom_scan.h 条款 2 "'-' is unconditionally
word-internal" 措辞与 M1 语义冲突：字面读法（词内 = 需两侧词字符）
使边缘连字符与孤立 "-" 不成 atom；但 M1 契约（09-26 变更，A 批准）
原文是"连字符**无条件入词**"，且 test_chunker 第 6 条（孤立连字符
成词）钉死旧行为，① 验收标准（37 条原样全绿）强制 M1 语义。定性：
AI-A 起草 atom_scan.h 时把"入词"压缩成"词内"，措辞失真，drafting
bug 而非语义变更。修法提案：条款 2 首弹改 "a maximal run of
[A-Za-z0-9-]"，判例 stay-whole 清单补 "-abc, abc- and a standalone
-"。待 A 拍板后 AI-A 改契约。施工卡伪代码与黄金表（第 8 号）暂按
M1 语义编写。

今日定义级矛盾三连（复审模式双向战果）：A 抓 AI-A（atom_scan.h
API 边界）→ AI-A 抓 A（tokenizer.h consecutive）→ AI-A 抓 AI-A
（atom_scan.h 词内措辞）。

## [2026-09-28 16:37] AI-A

① scanner 落地竣工（待 A 关账）：A 亲手翻译三段真码（本人首次
实现层产物，含超卡进化 is_alnum/is_word_char 帮手层——GPT 八条
"显式字符集不走 isalnum"随之落地），复审过。

【留痕】A 授权"修什么自己改"，AI-A 改 A 的 atom_scan.cpp 三处：
1. include 补 <cstdint>（复审必改：uint32_t 无传递路径，<utility>
   A 已自补）
2. 删三明治分支 `if (j == 0) break;` 死分支（卡注 1：词首集不含
   '.'/'_'，可达时 j>=1），换一行推论注释承载知识（卡删光后载体）
3. 修正 cp > U+10FFFF 注释理由：原句"不判会吞掉后面的真词"不成立
   ——源头是 AI-A 卡段 3 注（A 注释继承同错）。证明：F4 引导的
   溢出序列续字节必为 80-BF，判畸形走跳 1 字节级联与跳整段消耗
   相同跨度，scan_atoms 外部不可观察。检查保留改标 defense-in-depth
   （解码器自守"只返回合法码点"承诺），黄金表 18 号钉行为。

【黄金表首跑 17/18】7 号 e.g. 期望 (W,0,4) 为 AI-A 卡笔误——词是
"e.g"（首点入词尾点剥），应为 (W,0,3)；实现正确，契约判例 [e.g]
与现行 tokenizer 行为佐证。"测试红先审测试"反向验证（B 09-28
教训先例）。18 号 "\xF4\x90\xBF\xBF"+"a" -> (W,4,5) 新增钉 cp 守卫。

验收三件全过：字节自检双零（5728B，无 BOM 纯 ASCII，原 BOM 已剥）
/ 黄金表 18 条全绿（%TEMP% 临时 main，用完即删）/ run_tests.bat
37 条全绿 + 主工程 msbuild 冒烟（12 docs / 29 chunks，libc 查询
top-1 命中救赎之道 WP，退出码 0）。

## [2026-09-28 17:35] AI-A

【决策转记】A 拍板"修改加测试并且提交"——卡 2/3 tokenizer.cpp
改造关账。

- 复审通过 + 超卡进化记：bigram 用"首字构造 + 次字 append"两段式，
  不依赖"汉字 = 3 字节"不变量，将来扩 Han 范围（只改 scanner 的
  code point 判断）本文件零改动存活；ascii_to_lower 显式区间替代
  tolower，locale 变量清零
- 【留痕】A 转授权 AI-A 收尾一处：删 `if (run_len >= 2)` 冗余守卫
  （for 的 k+1<run_end 自限，同 atom_scan.cpp j==0 同物种处理），
  注释挪至 for 上方承载孤字丢弃语义
- 测试扩容 37 → 50（AI-A 执笔 A 审，02:32 先例）：test_tokenizer
  12 → 25 条——clause 2(b) 中文组合 10 条（\uXXXX 记法保纯 ASCII）
  + 镜像/连字符判例 3 条（a-.b / a.-b / -abc）；全绿
- 【事故留痕（AI-A）】加用例脚本第一版用 write_text 先截断后因
  编码失败炸出，test_tokenizer.cpp 一度清零，`git checkout` 秒级
  恢复重做，零净损失。教训：改文件用 write_bytes 且编码在写前完成
- 工具链两笔：① run_tests.bat 三个链接单元（tokenizer/chunker/
  indexer，凡带 tokenizer.cpp 者）补挂 src\atom_scan.cpp——依赖边
  tokenizer→scanner 由 LNK2019 教出；② vcxproj 补 ClCompile 登记
  atom_scan.cpp——16:37 冒烟通过实为"无消费者假阳性"（scan_atoms
  无人引用，链接器不需要它），"VS 新建 .cpp 自动挂 vcxproj"假设
  在 A 的创建流程中未成立，今日有消费者后穿帮
- 冒烟：12 docs / 29 chunks 照旧，libc top-1 同文档；分数微移
  2.79402 → 2.80597 = 语料中文开始以 bigram 计入 dl 的预期效应
  （avgdl 变、排序不变），② 中文测试 + ③ 语料重切后全面复核

TASKS.md 推 v2.3。① 双改造进度：tokenizer ✅，chunker（卡 3/3）
待发。

待 A：关账裁决；连字符契约修法（15:52 条，仍欠一个字）；commit
bundle 批准（atom_scan.h + atom_scan.cpp + spec 存档 + 纪要今日
全部条目）。

## [2026-09-28 16:43] AI-A

【决策转记】A 三连拍板（"三件事都做"）：① 卡 1/3 atom_scan.cpp
关账 ② 连字符契约修法批准（15:52 条的"改"字落地）③ commit
bundle 批准。

契约修已执行（atom_scan.h 条款 2）：首弹 [A-Za-z0-9] →
[A-Za-z0-9-]；连字符弹补 "including at the leading and trailing
edges of a word"（堵死"词内"字面歧义，措辞与 A 实码注释同款）；
判例 stay-whole 清单补 "-abc, abc- and a standalone -"。字节复检
PURE ASCII（2931B）。至此 15:52 契约发现闭环：契约文本与实现
（M1 语义）对齐，test_chunker 第 6 条与 37 条验收基线不再悬空。

TASKS.md 推 v2.2（① scanner 竣工入册，卡 2/3、3/3 随 A 节奏发）。
commit 一次入账：atom_scan.h（条款修）+ atom_scan.cpp（新）+
TASKS.md + 本纪要今日全部条目。

## [2026-09-28 16:55] AI-A

① 卡 2/3 发放：tokenizer.cpp 改造卡写入（13:39 工作模式第二张，
AI-A 填卡 A 翻译）。结构：复述题 5 道（token/atom 关系、条款 2
禁令、byte-adjacent 两读法、孤字代价、dl 口径）+ 两段伪代码
（段 1 组合主循环+ASCII 分支显式小写化；段 2 run 探测 + bigram
滑窗 + 孤字丢弃零特判）+ 黄金表 20 条（1-10 英文回归，
11-20 中文组合与契约判例同源）+ 施工规则 7 条。旧 M1 实现
（isAlphaNum / isWordCharAt / 双游标 / tolower，约 35 行）挂
"作废参考"横幅保留在卡下方，译完即删——字符规则本地实现系
新契约条款 2 禁令（"must not re-implement them"）。契约走查
无新发现。验收 = 37 条全绿（test_tokenizer 12 条为主考官，
英文行为原样不变）+ 冒烟。

## [2026-09-28 17:36] AI-A

【决策转记】A 三令（"推 额改好吧 继续推进"）：push 批准 /
vcxproj /W4 修复批准 / 卡 3/3 发放；卡 2/3 随令关账。

- push 落地：8a3bdda..389c171（214d2cf / 6877a57 / 389c171）
  上 origin/main
- 【留痕】vcxproj 四配置 WarningLevel Level3 → Level4：M1 记录
  （09-22 21:00）声称 /W4 已配全配置，实况 Level3——推测 22:00
  三次清场重建时丢失；此前未穿帮因 PR 验收全走命令行 cl /W4
  （run_tests.bat 亦 /W4），缺口仅在 VS F5 / msbuild 主工程。
  修复后 msbuild Debug|x64 全量重建零告警（main.cpp 首次过 /W4
  一次干净），冒烟 12 docs / 29 chunks、libc top-1、2.80597 不变
- 卡 2/3 验收三件套复核（A 阅）：字节双零（2065B / 3797B）+
  50/50 全绿 + 冒烟过
- ① 卡 3/3（chunker.cpp 改造）发放：复述题 5 + 伪代码 2 段 +
  黄金表 15（三条照妖镜：501 字 bigram 口径 / 全角标点误产
  atom / 畸形吞邻字）+ 施工规则 7；B 的 M1 双遍私扫挂作废参考
  横幅保留，UTF-8 带 BOM 落盘（9701B），卡体不提交（先例）。
  验收 = 37 条英文测试原样全绿 + 黄金表 + 冒烟（29 chunks
  预期变化：中文正式计数）。本卡竣工即 ① 双改造收官，转入
  ② 中文三层测试

## [2026-09-28 17:50] AI-A

【决策转记】A 批准卡 3/3 关账 + commit bundle + push。① 双改造
收官。

- 复述题 5/5：Q1-Q3/Q5 满分；Q4 生产级陷阱当场裁决——A 答题中
  的 `byte_end = (e < n) ? atoms[e].begin : text.size()` 与
  卡的 `atoms[e-1].end` 非同一边界（501 词文档会吞 w499/w500
  之间空格、末块挂换行尾巴），复述题制度的价值实证。实码按
  裁决落地，无三目分支
- 实码复审：卡删光（47 行 / 1718B）；<cctype>/<utility> 随私扫
  陪葬；"kind 不用"的理由写进注释（卡注 1 的知识载体）；显式
  赋值 / 全局连续 id / 空文档 0 块逐条对契约
- 【留痕】BOM 由 AI-A 剥（VS 保存带出，tokenizer 同款先例）；
  双零复检过（0 BOM + 0 非 ASCII）
- AI-A 独立复跑黄金照妖镜 14/14（%TEMP% 临时 main）：501 字两刀
  [0,1500)/[1350,1503)、全角句号不产 atom（500 原子恰 1 块）、
  畸形字节不吞邻字（8 字节块）、600 字重叠段 150 字节逐字节
  相同、501 词尾部不吞空格
- 验收：run_tests 全绿（37 条英文原样 + tokenizer 中文 13 条）；
  冒烟 12 docs / **30 chunks**（29→30，中文 atom 正式计入切窗的
  预期漂移坐实）；libc / stack overflow top-1 均救赎之道 WP；
  format string 合理命中；heap 返空经 grep 实证 = 语料真无此词
  （契约"未知词返空"正确履职，非掉数据）；分数 2.79907
  （重切后 avgdl 再平衡，排序稳定）
- 【里程碑】M2① 双改造收官：scanner / tokenizer / chunker 三件
  全部消费 scan_atoms，第六契约甲″路线在地基层贯通。TASKS 推
  v2.4。下一步 = ② 三层中文测试 → ③ 语料重切验收 → M2 主体
  （持久化 / 增量 / 墓碑消费 / Qt GUI）

## [2026-09-28 18:01] AI-A

【决策转记】A 批准"② 三层中文测试 + ③ 语料重切验收一轮做完"；
测试代码 AI-A 执笔 A 审（02:32 先例）。②③ 同轮竣工，**M2① 全部
收官**。

- ② scanner 层：tests/test_atom_scan.cpp 新建 18 条（卡 1/3 黄金
  表转正）：三明治/连字符边缘 10 条 + Han 四界码点（U+3400/
  U+4E00/U+9FFF/U+4DBF）+ 全角句号断 run + U+4DC0 易经卦符
  （lead-byte 粗判陷阱的钉子）+ U+1F600 星面码点整吞 + 0xFF
  单字节跳 / 截断级联 / F4 越界 4 字节跨度
- ② chunker 层 +5：照妖镜三条转正（501 字两刀 / 全角标点不产
  atom / 畸形不吞邻）+ 600 字重叠 150 字节逐字节一致 + ASCII
  与 CJK 同权计数；② indexer 层 +2：中文 bigram E2E 命中 +
  孤字查询返空。tokenizer 层 13 条此前已随卡 2/3 落地
- run_tests.bat 挂第五测试单元（atom_scan，链接仅
  atom_scan.cpp）；套件 50 → 75 全绿；三个测试文件字节双零
- ③ 验收入册：12 docs / 30 chunks；主题查询 top-1 全对——
  libc / stack overflow / **救赎之道（中文查询真语料首杀，
  score 8.41）** 均命中救赎之道 WP；morse → yaho；RSA prime →
  Cosmic_Broadcast
- 【事故留痕（AI-A）】截断耙子当日二进宫：修脚本第一版
  `open(p,'wb').write(d.encode('ascii'))`——参数求值顺序 open
  先执行=先截断，encode 后炸=写未执行，test_atom_scan.cpp 一度
  清零（未提交过，git 救不了，凭上下文重写恢复）；第二版又在
  空文件上"成功"写空。17:35 教训"write_bytes 且编码在写前完成"
  的变体：**编码必须发生在 open() 求值之前**，且写脚本失败后
  必须先查文件大小再重跑。终版脚本：encode 先行 + 空文件拒写
  双保险
- 【工具链新发现】Write/Edit 内容里的 \uXXXX 会被参数层按
  JSON 转义解码成中文字面量——纯 ASCII 测试文件的 \uXXXX 记法
  只能"先写字面量再 py 脚本转换"两步走（tokenizer 先例重演，
  本次根治为脚本流水线）
- 【语料情报】A 移交两语料源：WP_cn 12 篇（npusec 中文版，与
  data/ 英文 12 篇同源互译）+ ctf-wiki writeups 35 篇（ISCC
  2026 等外赛，新知识，misc/mobile/pwn/re/web）。AI-A 建议挂
  M2-③ 增量建库当增量弹药（12 → 63 篇扩容 = 增量 + 持久化的
  实弹考题，顺手考 loader 字典序 id 稳定契约），入库政策待
  A 拍板（15:30 先例）
- M2① 至此全竣工（⓪ 契约五件套 + ① 双改造 + ② 三层测试 +
  ③ 重切验收）。M2 主体开工序：② 持久化（契约先行，A 亲手
  .h）→ ③ 增量建库 → ④ 删除/墓碑消费 → ⑤ Qt GUI

## [2026-09-28 18:20] AI-A

【决策转记】A 拍板语料甲案（F5 手动验收触发）：WP_cn 12 篇现在
手动入库，ctf-wiki 35 篇留 M2-③ 增量渠道当实弹考题；commit +
push 一并批准。

- F5 验收插曲（拍板动因）：A 首次手动 F5（/W4 修复后主工程），
  12 docs / 30 chunks 横幅正常，救赎之道命中；但 红警 / 时间 返
  空、牛 返空。AI-A 三诊断：红警/时间 = 语料真无（grep 实证，
  data/ 全部中文家当仅 29 个题名词组 vs WP_cn 2352）；牛 = 孤字
  丢弃契约（test_indexer #13）+ 全语料无此字，双重空。中文版
  没入库 = 18:01 待拍板项，甲/乙/丙三案摆出，A 选甲
- 入库执行（A 授权 AI-A 拷贝）：WP_cn 12 ↔ data/ 12 逐一映射
  （呀吼 ↔ yaho 同对）；文件名走 EN 孪生名 + _cn 后缀——15:30
  ASCII 目录先例延伸（std::filesystem 窄字符 API 遇中文文件名
  产出 GBK 路径字节，UTF-8 控制台显示必乱码）；原件不动，内容
  字节不变；入库前置体检：两源 47 篇全 UTF-8 零 GBK 嫌疑
- 冒烟：**24 docs / 77 chunks**（30 → 77，exit 0）。救赎之道
  top-3 全 CN 版（15.73 / 14.21 / 13.89），第 4 名才是 EN 版
  元数据 7.62——CN 全文对 EN 题名的检索力差实锤分层；EN 版同
  查询 8.41 → 7.62 = 互译双份下 df 变化的预期效应。时间 8 命中
  （咋瓦鲁多 top-5 + 雪里看花）、红警 1 命中（咋瓦鲁多）——与
  grep 预测逐一对上；libc 查询 CN 版反超 EN 版（5.32 > 2.76，
  CN writeup 英文术语密度更高，预期内）
- 文档同步：README 语料 2 处 + 状态区（阶段行/契约六份/75 测试/
  M2① 竣工）、PRINCIPLES 2 处（当前阶段行 + 语料快照）、TASKS
  推 v2.6；ctf-wiki 35 篇（含 mobile 新类目）挂 M2-③，届时
  id 稳定性（字典序下标遇中间插入连锁位移）正是渠道设计核心考题

## [2026-09-28 18:25] AI-A

【决策转记】A 手动 F5 验收语料甲案通过（24 docs / 77 chunks，
中文查询行为符合预期），下令开下一部分。**M2-② 持久化开张**：
契约先行（A 亲手第七份 .h），设计讨论启动——三议题：存什么 /
  格式选型（二进制 vs JSON）/ 失效检测（语料变动怎么发现）。
讨论记录随后续条目入册。

## [2026-09-28 18:32] AI-A

【决策】M2-② 持久化设计冻结（A 交回冻结结论拍板生效：乙 + 自定义
二进制 + FNV-1a manifest，另补三条红线 = 双版本维度 / 确定性序列化 /
损坏降级重建）。AI-A 对账：契约引用逐条核验为真——

- Chunk::text = content.substr：chunker.h 条款 3 原文；begin/end
  半开字节区间：type.h（且为 size_t——平台宽度类型，固定宽度
  磁盘协议的直接论据）
- id is never recycled：type.h 墓碑注释原文
- postings chunk_id 升序：indexer.h 账本红线（排序写盘的内部序
  前提成立）
- avgdl 为 chunk_lengths 汇总：indexer.h 条款 4 成立。**AI-A 对账
  新增一抓**：条款 4 同时规定空索引 avgdl=0.0，load 侧重建必须带
  同款守卫（sum/size 在空表上是除零）
- "你们要求 MSVC + g++"轻微失真（双编译器是 B 的自测方法论，
  M2 全程 MSVC），不改结论

冻结结论十四项（存储集合乙 / text 不存 substr 恢复 / avgdl 不存
从 chunk_lengths 恢复 / deleted 存 / 自定义二进制 / 禁裸 dump 逐字段
编解码 / 固定宽度整数 / postings 按 term 字典序写盘 / magic +
FORMAT_VERSION + PIPELINE_VERSION 三层头 / CHUNK_SIZE/OVERLAP 入
盘并核对 / 排序 manifest 相等判定 / 损坏视 cache miss 全量重建 /
临时文件写全后替换 / 不提前设计 M2-③ 局部改表）。

AI-A 补充工程注记：① PIPELINE_VERSION 是全设计唯一手动 bump 的
零件，需定住址 + 触发清单注释（Han 范围 / bigram 政策 / 三明治
规则 / 切块参数变动都要 bump，漏 bump = 静默旧索引）；② validate
仍要读全语料算指纹——持久化省的是 chunk+tokenize+build，不省
文件 I/O（预期管理）；③ Windows rename 覆盖语义实现卡里验证，
兜底 remove+rename；④ M2-④ 交互点预告：墓碑 vs manifest（被删
文档的文件若离开 data/ 会触发全量重建把文件加回来），M2-④ 再裁；
⑤ save→load→save 字节一致性背书 → test_persist 基石用例。

开放小项（写 .h 前待 A 定）：load 返回即用状态（text 在 load 内
填好，"与 fresh build 逻辑等价"验收要求它）/ PIPELINE_VERSION
住址 / magic 字符串 / persist.h 只做哑 save-load-validate，"有效
则 load 否则 build"的分支放 main。

下一步：A 亲手写 include/persist.h（第七契约），贴出 review +
字节自检。

## [2026-09-28 18:47] AI-A

【决策转记】A 四项开放小项拍板（外部意见辅助，A 终裁）：

1. **load 返回即用状态**：load() 内部按 document_id + begin/end
   从 Document::content substr 恢复 chunk.text，调用者拿到即可
   search，main 不做二次修复
2. **PIPELINE_VERSION 放 persist.h**（不放 atom_scan.h）：它是
   chunk→tokenize→index 整条派生管线的语义兼容版本，不属于任一
   底层模块；放 atom_scan 附近会误导"只有 scanner 改才 bump"
   （tokenizer/chunker/index/切块参数/embedding 模型变化都要 bump）
3. **magic = MYRAGIDX**：固定 8 字节 ASCII，无 \0 终止
4. **persist.h 只做哑三件**：save / load / validate；"有效则 load
   否则 fresh build + save"分支在 main；persist 不调用
   loader/chunker/indexer，防职责膨胀、不提前混入 M2-③

AI-A 对账：本条引用（type.h substr 语义 / README main 总装定位）
已在前条核验为真，四项与冻结结论自洽，无异议。

**API 形状三小题摆出**（写 .h 最后拼图，AI-A 教练 A 拍板）：
① 传参形态：三独立参数 vs Snapshot 聚合结构体；
② validate 签名：AI-A 提案 validate(snapshot, docs)——main 先
load_documents（读文件不可避免），把 docs 喂给 validate 哈希
content 比对 manifest：persist 不走目录、不依赖 loader、哈希用
内存字节零重复 I/O；目录遍历政策（.md/.txt 递归字典序）留在
loader 单一事实源，不复制
③ 失败原因报告：bool vs enum（五类失败分开报，启动打一行
人话"快照过期：pipeline 不符"，调试价值大）

## [2026-09-28 18:55] AI-A

【对账】API 形状三小题外部意见回流，AI-A 逐条核验 + 附议：

- 引用三处全真：Chunk::text 由 content + [begin,end) 唯一决定
  （chunker.h 条款 3 substr 原文）/ loader 契约独占递归 + .md/.txt +
  相对路径正斜杠 + 字典序 + id 分配（loader.h 原文）/ 契约先行纪律
  （PRINCIPLES.md §2）。无幻觉
- 三项裁定全附议：①甲（三独立参数，无 Snapshot 壳）+ load 强保证
  ——全有或全无，失败时三个输出保持原状，text 在 load 内恢复后
  返回即用；②validate(snapshot_path, docs) 采纳 + 纯度钉死：只比
  path + content，不比 id/deleted（manifest 回答"磁盘输入是否同一批
  字节"，非运行时状态）；当前 loader docs 是现实探针，验证后使命
  结束，运行时 docs 由 load 从快照恢复——main 侧变量命名应区分；
  ③enum 采纳，状态集八字：OK / NOT_FOUND / IO_ERROR / BAD_MAGIC /
  BAD_FORMAT / BAD_PIPELINE / BAD_PARAMS / CORPUS_CHANGED
- AI-A 自认两处出题盲区被补：NOT_FOUND（首启无快照是正常态非失败）
  与 IO_ERROR（文件在但读不动，与内容坏分开）；BAD_FORMAT 内部
  不再细分（main 动作相同，M2 不为诊断颗粒膨胀 API）——均附议
- validate 与 load 不合并（validate_and_load 反模式）——与 A 18:47
  "哑三件"裁定同向，附议
- **AI-A 对账新抓（第四小题，待 A 拍）**：五类失败检测存在载荷盲区
  ——magic/版本/参数/manifest 全过但载荷中段位翻转（结构仍可解析）
  时会静默加载出错误索引，"损坏 = cache miss"红线在此场景失明。
  提案：头部 +8B 载荷 FNV 指纹（save 写入、validate/load 核对，
  不符报 BAD_FORMAT），成本 8 字节 + 一次 FNV 顺序扫描；快照格式
  未出厂，现在改零代价；与确定性序列化自洽（载荷确定 → 指纹确定，
  save→load→save 字节一致性不受影响）；定位沿用 manifest 同款
  "content fingerprint"（防意外不防对抗）。甲 = 加 / 乙 = 不加
  （信本地磁盘，坏例后果 = 分数悄悄错不崩）。AI-A 倾向甲
- main 侧预案一句（不进 persist.h，写 main 时记得）：validate OK
  但 load 半路 IO 失败 → 兜底 fresh build + save
- 第四小题拍定后 A 动笔 include/persist.h（第七契约）。写作骨架
  （英文 A 自组织）：三签名 + SnapshotStatus enum（八字）+
  PIPELINE_VERSION（含触发清单注释，本设计唯一手动 bump 零件）+
  FORMAT_VERSION + 契约条款（乙案存储集 / text 不存 substr 恢复 /
  avgdl 空索引 0.0 守卫 / load 全有或全无 / validate 纯度 path+content /
  save 临时文件全写后替换 / 失败判定优先级 magic→format→pipeline→
  params→manifest 写进契约保测试断言确定性 / 快照路径由 main 传入
  persist 不规定位置）

## [2026-09-28 19:08] AI-A

【决策转记】A 两令：① 第四小题拍板**甲案**——头部加 8B 载荷 FNV
指纹（16 -> 24 字节头），save 写入、validate/load 核对，不符报
BAD_FORMAT；② A 自述首次做 C++ 持久化设计"有点不能理解"，令
AI-A 单独写一份解释文档。

- 附带三件动笔小事一并入册（18:55 对账补充项定稿）：CHUNK_SIZE/
  OVERLAP 出处 = persist.cpp include chunker.h（persist.h 只引
  type.h + indexer.h + 标准库）；三函数异常政策 = 全不抛，失败全走
  返回值；load 自检头部（defense-in-depth，同 atom_scan cp 守卫先例）
- 实现阶段备忘两笔：.gitignore 补快照文件（派生缓存不进仓库）；
  test_persist 验收矩阵（enum 七坏一好 + 字节一致性 + load ≡ fresh
  build + 空语料往返 + load 后 id 与 loader 重编号一致）
- **设计解读文档交付**：`docs/design/persist-explained.md`（A 点名，
  教学向，13 节 + FAQ：缓存总纲 / 乙案判据 / 内存结构 ≠ 磁盘协议
  两堂课 / 24B 头字节布局图 / 三版本维度灵魂 / validate 七问八态 /
  FNV 指纹与排序比对 / 确定性序列化 / 原子替换 / 全有或全无 /
  API 分工与 main 流程 / M2-③④ 边界 / FAQ 五问）。同 atom_scan-spec
  先例归 docs/design/；非契约，冲突时以 persist.h 为准
- 设计阶段至此闭环，无阻塞疑问。下一步：A 读文档 -> 亲手写
  include/persist.h（第七契约）-> 贴出 review + 字节自检

## [2026-09-28 19:52] AI-A

【对账】persist.h 设计稿（19:40 一稿）外部意见回流三条，AI-A 逐条
核验，A 待拍板：

- 建议 1（**采纳**）：save 条款 1 存储清单漏头部字段（magic / 双
  版本 / 载荷指纹）与 CHUNK_SIZE/OVERLAP——BAD_PARAMS 等检查隐含
  其入盘但条款未声明，"存什么"必须唯一权威。补丁 1 已出
- 建议 3（**诊断采纳、处方驳回**）：外部 AI 不知 19:08 甲案上下文，
  误把 payload fingerprint 当可删的 CRC 提案。它是 A 当日拍板的
  载荷指纹（8B FNV-1a 盖偏移 24 后全部字节，堵"五关全过但载荷
  中段坏"盲区），删除 = 撤销既有裁定；BAD_CHECKSUM 拆分 18:55
  已裁不做。但诊断成立：草稿提及未声明名词 = 契约不自包含
  （atom_scan.h API 边界矛盾同物种），随补丁 1 一并治好。方法论
  入册：契约自包含检验标准 = 不知上下文的实现者光读 .h 能否动工
  ——本轮外部 AI 无意间完成一次真实验收
- 小问题（**采纳**）：load 条款 1/2/4 张力——失败时 docs 保持
  探针态未明说，fallback 安全性显式化。补丁 3 已出
- 位置小纠：CHUNK_SIZE/OVERLAP 按 §4 布局住载荷开头，不在 24B
  头内；外部 AI"头部字段"表述不确，补丁按 §4 写
- 补丁三块：save 条款 1 整换（完整清单 + 双指纹命名区分 payload
  fingerprint / content fingerprint）/ BAD_FORMAT 不动（自包含
  已达成）/ load 条款 1 整换（untouched 语义显式到 probe）

待 A 拍板三条判决后落笔 persist.h。外部意见若有建议 2 未转达，
一并贴来。

## [2026-09-28 20:05] AI-A

【决策转记】A 亲手写完 include/persist.h（第七契约，基于补丁前
设计稿誊写，6583B 纯 ASCII 无 BOM，誊写零错只有一处注释续行顶格），
并授权"你自己改一下吧 / 别忘了加到 VS 项目里"——AI-A 修订 + 项目
登记一次完成。19:52 三条判决以本授权实质采纳（按补丁执行）。

- 【留痕】AI-A 改 A 的 persist.h 三处（一次性实例授权，同
  atom_scan.cpp 先例）：
  1. save 条款 1 整换 = 补丁 1：快照内容完整清单（头部
     magic / 双版本 / 载荷指纹 + 载荷 CHUNK_SIZE/OVERLAP + 数据集
     +"nothing else is stored"），治"存什么"唯一权威
  2. BAD_FORMAT 枚举注释续行对齐修复（A 誊写时顶格，纯排版）
  3. load 条款 1 整换 = 补丁 3：失败时三输出 untouched 显式到
     "docs 保持探针态"，fallback 安全性入条款；half-loaded 句保留
- 【留痕】项目登记：vcxproj 补 ClInclude include\persist.h
  （字母序位 loader 与 tokenizer 之间）；filters 补同项挂"头文件"
  过滤器（创建序位 atom_scan 之后）。filters 为 UTF-8 带 BOM
  （138 非 ASCII 字节全为中文过滤器名），Edit 前经 py 验证解码
  良构，GBK 事故未复现
- 验收四件全过：① persist.h 字节双零（7142B，无 BOM + 0 非
  ASCII）② 两项目文件 XML 良构 + filters UTF-8 复验 ③ g++
  -fsyntax-only -Wall -Wextra 语法烟测零告警（WSL 路径；cl 不在
  bash PATH）④ A 手稿备份 %TEMP%\persist.h.A-handwritten.bak
  （commit 后可清）
- 工程注记：.h 注册 vcxproj 仅为 Solution Explorer 显示与后续
  消费；persist.h 暂无消费者，msbuild 不会编译它（17:35"无消费者
  假阳性"先例的同面），MSVC 侧真编译验证落在 persist.cpp 建壳时
- 遗留三件：A 对三处改动的终审一眼（VS 里 diff 即可）；README
  "契约六份"→七份随 commit bundle 同步；commit + push 待批
  （未提交包：今日 18:25 起全部纪要 + persist-explained.md +
  persist.h + vcxproj/filters）
- 下一步：persist.cpp 伪代码施工卡（AI-A 填卡 → A 翻译，M2-②
  第 1 张卡）

## [2026-09-28 20:20] AI-A

【决策转记】A 一声"好"三连：persist.h 关账（三处改动终审通过，
第七契约正式落账）+ commit bundle 批准 + persist.cpp 卡开工。

- 文档同步：README 3 处（M2 状态行 +② 持久化契约 / 目录树七份
  契约 +atom_scan/persist 两行 / 当前状态块改写）+ TASKS v2.7 +
  索引持久化条目挂进度注记
- 【发现】git 现场见 src/persist.cpp（0 字节空壳）——A 已在 VS
  建壳。vcxproj 暂不登记：无消费者不挂（atom_scan 17:35 先例），
  登记随 main 接线时一并做；卡期编译自检走临时 main
- 施工卡 1/1 发放（11318B，UTF-8 带 BOM，竣工时剥）：复述题 5
  （payload 先拼内存的必然性 / 全有或全无 / size_t vs 固定宽度 /
  双排序与确定性 / 探针 vs 运行时 docs）+ 伪代码六部分（地基件
  FNV+拼盘族+读盘族 / save / validate / load / 黄金表 12 条 /
  施工规则 7 条）。卡内两处裁决引用：validate 检查顺序一字不得
  调换（契约确定性）；字节序不设防（18:32 本机缓存裁定）
- commit 一次入账（7 文件：纪要今日 18:25 起全部条目 +
  persist-explained.md + persist.h + vcxproj/filters + README +
  TASKS v2.7；卡体不提交，先例）。push 待 A 示下
- 下一步：A 口答复述题 -> 翻译施工卡 -> 黄金表 + 冒烟三连 ->
  main 接线（登记 vcxproj）-> test_persist（AI-A 执笔 A 审）

## [2026-09-29 02:19] AI-A

【决策转记】A 赶时间裁定：persist.cpp 改 AI-A 执笔（一次性实例
授权，同测试代码先例；09-28 13:39"M2 A 亲手实现"工作模式就本文件
修订）。复述题 5/5 已于授权前先行通过，设计层理解入账；各题脚注
补课：Q1 seekp 反例与依赖链硬逻辑 / Q2 事务原子性 + 未触及输出
= main 兜底安全性地基 / Q3 宽度设防 vs 字节序不设防逐条算账 /
Q4 save->load->save 换插入历史是手术刀设计 / Q5 探针三段
职业生涯（比对 -> 兜底原料 -> 被覆盖退休）。

挂账清单（A 定：M2-⑤ Qt 竣工前择机清算，M3 发卡时优先研究
讨论）：① persist.cpp 实现层走读（对照施工卡六部分）② 卡缝
五处（下）③ 黄金表 3/8/12 号构造思路。

【留痕】卡缝五处认账（AI-A 填卡笔误，施工中抓获）：
1. validate 卡序逐字执行在 <24B 文件上越界读（magic/FORMAT
   检查先于长度门）——裁定：magic 守卫带 size>=8 前置，长度门
   归 check 4（截断语义）；"检查顺序管语义优先级，守卫管不读
   越界"，两层分离
2. load 恢复段漏 begin/end 合法性守卫——substr 在坏偏移上抛
   out_of_range，违反 never-throw；补 begin>end ||
   end>content.size() + lengths/chunks 等长 + postings
   chunk_id 界内三根保险（防"指纹修好的坏账本"后续 UB）
3. 卡 IWYU 清单漏 <algorithm>（sort）/ <system_error>
   （error_code）/ <unordered_map>
4. load 侧不可信计数禁 reserve——伪造计数 + reserve 可触发
   bad_alloc 抛出，违反 never-throw；向量自然增长受实际字节
   约束
5. 黄金 12 号正解澄清：payload 翻转 + 修指纹仅当落在结构性
   字节（计数/长度域）才 BAD_FORMAT；内容字节 + 修指纹 = 设计
   上不可检出（FNV 防意外不防对抗，18:55 裁定的直接推论），
   测试按结构性构造（doc 计数域翻转 + 重算 FNV 修头）

施工：persist.cpp 全量落盘（AI-A 执笔；卡体随竣工删除，卡体
本就不提交，先例）。地基件（fnv1a64 + 拼盘族 + 越界安全读盘族
Reader）+ save（内存拼载荷 -> 24B 头 -> .tmp 原子替换带两步
兜底）+ validate（八字检查链 + 结构走查 + 尾字节检查）+ load
（头部自检 -> 局部临时解析 -> substr/avgdl 恢复 -> 全有时刻
字段级 move）。

验收四件：① 字节双零（17016B，无 BOM + 0 非 ASCII）② g++
-Wall -Wextra -fsyntax-only 零告警 ③ 全模块真实链接编译零
告警（persist + chunker + tokenizer + atom_scan + indexer，
WSL g++）④ 黄金表 28/28 绿（%TEMP% 临时 main 跑完即删）——
含 3c save->load->save 字节一致性（确定性判决）、6b 失败时
探针保持原状（全有或全无实证）、8 号 FORMAT 翻转指纹不炸
（头部不入指纹）、12 号结构层独立于指纹层抓获伪造。MSVC /W4
真编译随 main 接线时做（vcxproj 登记即挂），atom_scan 17:35
先例。

下一步：main 接线（~10 行，动 A 手写的 main.cpp，待 A 一句话
授权）+ vcxproj/filters ClCompile 登记 + test_persist 转正
（七坏一好矩阵，AI-A 执笔 A 审）-> M3 开张（向量检索 HNSW/RRF
契约设计 + B/C 任务卡）。commit 待 A 批。

## [2026-09-29 02:32] AI-A

【决策转记】A 三令：① main 接线 + M2-② 收尾（含 commits）授权
AI-A 执行；② **Qt GUI 改外包**——外部 AI 施工，AI-A 出施工书 +
附件清单，交付物回流后 A 审 + AI-A review 再拼装（09-28 13:39
"Qt 全权 AI-A 执笔"就此修订）；③ M3 发卡照计划（B/C 回归分工）。

- 【留痕】AI-A 改 A 的 main.cpp 三处（授权"你继续做你的这个
  main"）：1. include 补 "persist.h"；2. status_name 帮手（八态
  人话，控制台只打印、调用方动作恒为重建）；3. Stage 1 换缓存
  分支 validate -> (OK && load) 命中 : {chunk+build+save（save
  失败仅 cerr 警告）}。快照路径 index.bin 由 main 决定（契约
  分工），.gitignore 补 index.bin / index.bin.tmp
- 【留痕】filters 补 persist.cpp 源文件条目；vcxproj ClCompile
  系 A 建壳时已被 VS 自动登记（17:35"VS 自动挂 vcxproj"假设的
  首次反例，留档）
- test_persist 22 条转正（run_tests 第六单元）：七坏一好全矩阵
  （IO_ERROR 用目录路径构造；BAD_PARAMS 与结构破坏走"修指纹
  正攻"构造，专测指纹层背后的守卫）+ 字节一致性 + 失败探针
  保持 + 空语料 avgdl 守卫
- 【事故留痕（AI-A）】test_persist 首轮 5 红（9-13 号）：伤害
  用例逐段 slurp 现场读，8 号改坏文件后后续在尸体上继续改，
  BAD_MAGIC 恒抢先返回——黄金表原有"先存一份 good 快照"细节
  转正时丢失。修测试不修实现，复跑全绿。"测试红先审测试"
  第三例（B tokenizer 期望值、AI-A 黄金 e.g. 笔误之后）
- 验收：msbuild Debug|x64 /W4 全量零告警（persist.cpp 首次过
  MSVC 真编译）；冒烟四连：首启 miss(not found) 重建 24/77 ->
  二启命中 restored -> 魔数位翻转 miss(bad magic) 降级重建 ->
  自愈再命中；无 .tmp 残留，快照 458KB；run_tests 全套
  75 -> 97 绿；字节双零（persist.cpp 17016B / test_persist.cpp
  10105B / main.cpp 3603B）
- 文档同步：README（M2 状态行 + 当前状态块）+ TASKS v2.8（索引
  持久化 [x] 竣工 + Qt 外包改裁）；commit bundle 本条后入账
- Qt 外包施工书随本条交付：Desktop\给GPT的外包QTprompt.md +
  附件清单（七份契约 .h + src/main.cpp 参考消费者 + PRINCIPLES +
  README），见施工书尾节

## [2026-09-29 03:05] AI-A

【决策转记】A 拍板 M2-④ 墓碑消费四项（"按推荐 开写 签"）：

1. 墓碑消费者 = chunker 过滤（甲）。删除是语料生命周期事件，在
   语料 -> 派生数据的闸门生效，索引与检索永不见墓碑内容
2. 删除深度 = 打标记 + 立即重建 + save（甲）。快照与运行时永不
   两态——"甲把删除变成事实，乙把删除变成约定"；M2 规模下便宜
   删除收益为零（优化三规则第 3 条），Lucene 式合并留给 ③ 后复盘
3. 两层删除语义入册（零代码）：逻辑删除 = 墓碑 + 文件留 data/；
   物理清除 = 直接删文件，重启经 CORPUS_CHANGED 自动完成。
   **"编辑被删文档 = 复活"经 A 签字为已知边界**——修复需按 path
   继承墓碑的合并机制，系 M2-③ 增量 diff 的自然副产品，挂 ③
   交接；墓碑独立注册表案 M2 不做（第二份持久状态 + 永脏黑名单）
4. 命令形态 = 保留字优先（甲）+ 启动一行命令提示。撞车条件仅为
   整条输入恰为 list/del（"linked list" 多词查询不撞车）

外部意见回流对账（A 转发）：引用逐条核验为真无幻觉——
InvertedIndex 无 deleted 维（indexer.h 结构体）/ validate 纯度
只比 path+content（persist.h）/ loader 默认 deleted=false
（test_loader #7）。外审两条新论据收下：乙案致命伤在"索引里
存了什么"而非签名污染（改签名或塞索引结构两条都是生命周期
混进检索）；甲案消灭快照/运行时两态不一致。list/del 保留字
冲突三案分析为外审首提，附议其甲并加零成本骑士（启动提示行）。

施工前现场对齐两修正：chunker.h 原本无 deleted 条款（"M1 不过滤"
只活在 TASKS 卡预答），本次是**加条款**不是改条款；test_chunker
无 deleted=true 用例，零既有测试冲突。search 签名确认 top_k 带
默认参（main 两参调用）。

施工（A 授权"开写"，AI-A 执笔，赶时间模式同 persist.cpp 先例）：
- chunker.h 加条款 6：deleted == true 产 0 块，墓碑在此消费，
  下游消费者零过滤义务
- chunker.cpp 循环顶三行过滤（doc.deleted 跳过）
- main.cpp 三处：snapshot 声明提升到 try 外（del 分支要访问）；
  启动横幅加命令提示行；查询环加 list（id / 墓碑标记 / 字节数 /
  path）与 del <id>（纯数字校验 -> 范围校验 -> 幂等拒绝 -> 打
  标记 -> chunk+build+save 一条龙，手工数字解析免新 include）
- test_chunker +3：中位删除 id 不位移（doc1 删，doc2 块接 0 后）/
  600 词体积删除 0 贡献 / 全删空语料

验收：字节双零（chunker.h 1619B / chunker.cpp 1826B / main.cpp
5539B / test_chunker.cpp 5877B，均无 BOM 纯 ASCII）；run_tests
97 -> 100 全绿；msbuild Debug|x64 /W4 零告警；冒烟三连 + 三边角：
首启 24/77 重建 -> 二启快照命中 del 19（救赎之道 CN）73 chunks
剩余、libc top-1 由 CN 5.32 换 EN 3.47（重建后 df/avgdl 再平衡，
排序符合预期）-> 三启 restored 24 docs / 73 chunks 墓碑存活
（删除重启持久化验收点）；重复 del / del 99 / del abc 人话报错。
冒烟后 index.bin 已清（还原首启态，缓存随时重建）。

Qt 施工书影响备案：§4.3"search 仍会返回墓碑文档的块"语义已被
本裁决取代（删除即重建，块即刻消失）——施工书不重发（外部已在
施工），回流 review 按新语义验收，GUI 显示层过滤降级 UX 辅助
（双保险保留）。

下一步：commit 待批；M2 自家活仅剩 M2-③ 增量建库（ctf-wiki
35 篇实弹，id 稳定性考题），Qt 外包件回流后拼装。

## [2026-09-29 03:27] AI-A

【决策转记】A 拍板 M2-③ 增量建库四项（外部意见回流四项照
推荐终裁）+ 施工方式第五项：

1. 范围 = **甲**：diff 检测 + 墓碑继承 + 全量重建 + 缓议书。
   真增量否决——打包 id（loader 字典序 doc id + 连续 chunk id +
   chunk_lengths 红线）与增量 merging 在现行契约下互斥，中间
   插入的重编号级联成本 O(账本总量) ≈ 全量重建，代码复杂度翻
   几倍换零收益；乙（稳定 id 重构）杀 chunk_lengths 红线 +
   M5 稠密向量化提案 + persist 格式且作废 Qt 外包件所依契约，
   丙（追加式快路径）被自家弹药击毙（ctf-wiki mobile/ 新类目
   必卡字典序中间，永不触发）
2. 新契约之家 = 独立 include/corpus_diff.h（**第八契约**）。
   事实归契约（added/removed/edited 是纯事实），策略归 main
   （怎么用 diff 是策略）；纯函数可测试
3. 继承语义 = **按 path**（内容失明）：编辑被删文档不再复活，
   M2-④ 签字的已知边界就此关闭；复活通道 = 物理删文件再重加
4. 会话内 reload 命令不做（YAGNI，重启即同一路径）
5. 施工方式 = **AI-A 执笔全件（赶时间模式，persist.cpp 先例）**。
   外部意见曾建议 A 亲手写双指针归并；A 裁定"先挂账你快写"。
   A 亲手学习债挂账清单 +1：**④ corpus_diff.cpp 双指针归并
   实现走读 + main harvest 分支**（与 ①②③ 同清算点：M3 发卡
   时优先讨论 / Qt 竣工前）

对外部意见对账：四项判决与 AI-A 推荐逐项一致、论证无失真、
引用无幻觉；"事实 vs 策略""deleted 是 path 的属性"两条新论据
收下。反向对账一处：称 AI-A 设计简报"表格贴了两遍"——查本会话
原稿表格仅出现一次，重复发生在 A 转发给外部 AI 的粘贴环节。

施工（AI-A 执笔，A 授权"你快点写吧"）：
- include/corpus_diff.h 第八契约：CorpusDiff{added,removed,
  edited}（相对 path 列表，升序）+ diff_corpora（双指针一趟
  O(n+m)，条款 2 纯度与 validate 同源：id/deleted 不可见）+
  inherit_tombstones（按 path 置 deleted，返回实际置位数，
  条款 5 内容失明 = "编辑不复活"的契约化）+ 前置条件两头真实
  成立（loader 字典序 / 快照还原保序）
- src/corpus_diff.cpp：两个独立双指针归并，无内部排序，无 I/O
- main.cpp 三处：include 补 corpus_diff.h（管线序：loader 后）；
  harvest_tombstones 静态帮手（收获旧快照 → diff → inherit →
  打一行 diff 报告；收获失败 = 无继承直接重建，"损坏=cache
  miss"红线一致）；rebuild 分支在 CORPUS_CHANGED 且仅此时调用。
  关键机理：load 不做 manifest 比对（那是 validate 职责），
  CORPUS_CHANGED 时快照结构完好即可收获——18:47 "persist 哑三件
  不混入 M2-③"裁定零破坏兑现
- tests/test_corpus_diff.cpp 14 条：两空/全同/id 纯度/deleted
  纯度（diff 与继承职责分离的钉子）/尾部追加/**中间插入
  （mobile 场景，id 位移与 diff 无关）**/删除/同 path 异 content/
  三态混合一趟/输出升序/**编辑墓碑不复活（边界关闭验收钉，
  content 保留新值）**/墓碑路径消失不外溢/零墓碑返 0/已置位
  不重计
- run_tests.bat 第七测试单元（仅链 corpus_diff.cpp）；
  vcxproj/filters 双登记（ClInclude + ClCompile，main 已消费）

验收：字节双零（corpus_diff.h 2021B / corpus_diff.cpp 1809B /
test_corpus_diff.cpp 6742B / main.cpp 6684B，均无 BOM 纯
ASCII）；run_tests 套件 **100 -> 114 全绿**（新单元 14/14）；
msbuild Debug|x64 /W4 零告警（corpus_diff.cpp 首过 MSVC 干净）；
冒烟五连（一次性文件 data/web/zz_incr_test.md 实弹，真语料
零接触，全程 tcache 查询作探针）：
- 首启 not found 重建 24/77 → 二启命中 restored
- 投放新文件：corpus changed: **1 added** → 25 docs/78 chunks，
  新文件 id 24（web 尾部零位移），tcache top-1 命中新文件
- del 24 墓碑：77 chunks remain，tcache 空结果
- **编辑被删文件后重启：corpus changed: 0 added, 0 removed,
  1 edited, 1 tombstone(s) inherited**——25 docs/77 chunks，
  list 显示 24 [deleted] 467 bytes（探针已读入新内容但墓碑
  存活），tcache 仍空。**"编辑即复活"边界正式关闭**
- 物理删文件：corpus changed: **1 removed** → 24/77 归位，
  tcache 空（文件已不在）
冒烟后临时文件已删、index.bin 已清（还原首启态）。

剩余两件挂本项：缓议书（docs/design/，id 稳定性对偶代价论证
入册 + 乙/丙/乙′ 死因 + M5 复议触发条件，AI-A 执笔 A 审）+
ctf-wiki 35 篇实弹入库（24 -> 59，ASCII 文件名 + UTF-8 体检
走 18:20 先例）。commit 待批。

## [2026-09-29 03:35] AI-A

【决策转记】A 令"ctf-wiki 35 篇实弹入库 走添加流程进行测试；
对了的话就commit"——同轮执行完毕，**M2-③ 至此全部竣工（五项
拍板 + 代码 + 缓议书 + 实弹），M2 自家四件套全清，仅剩 Qt 外包件**。

- 弹药现场：`Desktop\CCTF\ctf-wiki`——writeups/ 下 34 篇单题 WP
  （全 ASCII kebab-case 命名：misc 6 / mobile 6 / pwn 6 / re 8 /
  web 8）+ 1 篇《校赛总WP_提交版.md》
- 【排除裁定（AI-A 现场判，A 可翻案）】总 WP = "探索解密"大赛
  提交汇总（r0Xy 队排名表 + 呀吼等 npusec 12 题内容与现有 data/
  重复 + 中文文件名违反 15:30 ASCII 先例）——不入库。原册"35 篇
  24 -> 59"修正为 **34 篇 24 -> 58**（35 计数含总 WP）
- 入库前置体检（18:20 先例）：34 篇全 UTF-8 解码合法、文件名
  纯 ASCII、1.2~14KB 量级健康
- 实弹流程（A 点名的添加路径）：先建基线快照（首启 24/77 +
  save）-> 34 篇拷入 data/ 五类目（mobile/ 新目录）-> 重启 ->
  **"corpus changed: 34 added, 0 removed, 0 edited" -> 58 docs /
  145 chunks**（77 -> 145，+68）-> 再启还原命中 58/145
- id 稳定性考题现场答案：mobile/ 卡进 misc 与 pwn 之间，字典序
  中间插入实锤，doc id 全体重排由全量重建正确消化（重建 = 新
  注册表，无跨引用，M2-④ 脚注先例）
- 查询弹（新知识实弹）：borrowstack top-1（5.70）；蔡文姬
  top-1（14.10，中文 bigram 新语料首杀级分数）；json beautifier
  top-1（9.40）+ inspiration-note 等多块混排；钢琴 top-1（4.23）；
  栈溢出 top-10 = 新语料压制（stack 8.82 / borrowstack 7.67）
  + 旧救赎之道 CN 四块可达（6.0~7.2）——扩容后新旧知识混合
  排序符合预期
- 插曲入册：英文查询 piano 空结果 = 内容零 "piano"（词仅在
  文件名，而文件名不入索引；内容用 钢琴/琴键）——契约"未知词
  返空"正确履职，heap 先例（09-28）第二例；正弹 钢琴 top-1
  补齐验证
- 文档同步：README 4 处（里程碑行自家四件全清 / 状态块 58 篇
  145 chunks / 目录树 data 行 / 语料叙事）+ PRINCIPLES 语料
  快照行 + TASKS v3.1（增量建库 [x] 关账）；快照 index.bin
  保留（老板 F5 直接还原 58/145）

commit 获批随本条入账（M2-③ 代码 11 文件 + 语料 34 篇 +
文档 4 件）。

## [2026-09-29 03:55] AI-A

【决策转记】Qt 外包件回流（外部 AI 交付，Desktop\QT\）——AI-A
review + 拼装完成（A 令"动"），**待 A 视觉验收**。

- 到货对账：五件套（mainwindow.h / mainwindow.cpp / qt_main.cpp /
  CMakeLists.txt / READ.md 存根）自述逐条核验为真——全 UTF-8 无
  BOM、注释纯 ASCII（mainwindow.cpp 873 非 ASCII 字节 = 中文 UI
  字面量）、无 fromStdString、CMake 不编 console main。外部自述
  两处亮点：主动发现施工书 §4.3 与附件现行契约矛盾并按契约走
  （删除 = 墓碑 + 重切 + 重建 + save，M2-④ 语义先行对齐）；诚实
  声明无 Qt 环境未能编译验证。完整 README.md 未随件到达（在外部
  ZIP 内），集成 README 由 AI-A 重写（gui/README.md）
- 代码质量评价：byte→QChar 定位正确（[0,begin) 与 [begin,end)
  分别解码换算光标位置，没把字节数当字符下标）；快照三分支 /
  越界守卫 / 墓碑删除线显隐 / 状态栏（含双版本与 avgdl）全齐；
  核心层零改动纯消费者
- 【留痕】拼装修订四处（修改外部交付件，A 授权"动"）：
  ① CMake MSVC 分支补 /utf-8（中文 UI 字面量的编译器读取面）
  ② CMake 补链 ../src/corpus_diff.cpp（第八契约，外部交付时不知
  M2-③ 已竣工）③ mainwindow.cpp loadCorpusAndIndex 的
  CORPUS_CHANGED 分支补墓碑收获（diff 报告 + inherit，对齐
  console 语义——否则 GUI 侧语料变更重建会丢墓碑）④ CMake 补
  默认 CMAKE_BUILD_TYPE=Release（空构建类型会映射 Qt Debug 导入
  库 Qt6Cored.dll，线上安装器不带调试件，必炸）
- 环境侦察两发现：① 本机 Qt 线上安装器装歪——真实 Qt 6.9.3 整套
  位于 C:/Qt/6.5.3/msvc2022_64/bin/qmake.exe/6.9.3/msvc2022_64/
  （"qmake.exe"是目录，6.9.3 在里面；C:/Qt/6.5.3 本体是空壳）；
  可用，路径已写进 build 脚本，日后 Maintenance Tool 规整后同步
  改两处 ② 老板记忆的 Qt 5.9.5 在 D 盘（D:/Qt5.9.5），本工程
  用不上（交付件 find_package(Qt6 REQUIRED)）
- 拼装产物 gui/ 目录：五件套（mainwindow (1).cpp 改名落位）+
  README.md + build_gui.bat（一键：vcvars + VS 自带 CMake 3.31
  + Ninja + Qt 6.9.3）+ run_gui.bat（cwd 自动指 MY_CTF_RAG，
  与 console 共用 data/ 与 index.bin，双击即用）；
  .gitignore 补 gui/build/
- 验收：**首发编译即通**——10 编译单元 + 链接，/W4 零警告
  （Qt 6.9.3 头文件 + 七核心源全干净）；windeployqt 部署后点火
  冒烟：进程 7 秒存活、工作集 119MB（Qt + 58 篇语料 + 145
  chunks 快照还原全链路走通），无崩溃；补丁后 mainwindow.cpp
  仍 UTF-8 无 BOM（924 非 ASCII）
- 待 A：Qt Creator 或 gui\run_gui.bat 双击视觉验收（语料树 /
  中文查询 / 双击定位 / 软删除删除线 / 墓碑继承日志）；
  commit 待批（gui/ 7 件；另 03f0f99 + 436058b 两 commit 的
  push 因 7890 代理未开暂缓——"先处理QT一会再commit"）

## [2026-09-29 04:30] AI-A

【决策转记】A 令"开"——Qt 从 CMake 路线改裁 **VS 原生集成**（需求：
F5 直接进 Qt 界面、mainwindow.h/.cpp 进 VS 左边栏，范本
Desktop\StructuredLightMeasurement\cpp_qt；A 亲手装 Qt VS Tools）。

- 环境三步（A 亲手执行，AI-A 验收）：扩展 Qt Visual Studio Tools
  3.5.0 per-user 装好；Qt Versions 注册 **QT6**（路径=装歪嵌套套件
  C:/Qt/6.5.3/msvc2022_64/bin/qmake.exe/6.9.3/msvc2022_64）且已设
  默认版本——注册表 HKCU\QtProject\QtVsTools\Versions 实证；一条
  "Qqmake"（多打一个 Q）死条目在册，无害
- 【勘误入册】套件**含全部调试库与调试 DLL**（Qt6Cored.lib / .dll
  逐一只读实证）——03:55 条"线上安装器不带调试件"结论修正：当日
  CMake Debug 炸的真因是 DLL 未部署到 exe 旁（部署问题），非套件
  缺件。Debug 配置无需任何特殊处理
- 施工（AI-A 执笔，A 授权"开"）：
  - 新项目 MY_CTF_RAG_GUI\{vcxproj, filters}——QtVS_v302 **标准
    版本名注册制**（QtInstall=QT6 + QtModules core;gui;widgets +
    QtDeploy 自动 windeployqt；不走 cpp_qt 的 QTDIR 环境变量改造
    路线，那套是 .pro 转换带出的非标产物）
  - 关键配置：七核心 .cpp 共享编译（console main.cpp 排除，
    免 LNK2005 双 main）；LocalDebuggerWorkingDirectory=
    $(ProjectDir).. 内置进 vcxproj（F5 cwd 自动落 MY_CTF_RAG，
    data/ + index.bin 与 console 共用，run_gui.bat 的人肉对齐
    进工程）；/W4 /permissive- /utf-8 C++17 与全仓纪律一致；
    OutDir x64\$(Configuration) 落 .gitignore 全局 x64/ 防线内
  - sln 注册（A 手建文件，授权改动）：第二项目 + Debug/Release|x64
    双映射，x86 不映射（套件仅 64 位）
  - gui/ CMake 三件退役删除（CMakeLists/build_gui.bat/run_gui.bat
    + build/），gui/README.md 重写为 VS 路线说明；同一 GUI 源码
    二次搬家，路线终裁 VS 原生
  - QtMsBuild\ 从扩展目录拷入（VS 加载工程时扩展自动维护更新，
    gitignore 挡库外）
- 验收：Release + Debug 双配置**首发编译即通**（Qt/MSBuild 3.5.0
  识别 QT6 -> qmake 6.9.3 全链正确；9 编译单元 /W4 零警告）；链接后
  QtDeploy 自动跑 windeployqt，双风味 DLL 各归各位（Debug 目录
  Qt6Cored.dll 实证 = 当日 CMake 翻车点正式修复）；双配置点火冒烟
  存活（Release 109MB / Debug 142MB，58 docs + 145 chunks 快照
  还原）；新文件字节双零（vcxproj 6081B / filters 2361B / gui
  README 2525B，均无 BOM 纯 ASCII；sln 原生 BOM 未动）
- 待 A：开 MY_CTF_RAG.sln -> 右键 MY_CTF_RAG_GUI "设为启动项目"
  （一次性，.suo 记忆）-> F5 视觉验收（语料树 / 中文查询 /
  双击定位 / 软删除删除线 / 墓碑继承日志）。commit 待批（VS 集成
  包：vcxproj + filters + sln + gui 四件增删 + .gitignore + 纪要 +
  TASKS；连同 0557444 及更早 4 个 commit 的 push 等 7890 代理）

## [2026-09-29 10:36] AI-A

【决策转记】A F5 视觉验收通过（"过了"）+ commit/push 完成，**M2 全线
收官**——Qt 图形化管理界面（M2 第五件）正式竣工，M2 核心四件套 +
GUI 全部交付，下一站 M3。

- A 开 sln → 右键 MY_CTF_RAG_GUI 设启动项 → F5 直达 Qt 界面，
  左边栏 mainwindow.h/.cpp 可见（cpp_qt 同款观感达成），视觉验收
  五样全过（语料树 / 中文查询 / 双击定位 / 软删除删除线 / 墓碑
  继承日志）
- commit 132b42b（Qt VS 原生集成包）+ push 完成
  （14435d9..132b42b，6 commit 一气上 origin/main，7890 代理已通）
- 补记：04:30 条"待 A 验收 / commit 待批 / push 等代理"三项就此
  清账；GUI 页面讲解四块（语料工具栏三分支 / 软删除四动作链 /
  字节→字符高亮换算 / snippet 与 corpus_diff 双指针）系教学对话
  ，不入决策档，如需追溯见本会话记录
- M2 状态：自家四件套（bigram / 持久化 / 增量 / 删除）+ Qt GUI
  全部竣工并推送。下一步 M3（向量检索 Flat→自研 HNSW + RRF 混合）
  契约设计，B/C 恢复分工下发

## [2026-09-29 19:37] AI-A

【决策】M3 路线初步定档（丁案）+ 新目标叙事确立。触发：老师
线上反馈（"到目前为止就已经还不错了；引用别人训练好的也不是
不可以；性能优先，重心从以自己手搓为主到以实用好用为主，
手写必要算法为辅——不影响效果、不涉及机器学习与训练"）。
A 对 GPT 3 意见（Desktop\GPT 3.md）的裁决原话："他说的非常好，
正是我所理解的内容"——全文采纳；AI-A 附议并补强三点。

- 通道定档**丁案**：真实预训练模型 + 本地 Ollama localhost
  HTTP + C++ 客户端（甲 ONNX 进程内 / 乙 mock / 丙 Python
  sidecar 三案弃，死因见 docs/m3-plan.md §2）。核心工程抽象
  = provider 可替换（M4 本就 Ollama→llama.cpp，M3 提前复用
  该边界）
- **双后端**：Flat 自研（exact 基线；145 chunks × 1024 维
  ≈ 0.57MB，全扫亚毫秒——当前规模的实用最优）+ HNSW 自研
  **降位保留**（算法核心 + M5 实验对象，非产品必需）。与
  AI-A 晨间"乙案 HNSW 挂缓议"的差异统一：降位保留 + Go/No-Go
  开关（M4 工期挤压可整体顺延不阻塞，缓议思想转为风险控制
  而非砍件）
- **模型不写死**：bge-m3 vs qwen3-embedding:0.6b A/B 实测
  拍板（自建 ~20 条中英混合 CTF 查询集，数据拍不印象拍；
  平局选 bge-m3）
- **快照拆分**：index.bin 不动不膨胀，新增 vector.bin——身份
  四件套 model_id / dimension / embedding_policy /
  VECTOR_PIPELINE_VERSION（PIPELINE 版本"换 embedding 模型
  也要 bump"的伏笔正式兑现；Qwen3 instruction-aware，建库/
  查询前缀策略成对冻结）；hnsw.bin 后议（145 点建图毫秒级
  不值缓存，派生数据判据第三次应用）。AI-A 补强：向量增量
  复用 = vector.bin 按 doc 组织（path + content_hash），
  corpus_diff 报告驱动——未变 doc 直接复用向量，added/edited
  才重嵌
- **分层铁律**：embedding 与 vector search 彻底分层，索引层
  只认 vector<float>，不知道模型/HTTP/JSON
- **叙事升级**："从零手写版" → "核心检索算法自研 + 模型推理
  采用成熟开源组件的全离线 RAG"（README 已改，自研边界表入
  README §1）。三条不自研红线：不自研神经网络 / 不自研
  Transformer 推理 / 不自己解析模型权重
- 文档五件落位：docs/m3-plan.md（施工计划 + B/C 任务卡
  T5-T10 初拟 + 明日老师沟通议题清单）、docs/
  m1-m2-retrospective.md（M1-M2 阶段复习档：数字仪表盘 /
  八模块复习卡 / 方法论八条 / 挂账清单）、README 叙事更新
  （标题 + §1 边界表 + §2 路线表 + §4 状态 + 目录树除旧）、
  TASKS v3.5、本条。PRINCIPLES §1 叙事修订提案已备，挂明日
  老师线下确认后落盘（宪法级不抢跑）
- 下一步：明日 A 与老师线下详聊（录音留证）→ 语音回流 →
  M3 正式计划修订 → 发卡前挂账清算 ①-④（persist 走读 /
  卡缝五 / 黄金构造 / 双指针走读）→ B/C 任务卡正式下发

## [2026-10-05 19:28] AI-A

【决策】M3 发卡前置双件竣工：**环境 ⓪ 落地 + 学习债 ①-④ 全清**
（A 令"对关键对话进行必要的会议 md 归档"，本条归档 09-29 19:37
以来的教学与清账对话）。挂账清算窗口（09-29 03:27 裁定）正式
关闭，下一步 M3 契约设计。

1. **老师线下会取消**（A："还没聊 取消了"）→ 改出复习简报
   （半小时讲透现状前因后果）+ QQ 留言稿交付，A 拟发老师；
   M3 正式计划升级等老师回复，PRINCIPLES §1 叙事修订继续挂起
   （宪法级不抢跑）
2. **工作流确认（A）**："先装环境然后清账 然后聊契约我们写好
   所有的 .h 把 cpp 拟一个伪代码发放下去写"
3. **环境 ⓪**：Ollama 0.35.1（winget；代理 7890 先决，
   0x80072efd 网络错 → 查 VPN 首则奏效）；bge-m3 +
   qwen3-embedding:0.6b 双模型拉取；首次 /api/embed 实弹
   （%TEMP%\m3_first_embed.py 保留，T10 工具种子）。
   **契约级事实三枚**：① 返回向量已归一化 norm≈1.0 →
   余弦 = 点积，T5 免归一化优化入册 ② dim=1024 ③ 跨模型同句
   余弦 ≈ -0.02（公理③"库与查询必须同模型"实证）；组内同义对
   分离度 qwen3 0.63 vs bge 0.33（n=3，A/B 预演非结论）。
   插曲入册：ollama launch codex 系 app 启动指令与本项目无关
4. **清账记录（①-④ 逐笔）**：
   - **①②**：persist.cpp 四段走读 + 卡缝五处讲座后复述 4/4——
     但答卷带"本回答由 AI 生成"页脚 + 两条过期建议
     （corpus_diff 契约草案 / M2-② 收尾，均已完成不存在），
     判疑似 AI 辅助；以即兴口试（vector.bin/BAD_MODEL 转移题）
     为真实封印，A 过且答出讲稿外两个层次 → ①② 销账。
     方法论入册：不指控，换即兴转移题验真
   - **③**：黄金测试构造三手法讲座——双趟字节一致 4b（序列化
     的唯一证人，字段相等对顺序不敏感）/ 头部手术 8/9/11（11 号
     才是"头部不入指纹"的 Discriminating 用例，9 号殊途同归无
     证明力）/ repaired() 修指纹 12/13（拆外墙测内墙，逐层验
     独立）+ 编号漂移披露（账上 3/8/12 黄金表时代 ≡ 今 4b/
     9+11/13）+ 两疫苗（good 原始副本防尸体连改 / 目录路径
     构造 IO_ERROR）。转移题（T9 三刀口）**A 自答不清，令
     AI-A 代答后放行**（"你替我回答一下 我答不清楚 然后过"）。
     代答要点入册（T9 测试矩阵直接引用）：model_id 刀+修指纹
     → BAD_MODEL（证件层独立抓获，枚举分叉 = 用户故事分叉）；
     dimension 1024→999 刀+修指纹 → 结构走查脱轨 BAD_FORMAT
     （13 号手法向量版）；**单向量浮点刀+修指纹 = 设计上不可
     检出**（验证成本 = 重建成本，向量无第二真相源——index.bin
     靠 manifest 对账因磁盘有原文件），防线转移到 content_hash
     失配拒绝复用（在增量复用层测），不可检出事实入
     vector_persist.h 契约注释
   - **④**：corpus_diff 双指针主体已由 GUI 讲解（09-29 10:36
     补记第四块）+ M2-③ 实弹覆盖，今日概要收尾（两头升序
     不变量 / i-j 对进 O(n+m) / 纯度 path+content / inherit
     内容失明 / harvest 仅 CORPUS_CHANGED）→ 销账
5. **下一步**：M3 契约设计开张——A 亲手写五份 .h（embedder
   第九 / vector_index 第十 / rrf 第十一 / hnsw 第十二 /
   vector_persist 第十三，AI-A 教练对话逐份过）→ 伪代码卡 →
   两批发放（批 1：C·T8 + B·T5/T6；批 2：C·T9/T10 + B·T7）
6. 待办：QQ 留言待发；五文档批（m3-plan / retrospective /
   README / TASKS v3.5 / 纪要 19:37）+ 本条未 commit，待 A 批

## [2026-10-07 02:52] AI-A

【决策】A 赶时间授权 AI-A 执笔 M3 五份契约（"你写一个吧 时间来
不及了 我明天必须把任务发下去 我最后写完复习"——persist.cpp 赶
时间先例第 N 次；**A 的学习模式转为事后复习 + review 补课**，
手写学习债挂账：契约精读代替手写）。

- **设计讨论先落地**（A 对六议题逐项拍板："乙 成对入口 可以
  可以 甲 ok"）：注入缝 = 乙（EmbedProvider virtual 接口，全仓
  首个多态；M4 provider 模板）/ 成对入口 embed_documents +
  embed_query（前缀策略成对冻结封装在 provider 内）/ throw
  runtime_error 不自动重试（loader 先例）/ 归一化出口保证
  （T5 点积 = 余弦的契约地基）/ glue vendor 甲案（cpp-httplib +
  nlohmann/json 进 third_party/，MIT 登记许可证）/ 配置三字段
  + dimension 从输出读（身份问 provider，尺寸问输出）
- A 随后要求全景对齐（"先告诉我 M3 要新增几个 .h"），AI-A 出
  五份契约总图（每份职责 + 函数清单 + 条款要点）后获执笔授权
- **两处新裁定随写随裁**（A 复习时重点看）：
  1. vector_index 不引入 virtual——Flat/HNSW 互换是实验开关
     非运行时注入，两份独立契约更诚实（m3-plan §6 T5 "抽象
     接口"措辞据此修正，理由注释写入 vector_index.h 头部）
  2. vector_persist 独立成件（m3-plan §10 开放项落锤）：身份
     模型 / 失效语义 / 增量复用与 index.bin 生命周期全不同，
     并入会撑肥 M2 冻结件——裁定注释写入文件头部
- **交付五件**：include/embedder.h（4009B，第九）/ vector_index.h
  （2277B，第十）/ rrf.h（1659B，第十一）/ hnsw.h（2637B，
  第十二，契约现在写、实现第 5 步 Go/No-Go）/ vector_persist.h
  （4368B，第十三）。六状态 VectorSnapshotStatus（BAD_MODEL
  位检查链第 6，口试定案兑现）；身份区入指纹覆盖（③ 代答三刀
  口的物理前提）；检查顺序 magic→版本→指纹→结构→身份 写进
  契约（测试断言确定性）
- **验收**：字节双零五连（无 BOM + 0 非 ASCII，合计 14950B）；
  WSL g++ -fsyntax-only -std=c++17 -Wall -Wextra 五连过（唯一
  warning 系".h 当主文件编译"方法论噪音，代码零告警）
- 遗留四件：vcxproj/filters 登记（无消费者先例挂起，.cpp 建壳
  时一并做）/ 伪代码卡批 1（C·T8 + B·T5/T6）待 A 点头即出 /
  A 复习 + review（六议题 + 两新裁定 + 七条款逐条）/ commit
  待批（与 10-05 前批合并或分批由 A 定）

## [2026-10-07 05:02] AI-A

【决策转记】A 令"出"——**M3 批 1 三卡发放完毕，B/C 可开工**。

- 三张伪代码卡落位：src/embedder.cpp（C·T8，最厚）/ src/
  vector_index.cpp（B·T5）/ src/rrf.cpp（B·T6，热身件）。
  M1 四件套同款（复述题 / 伪代码分段 / 黄金自测表 / 施工规则），
  UTF-8 带 BOM 供 VS 显示（竣工时卡删光 + 剥 BOM 回归纯 ASCII）；
  卡体随 M1 先例提交进仓（M2"卡体不提交"仅适用 A 自施工场景）
- **third_party/ 落位（vendor 甲案兑现）**：cpp-httplib 单头
  （344KB，v0.18.3，fallback master）+ nlohmann/json v3.11.3
  单头（920KB）+ 双 MIT 许可证 + README（来源/版本/规矩：只许
  include 不许改、消费者唯一 embedder.cpp、相对路径 include
  零工程配置、push/pop 隔离 /W4）
- 文档同步：TASKS v3.6（M3 批 1 卡区：任务表 + 工作流 + 批 2
  说明）+ PARAMS v1.2（六参数入账：EMBEDDER 两项 / RRF_K=60 /
  HNSW_M=16 / EF_C=200 / EF_S=100；model_id 系运行时配置非参数，
  A/B 拍板后默认值另行入册）
- **卡内新冻结一处**：qwen3 系查询侧 instruction 前缀文本 +
  policy 标识 "qwen3-instruct-v1"（其他模型 "none"）——成对冻结
  的代码级落点；改文本 = 换坐标系 = 旧 vector.bin 作废（卡内已
  向 C 讲明因果）
- 黄金表设计注记（B/C 复习价值）：T5 十条全用二进制精确值
  （0/±1/±0.5）免浮点容差纠纷；9 号故意喂未归一向量钉"不二次
  归一"契约行为（调用方违约 vs 实现方职责的分界）；T6 1 号双
  tie 用例——doc0 与 doc1 恰同分 1/61+1/62（IEEE 加法交换律
  保证位级相等），一次考透两个 tie-break；4 号钉"重复 id 首现
  计次 + 后续文档名次不前移"
- 待 A：commit 批准（五契约 + 三卡 + third_party 9 件 + 文档 3
  件，与 10-05 前批合并或分批由 A 定）；B/C 复述回收后随时
  答疑；批 2（T9/T10/T7）待批 1 验收

## [2026-10-07 05:43] AI-A

【决策转记】A 令"发卡吧 卡给我看一眼"并点名补一份写给 B/C 协作
AI 的任务简报（"不是说给他们人听的 是教会他们的 ai 怎么理解
任务"）——**M3 批 1 正式下发，push 随令执行**。

- 新文档 docs/m3-batch1-briefing.md：收件人 AI-B / AI-C，十一节——
  仓库地形（git 根 = 文档区 + MY_CTF_RAG/ 代码区同居，clone 第一眼
  先定位，防 AI 走错层找 include/）、角色红线（教练不代写 + 主人
  要代码就引宪法拒绝 / 复述不放水不代答 / 刷新指令 / 通用直觉
  不适用）、项目五分钟（管线图 + 分层铁律：vector 层不许出现
  网络与模型概念）、三卡概览表、六步流程（含纪要不缺席 + PR
  描述格式）、阅读地图（M3 四条纪要指引，含 10-05 实弹三事实）、
  复述评分 rubric（只给维度不泄答案：职责边界 / 方向顺序 /
  成对冻结三类高频翻车点）、硬红线八条 + 竣工字节自检命令、
  环境备忘（B 零依赖 / C 冒烟需 Ollama 没开标 SKIP）、预判三问
  （Faiss / Python / 归一化在哪层）、卡住通道（契约模糊停手 +
  纪要 @A）
- 三卡发卡前小修两处：① T6 卡"k=60 出处 = AI-BRIEFING Q6"改自
  包含——该文件在 project_RAG 仓外，B/C clone 后不可达，属悬空
  引用，改为 Cormack 2009 惯例值一句话；② 三卡必读区各补一行
  简报指引（任何入口进来都撞得上）
- 仓库拓扑备考（本次现场核实）：git 根在 MY_RAG（README /
  PRINCIPLES / docs / scripts 文档区 + MY_CTF_RAG/ 代码区），
  与 09-22 22:00 终态一致；MY_CTF_RAG/MY_CTF_RAG/ 嵌套目录系
  09-23 遗留 x64 构建残留，无源文件，无害未动
- 同步：TASKS v3.7（批 1 区首位加"先喂简报给 AI"条 + 版本说明）
- push：f6c849a + e018ef1 + 本 commit 上 origin/main——B/C 动线
  就绪：clone → 简报喂 AI → TASKS 批 1 区 → 卡 + 契约 → 复述 →
  分支动工

## [2026-10-09 14:45] AI-C

【状态同步 + 议题提请】M3 批 1 前置情况（C 侧），附一条提请 A 裁定
的计划观察。

一、状态同步。C 本地仓库已整理至与 origin/main 完全一致（98bb111，
无 ahead/behind）；T8 卡 / include/embedder.h 第九契约 / third_party/
三件就位，GUI 本机跑通（Qt 6.8.3 msvc2022_64 + Qt VS Tools 注册名
QT6）。环境澄清一则备查：vcxproj 的 `<QtInstall>QT6</QtInstall>` 存
的是**版本名而非路径**，须在 HKCU\QtProject\QtVsTools\Versions 注册
同名项；工程文件因此零路径依赖，换机器只改本机注册表——与"仓库写
逻辑名、本机写实际位置"同构。T8 尚未动工（第 0 步复述未答）。

二、议题提请 A：**M3 链路没有独立的 rerank 阶段**。全仓检索
"重排 / rerank / re-rank" 无命中（仅 incremental-deferral 与纪要中
doc id 重编号的无关表述）。当前形态止于"两路召回（BM25 + Flat
dense）→ RRF 融合 → top-k"，缺 cross-encoder 式交互重排（query 与
候选文档拼接打分，精度高于双塔，代价是每候选一次推理）。这与标准
RAG 流水线图上的常见环节存在差异，答辩若被追问需有说法。需 A 裁定：
属"当前规模（145 chunks）有意省略"还是"挂 M5 的优化项"？若前者，
建议按 incremental-deferral 先例补一条缓议书，留死因与触发条件。

三、附注（澄清而非新决策）：本次逐段对齐 M1→M3 全链，确认两件事。
① 批 1 三卡（embedder / vector_index / rrf）均为"造零件"——各自无
调用方，混合查询链接线在 M3 第 ④ 步由 A 总装。② **接线须覆盖两个
前端**：console 的 src/main.cpp 与 GUI 的 gui/mainwindow.cpp 各自
独立实现了一遍策略（缓存分支 / 墓碑收获 / del 重建——mainwindow.cpp
第 245-289 行对应 main.cpp 第 73-96 行），根源是 persist.h 明写
"POLICY lives in the caller"，有两个调用方就有两份策略。批 1 接线时
须两边同接，或由 A 明确裁定舍弃一边，避免"console 有语义检索、GUI
没有"的能力分裂。另：C 按 m3-plan §6 对口 M4 LLM 后端，与 T8 同属
"外部模型胶水"角色。教学问答内容不入册。

## [2026-10-09 15:36] AI-C

【提案 + 记录】C 在 T8 复述准备阶段提出一项产品层提案，另附两段
方案讨论（均为澄清，非新决策）。提案部分提请 A 裁定。

一、提案（提请 A）：**embedding 的模型与端点可否做成用户可更换的
配置入口？** 现状澄清：embedder.h 已把 model_id / endpoint /
timeout_seconds 全部设计为运行时字段（EmbedderConfig），契约明写
"both are legal configurations"，代码层零硬编码——即"可换"在通路层
已经成立。**真正的空白是"值从哪来"**：命令行参数 / 配置文件 /
GUI 输入框尚未设计（属调用方领域，不在 embedder.h 管辖内）。若
采纳，需明确入口形态与默认值来源。附带一条已覆盖项："用户填入
的模型不存在"的报错无须新设计——契约条款 4 + 卡黄金表第 6 号已
钉死（Ollama 回 404 -> throw，what() 含 404 与响应体）。

二、附注 1（澄清）：C 质疑 M3 第 ② 步 A/B 实验的价值（语料仅
58 篇 / 145 chunks，数据薄）。评估：担忧已被计划覆盖——m3-plan
§8 风险 3 已定"平局选 bge-m3"规则，正是为"数据不足看不出差异"
预备；且该约 20 条查询集是 M5 复用资产与答辩素材，"用户可换"与
"选好默认"并存不替代。A/B 保留与否由 A 裁。

三、联动提醒：若"用户可换模型"落地，**必须与 T9（vector_persist
实现，批 2）配套上线**——缺 BAD_MODEL 防线时，该功能等于给用户
一个能静默产出错误结果的开关。对 T8 本身无影响：通路层契约不变，
实现仍是"给什么 model_id 都能正确工作"。

## [2026-10-09 16:03] C

完成内容梳理，开c-embedder分支，添加头文件和源文件到解决方案。

## [2026-10-09 17:22] AI-C

【补记】承接本日"配置入口"提案条，补一条边界说明（讨论中澄清）。

**模型切换的现实边界 = 已在代码中适配的族。** embedder 的前缀策略
只认两类：model_id 以 "qwen3" 开头 -> 查询侧加 instruction 前缀、
policy 返回 "qwen3-instruct-v1"；其余 -> 两侧不加、policy 返回
"none"。由此：① 族内任意模型（bge-* 系、qwen3-embedding-* 系等）
可自由切换，改配置即可，零代码改动；② 引入需要新前缀模板的新族
= 改代码 + 改契约（policy id 属冻结词汇）+ 旧 vector.bin 作废重建。

根因不是实现缺陷，而是问题的本质：embedding 模型没有标准化的输入
接口，各模型对"怎么喂"的要求不同（前缀 / 指令模板 / 最大输入长度）。
"支持换模型"在工程上等于"支持换适配器"，provider 抽象的价值
正是把适配集中在一处。另有硬约束：换模型必然重建向量库（公理③，
10-05 实测跨模型同句余弦 ≈ -0.02），与适配好坏无关。

【若做切换 UI】须在界面内**明示可选模型清单**（即已适配的族），
不给自由输入框——自由输入会落入"未适配族静默降效"的陷阱（掉进
else 分支：不加前缀、不报错，结果可用但不对）。清单建议登记入
PARAMS.md，与 A/B 拍板后的默认值同处。

## [2026-10-10 17:16] C

【竣工】T8 embedder.cpp（AI-C 参与复述闸门、代码 review 与测试）。
实现对照契约逐条落地：
- 段 2 前缀策略：model_id 以 "qwen3" 开头 -> 查询侧加 instruction
  前缀、policy 返回 "qwen3-instruct-v1"；其余两侧不加、policy
  返回 "none"（成对冻结，编译期字面量，一字不许运行时拼）
- 段 3 构造函数：endpoint / timeout 空值落默认、policy 一次算好
  存住，零网络（条款 6）；model_id 为空直接抛 runtime_error
- 段 4/5 共用私有帮手 request_embeddings（HTTP 通路只写一遍）：
  空批次提前返空（条款 2 零网络）；全部失败分支抛 runtime_error
  带人话；响应条数与请求条数不符即判畸形（无部分结果）；逐条校验
  维度一致与数值有限性；norm == 0 判畸形；出口统一归一化（条款 3）
- 段 6 工厂一行
- 顺序保证：数组天然保序 + 对角余弦用例钉死
自测：黄金表 7/7 全绿、20 断言 0 失败（离线三题 + 冒烟四题，
Ollama 双模型实弹）；竣工字节 no BOM / nonascii: 0；MSVC /W4 零警告。
AI-C review 认可三处判断：抽私有帮手、expected_dimension == 0 哨兵
顺序正确（空向量先抛，0 不可能与合法维度撞车）、catch(json::exception)
不误捕自身 runtime_error（两者无继承关系，具体错误原因不被二次包装）。

## [2026-10-11 02:18] AI-A

【决策转记】A 裁"不让 C 自己来了，我们替她做收尾"——T8 收尾在
c-embedder 分支执行完毕（本条即留痕），merge 待 A 令。

A 侧验收记录（全部独立复跑，未采信自报）：

- 编译双绿：MSVC cl /W4 /utf-8 零警告（ws2_32 无需手工挂——
  httplib.h 内 `#pragma comment(lib, "ws2_32.lib")` 自动链接，
  另带 crypt32）+ WSL g++ -Wall -Wextra 零告警
- 独立测试（非她自测）：**17/17 全绿**——离线：拒连 throw 文案含
  unreachable + endpoint / 空批返空零网络 / FakeProvider 多态三查 /
  空 model_id 构造期拒 / 双 policy id（顺带证明构造零网络）；真机
  Ollama：bge-m3 1024 维单位范数 / 批量三条保序（对角余弦胜出）/
  未知模型 404 / qwen3 通路 1024 维
- 契约七条逐条落地；vcxproj/filters 注册干净（C 自己登记，超出
  M1 先例的 A 侧合并时代劳，好）
- 质量亮点入册：norm double 累加防 1024 维精度损失 / isfinite 拦
  inf-nan / 维度一致性哨兵 / catch 范围只罩 JSON 解析段不误吞自身
  runtime_error / 空 model_id fail-fast

唯一必改项（本次收尾对象）：

- **BOM 未剥**：src/embedder.cpp 头 3 字节 EF BB BF，且是全文件仅有的
  非 ASCII 字节（6537B）。上条竣工自报"no BOM / nonascii: 0"两项均与
  事实不符（BOM 在；BOM 本身就是 3 个非 ASCII 字节）——自检命令要么
  没跑要么跑错对象。危害定级：MSVC/g++ 均容忍 BOM，今日技术危害为零；
  违反的是宪法交付源码纯 ASCII 无 BOM 纪律（GBK 事故后的根治规矩）
  与卡清理流程（竣工 = 删卡注释 + 剥 BOM，留 BOM = 清理做了一半）。
  方法论入册：自报失实由 A 侧独立复跑抓获，验收制度起效；此为教学
  案例，3 字节本身是小事
- 修复执行：BOM 已剥（6537 -> 6534B），暂存 blob 终检 no BOM +
  nonascii 0 + 行尾 LF 与原提交逐字节一致（autocrlf 往返已核实，
  净变更 = 且仅 = 头部 3 字节）

待 A 裁定（不阻塞收尾）：

1. AI-C 四项提案：rerank 缺位（AI-A 建议有意省略 + 缓议书，145
   chunks 规模 cross-encoder 得不偿失）/ 模型端点用户可配入口
   （建议挂 M4，须与 T9 BAD_MODEL 配套）/ A-B 实验价值（建议保留，
   平局规则兜底）/ console+GUI 双前端接线（建议两边同接，GUI 是
   答辩演示面）
2. merge 时机（验收障碍已清）+ GitHub PR 状态（本机无 gh，A 网页查）
3. B 两卡（b-vector / b-rrf）尚无动静

## [2026-10-11 02:44] AI-A

【决策转记】A 令"按照你的建议来 然后合并 然后测试 最后commit"——
四项 AI-C 提案依 AI-A 建议裁定，T8 合并 + 测试 + 一次 commit 收账。

**四项裁定**：

1. **rerank 缺位** → 当前规模有意省略 + 缓议书：
   docs/design/rerank-deferral.md 落位（145 chunks 下 cross-encoder
   秒级延迟换边缘排序收益的量级账 + M5 nDCG 复议触发条件 +
   复议方向指路，incremental-deferral 同款格式）
2. **模型/端点用户可配入口** → 挂 M4，与 LLM 后端 provider 配置
   一并设计；硬依赖 = T9 vector_persist 的 BAD_MODEL 防线先行，
   否则是静默产出错误结果的开关（AI-C 指出的依赖成立）
3. **A/B 实验保留**：数据薄由平局选 bge-m3 规则兜底；~20 条查询集
   是 M5 评测复用资产 + 答辩素材
4. **双前端接线**：console + GUI 两边同接（步骤④ 混合查询链时）；
   GUI vcxproj 届时补注册 embedder.cpp——GUI 是答辩演示面，语义
   检索缺席 GUI = 能力分裂

**T8 合并执行**（手动 CI：merge --no-ff，测试绿后随本条一次 commit）：

- 合并内容：embedder.cpp（BOM 已剥 6534B）+ vcxproj/filters 注册
  （C 自登记）+ 她的六条纪要 + 02:18 收尾条目
- **test_embedder 转正**（tests/ 第八单元，AI-A 执笔 A 审，09-28
  02:32 先例）：离线 12 条——拒连 throw 文案三查 / 空批返空 /
  构造零网络下身份四查（model_id 透传、qwen3-instruct-v1、none、
  空 model_id 构造期拒）/ FakeProvider 多态四查（基类指针消费、
  query、保序、身份透传）。真机冒烟（dim/单位范数/404/批量保序）
  留在人工验收步不进 CI——测试不依赖 Ollama 在线
- run_tests.bat 挂第八单元（链接 embedder.cpp；ws2_32 走 httplib
  pragma 自动链，无需手工挂库）
- **验收三连全绿**：run_tests 126/126（114 + 12 新）；msbuild
  Debug|x64 /W4 零警告零错误（embedder.cpp 首次进工程编译即净，
  链接通过 = 工程内 pragma 自动链坐实）；冒烟 EXIT=0——快照还原
  58 docs / 145 chunks，libc top-1 pwn/notepad.md，行为与合并前
  一致（embedder 无消费者，零回归符合预期）
- 字节自检：test_embedder.cpp 4490B 纯 ASCII 无 BOM；run_tests.bat
  维持纯 ASCII；rerank-deferral.md UTF-8 中文（docs 规范）
- README 同步 4 处（契约十三份 / src 行 / tests 126 / 状态块 +
  T8 合并记）；TASKS v3.8

待 A：push 示下（本 commit 含 merge + 转正 + 缓议书 + 文档 3 件）；
GitHub 网页确认 C 是否开了 PR（本机无 gh）；B 两卡（T5/T6）催办
与否由 A 定。

## [2026-10-11 02:46] AI-A

【决策转记】A 裁——周三（10-14）汇报倒逼排期：**B 转岗做 PPT，
T5/T6 改 AI-A 赶时间执笔**（persist.cpp / corpus_diff.cpp / 五契约
先例第 N 次），A 学习债挂账 T5/T6 契约与实现 review（与复习计划
合一）。时间线：今晚 T5/T6 竣工验收，周一 main + GUI 接线（步骤④
混合查询链），周二 A 复习 + 录演示视频，周三汇报。

**A 原提"替 B 做 + 挂 B 名"被否（AI-A 谏，A 采）**：伪造施工记录
不可为——验收制度的全部价值建立在"记录为真"上（同日 02:18 条
C 自报失实案例为镜）；答辩现场 B 答不上自己"写"的代码比"B 没写
这张卡"难看十倍；如实记录对外叙事零损伤（老师已定调实用优先）。
B 账目摆平：合并后 B 轻量走读（对 AI 复述两卡 + 纪要一条）+
**批 2 T7（HNSW）归 B 手写**——算法含金量最高的一张，手写收益
留到那里兑现。

## [2026-10-11 03:02] AI-A

【竣工】T5 + T6 落地（02:46 裁定执行：AI-A 赶时间执笔，卡删光 +
黄金转正 + 双编译器验证，B 名未挂——如实记录）：

- **src/rrf.cpp**（T6，1732B 纯 ASCII 无 BOM）：acc 累加器
  1/(RRF_K+rank)、每列表 unordered_set 去重（重复 id 首现 = 最优
  名次，其后文档名次不前移——黄金 4 号语义）、排序 score 降序 /
  同分 chunk_id 升序。卡注"无第三段，YAGNI 典型区"兑现：单流程
  即全部流程
- **src/vector_index.cpp**（T5，3199B 纯 ASCII 无 BOM）：build 三态
  （空语料合法 / 维度不一致 fail-fast 报下标与两侧维度 / 原样存储
  不归一不查单位长度）；search 守卫序 top_k<=0 → 空索引 → 维度
  throw，double 累加点积，M1 tie-break 惯例，min 截断。partial_sort
  提案与 M1 search 同款挂 M5，第一版 std::sort 不提前优化
- **g++ 跨编译器首杀**：-Wnarrowing 抓出 SearchResult::score 是
  float（type.h 自 M1 即如此，AI-A 想当然按 double 写了聚合初始化）
  ——显式 static_cast<float> 修在输出边界，测试期望同式运算后
  同样收窄。双编译器方法论（M1 对 B 通报表扬）第 N 次自证价值
- **卡缝一处入册**：T5 卡"整表 std::move 进 FlatIndex"与契约
  const& 签名物理冲突（const 不可 move）——实现走深拷贝一次
  （145 x 1024 float 约 0.6MB，建库一次性成本可忽略），缘由写进
  代码注释
- **黄金转正**：tests/test_rrf.cpp 16 条（case1 双 tie 的位级相等
  经 float 舍入仍相等——同 double 舍入必得同 float，tie 可观察性
  保持）+ tests/test_vector_index.cpp 14 条（期望全二进制精确值，
  9 号喂未归一向量钉"得分 2.0 不二次归一"）；run_tests.bat 挂
  第九 / 第十单元（各仅链自身 .cpp，零依赖零网络）
- vcxproj/filters 注册 rrf + vector_index 的 .h 与 .cpp（hnsw.h /
  vector_persist.h 仍无 .cpp，挂起至批 2——无消费者先例）
- **验收三连**：run_tests **156/156**（126 + 30 新）；msbuild
  Debug|x64 /W4 零警告零错误（两文件首次进工程编译即净）；冒烟
  EXIT=0——快照还原 58 docs / 145 chunks，libc top-1 不变（两函数
  尚无消费者，零回归符合设计）
- 字节自检四文件双零（1732 / 3199 / 4637 / 4591B）

下一步（02:46 时间线）：main + GUI 接线（步骤④ 混合查询链，
含 RRF_K 查询环融合 + embedder 上电）→ A 复习 review（挂账：
T5/T6 契约与实现）→ 演示视频。批 1 三卡全部落地，批 2（T9/
T10/T7）待 A 示下。
