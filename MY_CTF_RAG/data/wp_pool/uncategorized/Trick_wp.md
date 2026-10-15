# Trick wp

# Trick_wp
---

<!-- page 1 -->

Trick Writeup 
PWN-Trick 
知识点：heap 和int cast 
题目难度： 
建议分值：200 分 
题目说明：Heap tricks! 提交flag 格式：flag{xxxx}。 
Flag：flag{VulnEr4ble_1nt_c4st} 
解题过程： 
首先用IDA 进行逆向，结合程序执行信息，发现程序主要提供Add/Delete/Show/Edit/Comment
这5 个功能： 
 
核心的漏洞点在Show 与Edit 获取堆块idx 处，存在int 类型强转（64 位转32 位）导致的数
组越界。 
以Delete 函数（sub_C8F）为例： 
 
其中，sub_b20()用于获取idx，其返回类型为long long int，但最终在使用v1（idx）时做了(signed 
绿盟科技，禁止外传
技，禁止外传
科技，禁止外传
传
绿盟科技，禁止外传
绿盟科技，禁止外传
绿
绿盟科技，禁止
绿盟科技，禁


---

<!-- page 2 -->

int)类型转换。同时注意对于v1 的检查为(signed int)v1 < 0 || v1 > 31，那么当输入
-9223372036854775807 时即可绕过该检查，且(signed int) 9223372036854775807=32。因为
本题堆块最多为32 个，因此qword_202040[32]已经越界。 
 
进一步通过逆向可以发现，qword_202040 中存的是结构体指针，结构体如下： 
Struct chunk { 
 
Char *addr; 
 
Unsigned int size; 
} 
且qword_202040[32]指向comment 输入的区域，因此我们可以沟通泄露堆地址，在堆上伪
造fake_chunk，使fake_chunk.addr 指向free_hook，fake_chunk.size 等于8，并在comment
区域填上fake chunk 所在的堆地址，即可完成对free_hook 的改写。 
 
（本题可以通过常规方法泄露libc 地址和heap 地址，不再赘述，具体利用脚本见exp.py） 
绿盟科技，禁止外传
技，禁止外传
科技，禁止外传
传
绿盟科技，禁止外传
绿盟科技，禁止外传
绿
绿盟科技，禁止
绿盟科技，禁
