@echo off
rem run_tests.bat -- build and run every contract test in tests\.
rem Usage: tests\run_tests.bat   (from anywhere; MSVC via VS2022 vcvars)
rem Exit code 0 = all binaries built and all cases green.
rem cl prints errors to stdout, so each compile logs to a file that is
rem only shown when the build fails.

rem Locate Visual Studio the way ci.yml and build_web.bat do. This line
rem used to name a single install path, and because it also sent its output
rem to nul, a machine whose VS lives anywhere else failed silently and then
rem reported only "'cl' is not recognized" -- an error about the wrong
rem thing entirely. vswhere ships with the VS installer and knows where VS
rem actually is; the usual locations are kept as a fallback.
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
  ) do if not defined VCVARS if exist %%P set "VCVARS=%%~P"
)

if not defined VCVARS (
  echo [FAIL] vcvars64.bat not found.
  echo        Install the VS2022 "Desktop development with C++" workload.
  exit /b 1
)

echo [info] Visual Studio found at %VCVARS%
call "%VCVARS%" >nul 2>&1
cd /d "%~dp0.."

set CLFLAGS=/nologo /std:c++17 /W4 /utf-8 /EHsc /I include /I tests /Fo:tests\

cl %CLFLAGS% tests\test_atom_scan.cpp src\atom_scan.cpp /Fe:tests\test_atom_scan.exe 1>tests\_b_ascan.txt 2>&1
if errorlevel 1 goto failascan
cl %CLFLAGS% tests\test_tokenizer.cpp src\tokenizer.cpp src\atom_scan.cpp /Fe:tests\test_tokenizer.exe 1>tests\_b_tok.txt 2>&1
if errorlevel 1 goto failtok
cl %CLFLAGS% tests\test_loader.cpp src\loader.cpp /Fe:tests\test_loader.exe 1>tests\_b_load.txt 2>&1
if errorlevel 1 goto failload
cl %CLFLAGS% tests\test_chunker.cpp src\chunker.cpp src\tokenizer.cpp src\atom_scan.cpp /Fe:tests\test_chunker.exe 1>tests\_b_chunk.txt 2>&1
if errorlevel 1 goto failchunk
cl %CLFLAGS% tests\test_indexer.cpp src\indexer.cpp src\tokenizer.cpp src\atom_scan.cpp /Fe:tests\test_indexer.exe 1>tests\_b_idx.txt 2>&1
if errorlevel 1 goto failidx
cl %CLFLAGS% tests\test_persist.cpp src\persist.cpp /Fe:tests\test_persist.exe 1>tests\_b_per.txt 2>&1
if errorlevel 1 goto failper
cl %CLFLAGS% tests\test_corpus_diff.cpp src\corpus_diff.cpp /Fe:tests\test_corpus_diff.exe 1>tests\_b_cdiff.txt 2>&1
if errorlevel 1 goto failcdiff
cl %CLFLAGS% tests\test_embedder.cpp src\embedder.cpp /Fe:tests\test_embedder.exe 1>tests\_b_emb.txt 2>&1
if errorlevel 1 goto failemb
cl %CLFLAGS% tests\test_rrf.cpp src\rrf.cpp /Fe:tests\test_rrf.exe 1>tests\_b_rrf.txt 2>&1
if errorlevel 1 goto failrrf
cl %CLFLAGS% tests\test_vector_index.cpp src\vector_index.cpp /Fe:tests\test_vector_index.exe 1>tests\_b_vec.txt 2>&1
if errorlevel 1 goto failvec
cl %CLFLAGS% tests\test_vector_persist.cpp src\vector_persist.cpp /Fe:tests\test_vector_persist.exe 1>tests\_b_vper.txt 2>&1
if errorlevel 1 goto failvper
cl %CLFLAGS% tests\test_generator.cpp src\generator.cpp /Fe:tests\test_generator.exe 1>tests\_b_gen.txt 2>&1
if errorlevel 1 goto failgen
cl %CLFLAGS% tests\test_context.cpp src\context.cpp /Fe:tests\test_context.exe 1>tests\_b_ctx.txt 2>&1
if errorlevel 1 goto failctx
cl %CLFLAGS% tests\test_hnsw.cpp src\hnsw.cpp src\vector_index.cpp /Fe:tests\test_hnsw.exe 1>tests\_b_hnsw.txt 2>&1
if errorlevel 1 goto failhnsw
cl %CLFLAGS% tests\test_llama_cpp.cpp src\embedder.cpp src\generator.cpp /Fe:tests\test_llama_cpp.exe 1>tests\_b_lcpp.txt 2>&1
if errorlevel 1 goto faillcpp

tests\test_atom_scan.exe   || exit /b 1
tests\test_tokenizer.exe || exit /b 1
tests\test_loader.exe     || exit /b 1
tests\test_chunker.exe    || exit /b 1
tests\test_indexer.exe    || exit /b 1
tests\test_persist.exe     || exit /b 1
tests\test_corpus_diff.exe || exit /b 1
tests\test_embedder.exe || exit /b 1
tests\test_rrf.exe || exit /b 1
tests\test_vector_index.exe || exit /b 1
tests\test_vector_persist.exe || exit /b 1
tests\test_generator.exe || exit /b 1
tests\test_context.exe || exit /b 1
tests\test_hnsw.exe || exit /b 1
tests\test_llama_cpp.exe || exit /b 1
echo ALL_TESTS_GREEN
del tests\_b_*.txt 1>nul 2>nul
exit /b 0

:failascan
type tests\_b_ascan.txt
exit /b 1
:failtok
type tests\_b_tok.txt
exit /b 1
:failload
type tests\_b_load.txt
exit /b 1
:failchunk
type tests\_b_chunk.txt
exit /b 1
:failidx
type tests\_b_idx.txt
exit /b 1
:failper
type tests\_b_per.txt
exit /b 1
:failcdiff
type tests\_b_cdiff.txt
exit /b 1
:failemb
type tests\_b_emb.txt
exit /b 1
:failrrf
type tests\_b_rrf.txt
exit /b 1
:failvec
type tests\_b_vec.txt
exit /b 1
:failvper
type tests\_b_vper.txt
exit /b 1
:failgen
type tests\_b_gen.txt
exit /b 1
:failctx
type tests\_b_ctx.txt
exit /b 1
:failhnsw
type tests\_b_hnsw.txt
exit /b 1
:faillcpp
type tests\_b_lcpp.txt
exit /b 1
