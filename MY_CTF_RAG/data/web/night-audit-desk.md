# 夜班审计台

> **Category**: Web | **Difficulty**: Hard | **Competition**: ISCC 2026 School
> **Flag**: `ISCC{K6FRFyHAMaMmPZNmXXpA}`

---

## Attack Chain

```
JS发现.git泄露 → Git Objects手动提取源码 → 版本对比获取双份密钥 → JWT RS256→HS256算法混淆提权 → SSRF + HMAC签名伪造 → 读flag
```

## Key Techniques

### 1. Git源码泄露 + 版本对比

`main.js` 暴露 `window.__buildTrace = "/.git/HEAD"`

手动提取: `commit → tree → blob` (zlib解压)

**关键**: 两个commit的**同名文件内容不同**！
- 最新版: 账号密码 + JWT密钥 + 算法信息
- **旧版本**: SSRF签名密钥 + 签名格式（新版已删除）

### 2. JWT算法混淆攻击

```
RS256(非对称) → HS256(对称)
已知HS256密钥: ISCC_2026_JWT_DEBUG_KEY_#9527
→ 伪造 role: "auditor" 的Token
```

### 3. SSRF + HMAC签名伪造

```python
SERVER_SECRET = "ISCC_SERVER_SECRET_REAL"  # 从旧版获取
msg = f"core-storage-01:{unix_timestamp}"
sign = HMAC-SHA256(key=SERVER_SECRET, msg=msg).hexdigest()
```

服务器以127.0.0.1发起请求 → 天然绕过IP白名单。

## Distilled Knowledge

| 知识点 | 关键 |
|--------|------|
| Git泄露 | `.git/objects/` 手动恢复源码 |
| **版本对比** | 检查所有commit! 同名文件可能内容不同 |
| JWT算法混淆 | RS256→HS256 + 已知密钥 = 任意伪造 |
| SSRF | 服务器端请求天然满足IP白名单 |
| HMAC签名伪造 | 泄露密钥+消息格式=合法签名 |

---

*Original writeup: `C:\Users\48714\Desktop\wp\校赛\夜班审计台_wp.md`*
