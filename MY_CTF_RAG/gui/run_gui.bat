@echo off
rem run_gui.bat -- start the GUI with the code directory (MY_CTF_RAG\)
rem as the working directory, so ./data and index.bin resolve next to
rem it and the GUI shares the corpus and snapshot with the console app.
rem Build first with gui\build_gui.bat if gui\build\ is missing.

cd /d "%~dp0.."
if not exist "gui\build\MY_CTF_RAG_GUI.exe" (
    echo GUI not built yet. Run gui\build_gui.bat first.
    pause
    exit /b 1
)
start "" "gui\build\MY_CTF_RAG_GUI.exe"
