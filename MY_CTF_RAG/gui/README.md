# gui/ -- Qt GUI sources (built by MY_CTF_RAG_GUI project)

This directory only holds the three Qt source files. The Visual Studio
project that builds them is `..\MY_CTF_RAG_GUI\MY_CTF_RAG_GUI.vcxproj`
(Qt VS Tools format), registered in `MY_CTF_RAG.sln` as a second
project next to the console app.

## Files

| File | Role |
|---|---|
| `qt_main.cpp` | GUI entry point (QApplication + MainWindow) |
| `mainwindow.h/.cpp` | All UI built in code: corpus tree, search box, results table, log pane, soft-delete; no .ui, no Q_OBJECT, no moc/uic/rcc needed |

The GUI is a pure consumer of the eight contracts in `..\include\`:
it links the same seven core `.cpp` files from `..\src\` as the console
app (loader / chunker / tokenizer / atom_scan / indexer / persist /
corpus_diff). The console `src\main.cpp` is NOT part of this project.

## Build and run (Visual Studio F5)

1. Prerequisite (once per machine): Visual Studio extension
   **Qt Visual Studio Tools** (3.5.x), and a Qt version registered in
   `Extensions > Qt VS Tools > Qt Versions` under the name **QT6**
   (default version). Any Qt 6.x msvc2022_64 kit works; the reference
   machine uses Qt 6.9.3.
2. Open `MY_CTF_RAG.sln`.
3. Right-click `MY_CTF_RAG_GUI` > **Set as Startup Project** (once;
   remembered per user).
4. **F5**. The working directory is wired to `MY_CTF_RAG\` inside the
   project file, so `data\` and `index.bin` are shared with the
   console app. Qt DLLs are deployed automatically after each link
   (`<QtDeploy>true</QtDeploy>` runs windeployqt, debug or release
   flavor as appropriate).

Both Debug and Release|x64 are supported; the kit ships debug DLLs.
x86 is intentionally not mapped in the solution (kit is x64-only).

## Project notes

- `MY_CTF_RAG_GUI\QtMsBuild\` is generated/maintained by the
  extension on project load; it is gitignored, never edited by hand.
- Command-line build (no VS IDE needed):

  ```
  MSBuild MY_CTF_RAG_GUI\MY_CTF_RAG_GUI.vcxproj /p:Configuration=Release /p:Platform=x64
  ```

  (requires the QtMsBuild folder to exist, i.e. the project has been
  opened in VS at least once on this machine)
- Compiler discipline matches the repo: /W4, /permissive-, /utf-8,
  C++17, English ASCII comments in delivered code; Chinese only as
  UI string literals.
- History: this GUI was first assembled (2026-09-29) with a CMake
  bundle (external delivery + AI-A patches); the same sources moved
  to the native Qt VS Tools project the same day. See
  docs/MEETING_LOG.md 03:55 and the VS-integration entry.
