cmake_minimum_required(VERSION 3.28...3.31)
# std::format, which the core uses, needs macOS 13.3.
set(CMAKE_OSX_DEPLOYMENT_TARGET "13.3" CACHE STRING "The oldest macOS these binaries run on")
project(meta_g_link_direct VERSION 0.5.1 LANGUAGES C CXX)

include(CTest)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# The static libraries end up inside the OBS plugin, a shared module.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(MGL_BUILD_OBS_PLUGIN "Build the OBS module" ${WIN32})
option(MGL_BUILD_LINK "Build the glasses link and the probe on a POSIX platform" OFF)
option(MGL_SANITIZERS "Instrument portable tests with supported sanitizers" OFF)

add_library(mgl_protocol STATIC
  src/protocol/sdu.cpp
  src/protocol/address.cpp
  src/protocol/protobuf.cpp
  src/protocol/datax.cpp
  src/protocol/json.cpp
  src/protocol/encoding.cpp
  src/protocol/stream.cpp
  src/protocol/hevc.cpp
  src/protocol/playout.cpp
  src/protocol/battery.cpp
  src/protocol/buffered_transport.cpp)
target_include_directories(mgl_protocol PUBLIC include)

function(mgl_owned_target target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /WX /utf-8 $<$<COMPILE_LANGUAGE:CXX>:/permissive->)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
  endif()
  if(MGL_SANITIZERS)
    if(MSVC)
      target_compile_options(${target} PRIVATE /fsanitize=address)
      target_link_options(${target} PRIVATE /INCREMENTAL:NO)
    else()
      target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer)
      target_link_options(${target} PRIVATE -fsanitize=address,undefined -fno-sanitize-recover=all)
    endif()
  endif()
endfunction()
mgl_owned_target(mgl_protocol)

# Everything between the protocol and a program: the session logic in src/core, over one platform's
# Bluetooth, Wi-Fi, HTTP, storage and crypto.
set(mgl_core_sources
  src/core/crypto_common.cpp
  src/core/identity.cpp
  src/core/session.cpp
  src/core/provision.cpp
  src/core/setup.cpp
  src/core/media_session.cpp
  src/core/wifi_adapter.cpp
  src/protocol/airshield.cpp)

