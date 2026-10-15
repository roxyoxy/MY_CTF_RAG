# wp_pool 语料剔除记录（2026-10-16）

为通过 GitHub Push Protection（密钥扫描），从语料库剔除 **32 个**含"凭据样式字符串"的文件。
这些凭据全部是 CTF 题目/上游文档自带的**假凭据**（Grafana 8.2.6 文档集演示 token、
题目 README 里的本地开发用 AWS key / 私钥），无一是真实泄露，但扫描器按模式匹配一律拦截。

## 剔除清单

| 组 | 文件 | 命中模式 |
|---|---|---|
| starctf2022 grafana docs | `web-oh-my-grafana_docker_..._docs_so_182..208.md`（25 个，缺 188/194） | Grafana token `eyJrIjoi…` |
| starctf2022 grafana docs | `..._docs_so_95.md`、`..._contrib_15.md` | PRIVATE KEY / `eyJrIjoi` |
| starctf2022 | `wp_pool/web/starctf-2022_ha_test.md` | token 类 |
| business-ctf-2025 | `cloud_PipeDream_README.md` | AWS `AKIA…` + PRIVATE KEY |
| university-ctf-2024 | `fullpwn_Easy_Clouded_Clouded.md` | AWS `AKIA…` |
| sekaictf-2023 | `forensics_infected_solution_README.md` | PRIVATE KEY |
| sekaictf-2024 | `reverse_crack-me_solution_README.md` | token 类 |
| Challenges_2021_Public | `cloud_lost-n-found_solution.md` | PRIVATE KEY |

## 影响

- 语料总数 **3143 → 3111**。§5 规模梯度实验报告中的 3143 为实验时点快照，数字保留不改。
- 引入这些文件的 5 个提交做了历史重写（`ca8ab83/e5b4141/2687059` → 新哈希），旧哈希从未推送，队友无需处理。
- 原始文件仍保留在本地 `D:/ctf-rag-injection/raw-repos`（收割源），如需可做脱敏后重新入库。
- 后续收割新语料前，建议先跑一遍密钥模式扫描（`AKIA[0-9A-Z]{16}` / `eyJrIjoi` / `-----BEGIN … PRIVATE KEY-----` 等）再提交。
