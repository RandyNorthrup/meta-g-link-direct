// SPDX-License-Identifier: AGPL-3.0-only
// meta-g-link-setup.exe, the glasses setup window. It explains the change of owner, shows the Meta
// login inside the window (Microsoft Edge WebView2 in private mode, with a browser profile deleted
// afterwards), recognises the end of the login by its redirect address so nothing has to be
// copied, and runs the change of owner. Where WebView2 is unavailable it says so and offers the
// login in the browser, with a box for the final address. Exit codes: 0 done, 3 the setup failed,
// 8 cancelled.
#include "mgl/encoding.hpp"
#include "mgl/provision.hpp"
#include "mgl/setup.hpp"
#include "mgl/win_text.hpp"

#include <windows.h> // IWYU pragma: keep — SDK umbrella prerequisites.

#include <EventToken.h>
#include <WebView2.h>
#include <basetsd.h>
#include <combaseapi.h>
#include <commctrl.h>
#include <intsafe.h> // FAILED, SUCCEEDED and S_OK, as this include chain defines them first
#include <minwindef.h>
#include <objbase.h>
#include <processthreadsapi.h>
#include <shellapi.h>
#include <winbase.h>
#include <windef.h>
#include <wingdi.h>
#include <winnt.h>
#include <winuser.h>
#include <wrl/client.h>
#include <wrl/event.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <type_traits>
#include <utility>

namespace {
using Microsoft::WRL::Callback;
using Microsoft::WRL::ComPtr;

constexpr int ExitDone = 0;
constexpr int ExitFailed = 3;
constexpr int ExitCancelled = 8;
constexpr UINT MessageSetupFinished = WM_APP + 1;
constexpr UINT MessageLoginUnavailable = WM_APP + 2;
constexpr int BaseDpi = 96;
constexpr int IdBrowser = 100;
constexpr int IdAddress = 101;

// Layout in 96-DPI pixels.
constexpr int Margin = 20;
constexpr int Gap = 12;
constexpr int ButtonWidth = 130;
constexpr int ButtonHeight = 32;
constexpr int EditHeight = 28;
constexpr int ProgressHeight = 18;
constexpr int StartWidth = 640;
constexpr int StartHeight = 760;
constexpr int MinimumWidth = 480;
constexpr int MinimumHeight = 520;
constexpr int HeadingScalePercent = 135;

enum class Screen : std::uint8_t { Intro, Login, Paste, Working, Done, Failed };

struct FontDeleter {
    void operator()(HFONT font) const noexcept {
        if (font != nullptr) {
            DeleteObject(font);
        }
    }
};
using UniqueFont = std::unique_ptr<std::remove_pointer_t<HFONT>, FontDeleter>;

// The Win32 message ABI passes pointers and handles as integers. These helpers hold its casts.
template <typename Target> Target* pointerFrom(LPARAM value) {
    return reinterpret_cast<Target*>(value); // NOLINT(*-reinterpret-cast,performance-no-int-to-ptr)
}
HDC contextFrom(WPARAM value) {
    return reinterpret_cast<HDC>(value); // NOLINT(*-reinterpret-cast,performance-no-int-to-ptr)
}
LONG_PTR longFrom(const void* pointer) {
    return reinterpret_cast<LONG_PTR>(pointer); // NOLINT(*-reinterpret-cast)
}
WPARAM wparamFrom(HGDIOBJ object) {
    return reinterpret_cast<WPARAM>(object); // NOLINT(*-reinterpret-cast)
}
LRESULT resultFrom(HBRUSH brush) {
    return reinterpret_cast<LRESULT>(brush); // NOLINT(*-reinterpret-cast)
}

// The control id as the HMENU CreateWindowExW takes for a child window.
HMENU controlId(int id) {
    const auto value = static_cast<INT_PTR>(id);
    return reinterpret_cast<HMENU>(value); // NOLINT(*-reinterpret-cast,performance-no-int-to-ptr)
}
std::wstring windowText(HWND control) {
    const int length = GetWindowTextLengthW(control);
    std::wstring text(static_cast<std::size_t>(length) + 1, L'\0');
    const int copied = GetWindowTextW(control, text.data(), length + 1);
    text.resize(static_cast<std::size_t>(copied > 0 ? copied : 0));
    return text;
}

class SetupWindow {
  public:
    SetupWindow() = default;
    SetupWindow(const SetupWindow&) = delete;
    SetupWindow& operator=(const SetupWindow&) = delete;
    SetupWindow(SetupWindow&&) = delete;
    SetupWindow& operator=(SetupWindow&&) = delete;
    ~SetupWindow() {
        if (worker_.joinable()) {
            worker_.join();
        }
    }

