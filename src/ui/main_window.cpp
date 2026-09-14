#include "main_window.hpp"
#include <FL/Fl.H>
#include <FL/fl_ask.H>
#include <iostream>
#include <sstream>

MainWindow::MainWindow(int w, int h, const char* title) {
    config_ = AppConfig::load();
    setup_ui(w, h);
    setup_clients();

    // Drain background event queue into GUI
    Fl::add_timeout(0.05, cb_gui_update_timer, this);
}

MainWindow::~MainWindow() {
    Fl::remove_timeout(cb_gui_update_timer, this);
    Fl::remove_timeout(cb_revert_copy_btn, this);
    if (fz_client_) fz_client_->stop_websocket_thread();
    if (sync_client_) sync_client_->stop_polling();
    delete win_;
}

void MainWindow::show(int argc, char** argv) {
    win_->show(argc, argv);
}

void MainWindow::setup_ui(int w, int h) {
    // Dark mode colors
    Fl_Color bg_main   = fl_rgb_color(32, 35, 42);
    Fl_Color bg_dark   = fl_rgb_color(22, 24, 28);
    Fl_Color bg_btn    = fl_rgb_color(52, 58, 68);
    Fl_Color fg_text   = fl_rgb_color(230, 230, 230);
    Fl_Color fg_muted  = fl_rgb_color(165, 175, 190);
    Fl_Color col_start = fl_rgb_color(36, 128, 68);
    Fl_Color col_stop  = fl_rgb_color(158, 44, 44);

    win_ = new Fl_Window(w, h, "FactoMan - Factorio Server Manager");
    win_->color(bg_main);

    // 1. Menu Bar
    menu_bar_ = new Fl_Menu_Bar(0, 0, w, 24);
    menu_bar_->color(bg_dark);
    menu_bar_->textcolor(fg_text);
    menu_bar_->box(FL_FLAT_BOX);
    menu_bar_->add("&File/&Settings...", FL_CTRL + 's', cb_open_settings, this);
    menu_bar_->add("&File/E&xit", FL_CTRL + 'q', cb_menu_exit, this);
    menu_bar_->add("&Server/&Start Server", 0, cb_start_server, this);
    menu_bar_->add("&Server/S&top Server", 0, cb_stop_server, this);
    menu_bar_->add("&Server/&Clear Logs", 0, cb_clear_logs, this);
    menu_bar_->add("&Help/&About", 0, cb_menu_about, this);

    // 2. Status & IP Row
    int cur_y = 36;
    out_status_ = new Fl_Output(70, cur_y, 200, 26, "Status:");
    out_status_->box(FL_THIN_DOWN_BOX);
    out_status_->color(bg_dark);
    out_status_->textcolor(fg_text);
    out_status_->labelcolor(fg_text);
    out_status_->value("Offline (Connecting...)");

    out_ip_ = new Fl_Output(350, cur_y, 205, 26, "Server IP:");
    out_ip_->box(FL_THIN_DOWN_BOX);
    out_ip_->color(bg_dark);
    out_ip_->textcolor(fl_rgb_color(100, 220, 255));
    out_ip_->textfont(FL_COURIER_BOLD);
    out_ip_->labelcolor(fg_text);
    out_ip_->value("---.---.---.---:-----");

    btn_copy_ip_ = new Fl_Button(565, cur_y, 115, 26, "Copy IP");
    btn_copy_ip_->box(FL_FLAT_BOX);
    btn_copy_ip_->color(bg_btn);
    btn_copy_ip_->labelcolor(FL_WHITE);
    btn_copy_ip_->callback(cb_copy_ip, this);

    // 3. Action Buttons Row
    cur_y += 34;
    btn_start_ = new Fl_Button(15, cur_y, 130, 30, "Start Server");
    btn_start_->box(FL_FLAT_BOX);
    btn_start_->color(col_start);
    btn_start_->labelcolor(FL_WHITE);
    btn_start_->callback(cb_start_server, this);

    btn_stop_ = new Fl_Button(155, cur_y, 130, 30, "Stop Server");
    btn_stop_->box(FL_FLAT_BOX);
    btn_stop_->color(col_stop);
    btn_stop_->labelcolor(FL_WHITE);
    btn_stop_->callback(cb_stop_server, this);
    btn_stop_->deactivate();

    btn_clear_logs_ = new Fl_Button(295, cur_y, 100, 30, "Clear Logs");
    btn_clear_logs_->box(FL_FLAT_BOX);
    btn_clear_logs_->color(bg_btn);
    btn_clear_logs_->labelcolor(FL_WHITE);
    btn_clear_logs_->callback(cb_clear_logs, this);

    btn_settings_ = new Fl_Button(565, cur_y, 115, 30, "Settings...");
    btn_settings_->box(FL_FLAT_BOX);
    btn_settings_->color(bg_btn);
    btn_settings_->labelcolor(FL_WHITE);
    btn_settings_->callback(cb_open_settings, this);

    // 4. Details Label (Shows Region, Slot, Ver. NO launchId before starting!)
    cur_y += 38;
    box_details_ = new Fl_Box(15, cur_y, w - 30, 18, "Region: ap-south-1 | Slot: slot1 | Ver: 2.1.17");
    box_details_->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    box_details_->labelcolor(fg_muted);
    box_details_->labelsize(12);

    // 5. Console Display
    cur_y += 24;
    int console_h = h - cur_y - 75;
    buf_console_ = new Fl_Text_Buffer();
    txt_console_ = new Fl_Text_Display(15, cur_y, w - 30, console_h);
    txt_console_->buffer(buf_console_);
    txt_console_->box(FL_THIN_DOWN_BOX);
    txt_console_->color(fl_rgb_color(16, 18, 22));
    txt_console_->textcolor(fl_rgb_color(150, 240, 160));
    txt_console_->textfont(FL_COURIER);
    txt_console_->textsize(12);

    // 6. Command Input Row
    cur_y += console_h + 8;
    inp_command_ = new Fl_Input(85, cur_y, w - 195, 26, "Command:");
    inp_command_->box(FL_THIN_DOWN_BOX);
    inp_command_->color(bg_dark);
    inp_command_->textcolor(FL_WHITE);
    inp_command_->labelcolor(fg_text);
    inp_command_->textfont(FL_COURIER);
    inp_command_->textsize(12);
    inp_command_->when(FL_WHEN_ENTER_KEY);
    inp_command_->callback(cb_send_console, this);

    btn_send_command_ = new Fl_Button(w - 100, cur_y, 85, 26, "Send");
    btn_send_command_->box(FL_FLAT_BOX);
    btn_send_command_->color(bg_btn);
    btn_send_command_->labelcolor(FL_WHITE);
    btn_send_command_->callback(cb_send_console, this);

    // 7. Status Bar
    cur_y += 34;
    status_bar_ = new Fl_Box(15, cur_y, w - 30, 22, "Initializing...");
    status_bar_->box(FL_THIN_DOWN_BOX);
    status_bar_->color(bg_dark);
    status_bar_->labelcolor(fg_muted);
    status_bar_->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    status_bar_->labelsize(11);

    win_->end();
    win_->resizable(txt_console_);

    settings_dialog_ = std::make_unique<SettingsDialog>(510, 360, "FactoMan Settings");
}

