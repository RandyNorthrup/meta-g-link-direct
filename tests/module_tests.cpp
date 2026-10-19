// SPDX-License-Identifier: AGPL-3.0-only
#include <windows.h> // IWYU pragma: keep - required Windows API umbrella prerequisites.

#include <errhandlingapi.h>
#include <libloaderapi.h>
#include <minwindef.h>

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string_view>

namespace {
struct LibraryCloser {
    void operator()(HINSTANCE__* library) const noexcept { FreeLibrary(library); }
};
int runModuleTests(int argument_count, char** arguments) {
    constexpr int ExpectedArguments = 3;
    if (argument_count != ExpectedArguments || arguments[2][0] == '\0') {
        std::cerr << "Module ABI test requires plugin path and MGL_OBS_RUNTIME directory\n";
        return EXIT_FAILURE;
    }
    const auto runtime = std::filesystem::path(arguments[2]) / "obs.dll";
    constexpr DWORD SearchFlags =
        LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS;
    const std::unique_ptr<HINSTANCE__, LibraryCloser> obs(
        LoadLibraryExW(runtime.c_str(), nullptr, SearchFlags));
    if (!obs) {
        std::cerr << "Real OBS runtime could not load; Windows error " << GetLastError() << '\n';
        return EXIT_FAILURE;
    }
    const auto path = std::filesystem::absolute(arguments[1]);
    const std::unique_ptr<HINSTANCE__, LibraryCloser> plugin(
        LoadLibraryExW(path.c_str(), nullptr, SearchFlags));
    if (!plugin) {
        std::cerr << "Plugin DLL could not load; Windows error " << GetLastError() << '\n';
        return EXIT_FAILURE;
    }
    constexpr std::array RequiredExports{"obs_module_load",        "obs_module_set_pointer",
                                         "obs_module_ver",         "obs_module_name",
                                         "obs_module_description", "obs_module_author"};
    const auto missing = std::ranges::find_if(RequiredExports, [&plugin](const char* name) {
        return GetProcAddress(plugin.get(), name) == nullptr;
    });
    if (missing != RequiredExports.end()) {
        std::cerr << "Required OBS export missing: " << *missing << '\n';
        return EXIT_FAILURE;
    }
    using Metadata = const char* (*)();
    // GetProcAddress is callable only with the declared exported function's exact signature.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) - Verified Windows ABI signature.
    const auto name = reinterpret_cast<Metadata>(GetProcAddress(plugin.get(), "obs_module_name"));
    const auto description =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<Metadata>(GetProcAddress(plugin.get(), "obs_module_description"));
    if (std::string_view(name()) != "Meta G Link Direct" ||
        std::string_view(description()).find("camera source") == std::string_view::npos) {
        std::cerr << "OBS module metadata does not match the camera-source contract\n";
        return EXIT_FAILURE;
    }
    std::cout << "Real OBS runtime, module DLL and six required exports verified\n";
    return EXIT_SUCCESS;
}

} // namespace

int main(int argument_count, char** arguments) {
    try {
        return runModuleTests(argument_count, arguments);
    } catch (const std::exception& error) {
        constexpr int DiagnosticWriteFailed = 4;
        if (std::fputs(error.what(), stderr) == EOF || std::fputc('\n', stderr) == EOF) {
            return DiagnosticWriteFailed;
        }
        return EXIT_FAILURE;
    }
}
