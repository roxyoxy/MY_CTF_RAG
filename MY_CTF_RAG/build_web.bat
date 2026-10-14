@echo off
rem build_web.bat -- build the web UI server (web_ui.exe).
rem
rem Paths resolve from this script's own directory (%~dp0), so the file
rem works on any machine -- the earlier handoff copy hardcoded one
rem developer's drive and could not run anywhere else.
rem
rem The server needs no Qt and no special build steps, so it stays out
rem of the solution instead of becoming a third VS project.
setlocal
cd /d "%~dp0"

set "VCVARS="
for %%P in (
  "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
  "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
  "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
  "C:\VS\2022\VC\Auxiliary\Build\vcvars64.bat"
) do if not defined VCVARS if exist %%P set "VCVARS=%%~P"

if not defined VCVARS (
  echo [FAIL] vcvars64.bat not found.
  echo        Open this script and add your Visual Studio path to the list.
  pause
  exit /b 1
)

call "%VCVARS%" >nul 2>&1
if not exist "%TEMP%\traceRAG_web_obj" mkdir "%TEMP%\traceRAG_web_obj"

cl /nologo /std:c++17 /W4 /utf-8 /EHsc /I include /Fo:"%TEMP%\traceRAG_web_obj\" ^
  src\web_ui.cpp src\engine.cpp src\atom_scan.cpp src\chunker.cpp ^
  src\context.cpp src\corpus_diff.cpp src\embedder.cpp src\generator.cpp ^
  src\indexer.cpp src\loader.cpp src\persist.cpp src\rrf.cpp ^
  src\tokenizer.cpp src\vector_index.cpp src\vector_persist.cpp ^
  /Fe:web_ui.exe

if errorlevel 1 (
  echo.
  echo [FAIL] build failed -- read the error above.
) else (
  echo.
  echo [ OK ] web_ui.exe rebuilt.  Run it with run_web.bat
)
pause
