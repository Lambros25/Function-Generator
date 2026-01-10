@echo off
setlocal

REM Ensure we run from the repo root (where this .bat lives)
pushd "%~dp0"


  set MSG=Added engineering log(s).

git add logs/

git diff --cached --quiet
if %ERRORLEVEL%==0 (
  echo Nothing to commit in logs/.
) else (
  git commit -m "%MSG%"
  git push
)

popd
echo.
echo Done.
pause
