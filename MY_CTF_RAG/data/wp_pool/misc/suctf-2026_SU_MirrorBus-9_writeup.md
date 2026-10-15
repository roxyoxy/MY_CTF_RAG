# suctf-2026_SU_MirrorBus-9__writeup

﻿# MirrorBus-9 Writeup（作者版）

## 题目定位

MirrorBus-9 是一道黑盒系统辨识题。  
选手通过 TCP 连接一个半双工工业总线模拟器，只能观察协议回包，无法直接看到内部状态。

这道题既可以人工完成，也非常适合交给 agent 完成：

- 人工路线：自己做协议分析、重放实验、差分建模和方程求解。
- AI 路线：给 agent 一个足够准确的提示词，让它自己写脚本并完成求解。

本题真正考察的是下面几件事：

1. 能否识别这是一个 `ENQ -> COMMIT -> POLL` 的半双工 no-echo 协议。
2. 能否利用 `RESET` 把黑盒系统变成可重放实验台。
3. 能否从 `ARM_FAIL` 的二维残差中恢复控制关系。
4. 能否在同一条连接内完成 `CHAL -> PROVE -> flag`。

## 关键误导点

题目里有几层误导，但都可以通过实验推翻：

- `ENQ` 没有状态回显，不代表服务坏了，而是协议本来就这样设计。
- `QOK` 只代表入队成功，不代表命令已经执行。
- `POLL` 看到的是延迟后的帧，不是刚才那条命令的即时输出。
- `lane` 是混淆提示，不是实际物理通道。
- banner 中会送一个 gift 假 flag，它不能提交。
- 公共靶机模式下，每次重新连接都会换 session seed，所以建模和求解必须尽量在同一条 TCP 连接内完成。

## 人工解法

### 1. 先确认协议节奏

推荐最小交互：

```text
HELP
STATUS
ENQ INJ 0 1
COMMIT
POLL 16
```

可以观察到：

- `ENQ` 只会返回 `QOK`。
- `COMMIT` 才会推进内部 tick 并产生帧。
- `POLL` 才能真正读到 `F ...` 观测帧。

因此题目的正确观察顺序不是“发命令立刻看结果”，而是：

```text
ENQ -> COMMIT -> POLL
```

### 2. 用 `RESET` 做重放实验

继续做对照：

```text
RESET
ENQ INJ 0 1
COMMIT
POLL 16

RESET
ENQ INJ 0 1
COMMIT
POLL 16
```

如果两次结果完全一致，说明：

- 同 transcript 下回包完全一致。
- 噪声和延迟是确定性的，不是真随机。
- 黑盒建模可以通过重放和差分完成。

### 3. 理解终点 gate

真正的目标是执行 `ARM`。

当 `ARM` 被 `COMMIT` 执行时，系统会检查当前隐藏状态是否满足两条线性约束：

- 满足时返回 `CHAL`
- 不满足时返回 `ARM_FAIL`

`ARM_FAIL` 的关键是：

- 不直接给出“距离目标还差多少”
- 但会给出 `sig` 和 `aux`

这两个值本质上是二维残差经过线性混合后的结果。  
虽然服务没有把原始残差明文给出，但它给了一个可用于求解的二维投影。

### 4. 只用最简单的控制手段

协议支持：

- `INJ`
- `ROT`
- `MIX`
- `BIAS`

但参考解实际只用了 `INJ`。  
原因是 `INJ lane value` 的控制最直接，而且两路注入就足够建立二维方程。

所以固定实验模板：

```text
RESET
ENQ INJ i a
ENQ INJ j b
ENQ ARM
COMMIT
POLL ...
```

其中：

- `i, j` 是两条候选通道
- `a, b` 是要控制的两个变量

### 5. 做三组差分实验

对一对候选通道 `(i, j)`，做三组实验：

1. 基线：

```text
(a, b) = (0, 0)
```

得到：

