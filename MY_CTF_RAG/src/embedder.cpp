// ============================================================================
// T8 · embedder.cpp 施工卡（M3 批 1 · 执行人 C · 分支建议 c-embedder）
// 契约：include/embedder.h（第九契约）——动工前逐条读完条款 1-7
// 必读：PRINCIPLES.md §3（AI 规则）/ README §4（编码纪律）/
//       docs/MEETING_LOG.md 09-29 19:37 + 10-07 两条（M3 定档 + 契约执笔）/
//       third_party/README.md（三方库规矩）；
//       + docs/m3-batch1-briefing.md（你 AI 的任务简报，先喂给它）
// 交付：本卡翻译完毕（注释删光）+ 离线自测 + Ollama 冒烟 + PR
//
// ── 第 0 步 · 复述题（动工前答给你的 AI，答不清回炉）────────────
// Q1 embed_documents 与 embed_query 为什么是两个入口？调用方自己拼
//    前缀会破坏契约哪一条？vector_persist.h 的哪个字段因此失去来源？
// Q2 本契约是全仓第一个 virtual 接口。测试注入 FakeProvider 换来什么？
//    哪类测试永远不需要 Ollama 在跑？（"离线单测为主，真服务只冒烟"）
// Q3 什么情况 throw？为什么"永不返回部分结果"和"不自动重试"是配套
//    设计？（提示：145 块批量请求中途失败只返回前 80 块，建库会发生
//    什么？vector.bin 身份与实际内容还一致吗？）
// Q4 归一化保证是条款 3。下游 vector_index 的哪个优化以它为地基？
//    这里漏归一，下游错在哪一步、错成什么样？
// Q5 分层红线（条款 7）：embedder.cpp 可以 include 什么、禁止什么？
//    httplib.h / json.hpp 为什么只能出现在 .cpp 不能进 .h？
// Q6 make_ollama_provider 构造时为什么禁止网络 I/O（条款 6）？
//    第一次健康检查发生在哪次调用、报什么错？
//
// ── 伪代码（真代码写在注释下方，译一段删一段，全删 = 竣工）──────
//
// 【段 1 · 头部与三方隔离】
// #include "embedder.h"
// #include "../third_party/httplib.h"
// #include "../third_party/nlohmann/json.hpp"
//   —— 两行三方 include 外面包 #pragma warning(push) /
//   #pragma warning(pop)（MSVC；三方头过不了 /W4 不是你的责任，
//   但要隔离噪音，具体禁用警告号编译时看输出再定）
// #include <cmath>      // std::sqrt（归一化）
// #include <memory>     // std::make_unique
// #include <stdexcept>  // std::runtime_error
// #include <string>
// #include <vector>
// （IWYU：用到才写；MSVC 链接若报 winsock，加 ws2_32.lib）
//
// 【段 2 · 前缀策略常量（成对冻结，条款 1 的代码落点）】
// 契约说"前缀由 provider 内部施加，成对冻结"。落到代码：
//   - qwen3 系模型（model_id 以 "qwen3" 开头）：
//     查询侧前缀（constexpr 静态字符串，一字不许运行时拼）：
//       "Instruct: Given a CTF search query, retrieve relevant "
//       "writeup passages that answer the query\nQuery: "
//     文档侧：不加前缀
//   - 其他模型（bge-m3 等）：两侧都不加
//   - embedding_policy() 返回值与前缀一一对应：
//       qwen3 系 -> "qwen3-instruct-v1"；其他 -> "none"
//   为什么冻结：改前缀文本 = 换坐标系 = 旧 vector.bin 全部作废，
//   这正是 vector_persist.h 存 embedding_policy 字段的原因
//
// 【段 3 · OllamaProvider 类骨架】
// class OllamaProvider : public EmbedProvider {
//   成员：endpoint_（空则取 EMBEDDER_DEFAULT_ENDPOINT）、
//         timeout_s_（<=0 则取 EMBEDDER_DEFAULT_TIMEOUT_SECONDS）、
//         model_id_、policy_（构造时按段 2 规则算好存住）
//   构造函数：只做默认值落位与 policy 判定，零网络（条款 6）
//   覆写四个虚函数：embed_documents / embed_query / model_id() const
//   / embedding_policy() const
// };
//
// 【段 4 · embed_documents 主流程】
// if (texts.empty()) return {};          // 条款 2：零网络
// 组请求体（nlohmann::json）：
//   {"model": model_id_, "input": [texts 的每一条...]}
//   —— input 是字符串数组（Ollama /api/embed 原生多数组）
// 发送：
//   httplib::Client cli(endpoint_);
//   cli.set_connection_timeout(timeout_s_, 0);
//   cli.set_read_timeout(timeout_s_, 0);
//   auto res = cli.Post("/api/embed", body, "application/json");
// 失败分支（全部 throw std::runtime_error + 人话，条款 4）：
//   !res                     -> "embedding service unreachable at <endpoint>"
//   res->status != 200       -> 带 status + 响应体前 200 字符
//                               （未知模型 Ollama 回 404，落在这里，
//                                信息已够用，不必单列分支）
//   JSON 解析失败 / 无 "embeddings" -> "malformed response"
//   embeddings.size() != texts.size() -> "malformed response"（条数对不上）
// 成功分支：
//   转 vector<vector<float>>；逐条检查维度一致（不一致 = 畸形，throw）；
//   逐条归一化（条款 3）：norm = sqrt(sum(x*x))，各分量 /= norm；
//   norm == 0 视为畸形 throw
//   顺序保证：output[i] 必须对应 texts[i]（条款 2，数组天然保序）
//
// 【段 5 · embed_query】
// 与段 4 同一条 HTTP 通路（抽一个私有帮手：接收文本数组、发请求、
// 返回归一化后的向量组；段 4 段 5 都调它，逻辑只写一遍）
// 区别两处：输入是单个 text（包成一条的数组发给帮手）；
// qwen3 系先拼段 2 前缀再发。返回向量组里唯一一条
//
// 【段 6 · 工厂】
// std::unique_ptr<EmbedProvider>
// make_ollama_provider(const EmbedderConfig& cfg) {
//   return std::make_unique<OllamaProvider>(cfg);   // 一行
// }
//
// ── 黄金自测表（临时 main 跑完即删，不进 PR）──────────────────
// 编号 | 前置 | 用例 | 期望
//  1 | 离线 | 端点写死 http://localhost:1（必拒连），embed_query("x")
//    |      | throw runtime_error，what() 含 unreachable 与端点
//  2 | 离线 | embed_documents({})（空 vector）
//    |      | 返回空 vector，瞬间完成、不抛（条款 2 零网络）
//  3 | 离线 | 自建 FakeProvider（继承 EmbedProvider，返回确定性
//    |      | 向量，如首维=1 其余=0），经 unique_ptr<EmbedProvider>
//    |      | 多态调用四个接口
//    |      | 全部按 Fake 的返回值走通——注入缝成立的实证
//  4 | 冒烟 | Ollama 在跑、bge-m3 已 pull：
//    |      | embed_query("栈溢出 ret2libc 利用")
//    |      | 1024 维；|norm-1| < 1e-3
//  5 | 冒烟 | embed_documents 三条中英混合文本
//    |      | 3 条向量同维、全部归一；顺序语义验证：cos(v[i],
//    |      | q[i]) 应大于 cos(v[i], q[j])（i!=j，文本差异要够大）
//  6 | 冒烟 | model_id 写 "no-such-model"
//    |      | throw，what() 含 404 或错误响应体
//  7 | 冒烟 | 换 qwen3-embedding:0.6b 跑通 embed_query（前缀通路）
//    |      | 同样 1024 维；跨模型余弦没有比较意义，只验通路
// 离线三题是正式单测（永远不依赖服务活着）；4-7 是冒烟——
// Ollama 没开就在结果里标 SKIP，别把自测变成环境赌局
//
// ── 施工规则 ────────────────────────────────────────────────
// 1. 先答复述题，答不清回炉（TASKS §2 第 0 步）
// 2. 真代码写在注释下方，译一段删一段；本卡删光 = 竣工
// 3. 真代码注释英文纯 ASCII（破折号用 --）；本卡中文脚手架随之消失
// 4. IWYU；/W4 零告警（三方头 push/pop 隔离，见段 1）
// 5. 契约看死：发现 embedder.h 有 bug 或模糊——停手，纪要留言，
//    不得擅改（接口三层规则）
// 6. third_party/ 只许 include 不许改（见 third_party/README.md）
// 7. 自测全过再 push 开 PR；PR 描述写：自测结果 + 契约七条逐条
//    自查（哪条在哪个函数兑现）
// ============================================================================
