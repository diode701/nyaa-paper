#!/usr/bin/env python3
"""Generate flatpak-builder sources for the daemon's crates (offline build).

    scripts/cargo-sources.py daemon/Cargo.lock > packaging/flatpak/cargo-sources.json

Flathub builds without network access, so every crate in Cargo.lock is listed
as a pinned archive from static.crates.io (the sha256 is the checksum Cargo.lock
already records) and unpacked into cargo/vendor/, with the .cargo-checksum.json
cargo expects and a cargo config that replaces crates.io with that directory.
The module sets CARGO_HOME=/run/build/<module>/cargo and builds with --offline.

Only crates.io dependencies are supported; a git dependency makes it fail.
"""
import json
import sys
import tomllib

CRATES = "https://static.crates.io/crates/{name}/{name}-{version}.crate"
REGISTRY = "registry+https://github.com/rust-lang/crates.io-index"

CONFIG = """[source.vendored-sources]
directory = "cargo/vendor"

[source.crates-io]
replace-with = "vendored-sources"
"""


def main() -> int:
    with open(sys.argv[1], "rb") as lockfile:
        lock = tomllib.load(lockfile)

    sources = []
    for package in lock.get("package", []):
        source = package.get("source")
        if source is None:
            continue  # the workspace's own crates
        if source != REGISTRY:
            sys.exit(f"unsupported source for {package['name']}: {source}")

        name, version, checksum = package["name"], package["version"], package["checksum"]
        dest = f"cargo/vendor/{name}-{version}"
        sources.append({
            "type": "archive",
            "archive-type": "tar-gzip",
            "url": CRATES.format(name=name, version=version),
            "sha256": checksum,
            "dest": dest,
        })
        sources.append({
            "type": "inline",
            "contents": json.dumps({"package": checksum, "files": {}}),
            "dest": dest,
            "dest-filename": ".cargo-checksum.json",
        })

    sources.append({"type": "inline", "contents": CONFIG, "dest": "cargo", "dest-filename": "config"})

    json.dump(sources, sys.stdout, indent=2)
    sys.stdout.write("\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
