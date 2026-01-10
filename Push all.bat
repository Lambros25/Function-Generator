@echo off
setlocal

set /p MSG=Enter commit message: 

if "%MSG%"=="" (
  echo Commit message cannot be empty.
  pause
  exit /b 1
)

git add -A

git diff --cached --quiet
if %ERRORLEVEL%==0 (
  echo Nothing to commit.
) else (
  git commit -m "%MSG%"
  git push
)

echo.
echo Done.
pause
