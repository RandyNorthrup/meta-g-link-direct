#!/bin/bash
# Builds and tests the plugin for the OBS Flatpak, inside that Flatpak's own runtime and SDK, so it
# links the FFmpeg and libobs that OBS runs on. Run it through the OBS Flatpak, from the repository:
#   flatpak run --devel --share=network --filesystem="$PWD" --command=bash \
#       com.obsproject.Studio scripts/build-flatpak.sh
set -euo pipefail

build="${1:-out/flatpak}"
export PKG_CONFIG_PATH=/app/lib/pkgconfig:/app/lib/x86_64-linux-gnu/pkgconfig:/usr/lib/x86_64-linux-gnu/pkgconfig:/usr/share/pkgconfig
export CMAKE_PREFIX_PATH=/app
cmake -S . -B "$build" -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DMGL_BUILD_LINK=ON -DMGL_BUILD_OBS_PLUGIN=ON
cmake --build "$build"
ctest --test-dir "$build" --output-on-failure
