#pragma once

#include <FL/Fl_Window.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <functional>
#include "../config.hpp"

class SettingsDialog {
public:
    using SaveCallback = std::function<void(const AppConfig& cfg)>;

    SettingsDialog(int w, int h, const char* title);
    ~SettingsDialog();

    void show(const AppConfig& current_cfg, SaveCallback on_save);
    void hide();

private:
    static void cb_save(Fl_Widget* w, void* data);
    static void cb_cancel(Fl_Widget* w, void* data);

    Fl_Window* win_ = nullptr;
    Fl_Input* inp_token_ = nullptr;
    Fl_Input* inp_nick_ = nullptr;
    Fl_Input* inp_supabase_url_ = nullptr;
    Fl_Input* inp_supabase_key_ = nullptr;
    Fl_Input* inp_region_ = nullptr;
    Fl_Input* inp_slot_ = nullptr;
    Fl_Input* inp_version_ = nullptr;

    Fl_Button* btn_save_ = nullptr;
    Fl_Button* btn_cancel_ = nullptr;

    AppConfig current_cfg_;
    SaveCallback on_save_;
};
