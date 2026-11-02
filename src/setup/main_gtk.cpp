// SPDX-License-Identifier: AGPL-3.0-only
// meta-g-link-setup on Linux, the glasses setup window, with the same screens and exit codes as the
// Windows one (src/setup/main.cpp). It explains the change of owner, shows the Meta login inside
// the window where WebKitGTK is available (an ephemeral session, so nothing stays afterwards),
// recognises the end of the login by its redirect address, and runs the change of owner. Without
// WebKitGTK, as inside the OBS Flatpak, it offers the login in the browser with a box for the final
// address. Exit codes: 0 done, 3 the setup failed, 8 cancelled.
#include "mgl/provision.hpp"
#include "mgl/setup.hpp"

#include <gtk/gtk.h>
#ifdef MGL_HAVE_WEBKIT
#include <webkit2/webkit2.h>
#endif

#include <cstdint>
#include <exception>
#include <memory>
#include <string>

namespace {
constexpr int ExitDone = 0;
constexpr int ExitFailed = 3;
constexpr int ExitCancelled = 8;
constexpr int Margin = 20;
constexpr int Gap = 12;
constexpr int StartWidth = 640;
constexpr int StartHeight = 760;
constexpr int MinimumWidth = 480;
constexpr int MinimumHeight = 520;

enum class Screen : std::uint8_t { Intro, Login, Paste, Working, Done, Failed };

class SetupWindow {
  public:
    explicit SetupWindow(GtkApplication* application) {
        window_ = GTK_WINDOW(gtk_application_window_new(application));
        gtk_window_set_title(window_, "Set up Ray-Ban Meta glasses");
        gtk_window_set_default_size(window_, StartWidth, StartHeight);
        GdkGeometry minimum{};
        minimum.min_width = MinimumWidth;
        minimum.min_height = MinimumHeight;
        gtk_window_set_geometry_hints(window_, nullptr, &minimum, GDK_HINT_MIN_SIZE);
        g_signal_connect(window_, "delete-event", G_CALLBACK(&SetupWindow::onDelete), this);

        auto* const column = gtk_box_new(GTK_ORIENTATION_VERTICAL, Gap);
        gtk_container_set_border_width(GTK_CONTAINER(column), Margin);
        gtk_container_add(GTK_CONTAINER(window_), column);

        heading_ = GTK_LABEL(gtk_label_new(nullptr));
        gtk_label_set_xalign(heading_, 0);
        gtk_box_pack_start(GTK_BOX(column), GTK_WIDGET(heading_), FALSE, FALSE, 0);
        body_ = GTK_LABEL(gtk_label_new(nullptr));
        gtk_label_set_xalign(body_, 0);
        gtk_label_set_line_wrap(body_, TRUE);
        gtk_label_set_selectable(body_, TRUE);
        gtk_box_pack_start(GTK_BOX(column), GTK_WIDGET(body_), FALSE, FALSE, 0);

        browser_ = gtk_button_new_with_mnemonic("_Open the login in your browser");
        gtk_widget_set_halign(browser_, GTK_ALIGN_START);
        g_signal_connect(browser_, "clicked", G_CALLBACK(&SetupWindow::onBrowser), this);
        gtk_box_pack_start(GTK_BOX(column), browser_, FALSE, FALSE, 0);
        // The label names the box for keyboard and screen-reader users.
        address_label_ = gtk_label_new_with_mnemonic("_Address after the login:");
        gtk_label_set_xalign(GTK_LABEL(address_label_), 0);
        gtk_box_pack_start(GTK_BOX(column), address_label_, FALSE, FALSE, 0);
        address_ = gtk_entry_new();
        gtk_entry_set_activates_default(GTK_ENTRY(address_), TRUE);
        gtk_label_set_mnemonic_widget(GTK_LABEL(address_label_), address_);
        gtk_box_pack_start(GTK_BOX(column), address_, FALSE, FALSE, 0);
        progress_ = gtk_spinner_new();
        gtk_box_pack_start(GTK_BOX(column), progress_, FALSE, FALSE, 0);

        login_area_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_box_pack_start(GTK_BOX(column), login_area_, TRUE, TRUE, 0);

        auto* const buttons = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
        gtk_button_box_set_layout(GTK_BUTTON_BOX(buttons), GTK_BUTTONBOX_END);
        gtk_box_set_spacing(GTK_BOX(buttons), Gap);
        primary_ = gtk_button_new_with_mnemonic("_Continue");
        secondary_ = gtk_button_new_with_mnemonic("C_ancel");
        gtk_widget_set_can_default(primary_, TRUE);
        g_signal_connect(primary_, "clicked", G_CALLBACK(&SetupWindow::onPrimary), this);
        g_signal_connect(secondary_, "clicked", G_CALLBACK(&SetupWindow::onSecondary), this);
        gtk_container_add(GTK_CONTAINER(buttons), primary_);
        gtk_container_add(GTK_CONTAINER(buttons), secondary_);
        gtk_box_pack_end(GTK_BOX(column), buttons, FALSE, FALSE, 0);
        g_signal_connect(window_, "key-press-event", G_CALLBACK(&SetupWindow::onKey), this);

        gtk_widget_show_all(GTK_WIDGET(window_));
        showScreen(Screen::Intro);
    }
    SetupWindow(const SetupWindow&) = delete;
    SetupWindow& operator=(const SetupWindow&) = delete;
    SetupWindow(SetupWindow&&) = delete;
    SetupWindow& operator=(SetupWindow&&) = delete;
    ~SetupWindow() = default;