void MainWindow::setup_clients() {
    fz_client_ = std::make_unique<FactorioZoneClient>();
    sync_client_ = std::make_unique<SupabaseSync>();

    // Connect FactorioZone callbacks
    fz_client_->set_callbacks(
        [this](const std::string& line) {
            std::lock_guard<std::mutex> lock(event_mutex_);
            event_queue_.push_back({GuiEvent::LOG, line, "", "", 0, FzState::OFFLINE, {}});
        },
        [this](const std::string& ip) {
            std::lock_guard<std::mutex> lock(event_mutex_);
            event_queue_.push_back({GuiEvent::IP, ip, "", "", 0, FzState::RUNNING, {}});
        },
        [this](FzState state, const std::string& msg) {
            std::lock_guard<std::mutex> lock(event_mutex_);
            event_queue_.push_back({GuiEvent::STATUS, msg, "", "", 0, state, {}});
        },
        [this](long long lid) {
            std::lock_guard<std::mutex> lock(event_mutex_);
            event_queue_.push_back({GuiEvent::LAUNCH_ID, "", "", "", lid, FzState::OFFLINE, {}});
        },
        [this](const std::string& secret) {
            std::lock_guard<std::mutex> lock(event_mutex_);
            event_queue_.push_back({GuiEvent::SECRET, secret, "", "", 0, FzState::OFFLINE, {}});
        },
        [this](const std::string& slot, const std::string& region, const std::string& ver) {
            std::lock_guard<std::mutex> lock(event_mutex_);
            event_queue_.push_back({GuiEvent::SLOT, slot, region, ver, 0, FzState::OFFLINE, {}});
        }
    );

    // Connect Supabase callbacks
    sync_client_->set_callbacks(
        [this](const RemoteServerState& state) {
            std::lock_guard<std::mutex> lock(event_mutex_);
            GuiEvent ev;
            ev.type = GuiEvent::REMOTE_SYNC;
            ev.remote_state = state;
            event_queue_.push_back(ev);
        },
        [this](const std::string& msg, bool) {
            std::lock_guard<std::mutex> lock(event_mutex_);
            event_queue_.push_back({GuiEvent::STATUS, msg, "", "", 0, FzState::OFFLINE, {}});
        }
    );

    sync_client_->configure(config_.supabase_url, config_.supabase_key, config_.sync_interval_sec);
    sync_client_->start_polling();

    // Configure user token and start WebSocket
    fz_client_->set_user_token(config_.user_token);
    fz_client_->start_websocket_thread();

    append_log("[FactoMan] Application started.");
    append_log("[FactoMan] Connecting to factorio.zone WebSocket...");
    if (sync_client_->is_configured()) {
        append_log("[FactoMan] Supabase Sync active (" + config_.player_nick + ").");
    } else {
        append_log("[FactoMan] Supabase Sync not configured. Configure via Settings.");
    }
}

