#!/usr/bin/env bash
# Screenshot the real app on the real renderer (Vulkan), without touching the
# desktop: a hidden virtual KWin session, the GUI in mock mode inside it.
#   scripts/shot.sh out.png [WIDTHxHEIGHT] [keys] [extra env...]
#   scripts/shot.sh /tmp/a.png 1920x1050 "Right,Down" NYAA_VIEW=1
# The offscreen platform cannot do this: it uses Qt's software renderer,
# which draws no shader effects (blur, shadows).
set -euo pipefail
out="$1"; size="${2:-1280x800}"; keys="${3:-}"; shift $(( $# < 3 ? $# : 3 ))
w="${size%x*}"; h="${size#*x}"
root="$(cd "$(dirname "$0")/.." && pwd)"
sock="nyaa-shot-$$"
kwin_wayland --virtual --no-lockscreen --socket "$sock" --width "$((w + 20))" --height "$((h + 60))" >/dev/null 2>&1 &
kwin=$!
trap 'kill $kwin 2>/dev/null || true' EXIT
for _ in $(seq 50); do [ -S "$XDG_RUNTIME_DIR/$sock" ] && break; sleep 0.1; done
env WAYLAND_DISPLAY="$sock" QT_QPA_PLATFORM=wayland QT_FORCE_STDERR_LOGGING=1 NYAA_MOCK=1 \
    NYAA_WINDOW="$size" ${keys:+NYAA_KEYS="$keys"} NYAA_SHOT="$out" NYAA_SHOT_DELAY="${NYAA_SHOT_DELAY:-3000}" "$@" \
    timeout 30 "$root/gui/build/nyaa-paper-qt" 2>&1 \
    | grep -vE "portal|locale|UTF-8|manual|information|grabbing|^focus=" || true
[ -f "$out" ] && echo "saved $out"
