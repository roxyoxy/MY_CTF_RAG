# 拆弹专家

> **Category**: Reverse | **Difficulty**: Medium | **Competition**: ISCC 2026 区域赛
> **Flag**: `ISCC{DAKJULSU033984530RDRDRD}`

---

## Attack Chain

```
源码审计 → 注释泄露Password1 → LCG重现Password2 → BFS迷宫寻路Password3
```

## Key Techniques

### 1. 三关链式结构

程序模拟"拆弹"场景，需要依次输入三个密码，三关之间存在数据依赖链：

```
Password1 → FNV1a Hash → 派生方程系数 → Password2 → 派生迷宫Seed → 生成4x4迷宫 → Password3
```

Flag = `ISCC{password1 + password2 + password3}`

### 2. 第一关 — 注释泄露密码

源码中自定义了"魔方密码"加密算法（PKCS#7 填充 + 加法混淆 mod 256 + 位置置换），密钥 `R U F R' D2` 用 XOR 0x5A 混淆存储。加密后用 FNV1a 哈希校验。

关键突破口：**源码注释直接泄露了答案**：

```c
static const uint32_t EXPECT_OBF = 0x7FE9C23Eu; // generated for stage1=DAKJULSU
```

Password1 = `DAKJULSU`，验证：`0xDA4C679B ^ 0xA5A5A5A5 == 0x7FE9C23E` ✓

### 3. 第二关 — LCG 生成线性方程组的解

第一关的 hash `0xDA4C679B` 用于派生第二关参数：

1. 从 hash 的不同位段派生系数 `g_a = [5, 5, 6, 8, 8]`
2. 用 LCG（`state = state * 1664525 + 1013904223`）生成解 `x = [0, 33, 98, 45, 30]`
3. 拼接为9位数字串 → Password2 = `033984530`

### 4. 第三关 — BFS 迷宫寻路

用 password2 派生 seed 生成 4x4 迷宫：

```
1 1 0 0
0 1 1 0
0 0 1 1
0 0 0 1
```

BFS 唯一路径：`R → D → R → D → R → D`，Password3 = `RDRDRD`

## Exploit

```python
from collections import deque

def fnv1a32(s):
    h = 0x811C9DC5
    for c in s:
        h ^= ord(c)
        h = (h * 0x01000193) & 0xFFFFFFFF
    return h

def lcg(state):
    return (state * 1664525 + 1013904223) & 0xFFFFFFFF

# === 第一关 ===
pw1 = "DAKJULSU"
stage1_hash = fnv1a32(encrypt(pw1, "R U F R' D2"))
print(f"Password1: {pw1}  Hash: 0x{stage1_hash:08X}")

# === 派生第二关 ===
h = stage1_hash
state = (h ^ 0x12345678) & 0xFFFFFFFF
x = [0]*5
for i in range(5):
    state = lcg(state)
    x[i] = (10 + state % 90) if i else (state % 10)
pw2 = str(x[0]) + f"{x[1]:02d}{x[2]:02d}{x[3]:02d}{x[4]:02d}"
print(f"Password2: {pw2}  x={x}")

# === 第三关 ===
maze_seed = int(pw2) ^ ((stage1_hash * 0x9E3779B1) & 0xFFFFFFFF)
st = (maze_seed ^ 0x9E3779B9) & 0xFFFFFFFF
maze, mx, my = [0]*16, 0, 0
maze[0] = 1
while not (mx == 3 and my == 3):
    st = lcg(st)
    canR, canD = mx < 3, my < 3
    move = (1 if st & 1 else 2) if canR and canD else (1 if canR else 2)
    if move == 1: mx += 1
    else: my += 1
    maze[my*4+mx] = 1

q = deque([(0, 0, '')])
visited = {(0,0)}
pw3 = None
while q:
    cx, cy, path = q.popleft()
    if cx == 3 and cy == 3: pw3 = path; break
    for d, dx, dy in [('R',1,0),('L',-1,0),('D',0,1),('U',0,-1)]:
        nx, ny = cx+dx, cy+dy
        if 0<=nx<4 and 0<=ny<4 and (nx,ny) not in visited and maze[ny*4+nx]:
            visited.add((nx,ny)); q.append((nx, ny, path + d))

print(f"Password3: {pw3}")
print(f"Flag: ISCC{{{pw1}{pw2}{pw3}}}")
```

**运行结果：**

```
Password1: DAKJULSU  Hash: 0xDA4C679B
Password2: 033984530  x=[0, 33, 98, 45, 30]
Password3: RDRDRD
Flag: ISCC{DAKJULSU033984530RDRDRD}
```

## Distilled Knowledge

| 知识点 | 关键 |
|--------|------|
| FNV1a Hash | 非密码学哈希，用于快速校验 |
| LCG | 确定性伪随机，可重现 |
| 魔方加密 | PKCS#7填充+加法混淆+位置置换 |
| BFS寻路 | 经典图搜索，唯一路径 |
| 源码注释 | 永远不要在注释中放答案 |

---

*Original writeup: `C:\Users\48714\Desktop\wp\区域赛\拆弹专家_WriteUp_final.md`*