    [[nodiscard]] int exitCode() const noexcept { return exit_code_; }

  private:
    static gboolean onDelete(GtkWidget* /*widget*/, GdkEvent* /*event*/, gpointer self) {
        // A change of owner must not be cut off half way.
        return static_cast<SetupWindow*>(self)->screen_ == Screen::Working ? TRUE : FALSE;
    }
    static gboolean onKey(GtkWidget* /*widget*/, GdkEventKey* event, gpointer self) {
        if (event->keyval == GDK_KEY_Escape) {
            static_cast<SetupWindow*>(self)->secondary();
            return TRUE;
        }
        return FALSE;
    }
    static void onPrimary(GtkButton* /*button*/, gpointer self) {
        static_cast<SetupWindow*>(self)->primary();
    }
    static void onSecondary(GtkButton* /*button*/, gpointer self) {
        static_cast<SetupWindow*>(self)->secondary();
    }
    static void onBrowser(GtkButton* /*button*/, gpointer self) {
        auto* const window = static_cast<SetupWindow*>(self);
        GError* error = nullptr;
        if (gtk_show_uri_on_window(window->window_, window->request_.url.c_str(), GDK_CURRENT_TIME,
                                   &error) == FALSE) {
            g_clear_error(&error);
            gtk_label_set_text(
                window->body_,
                "This computer could not open a browser. Copy the address from "
                "--auth-url of the probe tool, or install a browser, and try again.");
        }
    }

    void setButtons(const char* primary, const char* secondary) {
        gtk_widget_set_visible(primary_, primary != nullptr ? TRUE : FALSE);
        gtk_widget_set_visible(secondary_, secondary != nullptr ? TRUE : FALSE);
        if (primary != nullptr) {
            gtk_button_set_label(GTK_BUTTON(primary_), primary);
            gtk_button_set_use_underline(GTK_BUTTON(primary_), TRUE);
        }
        if (secondary != nullptr) {
            gtk_button_set_label(GTK_BUTTON(secondary_), secondary);
            gtk_button_set_use_underline(GTK_BUTTON(secondary_), TRUE);
        }
    }

    void setHeading(const char* text) {
        char* const markup = g_markup_printf_escaped("<big><b>%s</b></big>", text);
        gtk_label_set_markup(heading_, markup);
        g_free(markup);
    }

