# MY_CTF_RAG GUI（Qt 6 管理界面）

> M2 第五件（收尾件）：核心检索层的**纯消费者**——八份契约一个不改，
> 换一个 Qt 6 Widgets 入口。外部 AI 施工（施工书 Desktop\
> 给GPT的外包QTprompt.md），回流后 AI-A review + 拼装修订
> （2026-09-29，修订清单见文末）。

## 功能

- 语料树（按类目分组 / 字节数 / 墓碑删除线 / 显示已删除开关）
- BM25 查询 + 结果表（rank / score / 路径 / chunk id / 摘要）
- 双击检索结果 → 按 UTF-8 字节偏移定位整个 chunk 并选中
  （先按字节切片再换算 Qt 光标位置，不把字节数当字符下标）
- 软删除 = 墓碑 + 立即重切 + 重建 + save（与控制台 main 语义一致）
- 启动三分支：validate → 快照命中恢复 / 未命中重建 + save；
  **语料变更（corpus changed）时收获旧快照：diff 报告 + 按 path
  继承墓碑**（与控制台 M2-③ 语义一致）
- 状态栏：docs / chunks / 快照状态 / format+pipeline 版本 / avgdl

## 构建

前置：Qt 6（本机 6.5.3 msvc2022_64）+ VS2022（含其自带 CMake/Ninja）。

路线 A（推荐，Qt Creator）：打开本目录 `CMakeLists.txt` →
Creator 自动检测 6.5.3 msvc2022_64 kit → 配置 → 运行。

路线 B（命令行，一键脚本 `gui\build_gui.bat`）：

```
gui\build_gui.bat
```

等价展开（Qt 实际安装位置见下"环境备注"）：

```
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
cmake -S gui -B gui/build -G Ninja ^
  -DCMAKE_PREFIX_PATH=C:/Qt/6.5.3/msvc2022_64/bin/qmake.exe/6.9.3/msvc2022_64 ^
  -DCMAKE_MAKE_PROGRAM="C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe"
cmake --build gui/build
```

环境备注：本机 Qt 线上安装器装歪了——真实 Qt 6.9.3 套件位于
`C:/Qt/6.5.3/msvc2022_64/bin/qmake.exe/6.9.3/msvc2022_64/`
（"qmake.exe" 是个目录，6.9.3 整套装在里面；`C:/Qt/6.5.3`
本身只有空壳）。C:/D: 盘另有 Qt 5.9.5/5.15.2 旧件，本工程用不上
（交付件要求 Qt 6）。日后可用 Qt Maintenance Tool 规整安装位置，
届时只需同步改本脚本与 README 两处路径。

MSVC 侧 /W4 /permissive- /utf-8 /UNICODE；核心六件 .cpp + corpus_diff.cpp
与本 GUI 一起编译，控制台 main.cpp 不编入。

## 运行注意

- **工作目录**：`./data` 与 `index.bin` 都相对 CWD。从仓库代码区
  （`MY_CTF_RAG/`）启动 = 与控制台共用同一份语料和快照；从
  `gui/build/` 启动 = 各用各的（validate 会各自兜底，不会错乱）。
- **编辑警告**：`mainwindow.cpp` 含中文 UI 字面量（UTF-8 无 BOM）。
  请用 Qt Creator 编辑；VS 直接改有按 GBK 回存的事故前科
  （09-22 type.h 教训），若必须用 VS，保存前确认编码未变。

## 回流修订清单（2026-09-29，AI-A 拼装时）

1. CMakeLists：MSVC 分支补 `/utf-8`（中文 UI 字面量的编译器读取面）
2. CMakeLists：补链 `../src/corpus_diff.cpp`（第八契约，交付时外部
   尚不知道 M2-③ 竣工）
3. mainwindow.cpp `loadCorpusAndIndex`：CORPUS_CHANGED 分支补墓碑
   收获（diff 报告 + inherit），对齐控制台语义——否则 GUI 侧语料
   变更重建会丢墓碑

其余交付源码逐行 review 通过，未改动。
