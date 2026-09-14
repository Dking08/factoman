#include "settings_dialog.hpp"
#include <FL/Fl.H>

SettingsDialog::SettingsDialog(int w, int h, const char* title) {
    win_ = new Fl_Window(w, h, title);
    win_->set_modal();
    win_->color(FL_GRAY);

    int start_y = 20;
    int label_w = 140;
    int input_w = w - label_w - 40;
    int input_h = 24;
    int gap = 34;

    inp_token_ = new Fl_Input(label_w + 20, start_y, input_w, input_h, "Factorio User Token:");
    inp_token_->align(FL_ALIGN_LEFT);
    inp_token_->box(FL_DOWN_BOX);

    start_y += gap;
    inp_nick_ = new Fl_Input(label_w + 20, start_y, input_w, input_h, "Player Nickname:");
    inp_nick_->align(FL_ALIGN_LEFT);
    inp_nick_->box(FL_DOWN_BOX);

    start_y += gap;
    inp_supabase_url_ = new Fl_Input(label_w + 20, start_y, input_w, input_h, "Supabase URL:");
    inp_supabase_url_->align(FL_ALIGN_LEFT);
    inp_supabase_url_->box(FL_DOWN_BOX);

    start_y += gap;
    inp_supabase_key_ = new Fl_Input(label_w + 20, start_y, input_w, input_h, "Supabase Anon Key:");
    inp_supabase_key_->align(FL_ALIGN_LEFT);
    inp_supabase_key_->box(FL_DOWN_BOX);

    start_y += gap;
    inp_region_ = new Fl_Input(label_w + 20, start_y, input_w, input_h, "Server Region:");
    inp_region_->align(FL_ALIGN_LEFT);
    inp_region_->box(FL_DOWN_BOX);

    start_y += gap;
    inp_slot_ = new Fl_Input(label_w + 20, start_y, input_w, input_h, "Save Slot:");
    inp_slot_->align(FL_ALIGN_LEFT);
    inp_slot_->box(FL_DOWN_BOX);

    start_y += gap;
    inp_version_ = new Fl_Input(label_w + 20, start_y, input_w, input_h, "Factorio Version:");
    inp_version_->align(FL_ALIGN_LEFT);
    inp_version_->box(FL_DOWN_BOX);

    start_y += gap + 10;
    int btn_w = 100;
    int btn_h = 28;
    int total_btns = btn_w * 2 + 20;
    int btn_x = (w - total_btns) / 2;

    btn_save_ = new Fl_Button(btn_x, start_y, btn_w, btn_h, "Save");
    btn_save_->box(FL_UP_BOX);
    btn_save_->callback(cb_save, this);

    btn_cancel_ = new Fl_Button(btn_x + btn_w + 20, start_y, btn_w, btn_h, "Cancel");
    btn_cancel_->box(FL_UP_BOX);
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
