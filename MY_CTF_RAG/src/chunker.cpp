// chunker.cpp
// Implements the contract in include/chunker.h.

#include "chunker.h"

// ================================================================
// 伪代码占位 -- 手写真代码写在本块下方，翻译完一段删一段注释，
// 注释删光 = 竣工（真代码的注释请用英文，保持纯 ASCII）。
// ================================================================
//
// 核心难题: 契约用"词"下刀，Chunk 存"字节" -- 数词、记字节。
// 一次扫描同时干两件事: 每数出一个词，就记下它的（起始字节, 结束字节）。
//
// 函数 chunk_documents(docs):
//     chunks = 空列表
//     对 docs 里每个 doc（按输入顺序）:
//         // 第一遍: 扫描 content，产出词位置表
//         words = 空列表，元素是（起始字节 start, 结束字节 end）
//         i = 0
//         当 i < content 长度:
//             如果 content[i] 是词内字符（字母/数字/连字符，与 tokenizer 同规则）:
//                 start = i
//                 一路吞到不是词内字符为止，记 end = i
//                 words 追加（start, end）
//             否则:
//                 i = i + 1
//         W = words.size()
//         如果 W == 0: 跳过这个文档，继续下一个   // 契约: 空文档 0 块
//
//         // 第二遍: 按 CHUNK_SIZE 词下刀，相邻块回退 CHUNK_OVERLAP 词
//         s = 0                                   // 本块从第 s 个词开始
//         循环:
//             e = min(s + CHUNK_SIZE, W)           // 左闭右开 [s, e)
//             begin = words[s].start
//             end   = words[e - 1].end             // 最后一个词的结束字节
//             chunks 追加一个 Chunk:
//                 id          = chunks.size()      // 全局连续编号，跨文档不重置
//                 document_id = doc.id
//                 text        = content.substr(begin, end - begin)
//                 begin, end  = begin, end
//             如果 e == W: 跳出                    // 最后一块已到手
//             s = e - CHUNK_OVERLAP                // 下一块回退 50 词
//     返回 chunks
//
// 提示:
// - 词内字符规则必须与 tokenizer.h 一致; 但这里不能调 tokenize()
//   （它只给词，不给字节位置），所以必须自己扫一遍
// - text 与 begin/end 的一致性由"同一张 words 表"天然保证
// - 按文档顺序处理，全局 id 自然连续递增
// - 短文档: W 不足 500 时 e = W，产 1 块后跳出，契约自动满足
// - 空格换行是分隔符但落在 substr 区间内，块文本保留原始空白
//
// 自测用例:
// - 空文档 / 只有空格的文档    -> 0 块
// - 恰好 10 词                 -> 1 块，覆盖全部 10 词
// - 恰好 500 词                -> 1 块
// - 600 词                     -> 2 块: 词 [0,500) 和 [450,600)，重叠 50
// - doc0 产 2 块后 doc1 产块   -> doc1 第一块 id = 2（全局连续）
