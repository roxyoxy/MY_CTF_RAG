# third_party — vendored single-header libraries

全仓唯一第三方代码区（M3 vendor 甲案裁定，2026-10-07）。
glue 不是研究对象：HTTP 与 JSON 只做接入，不做修改。

| 文件 | 版本 | 来源 | 许可证 |
|---|---|---|---|
| httplib.h | v0.18.3（fallback master） | github.com/yhirose/cpp-httplib | MIT（LICENSE-cpp-httplib） |
| nlohmann/json.hpp | v3.11.3 | github.com/nlohmann/json | MIT（nlohmann/LICENSE-json） |

规矩：

- **只许 include，不许改内容**——升级 = 整文件替换 + 本表版本同步
- 消费者唯一：`src/embedder.cpp`（分层红线，embedder.h 条款 7）
- include 写法（相对 src/ 的引号路径，零工程配置）：
  `#include "../third_party/httplib.h"`
  `#include "../third_party/nlohmann/json.hpp"`
- 三方头过不了 /W4：消费点用 `#pragma warning(push/pop)` 隔离
