@echo off
rem build_gui.bat -- configure + build the Qt GUI with VS2022's bundled
rem CMake/Ninja against the installed Qt 6.5.3 msvc2022_64 kit.
rem Usage: gui\build_gui.bat   (from anywhere; MSVC env set up inside)
rem Exit code 0 = MY_CTF_RAG_GUI.exe produced under gui\build\.

call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" 1>nul 2>nul

set "CMAKE=C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
set "NINJA=C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"

cd /d "%~dp0.."

"%CMAKE%" -S gui -B gui/build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="C:/Qt/6.5.3/msvc2022_64/bin/qmake.exe/6.9.3/msvc2022_64" -DCMAKE_MAKE_PROGRAM="%NINJA%" || exit /b 1
"%CMAKE%" --build gui/build || exit /b 1
