#!/usr/bin/env bash
# End to end, without touching the desktop: a private session bus, a real
# nyaa-paperd on it (own config and state dirs), and a fake engine that records
# the argv and GPU environment it was started with.
#   controller_test  one wallpaper on two monitors with two GPUs
#   playlist_test    playlist editing against the daemon's stored documents
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT

cat > "$work/fake-engine" <<'ENGINE'
#!/usr/bin/env bash
monitor="$2"   # argv: --screen-root <monitor> ...
{
  echo "argv: $*"
  env | grep -E '^(DRI_PRIME|__NV_PRIME_RENDER_OFFLOAD|__GLX_VENDOR_LIBRARY_NAME|__GL_THREADED_OPTIMIZATIONS)=' | sort | sed 's/^/env: /'
} > "$NYAA_E2E_LOG/$monitor.txt"
exec sleep 30
ENGINE
chmod +x "$work/fake-engine"
mkdir -p "$work/log" "$work/config"

export NYAA_E2E_LOG="$work/log"
dbus-run-session -- bash -c "
  XDG_CONFIG_HOME='$work/config' NYAA_ENGINE='$work/fake-engine' NYAA_STATE_DIR='$work/state' \
    '$root/daemon/target/debug/nyaa-paperd' 2>'$work/daemon.log' &
  daemon=\$!
  for i in \$(seq 50); do gdbus introspect --session -d org.nyaa.Paper -o /org/nyaa/Paper >/dev/null 2>&1 && break; sleep 0.1; done
  status=0
  NYAA_TEST_DAEMON=1 '$root/gui/build/controller_test' || status=\$?
  '$root/gui/build/playlist_test' || status=\$?
  sleep 0.5
  kill -TERM \$daemon; wait \$daemon 2>/dev/null || true   # daemon stops its engines
  exit \$status
"
for m in TEST-1 TEST-2; do echo "== $m"; cat "$work/log/$m.txt"; done
