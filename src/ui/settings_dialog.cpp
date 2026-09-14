#include "settings_dialog.hpp"
#include <FL/Fl.H>
#include "../supabase_sync.hpp"

SettingsDialog::SettingsDialog(int w, int h, const char* title) {
    win_ = new Fl_Window(w, h, title);
    win_->set_modal();

    Fl_Color bg_color   = fl_rgb_color(34, 37, 43);
    Fl_Color input_bg   = fl_rgb_color(24, 26, 30);
    Fl_Color text_color = fl_rgb_color(230, 230, 230);
    Fl_Color btn_color  = fl_rgb_color(56, 62, 74);

    win_->color(bg_color);

    int start_y = 20;
    int label_w = 150;
    int input_h = 25;
    int gap = 34;

    auto style_input = [input_bg, text_color](Fl_Input* inp) {
        inp->align(FL_ALIGN_LEFT);
        inp->box(FL_THIN_DOWN_BOX);
        inp->color(input_bg);
        inp->textcolor(text_color);
        inp->labelcolor(text_color);
        inp->textsize(12);
    };

    // 1. Supabase Credentials
    inp_supabase_url_ = new Fl_Input(label_w + 15, start_y, w - label_w - 35, input_h, "Supabase URL:");
    style_input(inp_supabase_url_);

    start_y += gap;
    inp_supabase_key_ = new Fl_Input(label_w + 15, start_y, w - label_w - 35, input_h, "Supabase Anon Key:");
    style_input(inp_supabase_key_);

    // 2. Nickname / Lookup Key
    start_y += gap;
    inp_nick_ = new Fl_Input(label_w + 15, start_y, w - label_w - 35, input_h, "Player / Token Key:");
    style_input(inp_nick_);

    // 3. User Token Field
    start_y += gap;
    inp_token_ = new Fl_Input(label_w + 15, start_y, w - label_w - 35, input_h, "Factorio User Token:");
    style_input(inp_token_);

    // 4. Cloud Token Action Buttons (Fetch, Save & List)
    start_y += 30;
    int btn_cloud_w = 115;
    btn_fetch_token_ = new Fl_Button(label_w + 15, start_y, btn_cloud_w, input_h, "Fetch Token");
    btn_fetch_token_->box(FL_FLAT_BOX);
    btn_fetch_token_->color(fl_rgb_color(45, 90, 160));
    btn_fetch_token_->labelcolor(FL_WHITE);
    btn_fetch_token_->callback(cb_fetch_token, this);

    btn_upload_token_ = new Fl_Button(label_w + 15 + btn_cloud_w + 10, start_y, btn_cloud_w, input_h, "Save Token");
    btn_upload_token_->box(FL_FLAT_BOX);
    btn_upload_token_->color(fl_rgb_color(40, 130, 70));
    btn_upload_token_->labelcolor(FL_WHITE);
    btn_upload_token_->callback(cb_upload_token, this);

    btn_list_keys_ = new Fl_Button(label_w + 15 + (btn_cloud_w + 10) * 2, start_y, 100, input_h, "List Keys");
    btn_list_keys_->box(FL_FLAT_BOX);
    btn_list_keys_->color(fl_rgb_color(80, 70, 120));
    btn_list_keys_->labelcolor(FL_WHITE);
    btn_list_keys_->callback(cb_list_keys, this);

    start_y += 26;
    box_fetch_status_ = new Fl_Box(label_w + 15, start_y, w - label_w - 35, 18, "");
    box_fetch_status_->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    box_fetch_status_->labelsize(11);
    box_fetch_status_->labelcolor(fl_rgb_color(160, 170, 185));

    // 5. Server Options
    start_y += 22;
    inp_region_ = new Fl_Input(label_w + 15, start_y, w - label_w - 35, input_h, "Server Region:");
    style_input(inp_region_);

    start_y += gap;
    inp_slot_ = new Fl_Input(label_w + 15, start_y, w - label_w - 35, input_h, "Save Slot:");
    style_input(inp_slot_);

    start_y += gap;
    inp_version_ = new Fl_Input(label_w + 15, start_y, w - label_w - 35, input_h, "Factorio Version:");
    style_input(inp_version_);

    // 6. Modal Actions
    start_y += gap + 10;
    int btn_w = 110;
    int btn_h = 28;
    int total_btns = btn_w * 2 + 20;
    int btn_x = (w - total_btns) / 2;

    btn_save_ = new Fl_Button(btn_x, start_y, btn_w, btn_h, "Save");
    btn_save_->box(FL_FLAT_BOX);
    btn_save_->color(fl_rgb_color(40, 130, 70));
    btn_save_->labelcolor(FL_WHITE);
    btn_save_->callback(cb_save, this);

    btn_cancel_ = new Fl_Button(btn_x + btn_w + 20, start_y, btn_w, btn_h, "Cancel");
    btn_cancel_->box(FL_FLAT_BOX);
    btn_cancel_->color(btn_color);
    btn_cancel_->labelcolor(FL_WHITE);
    btn_cancel_->callback(cb_cancel, this);

    win_->end();
}

