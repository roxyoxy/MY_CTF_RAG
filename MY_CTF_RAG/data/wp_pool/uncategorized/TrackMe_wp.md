# TrackMe_wp

# TrackMe_wp
---

<!-- page 1 -->

11：Reverse-TrackMe 
知识点：Reverse 
题目难度： 
建议分值： 
题目说明：请找出答案，flag 为用户名为nsfocus 时的密码，提交flag{密码}。提交flag 格
式：flag{xxxx}。 
Flag：flag{gmak`sr} 
解题过程： 
trackme.py 
 
username = "nsfocus" 
user_len = len(username) 
 
password = "" 
 
tmp = user_len 
for x in range(0, user_len): 
    password += chr(ord(username[x]) - tmp) 
    tmp -= 1 
 
print password 
绿盟科技，禁止外传
技，禁止外传
科技，禁止外传
传
绿盟科技，禁止外传
绿盟科技，禁止外传
绿
绿盟科技，禁止
绿盟科技，禁
