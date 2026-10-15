# comstego 3 wp

# comstego_3_wp
---

<!-- page 1 -->

Writeup 
1：Misc-comstego_3 
题目难度：中等 
建议分值：200 分 
题目说明：分析文件，找到flag。提交flag 格式：flag{xxxx}。 
Flag：flag{8ff6ce00747fae9c17846c8219c75b8f} 
解题过程： 
使用010Editor 查看PDF 文件，发现最后一个PDFobj 71 与上面重复 
 
且value 值与其他的都不一样。将此PDFobj 的数据提取出来 
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

 
发现为789c 开头（或者使用binwalk 分析），此为zlib 压缩数据。 
使用pyhton 的zlib 模块解压 
 
得到一段base64 字符，进行base64 解码，两次，成功得到flag 
 
绿盟科技，禁止外传
技，禁止外传
科技，禁止外传
传
绿盟科技，禁止外传
绿盟科技，禁止外传
绿
绿盟科技，禁止
绿盟科技，禁
