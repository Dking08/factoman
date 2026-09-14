#pragma once

#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Menu_Bar.H>
#include <string>
#include <memory>
#include <mutex>
#include <vector>
#include "../config.hpp"
#include "../factorio_zone_client.hpp"
#include "../supabase_sync.hpp"
#include "settings_dialog.hpp"

class MainWindow {
public:
    MainWindow(int w, int h, const char* title);
    ~MainWindow();

    void show(int argc, char** argv);

private:
    // UI Callbacks
    static void cb_start_server(Fl_Widget* w, void* data);
    static void cb_stop_server(Fl_Widget* w, void* data);
    static void cb_copy_ip(Fl_Widget* w, void* data);
    static void cb_send_console(Fl_Widget* w, void* data);
    static void cb_clear_logs(Fl_Widget* w, void* data);
    static void cb_open_settings(Fl_Widget* w, void* data);
    static void cb_revert_copy_btn(void* data);
    static void cb_gui_update_timer(void* data);

    // Menu callbacks
    static void cb_menu_exit(Fl_Widget* w, void* data);
    static void cb_menu_about(Fl_Widget* w, void* data);

    void setup_ui(int w, int h);
    void setup_clients();
    void apply_styling();
    void append_log(const std::string& line);
    void update_status_led(FzState state);
    void update_controls_state(FzState state);
    void process_pending_events();

    AppConfig config_;
    std::unique_ptr<FactorioZoneClient> fz_client_;
    std::unique_ptr<SupabaseSync> sync_client_;
    std::unique_ptr<SettingsDialog> settings_dialog_;

    // Widgets
    Fl_Window* win_ = nullptr;
    Fl_Menu_Bar* menu_bar_ = nullptr;
    Fl_Box* box_status_led_ = nullptr;
    Fl_Box* box_status_text_ = nullptr;
    Fl_Output* out_ip_ = nullptr;
    Fl_Button* btn_copy_ip_ = nullptr;
    Fl_Button* btn_start_ = nullptr;
    Fl_Button* btn_stop_ = nullptr;
    Fl_Button* btn_settings_ = nullptr;
    Fl_Button* btn_clear_logs_ = nullptr;

    Fl_Box* box_details_ = nullptr;
    Fl_Text_Display* txt_console_ = nullptr;
    Fl_Text_Buffer* buf_console_ = nullptr;

    Fl_Input* inp_command_ = nullptr;
    Fl_Button* btn_send_command_ = nullptr;

    Fl_Box* status_bar_ = nullptr;

    // Thread-safe event queue
    struct GuiEvent {
        enum Type {
            LOG,
            IP,
            STATUS,
            LAUNCH_ID,
            REMOTE_SYNC,
            SECRET
        } type;
        std::string str1;
        std::string str2;
        long long num = 0;
        FzState fz_state = FzState::OFFLINE;
        RemoteServerState remote_state;
    };

    std::mutex event_mutex_;
    std::vector<GuiEvent> event_queue_;
};