    void showScreen(Screen screen, const std::string& detail = {}) {
        screen_ = screen;
        const bool paste = screen == Screen::Paste;
        gtk_widget_set_visible(browser_, paste ? TRUE : FALSE);
        gtk_widget_set_visible(address_label_, paste ? TRUE : FALSE);
        gtk_widget_set_visible(address_, paste ? TRUE : FALSE);
        gtk_widget_set_visible(progress_, screen == Screen::Working ? TRUE : FALSE);
        gtk_widget_set_visible(login_area_, screen == Screen::Login ? TRUE : FALSE);
        if (screen == Screen::Working) {
            gtk_spinner_start(GTK_SPINNER(progress_));
        } else {
            gtk_spinner_stop(GTK_SPINNER(progress_));
        }
        switch (screen) {
        case Screen::Intro:
            setHeading("Set up your Ray-Ban Meta glasses");
            gtk_label_set_text(
                body_, "This makes THIS computer the owner of your glasses. The Meta AI phone app, "
                       "or another computer, loses them until they are removed and added again "
                       "there.\n\nBefore you continue:\n"
                       "•  Pair the glasses with this computer in your Bluetooth settings.\n"
                       "•  Turn off Bluetooth on your phone.\n"
                       "•  Keep the glasses charged and out of the case.\n\n"
                       "Next you log in to Meta. Nothing else needs to be copied.");
            setButtons("_Continue", "C_ancel");
            break;
        case Screen::Login:
            setHeading("Log in to Meta");
            gtk_label_set_text(body_, "Log in below with your Meta account. This window carries on "
                                      "by itself when the login is done.");
            setButtons(nullptr, "C_ancel");
            break;
        case Screen::Paste:
            setHeading("Log in to Meta in your browser");
            gtk_label_set_text(body_, "1.  Select Open the login in your browser, and log in.\n"
                                      "2.  When the login has finished, copy the whole address "
                                      "from the browser's address bar.\n"
                                      "3.  Paste it in the box below and select Continue.");
            setButtons("_Continue", "C_ancel");
            break;
        case Screen::Working:
            setHeading("Setting up the glasses");
            gtk_label_set_text(body_, "Changing the glasses' owner to this computer. This can take "
                                      "a minute; keep the glasses nearby.");
            setButtons(nullptr, nullptr);
            break;
        case Screen::Done:
            setHeading("Done");
            gtk_label_set_text(body_, "This computer now owns the glasses. In OBS they connect "
                                      "when a glasses source goes live.");
            setButtons("_Close", nullptr);
            break;
        case Screen::Failed:
            setHeading("The setup did not finish");
            gtk_label_set_text(body_, detail.c_str());
            setButtons("_Try again", "_Close");
            break;
        }
        // The browser screen starts at its first step; the others at their main button.
        GtkWidget* const first = paste                                       ? browser_
                                 : gtk_widget_get_visible(primary_) != FALSE ? primary_
                                                                             : secondary_;
        gtk_widget_grab_focus(first);
        if (gtk_widget_get_visible(primary_) != FALSE) {
            gtk_widget_grab_default(primary_);
        }
    }

    void primary() {
        switch (screen_) {
        case Screen::Intro:
        case Screen::Failed:
            startLogin();
            break;
        case Screen::Paste:
            useAddress(gtk_entry_get_text(GTK_ENTRY(address_)));
            break;
        case Screen::Done:
            exit_code_ = ExitDone;
            gtk_window_close(window_);
            break;
        case Screen::Login:
        case Screen::Working:
            break;
        }
    }

    void secondary() {
        if (screen_ == Screen::Working) {
            return;
        }
        if (screen_ == Screen::Failed) {
            exit_code_ = ExitFailed;
        } else if (screen_ == Screen::Done) {
            exit_code_ = ExitDone;
        } else {
            exit_code_ = ExitCancelled;
        }
        gtk_window_close(window_);
    }

