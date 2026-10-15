# twoday_wp

# twoday_wp
---

<!-- page 1 -->

twoday
 
环境
 
glibc2.33 , 题目因为Glibc版本问题所以自带一份编译带调试符号版本的libc，通过patchelf替换对应题目
附件的加载libc的路径即可
程序简介
 
开启沙盒的情况下，在free_safe_string() 功能下存在UAF漏洞，而对堆块查看仅有一次机会。
create_safe_string() 功能 申请2种chunk （ 0x20  ， 0x30
create_safe_string()  会检查申请的chunk 是否在heap的条件
check() 会检查 是否有chunk进入 unsorted bin
信息泄露
 
利用题目给的 数值 与 cookie 进行异或 可以得到 libc
利用 write_safe_string() 函数中 write(1,note[idx],0x10); 打印 0x10 个字节，通过UAF漏洞 ，可以泄露
heap。
利用
 
part1
 
利用  fastbin reverse into tcache  往 IO_2_1_stderr 的chain 填入 heap_base+0x10
part2
 
利用house of pig 的方法进行攻击。劫持 free_hook 为 magic gadget 。
part3
 
使用libc中的一个magic gadget可以控制参数并进行栈迁移成功读取并leak出flag
part4
 
最后利用check() 的exit(0) 进行触发。
绿盟科技，禁止外传
技，禁止外传
科技，禁止外传
传
绿盟科技，禁止外传
绿盟科技，禁止外传
绿
绿盟科技，禁止
绿盟科技，禁