    int run(HINSTANCE instance, int show) {
        const wchar_t* const class_name = L"MetaGLinkDirectSetup";
        WNDCLASSEXW kind{};
        kind.cbSize = sizeof(kind);
        kind.lpfnWndProc = &SetupWindow::windowProc;
        kind.hInstance = instance;
        kind.hCursor = LoadCursorW(nullptr, IDC_ARROW); // NOLINT(misc-include-cleaner) — winuser.h
        kind.hIcon =
            LoadIconW(nullptr, IDI_APPLICATION); // NOLINT(misc-include-cleaner) — winuser.h
        kind.hbrBackground = GetSysColorBrush(COLOR_WINDOW);
        kind.lpszClassName = class_name;
        if (RegisterClassExW(&kind) == 0) {
            return ExitFailed;
        }
        window_ = CreateWindowExW(0, class_name, L"Set up Ray-Ban Meta glasses",
                                  WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, StartWidth,
                                  StartHeight, nullptr, nullptr, instance, this);
        if (window_ == nullptr) {
            return ExitFailed;
        }
        dpi_ = GetDpiForWindow(window_);
        SetWindowPos(window_, nullptr, 0, 0, scaled(StartWidth), scaled(StartHeight),
                     SWP_NOMOVE | SWP_NOZORDER);
        createControls();
        showScreen(Screen::Intro);
        ShowWindow(window_, show);
        SetForegroundWindow(window_);
        MSG message{};
        while (GetMessageW(&message, nullptr, 0, 0) > 0) {
            if (IsDialogMessageW(window_, &message) == FALSE) {
                TranslateMessage(&message);
                DispatchMessageW(&message);
            }
        }
        closeBrowser();
        return exit_code_;
    }

