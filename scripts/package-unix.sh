#!/bin/bash
# Stages the end-user bundle that install-unix.sh installs from, then packs it with a SHA-256 file:
# dist/meta-g-link-direct-<version>-<platform>.tar.gz(.sha256).
#
# Linux: package-unix.sh <native build folder or -> <Flatpak build folder or ->
# macOS: package-unix.sh <build folder>
set -euo pipefail

root=$(cd "$(dirname "$0")/.." && pwd)
version=$(sed -nE 's/^project\(meta_g_link_direct VERSION ([0-9]+\.[0-9]+\.[0-9]+).*/\1/p' "$root/CMakeLists.txt")
if [ -z "$version" ]; then
    echo "Project version not found in CMakeLists.txt" >&2
    exit 1
fi

stage_common() { # stage folder, README
    mkdir -p "$1"
    cp "$root/scripts/install-unix.sh" "$1/install.sh"
    chmod +x "$1/install.sh"
    cp "$2" "$1/README.txt"
    cp "$root/LICENSE" "$root/THIRD_PARTY_NOTICES.md" "$1/"
}

stage_linux_build() { # build folder, target folder
    mkdir -p "$2"
    cp "$1/meta-g-link-direct.so" "$1/meta-g-link-probe" "$1/meta-g-link-setup" "$2/"
}

pack() { # bundle name
    local dist="$root/dist" archive="$1.tar.gz"
    tar -czf "$dist/$archive" -C "$dist" "$1"
    # sha256sum on Linux, shasum on macOS: both write the same checkable line.
    (cd "$dist" && { sha256sum "$archive" 2>/dev/null || shasum -a 256 "$archive"; } >"$archive.sha256")
    echo "Release bundle: $dist/$archive"
}

case "$(uname -s)" in
Linux)
    if [ $# -ne 2 ] || { [ "$1" = - ] && [ "$2" = - ]; }; then
        echo "Usage: $0 <native build folder or -> <Flatpak build folder or ->" >&2
        exit 2
    fi
    name="meta-g-link-direct-$version-linux-$(uname -m)"
    stage="$root/dist/$name"
    rm -rf "$stage"
    stage_common "$stage" "$root/packaging/README-linux.txt"
    mkdir -p "$stage/plugin/locale"
    cp "$root/data/locale/en-US.ini" "$stage/plugin/locale/"
    if [ "$1" != - ]; then
        stage_linux_build "$1" "$stage/plugin/native"
    fi
    if [ "$2" != - ]; then
        stage_linux_build "$2" "$stage/plugin/flatpak"
    fi
    ;;
Darwin)
    if [ $# -ne 1 ]; then
        echo "Usage: $0 <build folder>" >&2
        exit 2
    fi
    name="meta-g-link-direct-$version-macos-$(uname -m)"
    stage="$root/dist/$name"
    rm -rf "$stage"
    stage_common "$stage" "$root/packaging/README-macos.txt"
    mkdir -p "$stage/plugin" "$stage/tools"
    cp -R "$1/meta-g-link-direct.plugin" "$stage/plugin/"
    cp -R "$1/meta-g-link-helper.app" "$stage/tools/Meta G Link Helper.app"
    cp -R "$1/meta-g-link-setup.app" "$stage/tools/Meta G Link Setup.app"
    cp "$1/meta-g-link-probe" "$stage/tools/"
    # Not notarized: the ad-hoc signature is what lets macOS run these at all.
    for item in "$stage/plugin/meta-g-link-direct.plugin" "$stage/tools/Meta G Link Helper.app" \
        "$stage/tools/Meta G Link Setup.app" "$stage/tools/meta-g-link-probe"; do
        codesign --force --deep --sign - "$item"
    done
    ;;
*)
    echo "This packs Linux and macOS bundles. On Windows, run the package task." >&2
    exit 1
    ;;
esac
pack "$name"
