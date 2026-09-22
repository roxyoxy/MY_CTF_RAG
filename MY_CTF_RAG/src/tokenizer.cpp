// tokenizer.cpp
// Implements the contract in include/tokenizer.h.

#include "tokenizer.h"

// ================================================================
// 伪代码占位 -- 手写真代码写在本块下方，翻译完一段删一段注释，
// 注释删光 = 竣工（真代码的注释请用英文，保持纯 ASCII）。
// ================================================================
//
// 函数 tokenize(text):
//     tokens = 空字符串列表
//     i = 0
//     当 i < text 长度:
//         如果 text[i] 是词内字符（字母 / 数字 / 连字符）:
//             start = i
//             当 i < text 长度 且 text[i] 是词内字符:
//                 i = i + 1
//             tokens 追加 lowercase(text[start .. i))
//         否则:
//             i = i + 1                     // 分隔符，跳过
//     返回 tokens
//
// 提示（每条对应契约条款）:
// - 单趟扫描，两个移动指针（start 和 i）: 经典"扫描窗口"模式
// - "最长连续段" = 一路吞词内字符直到撞上分隔符
// - 判断词内字符: isalnum 或者自己写（字母 || 数字 || '-'）
// - 小写化可以扫字符时顺手做，也可以切出整词后做 -- 你定
// - 空文本根本进不了循环 -> 自然返回空 vector，不用特判
// - 英文语料下字母数字都是单字节，逐字节判断即可
//
// 自测用例（全过才算关账）:
// - "Hello, World"           -> [hello] [world]
// - "use-after-free"         -> [use-after-free]       连字符保留
// - ""                       -> 空 vector
// - "ret2libc NX libc-2.31"  -> [ret2libc] [nx] [libc-2.31]
// - "Phrack #49!"            -> [phrack] [49]          数字是词内字符
