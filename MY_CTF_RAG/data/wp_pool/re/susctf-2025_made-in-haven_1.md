# susctf 2025 made in haven~1

key：`deepdarkfantasy!`

key 加密存储，前 8 个字节 xor 19260817, 后 8 个字节 + 20251001

flag: `susctf{sp33d_up_time_t0_h34v3n!}`

MadeInHaven.cpp 是用天堂之门混淆后的源代码

main.cpp 是原本的源代码

```
需要使用 visual studio 创建项目编译 
项目-项目属性中更改以下设置：
关闭随机基址：
-链接器-高级-随机基址：否
保留未引用的函数/数据：
-链接器-常规-启用增量链接：否
-链接器-优化-链接时间代码生成：使用链接时间代码生成/LTCG
-链接器-优化-链接时间代码生成：引用：否
关闭其他优化：
-C/C++-优化-优化：已禁用
-C/C++-代码生成-运行库：多线程（/MT）
-C/C++-高级-调用约定：__fastcall
```
