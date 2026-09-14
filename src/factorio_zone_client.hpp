#pragma once

#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include "http_client.hpp"

enum class FzState {
    OFFLINE,
    CONNECTING,
    CONNECTED,
    LOGGED_IN,
    STARTING,
    RUNNING,
    STOPPING,
    ERROR_STATE
};

class FactorioZoneClient {
public:
    using LogCallback = std::function<void(const std::string& line)>;
    using IpCallback = std::function<void(const std::string& ip)>;
    using StatusCallback = std::function<void(FzState state, const std::string& message)>;
    using LaunchIdCallback = std::function<void(long long launch_id)>;
    using SecretCallback = std::function<void(const std::string& secret)>;
    using SlotCallback = std::function<void(const std::string& slot, const std::string& region, const std::string& version)>;

    FactorioZoneClient();
    ~FactorioZoneClient();

    void set_callbacks(LogCallback on_log,
                      IpCallback on_ip,
                      StatusCallback on_status,
                      LaunchIdCallback on_launch_id,
                      SecretCallback on_secret,
                      SlotCallback on_slot = nullptr);

    void set_user_token(const std::string& user_token);
    void start_websocket_thread();
    void stop_websocket_thread();

    bool login(const std::string& user_token);
    bool start_instance(const std::string& region,
                        const std::string& save_slot,
                        const std::string& version,
                        const std::string& options_json);
    bool stop_instance();
    bool send_console(const std::string& command);

    std::string get_visit_secret() const;
    std::string get_server_ip() const;
    long long get_launch_id() const;
    FzState get_state() const;
    bool is_logged_in() const;
    void set_state(FzState new_state, const std::string& message = "");

    void set_server_details(const std::string& ip, long long launch_id, FzState state);
    void cancel_queue();

private:
    void ws_worker_loop();
    void process_raw_ws_data(std::string& data);
    void parse_single_json(const std::string& json_str);

    std::atomic<bool> running_{false};
    std::atomic<bool> is_logged_in_{false};
    std::atomic<bool> cancel_queue_{false};
    std::thread ws_thread_;
    mutable std::mutex mutex_;

    std::string visit_secret_;
    std::string user_token_;
    std::string server_ip_;
    long long launch_id_ = 0;
    FzState state_ = FzState::OFFLINE;

    LogCallback on_log_;
    IpCallback on_ip_;
    StatusCallback on_status_;
    LaunchIdCallback on_launch_id_;
    SecretCallback on_secret_;
    SlotCallback on_slot_;
};
