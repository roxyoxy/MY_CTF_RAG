# -*- coding: utf-8 -*-
"""钉死 pwn 卡基数：多口径统计 D 盘 staging 卡数，对齐 104/113/124 分歧。

用法：py -X utf8 scripts/exp2_count_pwn.py
只读 D 盘 staging + Desktop assets，不写任何卡。
"""
import json
from pathlib import Path
from collections import Counter

STAGING = Path(r"D:\ctf-rag-injection\staging")
ASSETS = Path(r"C:\Users\48714\Desktop\MY_RAG\MY_CTF_RAG\assets")


def cards_from(jsons):
    cards = []
    for jf in jsons:
        try:
            d = json.loads(jf.read_text(encoding="utf-8"))
        except Exception:
            continue
        c = d.get("card", {})
        if c:
            cards.append(c)
    return cards


def report(tag, cards):
    cnt = Counter(c.get("category", "?") for c in cards)
    print(f"[{tag}] 总 {len(cards)} 卡")
    for cat, n in sorted(cnt.items(), key=lambda x: -x[1]):
        print(f"    {cat:12s} {n}")
    return cnt


print("== D 盘 staging 结构探查 ==")
depth_counter = Counter()
for jf in STAGING.rglob("*.json"):
    rel = jf.relative_to(STAGING)
    depth_counter[len(rel.parts)] += 1
for d in sorted(depth_counter):
    print(f"  深度 {d}: {depth_counter[d]} 个 json")

print("\n== 口径1：三层 glob */*/*.json（资产导出同款） ==")
report("三层", cards_from(STAGING.glob("*/*/*.json")))

print("\n== 口径2：任意深度 rglob ==")
report("全量", cards_from(STAGING.rglob("*.json")))

print("\n== 口径3：Desktop assets 已导出 ==")
try:
    ap = json.loads((ASSETS / "pwn_cards.json").read_text(encoding="utf-8"))
    print(f"  assets/pwn_cards.json: pwn {len(ap)} 张")
except Exception as e:
    print(f"  assets 读取失败: {e}")