```text
d0 = (sig0, aux0)
```

2. 第一维扰动：

```text
(a, b) = (1, 0)
```

得到：

```text
di = (sigi, auxi)
```

3. 第二维扰动：

```text
(a, b) = (0, 1)
```

得到：

```text
dj = (sigj, auxj)
```

然后做差：

```text
e_i = d0 - di
e_j = d0 - dj
```

这里的 `e_i`、`e_j` 就是这两条注入通道对二维残差的贡献方向。

### 6. 找一组可逆通道

不是任意两条 lane 都能用。  
如果 `e_i` 和 `e_j` 线性相关，就没法解二维方程。

因此需要枚举 lane 对，检查：

```text
det = e_i[0] * e_j[1] - e_i[1] * e_j[0] mod 65521
```

只要 `det != 0`，这对通道就能用。

### 7. 解 2x2 模线性方程

找到可逆通道后，目标就是把残差打到 0。

已知：

```text
e_i
e_j
d0
```

可以直接求：

```text
[ e_i  e_j ] [vi, vj]^T = d0
```

所有运算都在模 `65521` 下完成。  
解出来的 `(vi, vj)` 就是目标注入值。

### 8. 做少量修正

为了更稳，可以把求出的 `(vi, vj)` 再跑一次：

- 如果已经拿到 `CHAL`，直接结束。
- 如果还是 `ARM_FAIL`，再用当前残差解一次修正量。
- 把修正量加回去。

参考解里最多做 4 次修正，基本足够。

### 9. 拿到 `CHAL` 后计算 `PROVE`

成功后会拿到 `CHAL` 帧，其中已经给出：

- `nonce`
- `sig`
- `aux`

最终证明参数为：

- `p1 = sig`
- `p2 = aux`
- `p3 = CRC16-CCITT(nonce:p1:p2)`

提交：

```text
PROVE p1 p2 p3
```

通过后返回真实 flag。

## AI 辅助做法

### 给 Agent 的推荐提示词

下面这段提示词可以直接发给 Codex / ChatGPT / 其他代码型 agent：

```text
把 HOST:PORT 当黑盒 TCP 服务处理，不要假设能看到源码。忽略 banner 里的 gift 假 flag，在同一条 TCP 连接内完成全部求解。先用 HELP/STATUS 识别协议，再确认它是 `ENQ -> COMMIT -> POLL` 的半双工 no-echo 语义；用 RESET 做重放实验，验证同 transcript 下输出稳定一致。围绕 `RESET; ENQ INJ i a; ENQ INJ j b; ENQ ARM; COMMIT; POLL` 做差分实验，从 ARM_FAIL 的 `sig/aux` 两维残差建立 2x2 模线性方程，枚举 lane 对找到行列式非零的一组，解出能把残差消到 0 的两路注入值；如果第一次没到 CHAL，就根据新的 ARM_FAIL 残差做 1 到 4 次修正。拿到 CHAL 后，取 `p1=sig`、`p2=aux`，计算 `p3 = CRC16-CCITT(nonce:p1:p2)`，再提交 `PROVE p1 p2 p3`。请直接写并运行脚本，最终只输出真实 flag。
```

### 使用要点

如果 agent 没抓住下面几点，容易跑偏：

- 这是黑盒题，不是源码审计题。
- 需要始终保持同一条 TCP 连接。
- 不能把 `QOK` 当成命令执行结果。
- 不能把 banner 里的 gift 当真实 flag。
- 必须优先围绕 `RESET + 差分实验 + 2x2 求解` 组织思路。

## 完整求解脚本

下面是完整可运行脚本，逻辑与 `exp/exp.py` 一致，可直接使用：

