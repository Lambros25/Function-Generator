@echo off
setlocal

REM Run from repo root
powershell -NoProfile -ExecutionPolicy Bypass ^
  -File "%~dp0scripts\push_logs.ps1" ^
  -Message "Update engineering logs"

echo.
echo Done. If you saw "Nothing to commit.", there were no changes.
pause
