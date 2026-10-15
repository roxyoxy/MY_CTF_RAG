# suctf 2026 SU CyberTrack writeup

## 题面

### Site：
[https://evanlin-suctf.github.io/](https://evanlin-suctf.github.io/)
## 题目描述

互联网会记住很多人以为已经消失的信息。
你偶然发现了一个个人博客。表面上看，这只是一个记录日常生活和技术笔记的小站，但在这些零散的信息中，似乎隐藏着一些被忽视的线索。
尝试追踪这些数字痕迹，将散落在网络各处的信息拼接起来，看看它们最终会指向哪里。
最终的 **flag 是某个字符串的 MD5 值**。

---
## Flag 生成规则

该字符串由 **目标人物姓名 与 一个发现的字符串拼接而成**，然后将整个结果 **转换为小写** 后计算 MD5。

---
## 姓名格式
姓名需要按以下格式拼接：

```
姓 + 名
```

规则：
- 去掉所有空格    
- 姓名之间 **不添加任何分隔符**  
- 顺序为 **姓在前，名在后**  
---
## 最终计算规则

```
md5(lowercase(<姓><名> + "_" + <字符串>))
```

---
## Flag示例

```
姓名: Sanfeng Zhang
字符串: abcd-dcba

拼接结果:
zhangsanfeng_abcd-dcba

Flag:
SUCTF{md5(zhangsanfeng_abcd-dcba)}
```

## 题解
本题在解题路线上属于双向并行任务，且博客中都有具体的提示：
- 姓名
	1. 通过博客中信息了解邮箱设置了自动回复及 @foxmail.com 的邮箱域名
	2. 通过使用 Gravatar 托管的头像 url 获取邮箱 hash
	3. 使用博客中的已知信息构造社会工程学字典，爆破邮箱明文
	4. 向邮箱发送任意邮件，从自动回复邮件中获得姓名
- 特殊字符串
	1. 通过游戏图片获取到 Mnzn233 的游戏 ID
	2. 结合博客内容以及 Minecraft，搜索到游戏历史 ID
	3. 根据线索，；利用搜索工具找到指定社交账号，获取特殊字符串
	其中邮箱的获取途径似乎被非预期了，为了方便选手访问博客，使用了 GitPage 托管，却忘记了修改 git 提交记录中的邮箱，意识到的时候决定愿赌服输吧，还是我人菜导致的

这是一道非常传统的社工题，注意留意跟个人身份相关的关键信息
一共存在 7 篇博客日记
- [how they found me??](https://suctf.mnzn.me/archives/how-they-found-me)
- [play with me T_T](https://suctf.mnzn.me/archives/play-with-me-t_t)
- [Happy birthday~](https://suctf.mnzn.me/archives/happy-birthday~)
- [don't spam....](https://suctf.mnzn.me/archives/don-t-spam....)
- [normal life](https://suctf.mnzn.me/archives/normal-life)
- [sad](https://suctf.mnzn.me/archives/sad)
- [today](https://suctf.mnzn.me/archives/04-18)

比较直接的想法就是从里面获取信息然后寻找其他的社交媒体
最先注意到play with me篇中的图片

![id](https://raw.githubusercontent.com/quietdawn/picture/main/id.png)

昵称是Mnzn233，结合 Minecraft，Google 一下即可得到

![image-20260309124709994](https://raw.githubusercontent.com/quietdawn/picture/main/image-20260309124709994.png)

![image-20260309124908590](https://raw.githubusercontent.com/quietdawn/picture/main/image-20260309124908590.png)

同时注意到 how they found me?? 篇中提到的，证实了曾用名是正确的方向

```
“最近突然有几个人在留言或者私信里提到我以前用过的一个旧网名。那个名字其实已经很久没用了，所以突然看到有人提起来的时候还是有点意外。

更离谱的是，居然还有人根据这个名字在另一个平台找到了我的账号，然后给我发了消息。虽然内容也只是随便说认出了这个名字，但还是让人感觉有点不太舒服。”
```

思路转向在 namemc 查到的曾用名，尝试 TurbidCloud,使用 https://instantusername.com/ 来检索其他社媒，检索到了Twitter

![image-20260309130039205](https://raw.githubusercontent.com/quietdawn/picture/main/image-20260309130039205.png)
访问一下账号主页，能看到一个 discord 服务器的邀请链接，名为 Flag@here
说明找对了

![](attachments/Pasted%20image%2020260317125416.png)

![image-20260309125749481](https://raw.githubusercontent.com/quietdawn/picture/main/image-20260309125749481.png)

进入discord群聊即可得到字符串 `ddc7-4622-8a97`
![](attachments/Pasted%20image%2020260317125524.png)

现在还差姓名没有找到 ,思路转向其他几篇日记，注意到 don't spam.... 篇中的图片

![email](https://raw.githubusercontent.com/quietdawn/picture/main/email.png)

分析这张图片能得到两个点：一个是该邮箱的后缀名是 `@foxmail.com`,另外一个奇怪的点在于Auto Reply is enabled， 邮箱的自动回复是开着的，那么`不难注意到`，应该是要找到这个邮箱地址并且发一封邮件，自动回复的邮件里就会有目标人物的真实姓名了。
梳理已知的线索，然后最烧脑子的地方来了，浏览还没有得到利用的几篇日记：
today篇提到了叫Momo的布偶猫，检索了一下发现没有什么可用的信息
normal life篇提到了同事shukuang和女朋友kanna.seto 看起来也是干扰信息

![image-20260309131246673](https://raw.githubusercontent.com/quietdawn/picture/main/image-20260309131246673.png)

sad篇提到更换了头像，那么视角转到头像上

```
"下班回家之后也没什么特别想做的事情，随便做了点吃的，看了会儿视频，尽量不去想工作的事情。后来打开自己的账号页面，突然就想着把头像换掉了。之前那个头像其实已经用了挺久，正好换个新的。"
```

从网站资源发现头像使用 Gravatar 托管
https://gravatar.com/avatar/105e127d86711d05460e6072f7d809c5c9e0fe095ca7631e4c2e0ffc4acc3fa9

查阅资料得知：

Gravatar（**Globally Recognized Avatar**）是一个全球通用头像服务，它：

1. 以一个邮箱地址为基础生成一个哈希值。
2. 然后根据这个哈希值返回用户设置的头像。
3. 如果该哈希对应的邮箱没有设置过头像，则可能返回默认图像。

头像 URL 的格式通常是：

```
https://gravatar.com/avatar/<HASH>
```

其中 `<HASH>` 是 **邮箱地址经处理后哈希出来的值**

可以推理得到：这个sha256哈希值应该就是目标邮箱生成的哈希值。
这里使用 cupp 生成社会工程学字典进行爆破
结合一下博客中提供的已知信息
```
Today -> 布偶猫 Momo
Normal life -> 同事 shukuang
Happy birthday -> ⽣⽇ 2024年11⽉23⽇
Play with me t_t -> 游戏ID Mnzn233
以及博客提供的昵称 EvanLin
```
使用这些信息构建字典，然后写如下exp爆破哈希：

```python
import hashlib

target_hash = "105e127d86711d05460e6072f7d809c5c9e0fe095ca7631e4c2e0ffc4acc3fa9"

with open("evan.txt", "r", encoding="utf-8", errors="ignore") as f:
    for line in f:
        candidate = line.strip()
        email = candidate + "@foxmail.com"   
        sha256_hash = hashlib.sha256(email.encode()).hexdigest()

        if sha256_hash == target_hash:
            print("Match found:", email)
            break
```

爆破结果：

```
Match found: evanlin1123@foxmail.com
```

向 `evanlin1123@foxmail.com` 发送一封任意内容的邮件，即可收到回复：

![image-20260309132106058](https://raw.githubusercontent.com/quietdawn/picture/main/image-20260309132106058.png)

果然邮件最后出现了姓名Zeyuan Lin
直接拼接姓名和字符串提交md5即可

flag：`SUCTF{c4d1df3b3dbea17c886b447b7f913048}`