void MainWindow::process_pending_events() {
    std::vector<GuiEvent> events;
    {
        std::lock_guard<std::mutex> lock(event_mutex_);
        if (event_queue_.empty()) return;
        events.swap(event_queue_);
    }

    for (const auto& ev : events) {
        switch (ev.type) {
            case GuiEvent::LOG:
                append_log(ev.str1);
                break;

            case GuiEvent::IP: {
                out_ip_->value(ev.str1.c_str());
                out_status_->value("Running");
                update_controls_state(FzState::RUNNING);

                // Update details with LaunchID when running
                long long lid = fz_client_->get_launch_id();
                std::string det = "Region: " + config_.region + " | Slot: " + config_.save_slot +
                                  " | Ver: " + config_.factorio_version;
                if (lid > 0) det += " | Launch ID: #" + std::to_string(lid);
                box_details_->copy_label(det.c_str());

                // Broadcast to Supabase
                if (sync_client_->is_configured()) {
                    sync_client_->publish_state("RUNNING", ev.str1, lid,
                                                config_.player_nick, config_.region, config_.save_slot, config_.factorio_version);
                }
                break;
            }

            case GuiEvent::STATUS: {
                update_controls_state(ev.fz_state);

                std::string st_str;
                switch (ev.fz_state) {
                    case FzState::OFFLINE: st_str = "Offline (Ready)"; break;
                    case FzState::CONNECTING: st_str = "Connecting..."; break;
                    case FzState::CONNECTED: st_str = "Connected (Logging in...)"; break;
                    case FzState::LOGGED_IN: st_str = "Ready"; break;
                    case FzState::STARTING: st_str = "Starting Server..."; break;
                    case FzState::RUNNING: st_str = "Running"; break;
                    case FzState::STOPPING: st_str = "Stopping Server..."; break;
                    case FzState::ERROR_STATE: st_str = "Error"; break;
                }
                out_status_->value(st_str.c_str());
                if (!ev.str1.empty()) {
                    status_bar_->copy_label(ev.str1.c_str());
                }

                // If offline, reset details to hide launch ID
                if (ev.fz_state == FzState::OFFLINE) {
                    std::string det = "Region: " + config_.region + " | Slot: " + config_.save_slot +
                                      " | Ver: " + config_.factorio_version;
                    box_details_->copy_label(det.c_str());
                    out_ip_->value("---.---.---.---:-----");
                }
                break;
            }

            case GuiEvent::LAUNCH_ID: {
                long long lid = ev.num;
                if (lid > 0 && (fz_client_->get_state() == FzState::STARTING || fz_client_->get_state() == FzState::RUNNING)) {
                    std::string det = "Region: " + config_.region + " | Slot: " + config_.save_slot +
                                      " | Ver: " + config_.factorio_version + " | Launch ID: #" + std::to_string(lid);
                    box_details_->copy_label(det.c_str());
                }
                break;
            }

            case GuiEvent::SLOT: {
                if (!ev.str1.empty()) config_.save_slot = ev.str1;
                if (!ev.str2.empty()) config_.region = ev.str2;
                if (!ev.str3.empty()) config_.factorio_version = ev.str3;

                std::string det = "Region: " + config_.region + " | Slot: " + config_.save_slot +
                                  " | Ver: " + config_.factorio_version;
                long long lid = fz_client_->get_launch_id();
                if (lid > 0 && (fz_client_->get_state() == FzState::STARTING || fz_client_->get_state() == FzState::RUNNING)) {
                    det += " | Launch ID: #" + std::to_string(lid);
                }
                box_details_->copy_label(det.c_str());
                break;
            }

            case GuiEvent::SECRET: {
                std::string sb = "WS: Connected";
                if (sync_client_->is_configured()) {
                    sb += " | Supabase: Synced (" + config_.player_nick + ")";
                } else {
                    sb += " | Supabase: Not Configured";
                }
                status_bar_->copy_label(sb.c_str());
                break;
            }

            case GuiEvent::REMOTE_SYNC: {
                const auto& rs = ev.remote_state;
                std::ostringstream ss;
                ss << "Sync: " << rs.status;
                if (!rs.updated_by.empty()) {
                    ss << " (by " << rs.updated_by << ")";
                }
                if (!rs.server_ip.empty()) {
                    out_ip_->value(rs.server_ip.c_str());
                }

                if (rs.status == "RUNNING") {
                    out_status_->value("Running");
                    update_controls_state(FzState::RUNNING);
                    if (rs.launch_id > 0) {
                        fz_client_->set_server_details(rs.server_ip, rs.launch_id, FzState::RUNNING);
                        std::string det = "Region: " + rs.region + " | Slot: " + rs.save_slot +
                                          " | Ver: " + rs.version + " | Launch ID: #" + std::to_string(rs.launch_id);
                        box_details_->copy_label(det.c_str());
                    }
                } else if (rs.status == "OFFLINE") {
                    out_status_->value("Offline (Ready)");
                    update_controls_state(FzState::OFFLINE);
                    out_ip_->value("---.---.---.---:-----");
                    std::string det = "Region: " + config_.region + " | Slot: " + config_.save_slot +
                                      " | Ver: " + config_.factorio_version;
                    box_details_->copy_label(det.c_str());
                }

                append_log("[Sync] " + ss.str());
                status_bar_->copy_label(ss.str().c_str());
                break;
            }
        }
    }
}

