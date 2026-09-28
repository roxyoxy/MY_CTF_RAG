@echo off
rem run_tests.bat -- build and run every contract test in tests\.
rem Usage: tests\run_tests.bat   (from anywhere; MSVC via VS2022 vcvars)
rem Exit code 0 = all binaries built and all cases green.
rem cl prints errors to stdout, so each compile logs to a file that is
rem only shown when the build fails.

call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" 1>nul 2>nul
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

tests\test_atom_scan.exe   || exit /b 1
tests\test_tokenizer.exe || exit /b 1
tests\test_loader.exe     || exit /b 1
tests\test_chunker.exe    || exit /b 1
tests\test_indexer.exe    || exit /b 1
tests\test_persist.exe     || exit /b 1
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
