#!/bin/bash
# Fetches what a macOS build reads and checks each download against dependencies.lock.json: OBS's
# pinned sources (libobs, its frontend API and obs-websocket's header) and OBS's prebuilt dependencies
# (FFmpeg's headers). The build links against the OBS.app installed on the machine.
set -euo pipefail
root=$(cd "$(dirname "$0")/.." && pwd)
lock="$root/dependencies.lock.json"
deps="$root/.deps"
mkdir -p "$deps"

value() { python3 -c 'import json,sys; d=json.load(open(sys.argv[1]));
for k in sys.argv[2:]: d=d[k]
print(d)' "$lock" "$@"; }

fetch() { # url sha256 file
    if [ ! -f "$3" ] || [ "$(shasum -a 256 "$3" | cut -d' ' -f1)" != "$2" ]; then
        curl --fail --location --silent --show-error --output "$3" "$1"
    fi
    echo "$2  $3" | shasum -a 256 --check --quiet
}

sources_dir="$deps/$(value obs directory)"
sources_tar="$deps/obs-sources.tar.gz"
fetch "$(value obs url)" "$(value obs sha256)" "$sources_tar"
if [ ! -f "$sources_dir/libobs/obs-module.h" ]; then
    mkdir -p "$sources_dir"
    tar -xzf "$sources_tar" -C "$sources_dir" --strip-components=1
fi

prebuilt="$(value obs prebuilt_version)"
deps_dir="$deps/obs-deps-$prebuilt-universal"
deps_tar="$deps/macos-deps-$prebuilt-universal.tar.xz"
fetch "https://github.com/obsproject/obs-deps/releases/download/$prebuilt/macos-deps-$prebuilt-universal.tar.xz" \
    "$(value obs prebuilt_macos_universal_sha256)" "$deps_tar"
if [ ! -f "$deps_dir/include/libavcodec/avcodec.h" ]; then
    mkdir -p "$deps_dir"
    tar -xJf "$deps_tar" -C "$deps_dir"
fi
echo "macOS build inputs ready under $deps"
