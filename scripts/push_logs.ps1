\
param(
  [string]$Message = "Update engineering logs"
)

# Run from repo root:
#   powershell -ExecutionPolicy Bypass -File scripts\push_logs.ps1
# Optional message:
#   powershell -ExecutionPolicy Bypass -File scripts\push_logs.ps1 -Message "Add log"

git add logs/
git status --porcelain

# Commit only if there are staged changes
git diff --cached --quiet
if ($LASTEXITCODE -eq 0) {
  Write-Host "Nothing to commit."
} else {
  git commit -m "$Message"
}

git push