void MainWindow::append_log(const std::string& line) {
    if (!buf_console_) return;
    std::string text = line + "\n";
    buf_console_->append(text.c_str());
    txt_console_->scroll(buf_console_->length(), 0);
}

void MainWindow::update_controls_state(FzState state) {
    if (state == FzState::RUNNING || state == FzState::STARTING) {
        btn_start_->deactivate();
        btn_stop_->activate();
    } else {
        btn_start_->activate();
        btn_stop_->deactivate();
    }
}

void MainWindow::cb_start_server(Fl_Widget*, void* data) {
    auto* self = static_cast<MainWindow*>(data);
    self->append_log("[Action] Requesting server start...");
    self->btn_start_->deactivate();

    std::thread([self]() {
        bool ok = self->fz_client_->start_instance(
            self->config_.region,
            self->config_.save_slot,
            self->config_.factorio_version,
            self->config_.options_json
        );
        if (!ok) {
            std::lock_guard<std::mutex> lock(self->event_mutex_);
            self->event_queue_.push_back({GuiEvent::STATUS, "Failed to start server", "", "", 0, FzState::ERROR_STATE, {}});
        }
    }).detach();
}

void MainWindow::cb_stop_server(Fl_Widget*, void* data) {
    auto* self = static_cast<MainWindow*>(data);
    self->append_log("[Action] Stopping server...");
    self->btn_stop_->deactivate();

    std::thread([self]() {
        bool ok = self->fz_client_->stop_instance();
        if (ok) {
            if (self->sync_client_->is_configured()) {
                self->sync_client_->publish_state("OFFLINE", "", 0, self->config_.player_nick);
            }
        }
    }).detach();
}

