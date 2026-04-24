#!/bin/bash
set -e

DIST_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
INSTALL_DIR="$HOME/.local/share/NyaaPaper"
BIN_DIR="$HOME/.local/bin"
DESKTOP_DIR="$HOME/.local/share/applications"
DESKTOP_FILE="$DESKTOP_DIR/nyaa-paper.desktop"

echo "→ Installing Nyaa Paper to $INSTALL_DIR ..."

mkdir -p "$INSTALL_DIR"
mkdir -p "$BIN_DIR"
mkdir -p "$DESKTOP_DIR"

cp -f "$DIST_DIR/lib/nyaa-paper-launcher.jar" "$INSTALL_DIR/"
cp -f "$DIST_DIR/lib/nyaa-paper-app.jar" "$INSTALL_DIR/"
[ -f "$DIST_DIR/icon.png" ] && cp -f "$DIST_DIR/icon.png" "$INSTALL_DIR/"

# Launcher script
cat > "$INSTALL_DIR/nyaa-paper" << 'LAUNCHER'
#!/bin/bash
java -jar "$HOME/.local/share/NyaaPaper/nyaa-paper-launcher.jar" "$@"
LAUNCHER
chmod +x "$INSTALL_DIR/nyaa-paper"

# Symlink
rm -f "$BIN_DIR/nyaa-paper"
ln -sf "$INSTALL_DIR/nyaa-paper" "$BIN_DIR/nyaa-paper"

# Desktop entry
cat > "$DESKTOP_FILE" << DESKTOP
[Desktop Entry]
Name=Nyaa Paper
Comment=Wallpaper Engine frontend for Linux
Exec=$INSTALL_DIR/nyaa-paper
Icon=$INSTALL_DIR/icon.png
Terminal=false
Type=Application
Categories=Utility;
DESKTOP

if command -v update-desktop-database &> /dev/null; then
    update-desktop-database "$DESKTOP_DIR" 2>/dev/null || true
fi

echo "✅ Installation complete. Run 'nyaa-paper' from terminal or application menu."
