#pragma once

#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include "http_client.hpp"

struct RemoteServerState {
    std::string status = "OFFLINE"; // OFFLINE, STARTING, RUNNING, STOPPING
    std::string server_ip = "";
    long long launch_id = 0;
    std::string region = "ap-south-1";
    std::string save_slot = "slot1";
    std::string version = "2.1.17";
    std::string updated_by = "System";
    std::string updated_at = "";
};

class SupabaseSync {
public:
    using SyncCallback = std::function<void(const RemoteServerState& state)>;
    using StatusMessageCallback = std::function<void(const std::string& msg, bool is_error)>;

    SupabaseSync();
    ~SupabaseSync();

    void set_callbacks(SyncCallback on_sync, StatusMessageCallback on_status);
    void configure(const std::string& url, const std::string& anon_key, int interval_sec = 3);

    void start_polling();
    void stop_polling();

    bool fetch_remote_state(RemoteServerState& out_state);
    bool publish_state(const std::string& status,
                       const std::string& server_ip,
                       long long launch_id,
                       const std::string& player_nick,
                       const std::string& region = "",
                       const std::string& save_slot = "",
                       const std::string& version = "");

    bool fetch_token_by_key(const std::string& key, std::string& out_token, std::string& out_err);
    bool fetch_all_token_keys(std::vector<std::string>& out_keys, std::string& out_err);
    bool save_token_to_cloud(const std::string& key, const std::string& token, std::string& out_err);

    bool is_configured() const;

private:
    void polling_worker();

    std::string url_;
    std::string key_;
    int interval_sec_ = 3;

    std::atomic<bool> running_{false};
    std::thread poll_thread_;
    mutable std::mutex mutex_;

    SyncCallback on_sync_;
    StatusMessageCallback on_status_;

    RemoteServerState last_known_state_;
};
