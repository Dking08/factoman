#include "supabase_sync.hpp"
#include <nlohmann/json.hpp>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

using json = nlohmann::json;

SupabaseSync::SupabaseSync() = default;

SupabaseSync::~SupabaseSync() {
    stop_polling();
}

void SupabaseSync::set_callbacks(SyncCallback on_sync, StatusMessageCallback on_status) {
    std::lock_guard<std::mutex> lock(mutex_);
    on_sync_ = std::move(on_sync);
    on_status_ = std::move(on_status);
}

void SupabaseSync::configure(const std::string& url, const std::string& anon_key, int interval_sec) {
    std::lock_guard<std::mutex> lock(mutex_);
    url_ = url;
    // Strip trailing slash if present
    if (!url_.empty() && url_.back() == '/') {
        url_.pop_back();
    }
    key_ = anon_key;
    interval_sec_ = interval_sec > 0 ? interval_sec : 3;
}

bool SupabaseSync::is_configured() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return !url_.empty() && !key_.empty();
}

void SupabaseSync::start_polling() {
    if (running_.load()) return;
    running_.store(true);
    poll_thread_ = std::thread(&SupabaseSync::polling_worker, this);
}

void SupabaseSync::stop_polling() {
    running_.store(false);
    if (poll_thread_.joinable()) {
        poll_thread_.join();
    }
}

static std::string get_current_iso_time() {
    auto now = std::chrono::system_clock::now();
    std::time_t tt = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    gmtime_s(&tm, &tt);
#else
    gmtime_r(&tt, &tm);
#endif
    std::ostringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

bool SupabaseSync::fetch_remote_state(RemoteServerState& out_state) {
    std::string base_url, api_key;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        base_url = url_;
        api_key = key_;
    }

    if (base_url.empty() || api_key.empty()) {
        return false;
    }

    std::string endpoint = base_url + "/rest/v1/server_state?id=eq.1&select=*";
    std::vector<std::string> headers = {
        "apikey: " + api_key,
        "Authorization: Bearer " + api_key,
        "Accept: application/json"
    };

    auto resp = HttpClient::get(endpoint, headers, 8);
    if (!resp.success) {
        return false;
    }

    try {
        auto j = json::parse(resp.body);
        if (!j.is_array() || j.empty()) {
            return false;
        }

        const auto& row = j[0];
        out_state.status = row.value("status", "OFFLINE");
        out_state.server_ip = row.value("server_ip", "");
        out_state.launch_id = row.value("launch_id", 0LL);
        out_state.region = row.value("region", "ap-south-1");
        out_state.save_slot = row.value("save_slot", "slot1");
        out_state.version = row.value("version", "2.1.17");
        out_state.updated_by = row.value("updated_by", "Unknown");
        out_state.updated_at = row.value("updated_at", "");
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[Supabase] JSON parse error: " << e.what() << "\n";
        return false;
    }
}

bool SupabaseSync::publish_state(const std::string& status,
                                 const std::string& server_ip,
                                 long long launch_id,
                                 const std::string& player_nick,
                                 const std::string& region,
                                 const std::string& save_slot,
                                 const std::string& version) {
    std::string base_url, api_key;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        base_url = url_;
        api_key = key_;
    }

    if (base_url.empty() || api_key.empty()) {
        return false;
    }

    std::string endpoint = base_url + "/rest/v1/server_state?id=eq.1";
    std::vector<std::string> headers = {
        "apikey: " + api_key,
        "Authorization: Bearer " + api_key,
        "Content-Type: application/json",
        "Prefer: return=minimal"
    };

    json payload;
    payload["status"] = status;
    payload["server_ip"] = server_ip;
    payload["launch_id"] = launch_id;
    payload["updated_by"] = player_nick.empty() ? "Player" : player_nick;
    payload["updated_at"] = get_current_iso_time();
    if (!region.empty()) payload["region"] = region;
    if (!save_slot.empty()) payload["save_slot"] = save_slot;
    if (!version.empty()) payload["version"] = version;

    auto resp = HttpClient::patch(endpoint, payload.dump(), headers, 8);
    if (resp.success) {
        std::lock_guard<std::mutex> lock(mutex_);
        last_known_state_.status = status;
        last_known_state_.server_ip = server_ip;
        last_known_state_.launch_id = launch_id;
        last_known_state_.updated_by = payload["updated_by"];
        last_known_state_.updated_at = payload["updated_at"];
        return true;
    } else {
        std::cerr << "[Supabase] PATCH failed: " << resp.error << " (HTTP " << resp.status_code << "): " << resp.body << "\n";
        return false;
    }
}

void SupabaseSync::polling_worker() {
    while (running_.load()) {
        if (is_configured()) {
            RemoteServerState remote;
            if (fetch_remote_state(remote)) {
                bool changed = false;
                SyncCallback cb;
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    if (remote.status != last_known_state_.status ||
                        remote.server_ip != last_known_state_.server_ip ||
                        remote.launch_id != last_known_state_.launch_id ||
                        remote.updated_at != last_known_state_.updated_at) {
                        last_known_state_ = remote;
                        changed = true;
                        cb = on_sync_;
                    }
                }

                if (changed && cb) {
                    cb(remote);
                }
            }
        }

        int sleep_sec = interval_sec_;
        for (int i = 0; i < sleep_sec * 10 && running_.load(); ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}
