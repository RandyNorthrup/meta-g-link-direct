#!/bin/bash
# Installs the release bundle this script came in, for the current user, on Linux or macOS. It never
# closes or restarts OBS and copies only this plugin's files.
#
# Linux: into every OBS the bundle has a build for: native OBS (plugin/native) and the OBS Flatpak
# (plugin/flatpak), which also gets the permissions the plugin needs: Bluetooth, BlueZ, NetworkManager
# and the Secret Service.
# macOS: the plugin into OBS's plugin folder, and the helper and setup apps beside the setup tool.
#
# Options: --uninstall removes this plugin's files (and keeps the stored identity); --setup also opens
# the setup window afterwards; --dry-run prints what it would do.
set -euo pipefail

bundle=$(cd "$(dirname "$0")" && pwd)
name=meta-g-link-direct
flatpak_app=com.obsproject.Studio
helper_id=io.github.RandyNorthrup.MetaGLinkDirect.Helper
mode=install
open_setup=no
dry=no
for argument in "$@"; do
    case "$argument" in
    --uninstall) mode=uninstall ;;
    --setup) open_setup=yes ;;
    --dry-run) dry=yes ;;
    *)
        echo "Unknown option: $argument" >&2
        exit 2
        ;;
    esac
done

run() {
    if [ "$dry" = yes ]; then
        printf 'would run:'
        printf ' %q' "$@"
        printf '\n'
    else
        "$@"
    fi
}

obs_running() {
    pgrep -x obs >/dev/null 2>&1 || pgrep -x OBS >/dev/null 2>&1
}

warn_if_running() {
    if obs_running; then
        echo "OBS is running. Close it and start it again to load this version."
    fi
}

install_linux_target() { # plugin build folder, OBS config folder, tool folder
    local build="$1" config="$2" tools="$3"
    local plugin="$config/plugins/$name"
    run mkdir -p "$plugin/bin/64bit" "$plugin/data/locale" "$tools"
    run cp -f "$build/$name.so" "$plugin/bin/64bit/$name.so"
    run cp -f "$bundle/plugin/locale/en-US.ini" "$plugin/data/locale/en-US.ini"
    run cp -f "$build/meta-g-link-probe" "$build/meta-g-link-setup" "$tools/"
    echo "Installed for this OBS: $plugin"
    echo "Setup tool: $tools/meta-g-link-probe"
}

# A native build needs the OBS and FFmpeg of the system it was made on. The Flatpak build needs no
# such check: it runs on the runtime it was built with.
native_build_fits() {
    ! ldd "$bundle/plugin/native/$name.so" 2>/dev/null | grep -q 'not found'
}

uninstall_linux_target() { # OBS config folder, tool folder
    run rm -rf "$1/plugins/$name"
    run rm -f "$2/meta-g-link-probe" "$2/meta-g-link-setup"
    echo "Removed: $1/plugins/$name"
}

data_home="${XDG_DATA_HOME:-$HOME/.local/share}"
config_home="${XDG_CONFIG_HOME:-$HOME/.config}"
flatpak_root="$HOME/.var/app/$flatpak_app"

has_native_obs() { command -v obs >/dev/null 2>&1; }
has_flatpak_obs() { command -v flatpak >/dev/null 2>&1 && flatpak info "$flatpak_app" >/dev/null 2>&1; }

