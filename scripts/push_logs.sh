#!/usr/bin/env bash
set -e

# Run from repo root:
#   bash scripts/push_logs.sh
# Optional: pass a commit message:
#   bash scripts/push_logs.sh "Add engineering log"

MSG="${1:-Update engineering logs}"

git add logs/
git status --porcelain

# Commit only if there are staged changes
if git diff --cached --quiet; then
  echo "Nothing to commit."
else
  git commit -m "$MSG"
fi

git push
