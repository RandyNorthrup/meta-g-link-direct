cmake_minimum_required(VERSION 3.28...3.31)
project(meta_g_link_direct VERSION 0.1.0 LANGUAGES C CXX)

include(CTest)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(MGL_BUILD_OBS_PLUGIN "Build the native OBS module foundation" ${WIN32})
option(MGL_SANITIZERS "Instrument portable tests with supported sanitizers" OFF)

add_library(mgl_protocol STATIC
  src/protocol/sdu.cpp
  src/protocol/address.cpp
  src/protocol/protobuf.cpp
  src/protocol/datax.cpp
  src/protocol/json.cpp
  src/protocol/encoding.cpp
  src/protocol/stream.cpp
  src/protocol/hevc.cpp)
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

if(WIN32)
  add_library(mgl_windows STATIC
    src/windows/capabilities.cpp
    src/windows/discovery.cpp
    src/windows/classic.cpp
    src/windows/rfcomm.cpp
    src/windows/crypto.cpp
    src/windows/session.cpp
    src/windows/sdu_socket.cpp
    src/windows/rfcomm_transport.cpp
    src/windows/tcp_transport.cpp
    src/windows/http.cpp
    src/windows/identity.cpp
    src/windows/provision.cpp
    src/windows/wifi_station.cpp
    src/windows/media_session.cpp
    src/protocol/airshield.cpp)
  target_include_directories(mgl_windows PUBLIC include)
  target_compile_definitions(mgl_windows PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
  target_link_libraries(mgl_windows PUBLIC mgl_protocol PRIVATE wlanapi bthprops windowsapp ws2_32 bcrypt winhttp crypt32 shell32 ole32 iphlpapi)
  mgl_owned_target(mgl_windows)

  add_executable(meta-g-link-probe src/probe/main.cpp src/probe/private_input.cpp)
  target_link_libraries(meta-g-link-probe PRIVATE mgl_windows)
  mgl_owned_target(meta-g-link-probe)
endif()

if(MGL_BUILD_OBS_PLUGIN)
  if(NOT WIN32)
    message(FATAL_ERROR "The OBS module is currently supported only on Windows")
  endif()
  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/dependencies.lock.json" dependency_lock)
  string(JSON obs_source_directory GET "${dependency_lock}" obs directory)
  string(JSON obs_dependency_version GET "${dependency_lock}" obs prebuilt_version)
  set(obs_deps_root
    "${CMAKE_CURRENT_SOURCE_DIR}/.deps/${obs_source_directory}/.deps/obs-deps-${obs_dependency_version}-x64")
  list(APPEND CMAKE_PREFIX_PATH "${obs_deps_root}")
  find_package(libobs 32.2.2 EXACT REQUIRED CONFIG)
  # FFmpeg for HEVC decode; the matching runtime DLLs ship with OBS.
  find_library(MGL_AVCODEC NAMES avcodec PATHS "${obs_deps_root}/lib" REQUIRED)
  find_library(MGL_AVUTIL NAMES avutil PATHS "${obs_deps_root}/lib" REQUIRED)
  # libobs's _udiv128 header requires the MSVC C compiler; LLVM issue #59168. Every file that
  # includes OBS headers (module.c, source.c) is C17 with /analyze. The camera core and HEVC
  # decoder are C++20 behind src/plugin/camera.h, include no OBS headers, and are checked by
  # clang-tidy instead.
  add_library(meta-g-link-direct MODULE
    src/plugin/module.c
    src/plugin/source.c
    src/plugin/camera.cpp
    src/plugin/hevc_decoder.cpp)
  target_include_directories(meta-g-link-direct PRIVATE "${obs_deps_root}/include")
  target_link_libraries(meta-g-link-direct PRIVATE OBS::libobs mgl_windows
    "${MGL_AVCODEC}" "${MGL_AVUTIL}")
  set_source_files_properties(src/plugin/module.c src/plugin/source.c
    PROPERTIES COMPILE_OPTIONS "/analyze")
  target_compile_definitions(meta-g-link-direct PRIVATE MGL_VERSION="${PROJECT_VERSION}"
    WIN32_LEAN_AND_MEAN NOMINMAX)
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

  if(MGL_BUILD_OBS_PLUGIN)
    add_executable(mgl-module-tests tests/module_tests.cpp)
    target_compile_definitions(mgl-module-tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
    mgl_owned_target(mgl-module-tests)
    add_test(NAME module-abi COMMAND mgl-module-tests "$<TARGET_FILE:meta-g-link-direct>" "$ENV{MGL_OBS_RUNTIME}")
  endif()
endif()
