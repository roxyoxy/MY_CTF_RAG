@echo off
rem build_scale_eval.bat -- build the M5 scale-ladder eval driver
rem (scripts\scale_eval.exe). Same layout as build_web.bat: VS located
rem with vswhere, objects to %TEMP%, /W4 clean build.
setlocal
cd /d "%~dp0"

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "VSROOT="
if exist "%VSWHERE%" (
  for /f "usebackq tokens=*" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSROOT=%%I"
)

set "VCVARS="
if defined VSROOT if exist "%VSROOT%\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=%VSROOT%\VC\Auxiliary\Build\vcvars64.bat"

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
  pause
  exit /b 1
)

call "%VCVARS%" >nul 2>&1
if not exist "%TEMP%\tracerag_scale_obj" mkdir "%TEMP%\tracerag_scale_obj"

rem The doubled backslash before the closing quote is load-bearing
rem (same D8003 story as build_web.bat).
cl /nologo /std:c++17 /W4 /utf-8 /EHsc /I include /Fo:"%TEMP%\tracerag_scale_obj\\" ^
  scripts\scale_eval.cpp ^
  src\atom_scan.cpp src\chunker.cpp src\embedder.cpp src\indexer.cpp ^
  src\loader.cpp src\hnsw.cpp src\rrf.cpp src\tokenizer.cpp ^
  src\vector_index.cpp src\vector_persist.cpp ^
  /Fe:scripts\scale_eval.exe

if errorlevel 1 (
  echo.
  echo [FAIL] build failed -- read the error above.
) else (
  echo.
  echo [ OK ] scripts\scale_eval.exe built.
)
pause