  private:
    static LRESULT CALLBACK windowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
        if (message == WM_NCCREATE) {
            // The handle is needed from the first message on, before CreateWindowExW returns.
            const auto* const create = pointerFrom<const CREATESTRUCTW>(lparam);
            auto* const created = static_cast<SetupWindow*>(create->lpCreateParams);
            created->window_ = window;
            SetWindowLongPtrW(window, GWLP_USERDATA, longFrom(created));
        }
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast,performance-no-int-to-ptr) —
        // Win32 ABI.
        auto* const self = pointerFrom<SetupWindow>(GetWindowLongPtrW(window, GWLP_USERDATA));
        if (self == nullptr) {
            return DefWindowProcW(window, message, wparam, lparam);
        }
        try {
            return self->handle(message, wparam, lparam);
        } catch (const std::exception& error) {
            self->fail(std::string("The setup window hit an internal error: ") + error.what());
            return 0;
        }
    }

    LRESULT handle(UINT message, WPARAM wparam, LPARAM lparam) {
        switch (message) {
        case WM_COMMAND:
            onCommand(LOWORD(wparam));
            return 0;
        case WM_SIZE:
            layout();
            return 0;
        case WM_GETMINMAXINFO: {
            auto* const limits = pointerFrom<MINMAXINFO>(lparam);
            limits->ptMinTrackSize = {.x = scaled(MinimumWidth), .y = scaled(MinimumHeight)};
            return 0;
        }
        case WM_DPICHANGED: {
            dpi_ = HIWORD(wparam);
            const auto* const suggested = pointerFrom<const RECT>(lparam);
            applyFonts();
            SetWindowPos(window_, nullptr, suggested->left, suggested->top,
                         suggested->right - suggested->left, suggested->bottom - suggested->top,
                         SWP_NOZORDER | SWP_NOACTIVATE);
            return 0;
        }
        case WM_CTLCOLORSTATIC: {
            auto* const context = contextFrom(wparam);
            SetTextColor(context, GetSysColor(COLOR_WINDOWTEXT));
            SetBkColor(context, GetSysColor(COLOR_WINDOW));
            return resultFrom(GetSysColorBrush(COLOR_WINDOW));
        }
        case MessageSetupFinished:
            onFinished();
            return 0;
        case MessageLoginUnavailable:
            showPaste();
            return 0;
        case WM_CLOSE:
            // A change of owner must not be cut off half way.
            if (screen_ == Screen::Working) {
                MessageBeep(MB_ICONWARNING);
                return 0;
            }
            DestroyWindow(window_);
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProcW(window_, message, wparam, lparam);
        }
    }

    [[nodiscard]] int scaled(int value) const {
        return MulDiv(value, static_cast<int>(dpi_), BaseDpi);
    }

    HWND addControl(const wchar_t* kind, DWORD style, int id) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast,performance-no-int-to-ptr) —
        // Win32 ABI.
        auto* const instance = pointerFrom<std::remove_pointer_t<HINSTANCE>>(
            GetWindowLongPtrW(window_, GWLP_HINSTANCE));
        return CreateWindowExW(0, kind, L"", WS_CHILD | style, 0, 0, 0, 0, window_, controlId(id),
                               instance, nullptr);
    }

    // Created in tab order: the address box, then the browser button, then the main buttons.
    void createControls() {
        heading_ = addControl(WC_STATICW, WS_VISIBLE | SS_LEFT | SS_NOPREFIX, 0);
        body_ = addControl(WC_STATICW, WS_VISIBLE | SS_LEFT | SS_NOPREFIX, 0);
        address_ = addControl(WC_EDITW, WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL, IdAddress);
        browser_ = addControl(WC_BUTTONW, WS_TABSTOP | BS_PUSHBUTTON, IdBrowser);
        progress_ = addControl(PROGRESS_CLASSW, PBS_MARQUEE, 0);
        primary_ = addControl(WC_BUTTONW, WS_TABSTOP | BS_DEFPUSHBUTTON, IDOK);
        secondary_ = addControl(WC_BUTTONW, WS_TABSTOP | BS_PUSHBUTTON, IDCANCEL);
        SetWindowTextW(browser_, L"Open the login in your browser");
        applyFonts();
    }

    void applyFonts() {
        NONCLIENTMETRICSW metrics{};
        metrics.cbSize = sizeof(metrics);
        if (SystemParametersInfoForDpi(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0,
                                       dpi_) == FALSE) {
            return; // the controls keep the font they have
        }
        const LOGFONTW body = metrics.lfMessageFont;
        LOGFONTW heading = body;
        heading.lfHeight = MulDiv(heading.lfHeight, HeadingScalePercent, 100);
        heading.lfWeight = FW_SEMIBOLD;
        font_.reset(CreateFontIndirectW(&body));
        heading_font_.reset(CreateFontIndirectW(&heading));
        for (HWND control : {body_, address_, browser_, primary_, secondary_}) {
            SendMessageW(control, WM_SETFONT, wparamFrom(font_.get()), TRUE);
        }
        SendMessageW(heading_, WM_SETFONT, wparamFrom(heading_font_.get()), TRUE);
        layout();
    }

    // The height text needs at width in control's font.
    [[nodiscard]] int textHeight(HWND control, HFONT font, int width) const {
        const std::wstring text = windowText(control);
        HDC context = GetDC(window_);
        auto* const previous = SelectObject(context, font);
        RECT bounds{.left = 0, .top = 0, .right = width, .bottom = 0};
        DrawTextW(context, text.c_str(), static_cast<int>(text.size()), &bounds,
                  DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX);
        SelectObject(context, previous);
        ReleaseDC(window_, context);
        return bounds.bottom - bounds.top;
    }

    void layout() {
        if (window_ == nullptr || heading_ == nullptr) {
            return;
        }
        RECT client{};
        GetClientRect(window_, &client);
        const int margin = scaled(Margin);
        const int gap = scaled(Gap);
        const int width = client.right - (2 * margin);
        int top = margin;
        const int heading_height = textHeight(heading_, heading_font_.get(), width);
        MoveWindow(heading_, margin, top, width, heading_height, TRUE);
        top += heading_height + gap;
        const int body_height = textHeight(body_, font_.get(), width);
        MoveWindow(body_, margin, top, width, body_height, TRUE);
        top += body_height + gap;
        if (screen_ == Screen::Paste) {
            MoveWindow(browser_, margin, top, scaled(ButtonWidth * 2), scaled(ButtonHeight), TRUE);
            top += scaled(ButtonHeight) + gap;
            MoveWindow(address_, margin, top, width, scaled(EditHeight), TRUE);
            top += scaled(EditHeight) + gap;
        }
        if (screen_ == Screen::Working) {
            MoveWindow(progress_, margin, top, width, scaled(ProgressHeight), TRUE);
        }
        const int button_top = client.bottom - margin - scaled(ButtonHeight);
        const int button_width = scaled(ButtonWidth);
        MoveWindow(secondary_, client.right - margin - button_width, button_top, button_width,
                   scaled(ButtonHeight), TRUE);
        const int primary_left = IsWindowVisible(secondary_) != FALSE
                                     ? client.right - margin - (2 * button_width) - gap
                                     : client.right - margin - button_width;
        MoveWindow(primary_, primary_left, button_top, button_width, scaled(ButtonHeight), TRUE);
        if (controller_ != nullptr) {
            const RECT login{.left = margin,
                             .top = top,
                             .right = client.right - margin,
                             .bottom = button_top - gap};
            controller_->put_Bounds(login);
        }
    }

    void setButtons(const wchar_t* primary, const wchar_t* secondary) {
        ShowWindow(primary_, primary != nullptr ? SW_SHOW : SW_HIDE);
        ShowWindow(secondary_, secondary != nullptr ? SW_SHOW : SW_HIDE);
        if (primary != nullptr) {
            SetWindowTextW(primary_, primary);
        }
        if (secondary != nullptr) {
            SetWindowTextW(secondary_, secondary);
        }
    }

    void showScreen(Screen screen, const std::wstring& detail = {}) {
        screen_ = screen;
        ShowWindow(address_, screen == Screen::Paste ? SW_SHOW : SW_HIDE);
        ShowWindow(browser_, screen == Screen::Paste ? SW_SHOW : SW_HIDE);
        ShowWindow(progress_, screen == Screen::Working ? SW_SHOW : SW_HIDE);
        SendMessageW(progress_, PBM_SETMARQUEE, screen == Screen::Working ? TRUE : FALSE, 0);
        switch (screen) {
        case Screen::Intro:
            SetWindowTextW(heading_, L"Set up your Ray-Ban Meta glasses");
            SetWindowTextW(
                body_,
                L"This makes THIS PC the owner of your glasses. The Meta AI phone app, or another "
                L"PC, loses them until they are removed and added again there.\r\n\r\n"
                L"Before you continue:\r\n"
                L"•  Pair the glasses with this PC in Windows Settings > Bluetooth & devices.\r\n"
                L"•  Turn off Bluetooth on your phone.\r\n"
                L"•  Keep the glasses charged and out of the case.\r\n\r\n"
                L"Next you log in to Meta in this window. Nothing needs to be copied.");
            setButtons(L"Continue", L"Cancel");
            break;
        case Screen::Login:
            SetWindowTextW(heading_, L"Log in to Meta");
            SetWindowTextW(body_, L"Log in below with your Meta account. This window carries on "
                                  L"by itself when the login is done.");
            setButtons(nullptr, L"Cancel");
            break;
        case Screen::Paste:
            SetWindowTextW(heading_, L"Log in to Meta in your browser");
            SetWindowTextW(
                body_,
                L"This window cannot show the Meta login because Microsoft Edge WebView2 is not "
                L"available on this PC.\r\n\r\n"
                L"1.  Select Open the login in your browser, and log in.\r\n"
                L"2.  When the login has finished, copy the whole address from the browser's "
                L"address bar.\r\n"
                L"3.  Paste it in the box below and select Continue.");
            setButtons(L"Continue", L"Cancel");
            break;
        case Screen::Working:
            SetWindowTextW(heading_, L"Setting up the glasses");
            SetWindowTextW(body_, L"Changing the glasses' owner to this PC. This can take a "
                                  L"minute; keep the glasses nearby.");
            setButtons(nullptr, nullptr);
            break;
        case Screen::Done:
            SetWindowTextW(heading_, L"Done");
            SetWindowTextW(body_, L"This PC now owns the glasses. In OBS they connect when a "
                                  L"glasses source goes live.");
            setButtons(L"Close", nullptr);
            break;
        case Screen::Failed:
            SetWindowTextW(heading_, L"The setup did not finish");
            SetWindowTextW(body_, detail.c_str());
            setButtons(L"Try again", L"Close");
            break;
        }
        layout();
        // Narrator reads the new screen from its heading.
        SetFocus(IsWindowVisible(primary_) != FALSE ? primary_ : secondary_);
    }

    void onCommand(int id) {
        switch (id) {
        case IDOK:
            if (screen_ == Screen::Intro || screen_ == Screen::Failed) {
                startLogin();
            } else if (screen_ == Screen::Paste) {
                useAddress(mgl::narrowUtf8(windowText(address_)));
            } else if (screen_ == Screen::Done) {
                exit_code_ = ExitDone;
                DestroyWindow(window_);
            }
            break;
        case IDCANCEL:
            if (screen_ == Screen::Working) {
                break;
            }
            if (screen_ == Screen::Done) {
                exit_code_ = ExitDone;
            } else if (screen_ == Screen::Failed) {
                exit_code_ = ExitFailed;
            } else {
                exit_code_ = ExitCancelled;
            }
            DestroyWindow(window_);
            break;
        case IdBrowser:
            ShellExecuteW(window_, L"open", mgl::widenUtf8(request_.url).c_str(), nullptr, nullptr,
                          SW_SHOWNORMAL);
            break;
        default:
            break;
        }
    }

    void fail(const std::string& problem) {
        closeBrowser();
        exit_code_ = ExitFailed;
        showScreen(Screen::Failed, mgl::widenUtf8(problem));
    }

    void showPaste() {
        closeBrowser();
        SetWindowTextW(address_, L"");
        showScreen(Screen::Paste);
    }

    // A browser profile of this run only, removed afterwards.
    [[nodiscard]] static std::filesystem::path browserFolder() {
        std::error_code error;
        auto base = std::filesystem::temp_directory_path(error);
        if (error) {
            return {};
        }
        return base / (L"MetaGLinkDirect-setup-" + std::to_wstring(GetCurrentProcessId()));
    }

    void startLogin() {
        request_ = mgl::provision::authorizeRequest();
        showScreen(Screen::Login);
        browser_data_ = browserFolder();
        const HRESULT started = CreateCoreWebView2EnvironmentWithOptions(
            nullptr, browser_data_.empty() ? nullptr : browser_data_.c_str(), nullptr,
            Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
                [this](HRESULT result, ICoreWebView2Environment* environment) -> HRESULT {
                    return onEnvironment(result, environment);
                })
                .Get());
        if (FAILED(started)) {
            showPaste();
        }
    }

    HRESULT onEnvironment(HRESULT result, ICoreWebView2Environment* environment) {
        ComPtr<ICoreWebView2Environment10> private_capable;
        if (FAILED(result) || environment == nullptr ||
            FAILED(ComPtr<ICoreWebView2Environment>(environment).As(&private_capable))) {
            PostMessageW(window_, MessageLoginUnavailable, 0, 0);
            return S_OK;
        }
        environment_ = environment;
        ComPtr<ICoreWebView2ControllerOptions> options;
        if (FAILED(private_capable->CreateCoreWebView2ControllerOptions(&options)) ||
            FAILED(options->put_IsInPrivateModeEnabled(TRUE))) {
            PostMessageW(window_, MessageLoginUnavailable, 0, 0);
            return S_OK;
        }
        const HRESULT created = private_capable->CreateCoreWebView2ControllerWithOptions(
            window_, options.Get(),
            Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                [this](HRESULT outcome, ICoreWebView2Controller* controller) -> HRESULT {
                    if (FAILED(outcome) || controller == nullptr) {
                        PostMessageW(window_, MessageLoginUnavailable, 0, 0);
                        return S_OK;
                    }
                    return onController(controller);
                })
                .Get());
        if (FAILED(created)) {
            PostMessageW(window_, MessageLoginUnavailable, 0, 0);
        }
        return S_OK;
    }

    HRESULT onController(ICoreWebView2Controller* controller) {
        controller_ = controller;
        if (FAILED(controller_->get_CoreWebView2(&webview_)) || webview_ == nullptr) {
            PostMessageW(window_, MessageLoginUnavailable, 0, 0);
            return S_OK;
        }
        EventRegistrationToken token{};
        // The login ends by sending the browser to the redirect address; it carries the code.
        webview_->add_NavigationStarting(
            Callback<ICoreWebView2NavigationStartingEventHandler>(
                [this](ICoreWebView2* /*sender*/,
                       ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT {
                    LPWSTR raw = nullptr;
                    if (FAILED(args->get_Uri(&raw)) || raw == nullptr) {
                        return S_OK;
                    }
                    const std::string address = mgl::narrowUtf8(raw);
                    CoTaskMemFree(raw);
                    if (address.starts_with(mgl::provision::LoginRedirectUri)) {
                        args->put_Cancel(TRUE);
                        loginFinished(address);
                    }
                    return S_OK;
                })
                .Get(),
            &token);
        // Pages that open a new window (help, a second factor) stay in this one.
        webview_->add_NewWindowRequested(
            Callback<ICoreWebView2NewWindowRequestedEventHandler>(
                [this](ICoreWebView2* /*sender*/,
                       ICoreWebView2NewWindowRequestedEventArgs* args) -> HRESULT {
                    LPWSTR raw = nullptr;
                    if (SUCCEEDED(args->get_Uri(&raw)) && raw != nullptr) {
                        webview_->Navigate(raw);
                        CoTaskMemFree(raw);
                    }
                    args->put_Handled(TRUE);
                    return S_OK;
                })
                .Get(),
            &token);
        layout();
        controller_->put_IsVisible(TRUE);
        webview_->Navigate(mgl::widenUtf8(request_.url).c_str());
        return S_OK;
    }

    void loginFinished(const std::string& address) {
        closeBrowser();
        useAddress(address);
    }

    // Checks a finished login's address and starts the change of owner with its code.
    void useAddress(const std::string& address) {
        const auto state = mgl::encoding::queryParameter(address, "state");
        if (state && *state != request_.state) {
            fail("That address belongs to a different login. Select Try again and log in again.");
            return;
        }
        const auto code = mgl::encoding::queryParameter(address, "code");
        if (!code) {
            fail("The login did not return a code. Select Try again and log in again.");
            return;
        }
        showScreen(Screen::Working);
        if (worker_.joinable()) {
            worker_.join();
        }
        worker_ = std::thread([this, auth_code = *code] {
            mgl::setup::Outcome outcome;
            try {
                outcome = mgl::setup::changeOwner(auth_code);
            } catch (const std::exception& error) {
                outcome.problem = std::string("The setup hit an internal error: ") + error.what();
            }
            {
                const std::scoped_lock lock(result_mutex_);
                result_ = std::move(outcome);
            }
            PostMessageW(window_, MessageSetupFinished, 0, 0);
        });
    }

    void onFinished() {
        if (worker_.joinable()) {
            worker_.join();
        }
        mgl::setup::Outcome outcome;
        {
            const std::scoped_lock lock(result_mutex_);
            outcome = result_;
        }
        if (outcome.ok) {
            exit_code_ = ExitDone;
            showScreen(Screen::Done);
        } else {
            fail(outcome.problem);
        }
    }

    // Closes the login view and removes this run's browser profile. WebView2's helper processes
    // can hold the folder for a moment after the view closes, so removal is retried briefly; a
    // folder left behind holds no Meta cookies, because the view runs in private mode.
    void closeBrowser() {
        if (controller_ != nullptr) {
            controller_->Close();
        }
        webview_.Reset();
        controller_.Reset();
        environment_.Reset();
        if (browser_data_.empty()) {
            return;
        }
        constexpr int Attempts = 10;
        constexpr auto Pause = std::chrono::milliseconds(200);
        for (int attempt = 0; attempt < Attempts; ++attempt) {
            std::error_code error;
            std::filesystem::remove_all(browser_data_, error);
            if (!error) {
                break;
            }
            std::this_thread::sleep_for(Pause);
        }
        browser_data_.clear();
    }

    HWND window_ = nullptr;
    HWND heading_ = nullptr;
    HWND body_ = nullptr;
    HWND address_ = nullptr;
    HWND browser_ = nullptr;
    HWND progress_ = nullptr;
    HWND primary_ = nullptr;
    HWND secondary_ = nullptr;
    UniqueFont font_;
    UniqueFont heading_font_;
    UINT dpi_ = BaseDpi;
    Screen screen_ = Screen::Intro;
    int exit_code_ = ExitCancelled;
    mgl::provision::Authorize request_;
    ComPtr<ICoreWebView2Environment> environment_;
    ComPtr<ICoreWebView2Controller> controller_;
    ComPtr<ICoreWebView2> webview_;
    std::filesystem::path browser_data_;
    std::thread worker_;
    std::mutex result_mutex_;
    mgl::setup::Outcome result_;
};
} // namespace

// Parameter names as winbase.h declares them.
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPWSTR /*lpCmdLine*/,
                    int nShowCmd) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    if (FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED))) {
        return ExitFailed;
    }
    const INITCOMMONCONTROLSEX controls{.dwSize = sizeof(INITCOMMONCONTROLSEX),
                                        .dwICC = ICC_PROGRESS_CLASS};
    InitCommonControlsEx(&controls);
    int exit_code = ExitFailed;
    try {
        SetupWindow window;
        exit_code = window.run(hInstance, nShowCmd);
    } catch (const std::exception&) {
        exit_code = ExitFailed; // nothing can be shown once the window is gone
    }
    CoUninitialize();
    return exit_code;
}
