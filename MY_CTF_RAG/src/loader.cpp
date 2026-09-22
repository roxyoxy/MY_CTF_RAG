// loader.cpp
// Implements the contract in include/loader.h.

#include "loader.h"

// ================================================================
// 伪代码占位 -- 手写真代码写在本块下方，翻译完一段删一段注释，
// 注释删光 = 竣工（真代码的注释请用英文，保持纯 ASCII）。
// ================================================================
//
// 函数 load_documents(dir):
//     如果 dir 不存在:
//         throw（契约: fail fast，越早炸越好定位）
//     收集所有文件:
//         递归遍历 dir，捡出所有扩展名为 .md 和 .txt 的文件
//         （工具: C++17 的 std::filesystem::recursive_directory_iterator）
//     把文件路径按字典序排序             // 契约: 排完再编号
//     docs = 空列表
//     对排序后的每个文件 f:
//         content = 整个文件一次读入一个字符串（按字节原样，别做编码转换）
//         rel = f 相对 dir 的路径，反斜杠全部换成正斜杠 /
//         docs 追加一个 Document:
//             id      = docs.size()      // 谁生产谁编号: 加载器发 id
//             path    = rel
//             content = content
//             deleted = false            // 出生时都是活的
//     返回 docs
//
// 提示:
// - 读整个文件: ifstream + rdbuf 一次倒进 stringstream，两行搞定
// - "UTF-8 原样读" = 按字节读进来就行，别让 ifstream 转换任何东西
// - 扩展名比较忽略大小写（.MD / .Md 都算）
// - 字典序用默认 string 比较即可，比较相对路径
// - 空目录不 throw（契约只说"目录不存在"才 throw），返回空 vector
//
// 自测用例:
// - data 目录不存在           -> throw
// - 空目录                    -> 空 vector，不炸
// - 3 个文件 a.md b.txt c.md  -> id 0/1/2，path 是相对路径正斜杠
// - 子目录 data/pwn/x.md      -> 也能捡到，path = "pwn/x.md"
// - 文件内容                  -> content 与磁盘字节完全一致
