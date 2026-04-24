#!/bin/bash
set -e

echo "→ Uninstalling Nyaa Paper..."

rm -rf "$HOME/.local/share/NyaaPaper"
rm -f "$HOME/.local/bin/nyaa-paper"
rm -f "$HOME/.local/share/applications/nyaa-paper.desktop"

if command -v update-desktop-database &> /dev/null; then
    update-desktop-database "$HOME/.local/share/applications" 2>/dev/null || true
fi

echo "✅ Uninstalled. Config folder ~/.config/NyaaPaper left intact."
