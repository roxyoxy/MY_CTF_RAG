# M4 ③½ — 生成模型 A/B 实验档

> 冻结记录：`GENERATION_MODEL = qwen3:4b-instruct`（2026-10-12 A 拍板）。
> 方法论：同材料对比——context.h 条款 7（纯函数确定性）保证两模型收到
> 逐字节相同的 RagPrompt，差异全部归因于模型本身。
> 纪要：2026-10-12 01:34 条。

## 1. 实验条件

- 查询集 9 条（三类混编：词面直给 / 同义错位 / 中英互查）：

  1. ret2libc with NX enabled
  2. canary leak and control flow hijack
  3. 栈溢出覆盖返回地址
  4. 金丝雀保护与跳转地址覆盖
  5. 线性同余生成器复现密码
  6. where is the bunny hiding
  7. 乐谱里藏着最终答案
  8. 便签本泄露 libc 版本
  9. 提示注入攻击与语言模型安全

- 检索链 = 产品真实链（BM25 + qwen3-embedding:0.6b 双路 → RRF →
  top-10）；三轮日志 rrf/bm25/vec 分数逐条一致 = 材料逐字节相同
- 材料预算 `CONTEXT_BYTE_BUDGET = 16384` 字节；两模型所用材料完全相同
- 生成请求最终配置（封口后两模型完全一致）：
  `stream=false / think=false / temperature=0.2 / num_predict=512 /
  num_ctx=8192`
- 方法：改 main.cpp `GENERATION_MODEL` 常量 + 重编，查询集经 stdin 喂
  console；temperature 0.2 下一模型一样本，人眼判（施工单钦定）
- 环境：Ollama 0.35.1 本地 CPU；`qwen3:4b-instruct` =
  4b-instruct-2507-q4_K_M（2.5GB）；`qwen3:1.7b`（1.4GB，无 instruct
  变体，hybrid thinking 靠 think:false 压制）

## 2. 封口补跑（实验纪律）

首轮 1.7b 在 `num_ctx=8192` 修复**之前**运行（当时请求无该字段；
4b 首查触发 HTTP 400 exceeds_context_size_error 才暴露 4096 默认窗口
坑）。外部评审（`M4阶段评审与后续路线.md` §3.1）指出两轮 options
不一致，属实验纪律口子——补跑 1.7b 全 9 题封口，四点确认：

- 引用仍 **0 次**
- 语言跟随仍差（Q4 汉字占比 12/1441、Q8 全英文作答）
- Q1/Q4/Q8 失败模式**全部复现**（见 §4）
- 延迟均值 4.1s 不变

**4b 胜出在公平封口后原样成立。**

## 3. 五指标对比

| 指标 | qwen3:1.7b | qwen3:4b-instruct | 判 |
|---|---|---|---|
| Groundedness | Q1 多题缝合 + 事实错误；Q4 UAF/金丝雀缝合 | Q1 识别材料不支持前提，拒编造并给出材料支持的替代路线（经语料核实为真） | 4b |
| 引用 [n] | 9 题 **0 次**（封口轮；首轮 1 次） | **27 次**，覆盖 7/9 题 | 4b |
| 语言跟随 | 中文题英文答（Q4 汉字 12/1441、Q8 全英文）、英文题中文答（首轮 Q6）双向乱 | 全部跟随提问语言（中文题汉字占比 204~552） | 4b |
| Hallucination | Q8 编造地址算术（0x7f1c... 同类两轮均现）；`</think>` 标签间歇泄漏（首轮 Q8，封口轮未现——间歇性如实记） | 零幻觉样本；两次"无据"均如实声明 | 4b |
| Latency | 均值 **4.1s** | 均值 9.4s（含冷加载 17.5s），稳态 ~8.4s | 1.7b（但可接受） |

## 4. 三个代表案例

### Q1 ret2libc + NX —— RAG 分水岭

- 1.7b：把多篇不同题目的材料缝成一篇"流畅"writeup，且含事实错误
  （称"无需泄露 libc 基址可直接打 system"——泄露基址恰是 ret2libc
  的命门）。
- 4b：指出检索材料实际描述的是 `NX: enabled` + "优先找后门别硬上
  ret2libc"的场景（救赎之道 WP checksec 原文，经 grep 核实），拒答
  用户预设并给出材料支持的替代路线。
- 意义：RAG 的核心风险不是"答得漂亮"，而是"材料不足 / 冲突时能否
  克制"。本案例为答辩保留。

### Q4 金丝雀保护与跳转地址覆盖（中文）

- 1.7b：英文作答 + UAF 与金丝雀两题缝合。
- 4b：中文作答，[1] = entrance（24 字节 + 'X' 泄 canary）、
  [4] = combination（environ → saved RIP → one_gadget），引用与语料
  逐条对得上。

### Q8 便签本泄露 libc 版本（中文）

- 1.7b：全英文作答 + 编造具体地址算术；首轮答案头部泄漏
  `</think>` 标签（think:false 下温度采样的间歇性问题）。
- 4b：中文作答，给出 libc6_2.31-0ubuntu9.16_amd64 的判定依据
  （printf 低 12 位 0xe10），有据可查。

## 5. 判决

`GENERATION_MODEL = qwen3:4b-instruct`。产品价值排序（有据可溯 >
不胡编 > 引用正确 > 语言正确 > 延迟）下，质量差距远大于 ~4s 额外
延迟；1.7b 保留为未来 fast mode 候选，不作默认。模型选型到此停止，
不再扩大候选横评。

## 6. 实验期间实弹抓获的工程坑（已修，注进入 generator）

1. `/api/chat` 默认 NDJSON 流式——必须显式 `"stream": false`
   （`/api/embed` 默认非流式，embedder 从未踩过）
2. 4b-instruct 默认上下文窗口 4096 tokens，16KB 材料 ~5.2K tokens
   → HTTP 400 exceeds_context_size_error；修复 = `GENERATOR_NUM_CTX
   = 8192`（generator.h，PARAMS v1.7）。修复前首查失败同时真实验证
   了"生成失败 → 本会话降级检索列表"路径