SettingsDialog::~SettingsDialog() {
    delete win_;
}

void SettingsDialog::show(const AppConfig& current_cfg, SaveCallback on_save) {
    current_cfg_ = current_cfg;
    on_save_ = std::move(on_save);

    inp_token_->value(current_cfg_.user_token.c_str());
    inp_nick_->value(current_cfg_.player_nick.c_str());
    inp_supabase_url_->value(current_cfg_.supabase_url.c_str());
    inp_supabase_key_->value(current_cfg_.supabase_key.c_str());
    inp_region_->value(current_cfg_.region.c_str());
    inp_slot_->value(current_cfg_.save_slot.c_str());
    inp_version_->value(current_cfg_.factorio_version.c_str());
    box_fetch_status_->copy_label("");

    win_->show();
}

void SettingsDialog::hide() {
    win_->hide();
}

void SettingsDialog::cb_fetch_token(Fl_Widget*, void* data) {
    auto* self = static_cast<SettingsDialog*>(data);
    std::string key = self->inp_nick_->value();
    std::string url = self->inp_supabase_url_->value();
    std::string anon_key = self->inp_supabase_key_->value();

    if (url.empty() || anon_key.empty()) {
        self->box_fetch_status_->copy_label("Error: Supabase URL and Key must be filled in first.");
        self->box_fetch_status_->labelcolor(fl_rgb_color(255, 120, 120));
        return;
    }

    SupabaseSync sync;
    sync.configure(url, anon_key);

    if (key.empty()) {
        self->box_fetch_status_->copy_label("Fetching available keys from cloud...");
        self->box_fetch_status_->labelcolor(fl_rgb_color(180, 190, 210));
        Fl::check();

        std::vector<std::string> keys;
        std::string err;
        if (sync.fetch_all_token_keys(keys, err)) {
            if (keys.empty()) {
                self->box_fetch_status_->copy_label("Cloud table has no registered tokens yet. Enter token & click Save.");
                self->box_fetch_status_->labelcolor(fl_rgb_color(255, 200, 100));
            } else {
                std::string msg = "Keys in cloud: ";
                for (size_t i = 0; i < keys.size(); ++i) {
                    if (i > 0) msg += ", ";
                    msg += keys[i];
                }
                msg += " (type one in Player / Token Key)";
                self->box_fetch_status_->copy_label(msg.c_str());
                self->box_fetch_status_->labelcolor(fl_rgb_color(100, 220, 255));
                self->inp_nick_->value(keys[0].c_str());
            }
        } else {
            self->box_fetch_status_->copy_label(("Error: Enter a key to fetch. (" + err + ")").c_str());
            self->box_fetch_status_->labelcolor(fl_rgb_color(255, 120, 120));
        }
        return;
    }

    self->box_fetch_status_->copy_label("Fetching token from Supabase...");
    self->box_fetch_status_->labelcolor(fl_rgb_color(180, 190, 210));
    Fl::check();

    std::string token, err;
    if (sync.fetch_token_by_key(key, token, err)) {
        self->inp_token_->value(token.c_str());
        self->box_fetch_status_->copy_label(("Success: Token loaded for '" + key + "'").c_str());
        self->box_fetch_status_->labelcolor(fl_rgb_color(100, 240, 140));
    } else {
        std::vector<std::string> keys;
        std::string lerr;
        if (sync.fetch_all_token_keys(keys, lerr) && !keys.empty()) {
            std::string msg = "Key not found. Cloud keys: ";
            for (size_t i = 0; i < keys.size(); ++i) {
                if (i > 0) msg += ", ";
                msg += keys[i];
            }
            self->box_fetch_status_->copy_label(msg.c_str());
        } else {
            self->box_fetch_status_->copy_label(err.c_str());
        }
        self->box_fetch_status_->labelcolor(fl_rgb_color(255, 120, 120));
    }
}