    void startLogin() {
        request_ = mgl::provision::authorizeRequest();
#ifdef MGL_HAVE_WEBKIT
        showScreen(Screen::Login);
        // An ephemeral context keeps cookies and storage in memory, so the login leaves nothing.
        WebKitWebContext* const context = webkit_web_context_new_ephemeral();
        view_ = WEBKIT_WEB_VIEW(webkit_web_view_new_with_context(context));
        g_object_unref(context);
        g_signal_connect(view_, "decide-policy", G_CALLBACK(&SetupWindow::onPolicy), this);
        gtk_box_pack_start(GTK_BOX(login_area_), GTK_WIDGET(view_), TRUE, TRUE, 0);
        gtk_widget_show(GTK_WIDGET(view_));
        webkit_web_view_load_uri(view_, request_.url.c_str());
        gtk_widget_grab_focus(GTK_WIDGET(view_));
#else
        gtk_entry_set_text(GTK_ENTRY(address_), "");
        showScreen(Screen::Paste);
#endif
    }

#ifdef MGL_HAVE_WEBKIT
    // The login ends by sending the browser to the redirect address; it carries the code. Pages
    // that open a new window stay in this one.
    static gboolean onPolicy(WebKitWebView* view, WebKitPolicyDecision* decision,
                             WebKitPolicyDecisionType type, gpointer self) {
        if (type == WEBKIT_POLICY_DECISION_TYPE_NEW_WINDOW_ACTION) {
            auto* const navigation = WEBKIT_NAVIGATION_POLICY_DECISION(decision);
            auto* const action =
                webkit_navigation_policy_decision_get_navigation_action(navigation);
            webkit_web_view_load_request(view, webkit_navigation_action_get_request(action));
            webkit_policy_decision_ignore(decision);
            return TRUE;
        }
        if (type != WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION) {
            return FALSE;
        }
        auto* const navigation = WEBKIT_NAVIGATION_POLICY_DECISION(decision);
        auto* const action = webkit_navigation_policy_decision_get_navigation_action(navigation);
        const char* const uri =
            webkit_uri_request_get_uri(webkit_navigation_action_get_request(action));
        if (uri != nullptr && std::string(uri).starts_with(mgl::provision::LoginRedirectUri)) {
            webkit_policy_decision_ignore(decision);
            auto* const window = static_cast<SetupWindow*>(self);
            window->closeLogin();
            window->useAddress(uri);
            return TRUE;
        }
        return FALSE;
    }

    void closeLogin() {
        if (view_ != nullptr) {
            gtk_container_remove(GTK_CONTAINER(login_area_), GTK_WIDGET(view_));
            view_ = nullptr;
        }
    }
#endif

    void fail(const std::string& problem) {
        exit_code_ = ExitFailed;
        showScreen(Screen::Failed, problem);
    }

    // Checks a finished login's address and starts the change of owner with its code.
    void useAddress(const std::string& address) {
        const auto login = mgl::setup::readLoginAddress(address, request_.state);
        if (!login.code) {
            fail(login.problem);
            return;
        }
        showScreen(Screen::Working);
        change_.start(*login.code, [this] { g_idle_add(&SetupWindow::onFinished, this); });
    }

    static gboolean onFinished(gpointer self) {
        auto* const window = static_cast<SetupWindow*>(self);
        const auto outcome = window->change_.finish();
        if (outcome.ok) {
            window->exit_code_ = ExitDone;
            window->showScreen(Screen::Done);
        } else {
            window->fail(outcome.problem);
        }
        return G_SOURCE_REMOVE;
    }

    GtkWindow* window_ = nullptr;
    GtkLabel* heading_ = nullptr;
    GtkLabel* body_ = nullptr;
    GtkWidget* browser_ = nullptr;
    GtkWidget* address_label_ = nullptr;
    GtkWidget* address_ = nullptr;
    GtkWidget* progress_ = nullptr;
    GtkWidget* login_area_ = nullptr;
    GtkWidget* primary_ = nullptr;
    GtkWidget* secondary_ = nullptr;
#ifdef MGL_HAVE_WEBKIT
    WebKitWebView* view_ = nullptr;
#endif
    Screen screen_ = Screen::Intro;
    int exit_code_ = ExitCancelled;
    mgl::provision::Authorize request_;
    mgl::setup::OwnerChange change_;
};

std::unique_ptr<SetupWindow> shown;
void onActivate(GtkApplication* application, gpointer /*unused*/) {
    if (!shown) {
        shown = std::make_unique<SetupWindow>(application);
    }
}
} // namespace

int main(int argc, char* argv[]) {
    int exit_code = ExitFailed;
    try {
        GtkApplication* const application = gtk_application_new(
            "io.github.RandyNorthrup.MetaGLinkDirect.Setup", G_APPLICATION_NON_UNIQUE);
        g_signal_connect(application, "activate", G_CALLBACK(&onActivate), nullptr);
        (void)g_application_run(G_APPLICATION(application), argc, argv);
        g_object_unref(application);
        if (shown) {
            exit_code = shown->exitCode();
        }
        shown.reset();
    } catch (const std::exception&) {
        exit_code = ExitFailed; // nothing can be shown once the window is gone
    }
    return exit_code;
}
