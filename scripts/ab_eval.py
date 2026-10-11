# -*- coding: utf-8 -*-
"""M3 step-2 A/B experiment: bge-m3 vs qwen3-embedding:0.6b.

T10 tool (promoted from %TEMP% scratch 2026-10-11). Pure dense route,
same corpus chunks, Flat cosine top-10, doc-level labels matched by
path prefix (EN/CN twins both count). The qwen query side uses the
EXACT frozen prefix from src/embedder.cpp (policy qwen3-instruct-v1);
bge queries are fed raw, mirroring the product.

Verdict (2026-10-11, 23 queries x 145 chunks): qwen won every metric
-- top1 16/23 vs 15, hit@5 91% vs 78%, hit@10 96% vs 83%, MRR@10
0.777 vs 0.697, embedding 27.8s vs 40.2s. DENSE_MODEL flipped in both
frontends; the query set below is the reusable M5 evaluation asset.

Usage (needs local Ollama on :11434 with both models pulled):
  py scripts\\ab_eval.py chunks.jsonl     # made by dump_chunks.cpp
Writes ab_results.json next to the cwd and prints the comparison.
"""
import json
import sys
import time
import urllib.request

ENDPOINT = "http://localhost:11434/api/embed"
QWEN_PREFIX = ("Instruct: Given a CTF search query, retrieve relevant "
               "writeup passages that answer the query\nQuery: ")
MODELS = ["bge-m3", "qwen3-embedding:0.6b"]
SLICE = 32
TOPK = 10

QUERIES = [
    ("ret2libc with NX enabled", ["pwn/The_Path_of_Redemption"]),
    ("栈溢出覆盖返回地址", ["pwn/stack"]),
    ("金丝雀保护与跳转地址覆盖", ["web/reverse-traversal", "re/caterpillar"]),
    ("canary leak and control flow hijack", ["web/reverse-traversal", "re/caterpillar"]),
    ("摩斯电码解码", ["crypto/yaho"]),
    ("factoring RSA modulus with weak primes", ["crypto/Cosmic_Broadcast"]),
    ("钢琴琴键与音高", ["misc/piano-strings"]),
    ("乐谱里藏着最终答案", ["misc/piano-strings"]),
    ("让时间停止的能力", ["misc/ZA_WARUDO"]),
    ("猫咪不见了", ["mobile/vanished-cat"]),
    ("where is the bunny hiding", ["re/wheres-bunny"]),
    ("依次破解三道密码的拆弹模拟", ["re/bomb-expert"]),
    ("线性同余生成器复现密码", ["re/bomb-expert", "re/dual-protection"]),
    ("anti-debug bypass in PE reverse engineering", ["re/dual-protection"]),
    ("JSON 美化格式化工具", ["web/json-beautifier"]),
    ("base64 rot13 encoding chain", ["web/encoding-maze"]),
    ("钥匙消失之谜", ["web/vanished-key"]),
    ("盲相位阵列信号", ["misc/blind-phase-array"]),
    ("便签本泄露 libc 版本", ["pwn/notepad"]),
    ("灰色签名校验", ["mobile/gray-signature"]),
    ("飞行日志里隐藏的解密指令", ["misc/aurora-guard"]),
    ("Conway 生命游戏陷阱", ["re/conways-trap"]),
    ("提示注入攻击与语言模型安全", ["ai_security/Xiaozhis_Secret"]),
]


def embed(texts, model, timeout=600):
    body = json.dumps({"model": model, "input": texts}).encode("utf-8")
    req = urllib.request.Request(
        ENDPOINT, data=body, headers={"Content-Type": "application/json"})
    with urllib.request.urlopen(req, timeout=timeout) as r:
        data = json.loads(r.read().decode("utf-8"))
    embs = data["embeddings"]
    if len(embs) != len(texts):
        raise RuntimeError("count mismatch: %d != %d" % (len(embs), len(texts)))
    return embs


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def main():
    with open(sys.argv[1] if len(sys.argv) > 1 else "chunks.jsonl",
              encoding="utf-8") as f:
        chunks = [json.loads(line) for line in f if line.strip()]
    print("chunks: %d" % len(chunks))

    results = {}
    for model in MODELS:
        t0 = time.time()
        cvecs = []
        for i in range(0, len(chunks), SLICE):
            part = embed([c["text"] for c in chunks[i:i + SLICE]], model)
            cvecs.extend(part)
            print("  [%s] embedded %d/%d" %
                  (model, min(i + SLICE, len(chunks)), len(chunks)))
        qtexts = [(QWEN_PREFIX + q) if model.startswith("qwen3") else q
                  for q, _ in QUERIES]
        qvecs = embed(qtexts, model)
        embed_secs = time.time() - t0

        per_query = []
        for (q, expects), qv in zip(QUERIES, qvecs):
            scores = sorted(
                ((dot(qv, cv), cid) for cid, cv in enumerate(cvecs)),
                key=lambda t: (-t[0], t[1]))
            top = scores[:TOPK]
            rank = next((i + 1 for i, (s, cid) in enumerate(top)
                         if any(chunks[cid]["path"].startswith(p)
                                for p in expects)), 0)
            per_query.append({
                "query": q, "rank": rank,
                "top1_path": chunks[top[0][1]]["path"],
                "top1_score": round(top[0][0], 4),
            })
        results[model] = {"embed_secs": round(embed_secs, 1),
                          "per_query": per_query}
        print("[%s] done in %.1fs" % (model, embed_secs))

    with open("ab_results.json", "w", encoding="utf-8") as f:
        json.dump(results, f, ensure_ascii=False, indent=1)

    b, qn = results["bge-m3"], results["qwen3-embedding:0.6b"]

    def agg(per, k):
        hits = sum(1 for r in per if 0 < r["rank"] <= k)
        return hits, hits / len(per)

    print("\n=== per query (rank 0 = miss in top%d) ===" % TOPK)
    print("%-4s %-38s %6s %6s   %s" % ("#", "query", "bge", "qwen", "winner"))
    for i, (rb, rq) in enumerate(zip(b["per_query"], qn["per_query"])):
        rb_r, rq_r = rb["rank"], rq["rank"]

        def s(r):
            return str(r) if r else "-"
        win = ("tie" if rb_r == rq_r else
               ("bge" if (rq_r == 0 or (rb_r and rb_r < rq_r)) else "qwen"))
        print("%-4d %-38s %6s %6s   %s" % (i + 1, rb["query"][:36], s(rb_r),
                                            s(rq_r), win))
    for name, r in (("bge-m3", b), ("qwen3-embedding:0.6b", qn)):
        h10, r10 = agg(r["per_query"], 10)
        h5, r5 = agg(r["per_query"], 5)
        t1, _ = agg(r["per_query"], 1)
        mrr = sum((1.0 / x["rank"] if x["rank"] else 0.0)
                  for x in r["per_query"]) / len(r["per_query"])
        print("\n[%s] embed %.1fs | top1 %d | hit@5 %d (%.0f%%) | "
              "hit@10 %d (%.0f%%) | MRR@10 %.3f"
              % (name, r["embed_secs"], t1, h5, r5 * 100, h10, r10 * 100,
                 mrr))


if __name__ == "__main__":
    main()
