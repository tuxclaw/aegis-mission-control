#!/usr/bin/env bash
# Generate agents.json for AEGIS from OpenClaw CLI
set -euo pipefail
OUT="${1:-$HOME/.openclaw/agents.json}"
openclaw agents list --json 2>/dev/null | python3 -c "
import json, sys
agents = json.load(sys.stdin)
out = [{'name': a.get('id','unknown'), 'model': a.get('model','n/a'), 'state': 'active' if a.get('isDefault') else 'idle', 'detail': a.get('identityName','')} for a in agents]
json.dump(out, sys.stdout, indent=2)
" > "$OUT"