void SettingsDialog::cb_upload_token(Fl_Widget*, void* data) {
    auto* self = static_cast<SettingsDialog*>(data);
    std::string key = self->inp_nick_->value();
    std::string token = self->inp_token_->value();
    std::string url = self->inp_supabase_url_->value();
    std::string anon_key = self->inp_supabase_key_->value();

    if (key.empty()) {
        self->box_fetch_status_->copy_label("Error: Enter a key in Player / Token Key.");
        self->box_fetch_status_->labelcolor(fl_rgb_color(255, 120, 120));
        return;
    }
    if (token.empty()) {
        self->box_fetch_status_->copy_label("Error: Factorio User Token is empty.");
        self->box_fetch_status_->labelcolor(fl_rgb_color(255, 120, 120));
        return;
    }
    if (url.empty() || anon_key.empty()) {
        self->box_fetch_status_->copy_label("Error: Supabase URL and Key must be filled in first.");
        self->box_fetch_status_->labelcolor(fl_rgb_color(255, 120, 120));
        return;
    }

    self->box_fetch_status_->copy_label("Saving token to Supabase...");
    self->box_fetch_status_->labelcolor(fl_rgb_color(180, 190, 210));
    Fl::check();

    SupabaseSync sync;
    sync.configure(url, anon_key);
    std::string err;
    if (sync.save_token_to_cloud(key, token, err)) {
        self->box_fetch_status_->copy_label(("Success: Saved '" + key + "' to Supabase!").c_str());
        self->box_fetch_status_->labelcolor(fl_rgb_color(100, 240, 140));
    } else {
        self->box_fetch_status_->copy_label(err.c_str());
        self->box_fetch_status_->labelcolor(fl_rgb_color(255, 120, 120));
    }
}

void SettingsDialog::cb_list_keys(Fl_Widget*, void* data) {
    auto* self = static_cast<SettingsDialog*>(data);
    std::string url = self->inp_supabase_url_->value();
    std::string anon_key = self->inp_supabase_key_->value();

    if (url.empty() || anon_key.empty()) {
        self->box_fetch_status_->copy_label("Error: Supabase URL and Key must be filled in first.");
        self->box_fetch_status_->labelcolor(fl_rgb_color(255, 120, 120));
        return;
    }

    self->box_fetch_status_->copy_label("Querying cloud keys from Supabase...");
    self->box_fetch_status_->labelcolor(fl_rgb_color(180, 190, 210));
    Fl::check();

    SupabaseSync sync;
    sync.configure(url, anon_key);
    std::vector<std::string> keys;
    std::string err;
    if (sync.fetch_all_token_keys(keys, err)) {
        if (keys.empty()) {
            self->box_fetch_status_->copy_label("No keys registered in Supabase yet.");
            self->box_fetch_status_->labelcolor(fl_rgb_color(255, 200, 100));
        } else {
            std::string msg = "Registered keys: ";
            for (size_t i = 0; i < keys.size(); ++i) {
                if (i > 0) msg += ", ";
                msg += keys[i];
            }
            self->box_fetch_status_->copy_label(msg.c_str());
            self->box_fetch_status_->labelcolor(fl_rgb_color(100, 220, 255));
            std::string cur_nick = self->inp_nick_->value();
            if (cur_nick.empty()) {
                self->inp_nick_->value(keys[0].c_str());
            }
        }
    } else {
        self->box_fetch_status_->copy_label(err.c_str());
        self->box_fetch_status_->labelcolor(fl_rgb_color(255, 120, 120));
    }
}

void SettingsDialog::cb_save(Fl_Widget*, void* data) {
    auto* self = static_cast<SettingsDialog*>(data);
    AppConfig updated = self->current_cfg_;
    updated.user_token = self->inp_token_->value();
    updated.player_nick = self->inp_nick_->value();
    updated.supabase_url = self->inp_supabase_url_->value();
    updated.supabase_key = self->inp_supabase_key_->value();
    updated.region = self->inp_region_->value();
    updated.save_slot = self->inp_slot_->value();
    updated.factorio_version = self->inp_version_->value();

    // Automatically sync token to cloud if credentials are valid
    if (!updated.supabase_url.empty() && !updated.supabase_key.empty() &&
        !updated.player_nick.empty() && !updated.user_token.empty()) {
        SupabaseSync sync;
        sync.configure(updated.supabase_url, updated.supabase_key);
        std::string err;
        sync.save_token_to_cloud(updated.player_nick, updated.user_token, err);
    }

    if (self->on_save_) {
        self->on_save_(updated);
    }
    self->win_->hide();
}

void SettingsDialog::cb_cancel(Fl_Widget*, void* data) {
    auto* self = static_cast<SettingsDialog*>(data);
    self->win_->hide();
}
