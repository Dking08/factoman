#include "main_window.hpp"
#include <FL/Fl.H>
#include <FL/fl_ask.H>
#include <iostream>
#include <sstream>

MainWindow::MainWindow(int w, int h, const char* title) {
    config_ = AppConfig::load();
    setup_ui(w, h);
    apply_styling();
    setup_clients();

    // Schedule high-frequency GUI event drain
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
    win_ = new Fl_Window(w, h, "FactoMan - Factorio.Zone Manager");
    win_->color(fl_rgb_color(212, 208, 200)); // Classic 90s system gray

    // 1. Menu Bar
    menu_bar_ = new Fl_Menu_Bar(0, 0, w, 24);
    menu_bar_->box(FL_UP_BOX);
    menu_bar_->add("&File/&Settings...", FL_CTRL + 's', cb_open_settings, this);
    menu_bar_->add("&File/E&xit", FL_CTRL + 'q', cb_menu_exit, this);
    menu_bar_->add("&Server/&Start Server", 0, cb_start_server, this);
    menu_bar_->add("&Server/S&top Server", 0, cb_stop_server, this);
    menu_bar_->add("&Server/&Clear Logs", 0, cb_clear_logs, this);
    menu_bar_->add("&Help/&About FactoMan", 0, cb_menu_about, this);

    // 2. Status & IP Panel
    Fl_Box* pnl_status = new Fl_Box(12, 32, w - 24, 76);
    pnl_status->box(FL_ENGRAVED_FRAME);

    box_status_led_ = new Fl_Box(26, 46, 18, 18);
    box_status_led_->box(FL_OVAL_BOX);
    box_status_led_->color(fl_rgb_color(140, 140, 140)); // Initially gray

    box_status_text_ = new Fl_Box(52, 44, 250, 22, "SERVER OFFLINE");
    box_status_text_->labelfont(FL_HELVETICA_BOLD);
    box_status_text_->labelsize(14);
    box_status_text_->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

    box_details_ = new Fl_Box(52, 70, 280, 18, "Region: ap-south-1 | Slot: slot1 | Ver: 2.1.17");
    box_details_->labelfont(FL_HELVETICA);
    box_details_->labelsize(11);
    box_details_->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    box_details_->labelcolor(fl_rgb_color(80, 80, 80));

    Fl_Box* lbl_ip = new Fl_Box(340, 56, 70, 28, "Server IP:");
    lbl_ip->labelfont(FL_HELVETICA_BOLD);
    lbl_ip->labelsize(12);
    lbl_ip->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);

    out_ip_ = new Fl_Output(415, 56, 200, 28);
    out_ip_->box(FL_DOWN_BOX);
    out_ip_->color(FL_WHITE);
    out_ip_->textfont(FL_COURIER_BOLD);
    out_ip_->textsize(13);
    out_ip_->value("---.---.---.---:-----");

    btn_copy_ip_ = new Fl_Button(625, 56, 105, 28, "Copy IP");
    btn_copy_ip_->box(FL_UP_BOX);
    btn_copy_ip_->labelfont(FL_HELVETICA_BOLD);
    btn_copy_ip_->callback(cb_copy_ip, this);

    // 3. Action Controls Panel
    Fl_Box* pnl_ctrls = new Fl_Box(12, 116, w - 24, 46);
    pnl_ctrls->box(FL_UP_BOX);

    btn_start_ = new Fl_Button(22, 124, 130, 30, "START SERVER");
    btn_start_->box(FL_UP_BOX);
    btn_start_->labelfont(FL_HELVETICA_BOLD);
    btn_start_->labelcolor(fl_rgb_color(0, 100, 0));
    btn_start_->callback(cb_start_server, this);

    btn_stop_ = new Fl_Button(162, 124, 130, 30, "STOP SERVER");
    btn_stop_->box(FL_UP_BOX);
    btn_stop_->labelfont(FL_HELVETICA_BOLD);
    btn_stop_->labelcolor(fl_rgb_color(150, 0, 0));
    btn_stop_->callback(cb_stop_server, this);
    btn_stop_->deactivate(); // Initially deactivated

    btn_clear_logs_ = new Fl_Button(302, 124, 100, 30, "Clear Logs");
    btn_clear_logs_->box(FL_UP_BOX);
    btn_clear_logs_->callback(cb_clear_logs, this);

    btn_settings_ = new Fl_Button(w - 128, 124, 106, 30, "Settings...");
    btn_settings_->box(FL_UP_BOX);
    btn_settings_->callback(cb_open_settings, this);

    // 4. Terminal Console Display
    Fl_Box* lbl_console = new Fl_Box(12, 170, w - 24, 18, "=== LIVE FACTORIO SERVER LOGS ===");
    lbl_console->labelfont(FL_HELVETICA_BOLD);
    lbl_console->labelsize(11);
    lbl_console->box(FL_FLAT_BOX);
    lbl_console->color(fl_rgb_color(190, 186, 180));

    buf_console_ = new Fl_Text_Buffer();
    txt_console_ = new Fl_Text_Display(12, 190, w - 24, 340);
    txt_console_->buffer(buf_console_);
    txt_console_->box(FL_DOWN_BOX);
    txt_console_->color(FL_BLACK);
    txt_console_->textcolor(fl_rgb_color(0, 255, 100)); // CRT green
    txt_console_->textfont(FL_COURIER);
    txt_console_->textsize(12);

    // 5. Console Command Input
    inp_command_ = new Fl_Input(82, 538, w - 194, 28, "Command:");
    inp_command_->box(FL_DOWN_BOX);
    inp_command_->labelfont(FL_HELVETICA_BOLD);
    inp_command_->textfont(FL_COURIER);
    inp_command_->textsize(12);
    inp_command_->when(FL_WHEN_ENTER_KEY);
    inp_command_->callback(cb_send_console, this);

    btn_send_command_ = new Fl_Button(w - 104, 538, 92, 28, "Send");
    btn_send_command_->box(FL_UP_BOX);
    btn_send_command_->labelfont(FL_HELVETICA_BOLD);
    btn_send_command_->callback(cb_send_console, this);

    // 6. Bottom Status Bar
    status_bar_ = new Fl_Box(12, 574, w - 24, 24, "Ready | Sync: Initializing... | WS: Disconnected");
    status_bar_->box(FL_DOWN_BOX);
    status_bar_->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    status_bar_->labelfont(FL_HELVETICA);
    status_bar_->labelsize(11);

    win_->end();
    win_->resizable(txt_console_);

    settings_dialog_ = std::make_unique<SettingsDialog>(520, 360, "FactoMan Configuration");
}

