#include "settings_dialog.hpp"
#include <FL/Fl.H>

SettingsDialog::SettingsDialog(int w, int h, const char* title) {
    win_ = new Fl_Window(w, h, title);
    win_->set_modal();
    // Dark mode palette
    Fl_Color bg_color = fl_rgb_color(34, 37, 43);
    Fl_Color input_bg = fl_rgb_color(24, 26, 30);
    Fl_Color text_color = fl_rgb_color(230, 230, 230);
    Fl_Color btn_color = fl_rgb_color(56, 62, 74);

    win_->color(bg_color);

    int start_y = 20;
    int label_w = 145;
    int input_w = w - label_w - 35;
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

    inp_token_ = new Fl_Input(label_w + 15, start_y, input_w, input_h, "Factorio User Token:");
    style_input(inp_token_);

    start_y += gap;
    inp_nick_ = new Fl_Input(label_w + 15, start_y, input_w, input_h, "Player Nickname:");
    style_input(inp_nick_);

    start_y += gap;
    inp_supabase_url_ = new Fl_Input(label_w + 15, start_y, input_w, input_h, "Supabase URL:");
    style_input(inp_supabase_url_);

    start_y += gap;
    inp_supabase_key_ = new Fl_Input(label_w + 15, start_y, input_w, input_h, "Supabase Anon Key:");
    style_input(inp_supabase_key_);

    start_y += gap;
    inp_region_ = new Fl_Input(label_w + 15, start_y, input_w, input_h, "Server Region:");
    style_input(inp_region_);

    start_y += gap;
    inp_slot_ = new Fl_Input(label_w + 15, start_y, input_w, input_h, "Save Slot:");
    style_input(inp_slot_);

    start_y += gap;
    inp_version_ = new Fl_Input(label_w + 15, start_y, input_w, input_h, "Factorio Version:");
    style_input(inp_version_);

    start_y += gap + 12;
    int btn_w = 100;
    int btn_h = 28;
    int total_btns = btn_w * 2 + 20;
    int btn_x = (w - total_btns) / 2;

    btn_save_ = new Fl_Button(btn_x, start_y, btn_w, btn_h, "Save");
    btn_save_->color(fl_rgb_color(40, 130, 70));
    btn_save_->labelcolor(FL_WHITE);
    btn_save_->callback(cb_save, this);

    btn_cancel_ = new Fl_Button(btn_x + btn_w + 20, start_y, btn_w, btn_h, "Cancel");
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

    win_->show();
}

void SettingsDialog::hide() {
    win_->hide();
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

    if (self->on_save_) {
        self->on_save_(updated);
    }
    self->win_->hide();
}

void SettingsDialog::cb_cancel(Fl_Widget*, void* data) {
    auto* self = static_cast<SettingsDialog*>(data);
    self->win_->hide();
}
