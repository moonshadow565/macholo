#!/bin/sh
# Regression gate (docs/REFACTOR.md): run a League replay headless until it reaches
# GAMESTATE_GAMELOOP, then kill it — reaching the game loop is the pass condition,
# playing the replay out is not needed.
#
#   tools/gate.sh <version> <logfile>     e.g. tools/gate.sh 16.3 /tmp/gate.log
#
# Requires Xvfb on :9. cwd handling matters: the game resolves DATA/FINAL/... and
# writes ../SOFT_REPAIR relative to cwd, so we cd into Contents/MacOS.
set -eu
V=${1:?usage: gate.sh <version> <logfile>}
LOG=${2:?usage: gate.sh <version> <logfile>}
REPO=$(cd "$(dirname "$0")/.." && pwd)

cd ~/Game/"$V"/LeagueofLegends.app/Contents/MacOS
rm -f ../SOFT_REPAIR
R=$(ls ~/Game/"$V"-*.rofl | head -1)

DISPLAY=:9 "$REPO"/build/macholo ./LeagueofLegends "$R" > "$LOG" 2>&1 &
PID=$!
i=0
while [ $i -lt 120 ]; do
    if grep -q GAMESTATE_GAMELOOP "$LOG"; then
        kill "$PID" 2>/dev/null || true
        echo "gate $V: PASS (GAMELOOP after ~$((i * 5))s)"
        exit 0
    fi
    kill -0 "$PID" 2>/dev/null || { echo "gate $V: FAIL (exited early)"; tail -15 "$LOG"; exit 1; }
    sleep 5
    i=$((i + 1))
done
kill "$PID" 2>/dev/null || true
echo "gate $V: FAIL (no GAMELOOP within 600s)"
tail -15 "$LOG"
exit 1