linux() {
    local done_any=no
    if has_native_obs; then
        if [ "$mode" = uninstall ]; then
            uninstall_linux_target "$config_home/obs-studio" "$data_home/MetaGLinkDirect/bin"
            done_any=yes
        elif [ ! -f "$bundle/plugin/native/$name.so" ]; then
            echo "This bundle has no build for native OBS; see Build from source in the README."
        elif ! native_build_fits; then
            echo "This bundle's native build was made for another system's OBS and FFmpeg, so it"
            echo "was not installed. Use the OBS Flatpak, or see Build from source in the README."
        else
            install_linux_target "$bundle/plugin/native" "$config_home/obs-studio" \
                "$data_home/MetaGLinkDirect/bin"
            done_any=yes
        fi
    fi
    if has_flatpak_obs; then
        if [ "$mode" = uninstall ]; then
            uninstall_linux_target "$flatpak_root/config/obs-studio" \
                "$flatpak_root/data/MetaGLinkDirect/bin"
            # Takes back only what the install allowed; the user's other overrides stay.
            run flatpak override --user --disallow=bluetooth --system-no-talk-name=org.bluez \
                --system-no-talk-name=org.freedesktop.NetworkManager \
                --no-talk-name=org.freedesktop.secrets "$flatpak_app"
            done_any=yes
        elif [ -f "$bundle/plugin/flatpak/$name.so" ]; then
            install_linux_target "$bundle/plugin/flatpak" "$flatpak_root/config/obs-studio" \
                "$flatpak_root/data/MetaGLinkDirect/bin"
            run flatpak override --user --allow=bluetooth --system-talk-name=org.bluez \
                --system-talk-name=org.freedesktop.NetworkManager \
                --talk-name=org.freedesktop.secrets "$flatpak_app"
            echo "OBS Flatpak allowed: Bluetooth, BlueZ, NetworkManager, Secret Service."
            done_any=yes
        else
            echo "This bundle has no build for the OBS Flatpak; see Build from source in the README."
        fi
    fi
    if [ "$done_any" = no ]; then
        echo "No OBS found. Install OBS Studio 32 or newer first, then run this again." >&2
        exit 1
    fi
    if [ "$open_setup" = yes ] && [ "$mode" = install ]; then
        if has_flatpak_obs && [ -f "$bundle/plugin/flatpak/$name.so" ]; then
            run flatpak run --command="$flatpak_root/data/MetaGLinkDirect/bin/meta-g-link-setup" \
                "$flatpak_app"
        else
            run "$data_home/MetaGLinkDirect/bin/meta-g-link-setup"
        fi
    fi
}

macos() {
    local plugins="$HOME/Library/Application Support/obs-studio/plugins"
    local tools="$HOME/Library/Application Support/MetaGLinkDirect/bin"
    if [ "$mode" = uninstall ]; then
        run rm -rf "$plugins/$name.plugin" "$tools/Meta G Link Helper.app" \
            "$tools/Meta G Link Setup.app" "$tools/meta-g-link-probe"
        echo "Removed the plugin and its tools."
        return
    fi
    if [ ! -d /Applications/OBS.app ]; then
        echo "OBS.app is not in /Applications. Install OBS Studio 32 or newer first." >&2
        exit 1
    fi
    run mkdir -p "$plugins" "$tools"
    run rm -rf "$plugins/$name.plugin" "$tools/Meta G Link Helper.app" "$tools/Meta G Link Setup.app"
    run cp -R "$bundle/plugin/$name.plugin" "$plugins/"
    run cp -R "$bundle/tools/Meta G Link Helper.app" "$bundle/tools/Meta G Link Setup.app" "$tools/"
    run cp -f "$bundle/tools/meta-g-link-probe" "$tools/"
    # A new build is a new app to macOS: an old Bluetooth answer makes it ask again with no window
    # in front, so the question stays hidden. Without the old answer, the helper asks in the open.
    run tccutil reset BluetoothAlways "$helper_id"
    # Files from a download carry a quarantine mark; these builds are not notarized.
    run xattr -dr com.apple.quarantine "$plugins/$name.plugin" "$tools/Meta G Link Helper.app" \
        "$tools/Meta G Link Setup.app" "$tools/meta-g-link-probe"
    echo "Installed for OBS: $plugins/$name.plugin"
    echo "Setup tool and helper: $tools"
    echo "Now allow Bluetooth and Location: macOS asks when Meta G Link Helper first runs."
    run open "$tools/Meta G Link Helper.app"
    if [ "$open_setup" = yes ]; then
        run open "$tools/Meta G Link Setup.app"
    fi
}

case "$(uname -s)" in
Linux) linux ;;
Darwin) macos ;;
*)
    echo "This installer is for Linux and macOS. On Windows, use Install.cmd." >&2
    exit 1
    ;;
esac
warn_if_running
echo "Done."