void MainWindow::cb_copy_ip(Fl_Widget*, void* data) {
    auto* self = static_cast<MainWindow*>(data);
    const char* ip_val = self->out_ip_->value();
    if (ip_val && std::string(ip_val) != "---.---.---.---:-----") {
        Fl::copy(ip_val, static_cast<int>(strlen(ip_val)), 1);
        Fl::copy(ip_val, static_cast<int>(strlen(ip_val)), 0);
        self->btn_copy_ip_->label("Copied!");
        Fl::remove_timeout(cb_revert_copy_btn, self);
        Fl::add_timeout(1.5, cb_revert_copy_btn, self);
    }
}

void MainWindow::cb_revert_copy_btn(void* data) {
    auto* self = static_cast<MainWindow*>(data);
    self->btn_copy_ip_->label("Copy IP");
}

void MainWindow::cb_send_console(Fl_Widget*, void* data) {
    auto* self = static_cast<MainWindow*>(data);
    const char* cmd = self->inp_command_->value();
    if (cmd && strlen(cmd) > 0) {
        std::string cmd_str = cmd;
        self->inp_command_->value("");
        self->append_log("> " + cmd_str);

        std::thread([self, cmd_str]() {
            bool ok = self->fz_client_->send_console(cmd_str);
            if (!ok) {
                self->append_log("[Error] Command failed to send");
            }
        }).detach();
    }
}

void MainWindow::cb_clear_logs(Fl_Widget*, void* data) {
    auto* self = static_cast<MainWindow*>(data);
    if (self->buf_console_) {
        self->buf_console_->text("");
    }
}

void MainWindow::cb_open_settings(Fl_Widget*, void* data) {
    auto* self = static_cast<MainWindow*>(data);
    self->settings_dialog_->show(self->config_, [self](const AppConfig& updated) {
        self->config_ = updated;
        self->config_.save();

        std::string det = "Region: " + self->config_.region + " | Slot: " + self->config_.save_slot +
                          " | Ver: " + self->config_.factorio_version;
        long long lid = self->fz_client_->get_launch_id();
        if (lid > 0 && (self->fz_client_->get_state() == FzState::STARTING || self->fz_client_->get_state() == FzState::RUNNING)) {
            det += " | Launch ID: #" + std::to_string(lid);
        }
        self->box_details_->copy_label(det.c_str());

        self->sync_client_->configure(self->config_.supabase_url, self->config_.supabase_key, self->config_.sync_interval_sec);
        self->fz_client_->set_user_token(self->config_.user_token);

        self->append_log("[Settings] Configuration updated.");
    });
}

void MainWindow::cb_gui_update_timer(void* data) {
    auto* self = static_cast<MainWindow*>(data);
    self->process_pending_events();
    Fl::repeat_timeout(0.05, cb_gui_update_timer, data);
}

void MainWindow::cb_menu_exit(Fl_Widget*, void* data) {
    auto* self = static_cast<MainWindow*>(data);
    self->win_->hide();
}

void MainWindow::cb_menu_about(Fl_Widget*, void*) {
    fl_message_title("About FactoMan");
    fl_message("FactoMan v1.0\nFactorio Server Manager\nBuilt with C++ & FLTK");
}