void MainWindow::apply_styling() {
    Fl::set_color(FL_GRAY, 212, 208, 200);
}

void MainWindow::setup_clients() {
    fz_client_ = std::make_unique<FactorioZoneClient>();
    sync_client_ = std::make_unique<SupabaseSync>();

    // Connect FZ callbacks
    fz_client_->set_callbacks(
        [this](const std::string& line) {
            std::lock_guard<std::mutex> lock(event_mutex_);
            event_queue_.push_back({GuiEvent::LOG, line, "", 0, FzState::OFFLINE, {}});
        },
        [this](const std::string& ip) {
            std::lock_guard<std::mutex> lock(event_mutex_);
            event_queue_.push_back({GuiEvent::IP, ip, "", 0, FzState::RUNNING, {}});
        },
        [this](FzState state, const std::string& msg) {
            std::lock_guard<std::mutex> lock(event_mutex_);
            event_queue_.push_back({GuiEvent::STATUS, msg, "", 0, state, {}});
        },
        [this](long long lid) {
            std::lock_guard<std::mutex> lock(event_mutex_);
            event_queue_.push_back({GuiEvent::LAUNCH_ID, "", "", lid, FzState::OFFLINE, {}});
        },
        [this](const std::string& secret) {
            std::lock_guard<std::mutex> lock(event_mutex_);
            event_queue_.push_back({GuiEvent::SECRET, secret, "", 0, FzState::OFFLINE, {}});
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
            event_queue_.push_back({GuiEvent::STATUS, msg, "", 0, FzState::OFFLINE, {}});
        }
    );

    // Configure and start
    sync_client_->configure(config_.supabase_url, config_.supabase_key, config_.sync_interval_sec);
    sync_client_->start_polling();

    // Start WebSocket
    fz_client_->login(config_.user_token);
    fz_client_->start_websocket_thread();

    append_log("[FactoMan] Application initialized.");
    append_log("[FactoMan] Connecting to factorio.zone WebSocket...");
    if (sync_client_->is_configured()) {
        append_log("[FactoMan] Supabase Multi-User Sync enabled.");
    } else {
        append_log("[FactoMan] Notice: Supabase Sync not configured. You can configure it in Settings.");
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
                std::string status_msg = "SERVER RUNNING: " + ev.str1;
                box_status_text_->copy_label(status_msg.c_str());
                update_status_led(FzState::RUNNING);
                update_controls_state(FzState::RUNNING);

                // Publish to Supabase
                if (sync_client_->is_configured()) {
                    sync_client_->publish_state("RUNNING", ev.str1, fz_client_->get_launch_id(),
                                                config_.player_nick, config_.region, config_.save_slot, config_.factorio_version);
                }
                break;
            }

            case GuiEvent::STATUS: {
                update_status_led(ev.fz_state);
                update_controls_state(ev.fz_state);

                std::string st_str;
                switch (ev.fz_state) {
                    case FzState::OFFLINE: st_str = "SERVER OFFLINE"; break;
                    case FzState::CONNECTING: st_str = "CONNECTING TO WEBSOCKET..."; break;
                    case FzState::CONNECTED: st_str = "CONNECTED TO FACTORIO.ZONE"; break;
                    case FzState::LOGGED_IN: st_str = "LOGGED IN"; break;
                    case FzState::STARTING: st_str = "STARTING INSTANCE..."; break;
                    case FzState::RUNNING: st_str = "SERVER RUNNING"; break;
                    case FzState::STOPPING: st_str = "STOPPING INSTANCE..."; break;
                    case FzState::ERROR_STATE: st_str = "ERROR"; break;
                }
                if (!ev.str1.empty()) {
                    st_str += " (" + ev.str1 + ")";
                }
                box_status_text_->copy_label(st_str.c_str());
                break;
            }

            case GuiEvent::LAUNCH_ID: {
                std::string det = "Region: " + config_.region + " | Slot: " + config_.save_slot +
                                  " | LaunchID: #" + std::to_string(ev.num);
                box_details_->copy_label(det.c_str());
                break;
            }

            case GuiEvent::SECRET: {
                std::string sb = "WS: Connected (Secret: " + ev.str1.substr(0, 6) + "...)";
                if (sync_client_->is_configured()) {
                    sb += " | Sync: Supabase Active (" + config_.player_nick + ")";
                } else {
                    sb += " | Sync: Local Only";
                }
                status_bar_->copy_label(sb.c_str());
                break;
            }

            case GuiEvent::REMOTE_SYNC: {
                const auto& rs = ev.remote_state;
                std::ostringstream ss;
                ss << "Sync from [" << rs.updated_by << "]: " << rs.status;
                if (!rs.server_ip.empty()) {
                    ss << " (" << rs.server_ip << ")";
                    out_ip_->value(rs.server_ip.c_str());
                }

                if (rs.status == "RUNNING") {
                    update_status_led(FzState::RUNNING);
                    update_controls_state(FzState::RUNNING);
                    box_status_text_->copy_label(("SERVER RUNNING (" + rs.updated_by + ")").c_str());
                    if (rs.launch_id > 0) {
                        fz_client_->set_server_details(rs.server_ip, rs.launch_id, FzState::RUNNING);
                    }
                } else if (rs.status == "OFFLINE") {
                    update_status_led(FzState::OFFLINE);
                    update_controls_state(FzState::OFFLINE);
                    out_ip_->value("---.---.---.---:-----");
                    box_status_text_->copy_label("SERVER OFFLINE");
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

    // Auto-scroll to bottom
    txt_console_->scroll(buf_console_->length(), 0);
}

void MainWindow::update_status_led(FzState state) {
    switch (state) {
        case FzState::OFFLINE:
            box_status_led_->color(fl_rgb_color(140, 140, 140)); // Gray
            break;
        case FzState::CONNECTING:
        case FzState::STARTING:
            box_status_led_->color(fl_rgb_color(240, 180, 0)); // Amber
            break;
        case FzState::CONNECTED:
        case FzState::LOGGED_IN:
            box_status_led_->color(fl_rgb_color(0, 180, 240)); // Cyan
            break;
        case FzState::RUNNING:
            box_status_led_->color(fl_rgb_color(0, 230, 0)); // Bright Green
            break;
        case FzState::STOPPING:
        case FzState::ERROR_STATE:
            box_status_led_->color(fl_rgb_color(220, 40, 40)); // Red
            break;
    }
    box_status_led_->redraw();
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
    self->append_log("[Action] Starting server instance...");
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
            self->event_queue_.push_back({GuiEvent::STATUS, "Start instance failed", "", 0, FzState::ERROR_STATE, {}});
        }
    }).detach();
}

