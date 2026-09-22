// indexer.cpp
// Implements the contract in include/indexer.h.

#include "indexer.h"

// ================================================================
// 伪代码占位 -- 手写真代码写在本块下方，翻译完一段删一段注释，
// 注释删光 = 竣工（真代码的注释请用英文，保持纯 ASCII）。
// ================================================================
//
// ---------- 建账 build_index(chunks) ----------
//
// 函数 build_index(chunks):
//     index = 空的 InvertedIndex
//     对每个 chunk c（按顺序，id 升序）:
//         tokens = tokenize(c.text)                // 契约: 同一个函数
//         index.chunk_lengths[c.id] = tokens 的个数
//         // 本块内按词分组计数
//         counts = 空映射: term -> tf
//         对 tokens 里每个 t: counts[t] 加一
//         对 counts 里每对 (term, tf):
//             index.postings[term] 追加 {c.id, tf}
//         // 按块顺序处理，posting 列表天然按 chunk_id 升序
//     total = 所有 chunk_lengths 之和
//     index.avg_chunk_length = 块数为 0 ? 0.0 : total / 块数
//     返回 index
//
// ---------- 查账 search(index, query, top_k) ----------
//
// 函数 search(index, query, top_k):
//     N = index.chunk_lengths.size()
//     qtokens = tokenize(query)                    // 契约: 两端同源
//     scores = 空映射: chunk_id -> float           // 累加器
//     对 qtokens 里每个 t（不去重，公式本来就是逐词求和）:
//         posting = index.postings 里查 t
//         查不到 -> 跳过这个 t
//         df = posting.size()                      // df 免费得到: 表长就是 df
//         idf = ln(1 + (N - df + 0.5) / (df + 0.5))
//         对 posting 里每项 (cid, tf):
//             dl  = index.chunk_lengths[cid]
//             分数 = idf * tf * (BM25_K1 + 1)
//                    / (tf + BM25_K1 * (1 - BM25_B + BM25_B * dl / avgdl))
//             scores[cid] += 分数
//     把 scores 倒进 vector，排序:
//         主关键字 score 降序; 同分 chunk_id 升序
//     截取前 top_k 条，逐个装进 vector<SearchResult>
//     返回
//
// 提示:
// - build 和 search 都要调 tokenize() -> 想想本文件还要 include 谁（IWYU）
// - 排序用 std::sort + 自写比较器; "严格弱序"，同分比 id 时方向别写反
// - top_k 大于结果总数时: 全部返回，别越界
// - 空查询 / 空索引: qtokens 为空或 N 为 0，自然返回空 vector
// - 整数除法陷阱: total / 块数 两边都是 int 会截断，先转 double;
//   dl / avgdl 里 avgdl 是 double，安全
//
// 自测用例:
// - chunks 为空              -> 空索引（三个字段全空 / 0.0）
// - 1 个 chunk 含 3 个词     -> chunk_lengths[0] == 3
// - 同一词块内出现 3 次      -> postings[词] = [{0, 3}]
// - 同一词跨 2 块            -> posting 两项，chunk_id 升序
// - 查询含未登录词           -> 该词跳过，不炸不出 NaN
// - 查 "gadget" 命中 2 块    -> 分高在前，同分 id 小在前，最多 top_k 条
// - 空查询串                 -> 空 vector
