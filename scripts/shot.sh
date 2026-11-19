#!/usr/bin/env bash
# Screenshot the real app on the real renderer (Vulkan), without touching the
# desktop: a hidden virtual KWin session, the GUI in mock mode inside it.
# SHOT_PLAYLISTS=<dir of monitor.json> also starts a sandboxed daemon.
#   scripts/shot.sh out.png [WIDTHxHEIGHT] [keys] [extra env...]
#   scripts/shot.sh /tmp/a.png 1920x1050 "Right,Down" NYAA_VIEW=1
# The offscreen platform cannot do this: it uses Qt's software renderer,
# which draws no shader effects (blur, shadows).
set -euo pipefail
out="$1"; size="${2:-1280x800}"; keys="${3:-}"; shift $(( $# < 3 ? $# : 3 ))
w="${size%x*}"; h="${size#*x}"
root="$(cd "$(dirname "$0")/.." && pwd)"
sock="nyaa-shot-$$"
# Isolation: the virtual KWin must never touch the user's session. On Plasma 6
# KWin also hosts the global-shortcut service (org.kde.kglobalaccel), so on
# the session bus it would take that name and break the desktop's shortcuts.
# It gets its own D-Bus session and its own config directory instead.
sandbox="$(mktemp -d)"
mkdir -p "$sandbox/config/nyaa-paper"
[ -f "${XDG_CONFIG_HOME:-$HOME/.config}/nyaa-paper/config.json" ] && \
    cp "${XDG_CONFIG_HOME:-$HOME/.config}/nyaa-paper/config.json" "$sandbox/config/nyaa-paper/"
trap 'rm -rf "$sandbox"' EXIT
export SHOT_OUT="$out" SHOT_SIZE="$size" SHOT_KEYS="$keys" SHOT_SOCK="$sock" SHOT_ROOT="$root" SHOT_W="$w" SHOT_H="$h"
XDG_CONFIG_HOME="$sandbox/config" QT_NO_XDG_DESKTOP_PORTAL=1 dbus-run-session -- bash -c '
  kwin_wayland --virtual --no-lockscreen --socket "$SHOT_SOCK" --width "$((SHOT_W + 20))" --height "$((SHOT_H + 60))" >/dev/null 2>&1 &
  kwin=$!
  for _ in $(seq 50); do [ -S "$XDG_RUNTIME_DIR/$SHOT_SOCK" ] && break; sleep 0.1; done
  # SHOT_PLAYLISTS=dir: run a daemon (fake engine) with those playlist files
  if [ -n "${SHOT_PLAYLISTS:-}" ]; then
    state="$(mktemp -d)"; mkdir -p "$state/playlists"; cp "$SHOT_PLAYLISTS"/*.json "$state/playlists/"
    NYAA_STATE_DIR="$state" NYAA_ENGINE=/bin/sleep "$SHOT_ROOT/daemon/target/debug/nyaa-paperd" >/dev/null 2>&1 &
    for _ in $(seq 50); do gdbus introspect --session -d org.nyaa.Paper -o /org/nyaa/Paper >/dev/null 2>&1 && break; sleep 0.1; done
  fi
  env WAYLAND_DISPLAY="$SHOT_SOCK" QT_QPA_PLATFORM=wayland QT_FORCE_STDERR_LOGGING=1 NYAA_MOCK=1 \
      NYAA_WINDOW="$SHOT_SIZE" ${SHOT_KEYS:+NYAA_KEYS="$SHOT_KEYS"} NYAA_SHOT="$SHOT_OUT" \
      NYAA_SHOT_DELAY="${NYAA_SHOT_DELAY:-3000}" "$@" timeout 30 "$SHOT_ROOT/gui/build/nyaa-paper-qt" 2>&1 \
      | grep -vE "portal|locale|UTF-8|manual|information|grabbing|^focus=" || true
  kill $kwin 2>/dev/null; wait $kwin 2>/dev/null || true
' shot "$@" 2> >(grep -vE '^dbus-daemon|portal|^$|AT-SPI|atk-bridge|Gtk-WARNING|fallback|fusermount|connection to the bus' >&2)
[ -f "$out" ] && echo "saved $out"