```python
#!/usr/bin/env python3
"""
MirrorBus-9 complete exp script (black-box solve).

Usage:
  python exp.py --host 127.0.0.1 --port 9999
"""

from __future__ import annotations

import argparse
import socket
from dataclasses import dataclass
from typing import Dict, Iterable, List, Tuple


MOD = 65521


def parse_kv_line(line: str) -> tuple[str, Dict[str, str]]:
    tokens = line.strip().split()
    if not tokens:
        return "", {}
    head = tokens[0]
    fields: Dict[str, str] = {}
    for token in tokens[1:]:
        if "=" not in token:
            continue
        k, v = token.split("=", 1)
        fields[k] = v
    return head, fields


def inv_mod(x: int, mod: int = MOD) -> int:
    x %= mod
    if x == 0:
        raise ValueError("non-invertible value")
    return pow(x, -1, mod)


def solve_2x2(
    a11: int,
    a12: int,
    a21: int,
    a22: int,
    b1: int,
    b2: int,
    mod: int = MOD,
) -> Tuple[int, int]:
    det = (a11 * a22 - a12 * a21) % mod
    inv_det = inv_mod(det, mod)
    x1 = ((b1 * a22 - b2 * a12) * inv_det) % mod
    x2 = ((a11 * b2 - a21 * b1) * inv_det) % mod
    return x1, x2


def crc16_ccitt(data: bytes) -> int:
    crc = 0xFFFF
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ 0x1021) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF
    return crc


@dataclass
class MB9Client:
    host: str
    port: int
    timeout: float = 5.0

    def __post_init__(self) -> None:
        self.sock = socket.create_connection((self.host, self.port), timeout=self.timeout)
        self.sock.settimeout(self.timeout)
        self.rfile = self.sock.makefile("r", encoding="utf-8", newline="\n")
        self.wfile = self.sock.makefile("w", encoding="utf-8", newline="\n")
        self.banner = [self._readline(), self._readline()]

    def close(self) -> None:
        try:
            self.wfile.close()
        finally:
            try:
                self.rfile.close()
            finally:
                self.sock.close()

    def _readline(self) -> str:
        line = self.rfile.readline()
        if line == "":
            raise RuntimeError("connection closed")
        return line.rstrip("\r\n")

    def _send(self, cmd: str) -> None:
        self.wfile.write(cmd + "\n")
        self.wfile.flush()

    def cmd(self, cmd: str) -> List[str]:
        cmd_name = cmd.strip().split()[0].upper()
        self._send(cmd)
        if cmd_name == "POLL":
            out: List[str] = []
            while True:
                line = self._readline()
                out.append(line)
                if line == "END":
                    break
            return out
        if cmd_name == "HELP":
            out = []
            while True:
                line = self._readline()
                out.append(line)
                if line.startswith("OK cmd=HELP") or line.startswith("ERR "):
                    break
            return out
        return [self._readline()]


def poll_until(client: MB9Client, wanted_tags: Iterable[str], rounds: int = 8) -> Dict[str, str]:
    wanted = set(wanted_tags)
    for _ in range(rounds):
        lines = client.cmd("POLL 64")
        for line in lines:
            head, fields = parse_kv_line(line)
            if head != "F":
                continue
            if fields.get("tag") in wanted:
                return fields
    raise RuntimeError(f"no wanted frame after polling: {sorted(wanted)}")


def run_trial(client: MB9Client, injections: Dict[int, int]) -> Dict[str, str]:
    client.cmd("RESET")
    for lane, value in sorted(injections.items()):
        client.cmd(f"ENQ INJ {lane} {value}")
    client.cmd("ENQ ARM")
    client.cmd("COMMIT")
    return poll_until(client, {"ARM_FAIL", "CHAL"})


def run_pair_trial(client: MB9Client, lane_i: int, value_i: int, lane_j: int, value_j: int) -> Dict[str, str]:
    client.cmd("RESET")
    client.cmd(f"ENQ INJ {lane_i} {value_i}")
    client.cmd(f"ENQ INJ {lane_j} {value_j}")
    client.cmd("ENQ ARM")
    client.cmd("COMMIT")
    return poll_until(client, {"ARM_FAIL", "CHAL"})


def extract_delta(frame: Dict[str, str]) -> Tuple[int, int]:
    return int(frame["sig"]) % MOD, int(frame["aux"]) % MOD


def compute_prove_from_chal(chal: Dict[str, str]) -> Tuple[int, int, int]:
    nonce = chal["nonce"]
    p1 = int(chal["sig"]) % MOD
    p2 = int(chal["aux"]) % MOD
    p3 = crc16_ccitt(f"{nonce}:{p1}:{p2}".encode("utf-8"))
    return p1, p2, p3


def solve(client: MB9Client) -> str:
    base = run_trial(client, {})
    if base.get("tag") == "CHAL":
        p1, p2, p3 = compute_prove_from_chal(base)
        return client.cmd(f"PROVE {p1} {p2} {p3}")[0]

    chosen = None
    d0_1 = 0
    d0_2 = 0
    for li in range(9):
        for lj in range(li + 1, 9):
            base2 = run_pair_trial(client, li, 0, lj, 0)
            if base2.get("tag") == "CHAL":
                p1, p2, p3 = compute_prove_from_chal(base2)
                return client.cmd(f"PROVE {p1} {p2} {p3}")[0]
            d0_1, d0_2 = extract_delta(base2)

            tri = run_pair_trial(client, li, 1, lj, 0)
            trj = run_pair_trial(client, li, 0, lj, 1)
            if tri.get("tag") != "ARM_FAIL" or trj.get("tag") != "ARM_FAIL":
                continue
            di_1, di_2 = extract_delta(tri)
            dj_1, dj_2 = extract_delta(trj)
            e_i = ((d0_1 - di_1) % MOD, (d0_2 - di_2) % MOD)
            e_j = ((d0_1 - dj_1) % MOD, (d0_2 - dj_2) % MOD)
            det = (e_i[0] * e_j[1] - e_i[1] * e_j[0]) % MOD
            if det != 0:
                chosen = (li, lj, e_i, e_j)
                break
        if chosen is not None:
            break
    if chosen is None:
        raise RuntimeError("no invertible lane pair found")

    li, lj, e_i, e_j = chosen
    vi, vj = solve_2x2(e_i[0], e_j[0], e_i[1], e_j[1], d0_1, d0_2, mod=MOD)
    vi %= MOD
    vj %= MOD

    for _ in range(4):
        result = run_pair_trial(client, li, vi, lj, vj)
        if result.get("tag") == "CHAL":
            p1, p2, p3 = compute_prove_from_chal(result)
            return client.cmd(f"PROVE {p1} {p2} {p3}")[0]
        r1, r2 = extract_delta(result)
        c_i, c_j = solve_2x2(e_i[0], e_j[0], e_i[1], e_j[1], r1, r2, mod=MOD)
        vi = (vi + c_i) % MOD
        vj = (vj + c_j) % MOD

    raise RuntimeError("failed to reach challenge state")


def main() -> int:
    parser = argparse.ArgumentParser(description="MirrorBus-9 complete exp")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=9999)
    parser.add_argument("--timeout", type=float, default=5.0)
    args = parser.parse_args()

    client = MB9Client(host=args.host, port=args.port, timeout=args.timeout)
    try:
        line = solve(client)
    finally:
        client.close()

    print(line)
    if "status=PASS" not in line or "flag=" not in line:
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
```

## 运行方式

```bash
python exp.py --host 127.0.0.1 --port 9999
```

远程靶机只需要替换成实际地址即可。

## 总结

这道题的 intended solution 本质上是一致的：

- 人工做法：自己完成黑盒重放、差分实验和模线性求解。
- AI 做法：把这些步骤准确地描述给 agent，让它代为实现和执行。

所以这题最核心的一句话可以写成：

**既可以人工解，也可以 AI 辅助解；真正关键是你是否理解了黑盒系统该怎么被建模。**