if(WIN32)
  add_library(mgl_link STATIC ${mgl_core_sources}
    src/windows/capabilities.cpp
    src/windows/discovery.cpp
    src/windows/classic.cpp
    src/windows/rfcomm.cpp
    src/windows/crypto.cpp
    src/windows/sdu_socket.cpp
    src/windows/rfcomm_transport.cpp
    src/windows/tcp_transport.cpp
    src/windows/http.cpp
    src/windows/identity.cpp
    src/windows/platform.cpp
    src/windows/wifi_station.cpp
    src/windows/win_text.cpp)
  target_include_directories(mgl_link PUBLIC include PRIVATE src/core)
  target_compile_definitions(mgl_link PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
  target_link_libraries(mgl_link PUBLIC mgl_protocol PRIVATE wlanapi bthprops windowsapp ws2_32 bcrypt winhttp crypt32 shell32 ole32 iphlpapi)
  mgl_owned_target(mgl_link)

  add_executable(meta-g-link-probe src/probe/main.cpp src/probe/windows_diagnostics.cpp
    src/probe/private_input.cpp)
  target_link_libraries(meta-g-link-probe PRIVATE mgl_link)
  mgl_owned_target(meta-g-link-probe)

  # The setup window. The WebView2 SDK (pinned in dependencies.lock.json, fetched by bootstrap)
  # provides its headers and the static loader, so no WebView2 DLL ships.
  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/dependencies.lock.json" setup_dependency_lock)
  string(JSON webview2_directory GET "${setup_dependency_lock}" webview2 directory)
  set(webview2_root "${CMAKE_CURRENT_SOURCE_DIR}/.deps/${webview2_directory}/build/native")
  add_executable(meta-g-link-setup WIN32 src/setup/main.cpp src/setup/meta-g-link-setup.manifest)
  target_include_directories(meta-g-link-setup SYSTEM PRIVATE "${webview2_root}/include")
  # Without WIN32_LEAN_AND_MEAN: WebView2.h needs the COM declarations windows.h then brings in.
  target_compile_definitions(meta-g-link-setup PRIVATE NOMINMAX UNICODE _UNICODE)
  target_link_libraries(meta-g-link-setup PRIVATE mgl_link
    "${webview2_root}/x64/WebView2LoaderStatic.lib" comctl32 shell32 ole32 user32 gdi32 advapi32)
  mgl_owned_target(meta-g-link-setup)
elseif(MGL_BUILD_LINK)
  # mbedTLS is pinned and fetched by hash (dependencies.lock.json), and linked statically so its
  # symbols stay inside this project's binaries.
  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/dependencies.lock.json" link_dependency_lock)
  string(JSON mbedtls_url GET "${link_dependency_lock}" mbedtls url)
  string(JSON mbedtls_sha256 GET "${link_dependency_lock}" mbedtls sha256)
  include(FetchContent)
  set(ENABLE_TESTING OFF CACHE BOOL "" FORCE)
  set(ENABLE_PROGRAMS OFF CACHE BOOL "" FORCE)
  set(USE_SHARED_MBEDTLS_LIBRARY OFF CACHE BOOL "" FORCE)
  set(USE_STATIC_MBEDTLS_LIBRARY ON CACHE BOOL "" FORCE)
  set(MBEDTLS_FATAL_WARNINGS OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(mbedtls URL "${mbedtls_url}" URL_HASH "SHA256=${mbedtls_sha256}"
    DOWNLOAD_EXTRACT_TIMESTAMP FALSE SYSTEM EXCLUDE_FROM_ALL)
  FetchContent_MakeAvailable(mbedtls)

  add_library(mgl_link STATIC ${mgl_core_sources}
    src/posix/crypto_mbedtls.cpp
    src/posix/fd_transport.cpp
    src/posix/http_curl.cpp
    src/posix/interface_address.cpp
    src/posix/platform.cpp
    src/posix/tcp_transport.cpp)
  target_include_directories(mgl_link PUBLIC include PRIVATE src/core src/posix)
  find_package(CURL REQUIRED)
  target_link_libraries(mgl_link PUBLIC mgl_protocol PRIVATE MbedTLS::mbedcrypto CURL::libcurl)
  if(APPLE)
    enable_language(OBJCXX)
    target_sources(mgl_link PRIVATE
      src/macos/identity_keychain.mm
      src/macos/helper_client.cpp
      src/macos/region.mm
      src/macos/rfcomm_helper_client.cpp
      src/macos/wifi_corewlan.mm)
    set_source_files_properties(src/macos/identity_keychain.mm src/macos/region.mm
      src/macos/wifi_corewlan.mm PROPERTIES COMPILE_OPTIONS "-fobjc-arc")
    target_link_libraries(mgl_link PRIVATE "-framework Foundation" "-framework CoreWLAN"
      "-framework Security")
  else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(MGL_LINUX REQUIRED IMPORTED_TARGET gio-2.0 libsecret-1 bluez)
    target_sources(mgl_link PRIVATE
      src/linux/identity_secret_service.cpp
      src/linux/rfcomm_bluez.cpp
      src/linux/wifi_networkmanager.cpp)
    target_include_directories(mgl_link PRIVATE src/linux)
    target_link_libraries(mgl_link PRIVATE PkgConfig::MGL_LINUX)
  endif()
  mgl_owned_target(mgl_link)

  add_executable(meta-g-link-probe src/probe/main.cpp)
  target_link_libraries(meta-g-link-probe PRIVATE mgl_link)
  mgl_owned_target(meta-g-link-probe)

  if(APPLE)
    # The app that holds the Bluetooth connection and joins the glasses' network, under its own
    # macOS permissions.
    add_executable(meta-g-link-helper MACOSX_BUNDLE src/macos/rfcomm_helper.mm
      src/macos/wifi_helper.mm)
    set_target_properties(meta-g-link-helper PROPERTIES
      MACOSX_BUNDLE_BUNDLE_NAME "Meta G Link Helper"
      MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
      MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/src/macos/helper-Info.plist"
      RUNTIME_OUTPUT_NAME meta-g-link-helper)
    target_link_libraries(meta-g-link-helper PRIVATE "-framework Cocoa" "-framework IOBluetooth"
      "-framework CoreBluetooth" "-framework CoreLocation" "-framework CoreWLAN")
    target_compile_options(meta-g-link-helper PRIVATE -fobjc-arc)
    mgl_owned_target(meta-g-link-helper)

    # The setup window.
    add_executable(meta-g-link-setup MACOSX_BUNDLE src/setup/main_macos.mm)
    set_target_properties(meta-g-link-setup PROPERTIES
      MACOSX_BUNDLE_BUNDLE_NAME "Meta G Link Setup"
      MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
      MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/src/macos/setup-Info.plist")
    target_link_libraries(meta-g-link-setup PRIVATE mgl_link "-framework Cocoa" "-framework WebKit")
    target_compile_options(meta-g-link-setup PRIVATE -fobjc-arc)
    mgl_owned_target(meta-g-link-setup)
  else()
    # The setup window: the login in the window where WebKitGTK is present, else in the browser.
    pkg_check_modules(MGL_GTK REQUIRED IMPORTED_TARGET gtk+-3.0)
    pkg_check_modules(MGL_WEBKIT IMPORTED_TARGET webkit2gtk-4.1)
    add_executable(meta-g-link-setup src/setup/main_gtk.cpp)
    target_link_libraries(meta-g-link-setup PRIVATE mgl_link PkgConfig::MGL_GTK)
    if(MGL_WEBKIT_FOUND)
      target_compile_definitions(meta-g-link-setup PRIVATE MGL_HAVE_WEBKIT)
      target_link_libraries(meta-g-link-setup PRIVATE PkgConfig::MGL_WEBKIT)
    endif()
    mgl_owned_target(meta-g-link-setup)
  endif()
endif()

if(MGL_BUILD_OBS_PLUGIN)
  add_library(meta-g-link-direct MODULE
    src/plugin/module.c
    src/plugin/source.c
    src/plugin/link_settings.c
    src/plugin/battery_status.c
    src/plugin/glasses_link.cpp
    src/plugin/video_pacer.cpp
    src/plugin/hevc_decoder.cpp
    src/plugin/aac_decoder.cpp)
  if(WIN32)
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/dependencies.lock.json" dependency_lock)
    string(JSON obs_source_directory GET "${dependency_lock}" obs directory)
    string(JSON obs_dependency_version GET "${dependency_lock}" obs prebuilt_version)
    set(obs_deps_root
      "${CMAKE_CURRENT_SOURCE_DIR}/.deps/${obs_source_directory}/.deps/obs-deps-${obs_dependency_version}-x64")
    list(APPEND CMAKE_PREFIX_PATH "${obs_deps_root}")
    find_package(libobs 32.2.2 EXACT REQUIRED CONFIG)
    # The Tools menu entry; obs-frontend-api.dll ships with every OBS that has a user interface.
    find_package(obs-frontend-api REQUIRED CONFIG)
    # FFmpeg for HEVC decode; the matching runtime DLLs ship with OBS.
    find_library(MGL_AVCODEC NAMES avcodec PATHS "${obs_deps_root}/lib" REQUIRED)
    find_library(MGL_AVUTIL NAMES avutil PATHS "${obs_deps_root}/lib" REQUIRED)
    target_sources(meta-g-link-direct PRIVATE src/plugin/setup_process_windows.cpp)
    target_include_directories(meta-g-link-direct PRIVATE "${obs_deps_root}/include")
    # obs-websocket's header-only plugin API, from the pinned OBS sources (GPL-2.0-or-later).
    target_include_directories(meta-g-link-direct SYSTEM PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/.deps/${obs_source_directory}/plugins/obs-websocket/lib")
    # libobs's _udiv128 header requires the MSVC C compiler; LLVM issue #59168. Every file that
    # includes OBS headers (module.c, source.c, link_settings.c, battery_status.c) is C17 with
    # /analyze. The glasses link core and the decoders are C++20 behind src/plugin/glasses_link.h,
    # include no OBS headers, and are checked by clang-tidy instead.
    set_source_files_properties(src/plugin/module.c src/plugin/source.c src/plugin/link_settings.c
      src/plugin/battery_status.c PROPERTIES COMPILE_OPTIONS "/analyze")
    target_compile_definitions(meta-g-link-direct PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
    target_link_libraries(meta-g-link-direct PRIVATE OBS::libobs OBS::obs-frontend-api
      "${MGL_AVCODEC}" "${MGL_AVUTIL}")
  elseif(APPLE)
    # OBS.app ships no headers, so they come from the pinned sources and FFmpeg's from OBS's pinned
    # dependencies (scripts/bootstrap-macos.sh), and the plugin links what the installed OBS.app
    # runs on. libobs, its frontend API and FFmpeg are found at run time inside OBS.app.
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/dependencies.lock.json" dependency_lock)
    string(JSON obs_source_directory GET "${dependency_lock}" obs directory)
    string(JSON obs_dependency_version GET "${dependency_lock}" obs prebuilt_version)
    set(obs_sources "${CMAKE_CURRENT_SOURCE_DIR}/.deps/${obs_source_directory}")
    set(obs_deps_root
      "${CMAKE_CURRENT_SOURCE_DIR}/.deps/obs-deps-${obs_dependency_version}-universal")
    set(MGL_OBS_APP "/Applications/OBS.app" CACHE PATH "The OBS.app the plugin links against")
    set(obs_frameworks "${MGL_OBS_APP}/Contents/Frameworks")
    # OBS generates this header in its own build; nothing a plugin reads depends on its values.
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/obs-generated/obsconfig.h" "#pragma once
")
    target_sources(meta-g-link-direct PRIVATE src/plugin/setup_process_posix.cpp)
    target_include_directories(meta-g-link-direct SYSTEM PRIVATE "${obs_sources}/libobs"
      "${obs_sources}/frontend/api" "${obs_sources}/plugins/obs-websocket/lib"
      "${obs_deps_root}/include" "${CMAKE_CURRENT_BINARY_DIR}/obs-generated")
    target_compile_options(meta-g-link-direct PRIVATE "-F${obs_frameworks}")
    target_link_options(meta-g-link-direct PRIVATE "-F${obs_frameworks}" "-Wl,-framework,libobs"
      "-Wl,-rpath,@executable_path/../Frameworks")
    target_link_libraries(meta-g-link-direct PRIVATE "${obs_frameworks}/libobs-frontend-api.1.dylib"
      "${obs_frameworks}/libavcodec.dylib" "${obs_frameworks}/libavutil.dylib")
    set_target_properties(meta-g-link-direct PROPERTIES BUNDLE TRUE BUNDLE_EXTENSION plugin
      SUFFIX "" C_VISIBILITY_PRESET hidden CXX_VISIBILITY_PRESET hidden
      VISIBILITY_INLINES_HIDDEN ON)
    # OBS looks for a bundled plugin's data under Resources.
    target_sources(meta-g-link-direct PRIVATE data/locale/en-US.ini)
    set_source_files_properties(data/locale/en-US.ini PROPERTIES
      MACOSX_PACKAGE_LOCATION Resources/locale)
  else()
    # The OBS this machine has provides libobs, its frontend API, its obs-websocket header and the
    # FFmpeg it runs on, so the plugin matches that OBS, whichever version it is.
    find_package(libobs REQUIRED CONFIG)
    if(libobs_VERSION AND libobs_VERSION VERSION_LESS 30)
      message(FATAL_ERROR "OBS Studio 32 or newer is required; found libobs ${libobs_VERSION}")
    endif()
    find_package(obs-frontend-api REQUIRED CONFIG)
    find_path(MGL_OBS_WEBSOCKET_INCLUDE obs-websocket-api.h
      PATH_SUFFIXES obs obs-websocket-api REQUIRED)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(MGL_FFMPEG REQUIRED IMPORTED_TARGET libavcodec libavutil)
    target_sources(meta-g-link-direct PRIVATE src/plugin/setup_process_posix.cpp)
    target_include_directories(meta-g-link-direct SYSTEM PRIVATE "${MGL_OBS_WEBSOCKET_INCLUDE}")
    target_link_libraries(meta-g-link-direct PRIVATE OBS::libobs OBS::obs-frontend-api
      PkgConfig::MGL_FFMPEG)
    # Hide everything but the module entry points, so the static mbedTLS cannot clash with OBS's.
    set_target_properties(meta-g-link-direct PROPERTIES C_VISIBILITY_PRESET hidden
      CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)
    target_link_options(meta-g-link-direct PRIVATE "LINKER:--exclude-libs,ALL" "LINKER:-z,defs")
  endif()
  target_link_libraries(meta-g-link-direct PRIVATE mgl_link)
  target_compile_definitions(meta-g-link-direct PRIVATE MGL_VERSION="${PROJECT_VERSION}")
  set_target_properties(meta-g-link-direct PROPERTIES PREFIX "")
  mgl_owned_target(meta-g-link-direct)
  install(TARGETS meta-g-link-direct RUNTIME DESTINATION obs-plugins/64bit LIBRARY DESTINATION obs-plugins/64bit)
  install(FILES LICENSE THIRD_PARTY_NOTICES.md DESTINATION data/obs-plugins/meta-g-link-direct)
  install(DIRECTORY data/locale DESTINATION data/obs-plugins/meta-g-link-direct)
  install(FILES README.md DESTINATION .)
endif()

if(BUILD_TESTING)
  add_executable(mgl-sdu-tests tests/sdu_tests.cpp)
  target_link_libraries(mgl-sdu-tests PRIVATE mgl_protocol)
  mgl_owned_target(mgl-sdu-tests)
  add_test(NAME protocol-prefix COMMAND mgl-sdu-tests)
  add_executable(mgl-address-tests tests/address_tests.cpp)
  target_link_libraries(mgl-address-tests PRIVATE mgl_protocol)
  mgl_owned_target(mgl-address-tests)
  add_test(NAME private-address COMMAND mgl-address-tests)

  add_executable(mgl-protocol-tests tests/protocol_tests.cpp)
  target_link_libraries(mgl-protocol-tests PRIVATE mgl_protocol)
  mgl_owned_target(mgl-protocol-tests)
  add_test(NAME protocol-datax COMMAND mgl-protocol-tests)

  add_executable(mgl-transport-tests tests/transport_tests.cpp)
  target_link_libraries(mgl-transport-tests PRIVATE mgl_protocol)
  mgl_owned_target(mgl-transport-tests)
  add_test(NAME buffered-transport COMMAND mgl-transport-tests)

  if(TARGET mgl_link)
    add_executable(mgl-crypto-tests tests/crypto_tests.cpp)
    target_link_libraries(mgl-crypto-tests PRIVATE mgl_link)
    mgl_owned_target(mgl-crypto-tests)
    add_test(NAME crypto-provider COMMAND mgl-crypto-tests)

    add_executable(mgl-setup-tests tests/setup_tests.cpp)
    target_link_libraries(mgl-setup-tests PRIVATE mgl_link)
    mgl_owned_target(mgl-setup-tests)
    add_test(NAME setup-login COMMAND mgl-setup-tests)
  endif()

  if(MGL_BUILD_OBS_PLUGIN)
    if(WIN32)
      add_executable(mgl-module-tests tests/module_tests.cpp)
      target_compile_definitions(mgl-module-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
      mgl_owned_target(mgl-module-tests)
      add_test(NAME module-abi COMMAND mgl-module-tests "$<TARGET_FILE:meta-g-link-direct>" "$ENV{MGL_OBS_RUNTIME}")
    endif()
  endif()
endif()
