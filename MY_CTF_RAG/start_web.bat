@echo off
rem start_web.bat -- one click: start the web UI server, wait for the
rem port to answer, open the default browser at the front end.
rem
rem The server keeps running in THIS window on purpose (same policy as
rem run_web.bat): this window owns the process, so Ctrl+C stops it
rem cleanly. Closing the window also stops it.
setlocal
cd /d "%~dp0"

set "URL=http://127.0.0.1:18080"

rem A second instance would fail the exclusive bind and die next to a
rem working one, which reads like a crash. If the port already answers,
rem just open the browser against the running server.
netstat -ano | findstr /c:":18080 " | findstr /c:"LISTENING" >nul
if not errorlevel 1 (
  echo [info] web UI already running -- opening %URL%
  start "" "%URL%"
  exit /b 0
)

if not exist "%~dp0web_ui.exe" (
  echo [FAIL] web_ui.exe not found -- run build_web.bat first.
  pause
  exit /b 1
)

echo [info] starting web_ui.exe -- this window is the server console.
echo [info] the browser opens by itself once the server is listening.

rem The server binds its port before the engine finishes coming up, so
rem "port answers" already means "page loadable". The probe runs in the
rem background because web_ui.exe below never returns until stopped.
start "" /min cmd /c powershell -NoProfile -Command "for($i=0;$i -lt 120;$i++){try{$c=New-Object Net.Sockets.TcpClient;$c.Connect('127.0.0.1',18080);$c.Close();Start-Process('http://127.0.0.1:18080');exit 0}catch{Start-Sleep -m 500}};Write-Host '[FAIL] the server never came up -- read its window.'"

"%~dp0web_ui.exe"
