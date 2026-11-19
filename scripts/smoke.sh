#!/bin/sh
# End-to-end smoke test with a fake workshop and /bin/sleep as the engine.
# Self-contained: builds its own temp dirs, starts and stops the daemon.
set -eu

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TMP="$(mktemp -d)"
trap 'pkill -x nyaa-paperd 2>/dev/null || true; rm -rf "$TMP"' EXIT

WS="$TMP/workshop"
CFG="$TMP/config/nyaa-paper"
mkdir -p "$WS/111" "$WS/222" "$CFG"
printf '{"title":"one","type":"scene","file":"a.pkg"}' > "$WS/111/project.json"
printf '{"title":"two","type":"video","file":"b.mp4"}' > "$WS/222/project.json"
cat > "$CFG/config.json" <<JSON
{ "version": 1, "paths": { "workshop": "$WS", "engine": "/bin/sleep" } }
JSON

pkill -x nyaa-paperd 2>/dev/null || true
sleep 0.2
XDG_CONFIG_HOME="$TMP/config" XDG_STATE_HOME="$TMP/state" "$ROOT/daemon/target/debug/nyaa-paperd" \
    > "$TMP/daemon.log" 2>&1 &
sleep 1

B="busctl --user call org.nyaa.Paper /org/nyaa/Paper"
fail() { echo "FAIL: $1"; exit 1; }

count="$($B org.nyaa.Paper.Library Count | awk '{print $2}')"
[ "$count" = "2" ] || fail "Library.Count expected 2, got $count"

mkdir -p "$WS/333"
printf '{"title":"three"}' > "$WS/333/project.json"
sleep 1.2
count="$($B org.nyaa.Paper.Library Count | awk '{print $2}')"
[ "$count" = "3" ] || fail "watcher did not pick up the new folder (count $count)"

$B org.nyaa.Paper.Engine Apply ssasas eDP-1 111 1 20 0 > /dev/null
running="$($B org.nyaa.Paper.Engine ListRunning)"
echo "$running" | grep -q eDP-1 || fail "engine not running after Apply"

J='{"schema":1,"mode":"sequential","intervalSec":60,"loop":true,"currentIndex":0,"entries":[{"wallpaperId":"a","argv":["20"],"env":[],"durationSec":1},{"wallpaperId":"b","argv":["20"],"env":[],"durationSec":1}]}'
$B org.nyaa.Paper.Playlist Set ss "eDP-1" "$J" > /dev/null
$B org.nyaa.Paper.Playlist Play s "eDP-1" > /dev/null
sleep 1.2
state="$($B org.nyaa.Paper.Playlist State s eDP-1)"
echo "$state" | grep -q playing || fail "playlist did not start ($state)"
$B org.nyaa.Paper.Playlist Stop s "eDP-1" > /dev/null
state="$($B org.nyaa.Paper.Playlist State s eDP-1)"
echo "$state" | grep -q idle || fail "playlist did not stop ($state)"

# the client renders a frame against this daemon (offscreen: layout only)
if [ -x "$ROOT/gui/build/nyaa-paper-qt" ]; then
    XDG_CONFIG_HOME="$TMP/config" QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software \
        NYAA_SHOT="$TMP/frame.png" NYAA_SHOT_DELAY=1200 \
        timeout 20 "$ROOT/gui/build/nyaa-paper-qt" > "$TMP/client.log" 2>&1 || true
    [ -s "$TMP/frame.png" ] || fail "client did not render a frame"
fi

echo "smoke: all checks passed"
