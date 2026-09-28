# borrowstack

> **Category**: PWN | **Difficulty**: Easy | **Competition**: ISCC 2026 区域赛(第二场)
> **Flag**: `ISCC{ce735452-9d52-4bae-a142-0c432d8ea5e6}`

---

## Attack Chain

```
栈溢出(No Canary/No PIE) → ret2libc泄露puts → 回到vul → 写"sh"到bss → system("sh")
```

## Key Techniques

### 1. 两段式 Ret2Libc

**阶段一**: 构造ROP链 `puts@plt(puts@got) → vul`，泄露libc基址。

**阶段二**: `read(0, bss, 0x10) → system(bss)`，手动写入 `"sh\x00"` 到 .bss 段。

### 2. 填充长度计算

```c
void vul() {
    char buf[0x50];        // 80字节
    read(0, buf, 0x100);   // 读256字节→溢出
}
```

IDA发现 `push ebx` 额外保存4字节：pad = 0x50 + 4 = 0x54

### 3. 远程命令白名单

远程环境仅接受 `system("sh")` 或 `system("/bin/sh")`，libc内嵌字符串不稳定，需手动写入。

## Distilled Knowledge

| 知识点 | 关键 |
|--------|------|
| ret2libc | 泄露GOT → 计算system地址 |
| .bss写入 | 可控可写段，存命令字符串 |
| 命令白名单 | 远程可能过滤非标准参数 |
| 填充计算 | 注意 push ebx/padding |

---

*Original writeup: `C:\Users\48714\Desktop\wp\区域赛-2\ISCC2026_WP_PWN_borrowstack.md`*
