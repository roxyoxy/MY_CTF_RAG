@echo off
rem run_web.bat -- start the web UI and open it in the default browser.
rem
rem The server must run with this directory as its working directory:
rem data/, index.bin, vector.bin and web/ all resolve from here.
setlocal
cd /d "%~dp0"

if not exist web_ui.exe (
  echo [FAIL] web_ui.exe not found -- run build_web.bat first.
  pause
  exit /b 1
)

start "TraceRAG web UI" /min web_ui.exe
timeout /t 3 /nobreak >nul
start "" http://127.0.0.1:18080

echo.
echo   TraceRAG web UI  -^>  http://127.0.0.1:18080
echo.
echo   The server window is minimized; close it to stop the service.
echo   If the page shows "engine not connected", the port is probably
echo   held by an older web_ui.exe -- end it in Task Manager and rerun.
echo.
pause
