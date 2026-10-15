@echo off
rem run_web.bat -- run the web UI server in this window.
rem
rem The server must run with this directory as its working directory:
rem data/, index.bin, vector.bin and web/ all resolve from here.
setlocal
cd /d "%~dp0"

rem The executable is named by full path rather than as a bare "web_ui.exe"
rem on purpose. cmd resolves a bare name by searching the current
rem directory, but that search is switched off wherever
rem NoDefaultCurrentDirectoryInExePath is set (Git Bash and mintty set it,
rem among others), and the failure looks like a missing build.
if not exist "%~dp0web_ui.exe" (
  echo [FAIL] web_ui.exe not found -- run build_web.bat first.
  pause
  exit /b 1
)

rem The server runs in the foreground on purpose. It used to be launched
rem detached with "start /min", which gave it no window of its own: this
rem terminal could be closed while the server kept running, and the only
rem way to stop it afterwards was Task Manager. In the foreground this
rem window owns the process, so Ctrl+C stops it -- cleanly, because
rem web_ui.exe installs a console control handler that closes its
rem listening socket and returns from listen() instead of being killed.
rem The banner it prints (the URL, and how to stop it) goes to this
rem window.
"%~dp0web_ui.exe"