void MainWindow::cb_stop_server(Fl_Widget*, void* data) {
    auto* self = static_cast<MainWindow*>(data);
    self->append_log("[Action] Stopping server instance...");
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
        Fl::copy(ip_val, static_cast<int>(strlen(ip_val)), 1); // Clipboard
        Fl::copy(ip_val, static_cast<int>(strlen(ip_val)), 0); // Primary selection
        self->btn_copy_ip_->copy_label("Copied!");
        Fl::remove_timeout(cb_revert_copy_btn, self);
        Fl::add_timeout(1.5, cb_revert_copy_btn, self);
    }
}

void MainWindow::cb_revert_copy_btn(void* data) {
    auto* self = static_cast<MainWindow*>(data);
    self->btn_copy_ip_->copy_label("Copy IP");
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
                self->append_log("[Error] Failed to send console command");
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
        self->box_details_->copy_label(det.c_str());

        // Reconfigure sync
        self->sync_client_->configure(self->config_.supabase_url, self->config_.supabase_key, self->config_.sync_interval_sec);

        // Re-login FZ
        self->fz_client_->login(self->config_.user_token);

        self->append_log("[Settings] Configuration saved.");
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
    fl_message("FactoMan v1.0\nFactorio.Zone Server Manager & Multi-User Sync Client\nBuilt with C++ & FLTK");
}
