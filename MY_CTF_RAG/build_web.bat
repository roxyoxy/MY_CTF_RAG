@echo off
rem build_web.bat -- build the web UI server (web_ui.exe).
rem
rem Paths resolve from this script's own directory (%~dp0), and Visual
rem Studio is located with vswhere.exe rather than assumed to be in one
rem place, so the file works on a machine it has never seen -- the earlier
rem handoff copy hardcoded one developer's drive and could not run
rem anywhere else.
rem
rem The server needs no Qt and no special build steps, so it stays out
rem of the solution instead of becoming a third VS project.
setlocal
cd /d "%~dp0"

rem Locate Visual Studio. vswhere.exe ships with the VS installer and
rem reports where VS is actually installed, which covers a non-system
rem drive, a Build Tools edition, or a newer major version -- none of which
rem a hardcoded candidate list can. It is asked for the newest install that
rem actually carries the x64 C++ toolset.
rem
rem %ProgramFiles(x86)% is expanded into VSWHERE here, outside any
rem parenthesised block: the ")" inside that variable name would otherwise
rem close the block early.
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "VSROOT="
if exist "%VSWHERE%" (
  for /f "usebackq tokens=*" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSROOT=%%I"
)

set "VCVARS="
if defined VSROOT if exist "%VSROOT%\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=%VSROOT%\VC\Auxiliary\Build\vcvars64.bat"

rem Fall back to the usual locations, for a machine where the installer left
rem no vswhere behind, or where VS is present but has no registered C++
rem workload for vswhere to match on.
if not defined VCVARS (
  for %%P in (
    "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
    "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
    "C:\VS\2022\VC\Auxiliary\Build\vcvars64.bat"
  ) do if not defined VCVARS if exist %%P set "VCVARS=%%~P"
)

if not defined VCVARS (
  echo [FAIL] vcvars64.bat not found.
  echo        Looked it up with vswhere.exe and in the usual VS2022 places.
  echo        Open this script and add your Visual Studio path to the list.
  pause
  exit /b 1
)

echo [info] Visual Studio found at %VCVARS%

call "%VCVARS%" >nul 2>&1
if not exist "%TEMP%\traceRAG_web_obj" mkdir "%TEMP%\traceRAG_web_obj"

rem The doubled backslash below is load-bearing, do not "tidy" it to one.
rem A backslash immediately before the closing quote escapes that quote, so
rem the /Fo argument never ends and swallows every source file listed after
rem it -- cl then reports D8003 (missing source file name). Two backslashes
rem collapse to one literal backslash and leave a quote that still
rem terminates. Dropping the backslash altogether is not a fix either: cl
rem then treats the path as a file prefix and scatters the .obj files into
rem the current directory. Measured both ways on 2026-10-15.
cl /nologo /std:c++17 /W4 /utf-8 /EHsc /I include /Fo:"%TEMP%\traceRAG_web_obj\\" ^
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
