# [校赛 密码学] 炼丹 — Writeup

> **题型**：CRYPTO / 古典密码套娃（Morse + ZipCrypto + 同音汉字混淆 + Base64 + Caesar）
> **附件**：`key.png`（2560×1440 截图）+ `Pandora's_box.zip`（加密，内含 `不可名状之物.txt` 60 字节）
> **题干**：密码学炼金术士 / 400 pts —— "千年老山参，野生蜂王浆……全部融于一炉。caesar，base64，混淆，盲文，我的古典密码完成了。"
> **梗底**：炼丹 = 把一堆"药材"（密码）炼成一炉（套娃）；潘多拉魔盒 = 打开后飞出的"不可名状之物"（克苏鲁味），希望留在盒底的是 flag。

## Flag

```
npusec{base64&decrypt_is_good!}
```

---

## 一、观察附件

- `Pandora's_box.zip`：**加密 zip**（标志位 bit0，ZipCrypto），里面只有一个 60 字节的 `不可名状之物.txt` —— 没钥匙打不开
- `key.png`：一张记事本截图，正文是一串 `.` 和 `-`：

![图1：key.png 记事本中的摩斯码](图片/1_摩斯码截图.png)
<!-- 截图位①：key.png 原图（记事本摩斯码窗口） -->

```
-- --- .-. ... . .. ... .-- --- -. -.. . .-. ..-. ..- .-..
```

## 二、Morse 开盒（密码：morseiswonderful）

逐词查表：

```
-- --- .-. ... .  .. ...  .-- --- -. -.. . .-. ..-. ..- .-..
M  O    R    S   E   I  S    W   O   N   D  E   R  F   U  L
```

= `MORSEISWONDERFUL`（摩斯真美妙）。

大写直接当密码失败，**小写 `morseiswonderful` 成功解压**（教训：古典密码题密码大小写都要试）。

## 三、不可名状之物 —— 带"毒"的 base64

解出的文本（44 字符）：

![图2：解压成功与不可名状之物.txt 内容](图片/2_开盒与密文.png)
<!-- 截图位②：zip 输入密码解压成功 + txt 原文（注意其中的生僻汉字） -->

```
@GpvbXl叄e叄Z懿bXk迩NCZ柶eXdsc迩puX迩NtX迩Fp@XghfQ==
```

特征：
- 尾部 `==` 是 base64 填充，长度 44 = 11×4，结构合法
- 但混入 **8 个生僻汉字**（叄×2 懿×1 迩×4 柶×1）和 2 个 `@`——都不是 base64 字符 → 这就是题干说的"混淆"层

### 同音汉字 → 数字

四个字的读音暗藏数字（汉语谐音）：

| 字 | 读音 | 谐音 | 替换 |
|---|---|---|---|
| 懿 | yì | 一 | `1` |
| 迩 | ěr | 二 | `2` |
| 叄 | sān | 三 | `3` |
| 柶 | sì | 四 | `4` |

`@` → `a`（字形混淆，键盘 shift 关系：@ 与 a 同键）。

替换后得到纯 base64：

```
aGpvbXl3e3Z1bXk2NCZ4eXdsc2puX2NtX2FpaXghfQ==
```

## 四、Base64 + Caesar 双层收尾

替换后直接 b64 解码：

```
hjomyw{vumy64&xywlsjn_cm_aiix!}
```

结构像 flag 但字母是乱的——最后的 Caesar。逐字母试移位，**ROT-6** 命中：

```
h jomyw { vumy64 & xywlsjn _ cm _ aii x ! }
  ↓ +6
npusec{base64&decrypt_is_good!}
```

（校验点：`vumy64`+6=`base64`、`xywlsjn`+6=`decrypt`，语义自证。）

![图3：解题脚本输出（替换→b64→ROT6 全过程）](图片/3_解题脚本输出.png)
<!-- 截图位③：Python 端到端验证输出，末行 VERIFIED + flag -->

## 五、完整攻击链

```
key.png 摩斯 → MORSEISWONDERFUL → 小写作 zip 密码
  → 不可名状之物.txt
  → 混淆层：懿→1 迩→2 叄→3 柶→4，@→a
  → base64 解码
  → Caesar ROT-6
  → npusec{base64&decrypt_is_good!}
```

题干的"药材单"（caesar、base64、混淆、盲文）就是配方表；Morse 不在单子里，是"炼丹炉的门"。盲文一层在本解法中未出现（可能出题人把 Morse 也算作盲文类点写字，或留作别路）。

## 六、工具与坑

- 解密用 Python `zipfile`（`pwd=b'morseiswonderful'`）即可，无需暴力
- 坑1：密码大小写——`MORSEISWONDERFUL` 不对，`morseiswonderful` 对
- 坑2：汉字必须按 UTF-8 读（60 字节 = 44 字符），按字节看会误判
- 坑3：`@` 的还原可以靠逆向验证：`@` 出现在两个 b64 分组首，解出字节与目标差恒为 0x68 → 反推出 `@`→`a`
