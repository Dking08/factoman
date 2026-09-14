#include "config.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

AppConfig AppConfig::load(const std::string& filepath) {
    AppConfig cfg;
    std::ifstream file(filepath);
    if (!file.is_open()) {
        // Create default config file if it does not exist
        cfg.save(filepath);
        return cfg;
    }

    try {
        json j;
        file >> j;
        if (j.contains("user_token")) cfg.user_token = j["user_token"].get<std::string>();
        if (j.contains("supabase_url")) cfg.supabase_url = j["supabase_url"].get<std::string>();
        if (j.contains("supabase_key")) cfg.supabase_key = j["supabase_key"].get<std::string>();
        if (j.contains("player_nick")) cfg.player_nick = j["player_nick"].get<std::string>();
        if (j.contains("region")) cfg.region = j["region"].get<std::string>();
        if (j.contains("save_slot")) cfg.save_slot = j["save_slot"].get<std::string>();
        if (j.contains("factorio_version")) cfg.factorio_version = j["factorio_version"].get<std::string>();
        if (j.contains("options_json")) cfg.options_json = j["options_json"].get<std::string>();
        if (j.contains("sync_interval_sec")) cfg.sync_interval_sec = j["sync_interval_sec"].get<int>();
    } catch (const std::exception& e) {
        std::cerr << "[Config] Error parsing " << filepath << ": " << e.what() << "\n";
    }

    return cfg;
}

bool AppConfig::save(const std::string& filepath) const {
    try {
        json j;
        j["user_token"] = user_token;
        j["supabase_url"] = supabase_url;
        j["supabase_key"] = supabase_key;
        j["player_nick"] = player_nick;
        j["region"] = region;
        j["save_slot"] = save_slot;
        j["factorio_version"] = factorio_version;
        j["options_json"] = options_json;
        j["sync_interval_sec"] = sync_interval_sec;

        std::ofstream file(filepath);
        if (!file.is_open()) return false;
        file << j.dump(4);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[Config] Error saving " << filepath << ": " << e.what() << "\n";
        return false;
    }
}
