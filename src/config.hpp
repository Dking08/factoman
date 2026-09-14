#pragma once

#include <string>

struct AppConfig {
    std::string user_token = "TOKEN";
    std::string supabase_url = "";
    std::string supabase_key = "";
    std::string player_nick = "Player";
    std::string region = "ap-south-1";
    std::string save_slot = "slot1";
    std::string factorio_version = "2.1.17";
    std::string options_json = "{\"elevated-rails\":true,\"quality\":true,\"recycler\":true,\"space-age\":true}";
    int sync_interval_sec = 3;

    static AppConfig load(const std::string& filepath = "config.json");
    bool save(const std::string& filepath = "config.json") const;
};
