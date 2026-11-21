#!/bin/sh
# Build + install the Flatpak, optionally export a bundle.
#   scripts/flatpak.sh            build and install (user)
#   scripts/flatpak.sh bundle     ...and write release/nyaa-paper-<ver>-x86_64.flatpak
# State lives outside the repo (FP_STATE, default ~/OOPSS/fp-build, ~10 GB).
# ccache keeps unchanged engine files cached across builds: a one-file change
# rebuilds in minutes instead of a full ~15 min engine compile.
set -e
here=$(cd "$(dirname "$0")/.." && pwd)
state=${FP_STATE:-$HOME/OOPSS/fp-build}
jobs=${FP_JOBS:-8}          # ~1 GB RAM per compiler on the heavy engine files
manifest="$here/packaging/flatpak/io.github.diode701.NyaaPaper.json"
mkdir -p "$state"
cd "$state"
# --disable-updates: sources are pinned, reuse mirrors already downloaded
# (a build then works while e.g. code.videolan.org is down)
flatpak-builder --user --install --force-clean --ccache --disable-updates --jobs="$jobs" \
    --state-dir=.flatpak-builder build-dir "$manifest"
if [ "$1" = bundle ]; then
    version=$(grep -o 'release version="[^"]*"' "$here/packaging/flatpak/io.github.diode701.NyaaPaper.metainfo.xml" | head -1 | cut -d'"' -f2)
    out=${FP_RELEASE:-$HOME/OOPSS/release}
    mkdir -p "$out"
    flatpak build-export repo build-dir >/dev/null
    flatpak build-bundle --runtime-repo=https://dl.flathub.org/repo/flathub.flatpakrepo \
        repo "$out/nyaa-paper-$version-x86_64.flatpak" io.github.diode701.NyaaPaper
    echo "bundle: $out/nyaa-paper-$version-x86_64.flatpak"
fi
