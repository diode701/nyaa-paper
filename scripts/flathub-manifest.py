#!/usr/bin/env python3
"""Write the Flathub submission files from the local Flatpak manifest.

    scripts/flathub-manifest.py <commit> [out-dir]     (default out-dir: packaging/flathub)

The local manifest (packaging/flatpak/) builds this checkout through "dir"
sources. Flathub builds from public sources only, so the copy written here
replaces every "dir" source with this repository's git URL pinned to <commit>
(push that commit first). The engine module and cargo-sources.json are copied
alongside, as Flathub expects them next to the manifest.
"""
import json
import pathlib
import shutil
import subprocess
import sys

REPO_URL = "https://github.com/diode701/nyaapaper.git"
APP_ID = "io.github.diode701.NyaaPaper"


def pin_sources(module: dict, commit: str) -> None:
    sources = module.get("sources", [])
    for index, source in enumerate(sources):
        if isinstance(source, dict) and source.get("type") == "dir":
            sources[index] = {"type": "git", "url": REPO_URL, "commit": commit}


def main() -> int:
    if len(sys.argv) < 2:
        sys.exit(__doc__)

    root = pathlib.Path(__file__).resolve().parent.parent
    local = root / "packaging" / "flatpak"
    out = pathlib.Path(sys.argv[2]) if len(sys.argv) > 2 else root / "packaging" / "flathub"

    commit = subprocess.run(
        ["git", "-C", str(root), "rev-parse", sys.argv[1]], check=True, capture_output=True, text=True
    ).stdout.strip()

    manifest = json.loads((local / f"{APP_ID}.json").read_text())
    for module in manifest["modules"]:
        if isinstance(module, dict):
            pin_sources(module, commit)

    out.mkdir(parents=True, exist_ok=True)
    (out / f"{APP_ID}.json").write_text(json.dumps(manifest, indent=2) + "\n")
    for name in ("linux-wallpaperengine.json", "cargo-sources.json"):
        shutil.copy2(local / name, out / name)

    print(f"{out}: app pinned to {commit}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
