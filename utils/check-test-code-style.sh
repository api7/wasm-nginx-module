#!/usr/bin/env bash


set -x -euo pipefail

find t -name '*.t' -exec grep -E "\-\-\-\s+(SKIP|ONLY|LAST|FIRST)$" {} + > /tmp/error.log || true
if [ -s /tmp/error.log ]; then
    echo "Forbidden directives to found. Bypass test cases without reason are not allowed."
    cat /tmp/error.log
    exit 1
fi

find t -name '*.t' -exec ./utils/reindex {} + > \
    /tmp/check.log 2>&1 || (cat /tmp/check.log && exit 1)

grep "done." /tmp/check.log > /tmp/error.log || true
if [ -s /tmp/error.log ]; then
    echo "=====bad style====="
    cat /tmp/error.log
    echo "you need to run 'reindex' to fix them. Read CONTRIBUTING.md for more details."
    exit 1
fi
