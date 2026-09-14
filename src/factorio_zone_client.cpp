#include "factorio_zone_client.hpp"
#include <curl/curl.h>
#include <curl/websockets.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <chrono>
#include <regex>

using json = nlohmann::json;

FactorioZoneClient::FactorioZoneClient() = default;

FactorioZoneClient::~FactorioZoneClient() {
    stop_websocket_thread();
}

void FactorioZoneClient::set_callbacks(LogCallback on_log,
                                      IpCallback on_ip,
                                      StatusCallback on_status,
                                      LaunchIdCallback on_launch_id,
                                      SecretCallback on_secret,
                                      SlotCallback on_slot) {
    std::lock_guard<std::mutex> lock(mutex_);
    on_log_ = std::move(on_log);
    on_ip_ = std::move(on_ip);
    on_status_ = std::move(on_status);
    on_launch_id_ = std::move(on_launch_id);
    on_secret_ = std::move(on_secret);
    on_slot_ = std::move(on_slot);
}

void FactorioZoneClient::set_user_token(const std::string& user_token) {
    std::string secret;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        user_token_ = user_token;
        secret = visit_secret_;
    }
    if (!secret.empty() && !user_token.empty()) {
        login(user_token);
    }
}

void FactorioZoneClient::start_websocket_thread() {
    if (running_.load()) return;
    running_.store(true);
    ws_thread_ = std::thread(&FactorioZoneClient::ws_worker_loop, this);
}

void FactorioZoneClient::stop_websocket_thread() {
    running_.store(false);
    if (ws_thread_.joinable()) {
        ws_thread_.join();
    }
}

std::string FactorioZoneClient::get_visit_secret() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return visit_secret_;
}

std::string FactorioZoneClient::get_server_ip() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return server_ip_;
}

long long FactorioZoneClient::get_launch_id() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return launch_id_;
}

FzState FactorioZoneClient::get_state() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return state_;
}

bool FactorioZoneClient::is_logged_in() const {
    return is_logged_in_.load();
}

void FactorioZoneClient::set_state(FzState new_state, const std::string& message) {
    StatusCallback cb;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        state_ = new_state;
        cb = on_status_;
    }
    if (cb) cb(new_state, message);
}

void FactorioZoneClient::set_server_details(const std::string& ip, long long launch_id, FzState state) {
    IpCallback ip_cb;
    LaunchIdCallback lid_cb;
    StatusCallback st_cb;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        server_ip_ = ip;
        launch_id_ = launch_id;
        state_ = state;
        ip_cb = on_ip_;
        lid_cb = on_launch_id_;
        st_cb = on_status_;
    }
    if (ip_cb && !ip.empty()) ip_cb(ip);
    if (lid_cb && launch_id > 0) lid_cb(launch_id);
    if (st_cb) st_cb(state, "Synced from network");
}

bool FactorioZoneClient::login(const std::string& user_token) {
    std::string secret;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        user_token_ = user_token;
        secret = visit_secret_;
    }

    if (secret.empty()) {
        std::cout << "[FZ] Waiting for fresh visitSecret before login...\n";
        return false;
    }

    std::string body = "reconnected=false&revision=4&userToken=" + HttpClient::url_encode(user_token)
                     + "&visitSecret=" + HttpClient::url_encode(secret);

    std::vector<std::string> headers = {
        "Content-Type: application/x-www-form-urlencoded",
        "Origin: https://factorio.zone",
        "Referer: https://factorio.zone/"
    };

    auto resp = HttpClient::post("https://factorio.zone/api/user/login", body, headers);
    if (resp.success) {
        is_logged_in_.store(true);
        set_state(FzState::LOGGED_IN, "Logged in successfully");
        return true;
    } else {
        is_logged_in_.store(false);
        std::cerr << "[FZ] Login failed: " << resp.error << " (HTTP " << resp.status_code << "): " << resp.body << "\n";
        return false;
    }
}

bool FactorioZoneClient::start_instance(const std::string& region,
                                       const std::string& save_slot,
                                       const std::string& version,
                                       const std::string& options_json) {
    std::string secret;
    std::string u_token;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        secret = visit_secret_;
        u_token = user_token_;
    }

    if (secret.empty()) {
        std::cerr << "[FZ] Cannot start: visitSecret empty. Reconnecting to factorio.zone...\n";
        set_state(FzState::ERROR_STATE, "Not connected to factorio.zone yet. Please wait...");
        return false;
    }

    if (!is_logged_in_.load()) {
        std::cout << "[FZ] Not logged in yet, attempting login before start...\n";
        if (!login(u_token)) {
            set_state(FzState::ERROR_STATE, "Login failed. Check your userToken.");
            return false;
        }
    }

    cancel_queue_.store(false);

    std::vector<std::string> headers = {
        "Content-Type: application/x-www-form-urlencoded",
        "Origin: https://factorio.zone",
        "Referer: https://factorio.zone/"
    };

    long long turn_time_to_send = 0;

    while (running_.load() && !cancel_queue_.load()) {
        std::string body = "options=" + HttpClient::url_encode(options_json)
                         + "&region=" + HttpClient::url_encode(region)
                         + "&save=" + HttpClient::url_encode(save_slot);
        if (turn_time_to_send > 0) {
            body += "&turnTime=" + std::to_string(turn_time_to_send);
        }
        body += "&version=" + HttpClient::url_encode(version)
             + "&visitSecret=" + HttpClient::url_encode(secret);

        set_state(FzState::STARTING, turn_time_to_send > 0 ? "Claiming turn in queue..." : "Starting server instance...");
        auto resp = HttpClient::post("https://factorio.zone/api/instance/start", body, headers);

        if (!resp.success) {
            set_state(FzState::ERROR_STATE, "Failed to start: " + resp.body);
            return false;
        }

        std::cout << "[FZ] Start instance response: " << resp.body << "\n";

        json j;
        try {
            j = json::parse(resp.body);
        } catch (const std::exception& e) {
            set_state(FzState::ERROR_STATE, "Invalid JSON from server");
            return false;
        }

        int status_code = j.value("statusCode", 0);

        // Check if we are in queue (HTTP 202 or statusCode == 202 or contains turnTime)
        if (status_code == 202 || (j.contains("turnTime") && !j.contains("launchId"))) {
            long long turn_time = j.value("turnTime", 0LL);
            long long cur_time = j.value("currentTime", 0LL);
            if (cur_time == 0) {
                cur_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            }

            long long wait_ms = turn_time - cur_time;
            if (wait_ms < 0) wait_ms = 0;
            int wait_sec = static_cast<int>((wait_ms + 999) / 1000);

            std::string q_msg = "In queue. Waiting " + std::to_string(wait_sec) + "s for turn (turnTime: " + std::to_string(turn_time) + ")...";
            LogCallback log_cb;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                log_cb = on_log_;
            }
            if (log_cb) log_cb("[Queue] " + q_msg);
            set_state(FzState::STARTING, "In queue (" + std::to_string(wait_sec) + "s remaining)");

            // Wait loop with responsive cancellation check
            auto start_wait = std::chrono::steady_clock::now();
            auto target_wait_ms = wait_ms + 250; // Small 250ms buffer so we don't request before turn
            while (running_.load() && !cancel_queue_.load()) {
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - start_wait).count();
                if (elapsed >= target_wait_ms) break;

                long long rem_sec = (target_wait_ms - elapsed + 999) / 1000;
                set_state(FzState::STARTING, "In queue (" + std::to_string(rem_sec) + "s remaining)");

                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }

            if (cancel_queue_.load() || !running_.load()) {
                set_state(FzState::OFFLINE, "Queue wait cancelled");
                if (log_cb) log_cb("[Queue] Queue wait cancelled.");
                return false;
            }

            // Set turnTime for the next start request
            turn_time_to_send = turn_time;
            continue; // Loop back and resend with &turnTime=...
        }

        // Server started!
        if (j.contains("launchId")) {
            long long lid = j["launchId"].get<long long>();
            LaunchIdCallback lid_cb;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                launch_id_ = lid;
                lid_cb = on_launch_id_;
            }
            if (lid_cb) lid_cb(lid);
        }
        if (j.contains("socket")) {
            std::string sock = j["socket"].get<std::string>();
            IpCallback ip_cb;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                server_ip_ = sock;
                state_ = FzState::RUNNING;
                ip_cb = on_ip_;
            }
            if (ip_cb) ip_cb(sock);
        }
        return true;
    }

    return false;
}

void FactorioZoneClient::cancel_queue() {
    cancel_queue_.store(true);
}

bool FactorioZoneClient::stop_instance() {
    cancel_queue_.store(true);

    std::string secret;
    long long lid = 0;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        secret = visit_secret_;
        lid = launch_id_;
    }

    if (secret.empty() || lid == 0) {
        set_state(FzState::OFFLINE, "Server stopped / Queue cancelled");
        return true;
    }

    std::string body = "launchId=" + std::to_string(lid)
                     + "&visitSecret=" + HttpClient::url_encode(secret);

    std::vector<std::string> headers = {
        "Content-Type: application/x-www-form-urlencoded",
        "Origin: https://factorio.zone",
        "Referer: https://factorio.zone/"
    };

    set_state(FzState::STOPPING, "Stopping server instance...");
    auto resp = HttpClient::post("https://factorio.zone/api/instance/stop", body, headers);
    if (resp.success) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            server_ip_ = "";
            launch_id_ = 0;
            state_ = FzState::OFFLINE;
        }
        set_state(FzState::OFFLINE, "Server stopped");
        return true;
    } else {
        set_state(FzState::ERROR_STATE, "Failed to stop: " + resp.body);
        return false;
    }
}

bool FactorioZoneClient::send_console(const std::string& command) {
    std::string secret;
    long long lid = 0;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        secret = visit_secret_;
        lid = launch_id_;
    }

    if (secret.empty() || lid == 0) {
        std::cerr << "[FZ] Cannot send console: visitSecret or launchId missing\n";
        return false;
    }

    std::string body = "input=" + HttpClient::url_encode(command)
                     + "&launchId=" + std::to_string(lid)
                     + "&visitSecret=" + HttpClient::url_encode(secret);

    std::vector<std::string> headers = {
        "Content-Type: application/x-www-form-urlencoded",
        "Origin: https://factorio.zone",
        "Referer: https://factorio.zone/"
    };

    auto resp = HttpClient::post("https://factorio.zone/api/instance/console", body, headers);
    return resp.success;
}

void FactorioZoneClient::ws_worker_loop() {
    while (running_.load()) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            visit_secret_ = "";
            is_logged_in_.store(false);
        }
        set_state(FzState::CONNECTING, "Connecting to factorio.zone WebSocket...");

        CURL* curl = curl_easy_init();
        if (!curl) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }

        curl_easy_setopt(curl, CURLOPT_URL, "wss://factorio.zone/ws");
        curl_easy_setopt(curl, CURLOPT_CONNECT_ONLY, 2L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/151.0.0.0 Safari/537.36");
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPIDLE, 30L);
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPINTVL, 15L);

#if defined(CURLSSLOPT_NATIVE_CA)
        curl_easy_setopt(curl, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA);
#endif
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Origin: https://factorio.zone");
        headers = curl_slist_append(headers, "Accept-Language: en-US,en;q=0.9");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "[FZ WS] Connect failed: " << curl_easy_strerror(res) << "\n";
            set_state(FzState::ERROR_STATE, "WS connect error: " + std::string(curl_easy_strerror(res)));
            if (headers) curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }

        set_state(FzState::CONNECTED, "Connected to factorio.zone");

        char recv_buf[8192];
        std::string accumulated_data;
        auto last_traffic_time = std::chrono::steady_clock::now();

        while (running_.load()) {
            // Keepalive ping every 15 seconds to ensure connection stability
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(now - last_traffic_time).count() >= 15) {
                size_t sent = 0;
                curl_ws_send(curl, "ping", 4, &sent, 0, CURLWS_PING);
                last_traffic_time = now;
            }

            size_t rlen = 0;
            const struct curl_ws_frame* meta = nullptr;
            res = curl_ws_recv(curl, recv_buf, sizeof(recv_buf), &rlen, &meta);

            if (res == CURLE_OK && rlen > 0) {
                last_traffic_time = std::chrono::steady_clock::now();
                // Ignore PING and PONG frames from JSON parser
                if (meta && (meta->flags & (CURLWS_PING | CURLWS_PONG))) {
                    continue;
                }
                accumulated_data.append(recv_buf, rlen);
                process_raw_ws_data(accumulated_data);
            } else if (res == CURLE_AGAIN) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            } else {
                std::cerr << "[FZ WS] Stream closed or error: " << curl_easy_strerror(res) << "\n";
                break;
            }
        }

        if (headers) curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        {
            std::lock_guard<std::mutex> lock(mutex_);
            visit_secret_ = "";
            is_logged_in_.store(false);
        }

        if (running_.load()) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }
}

void FactorioZoneClient::process_raw_ws_data(std::string& data) {
    size_t start = 0;
    while (start < data.size()) {
        while (start < data.size() && (data[start] == ' ' || data[start] == '\n' || data[start] == '\r' || data[start] == '\t')) {
            start++;
        }
        if (start >= data.size()) break;

        if (data[start] != '{') {
            start++;
            continue;
        }

        int depth = 0;
        bool in_string = false;
        bool escape = false;
        size_t end = start;

        for (; end < data.size(); ++end) {
            char c = data[end];
            if (escape) {
                escape = false;
                continue;
            }
            if (c == '\\') {
                escape = true;
                continue;
            }
            if (c == '"') {
                in_string = !in_string;
                continue;
            }
            if (!in_string) {
                if (c == '{') depth++;
                else if (c == '}') {
                    depth--;
                    if (depth == 0) {
                        end++;
                        break;
                    }
                }
            }
        }

        if (depth == 0 && end <= data.size()) {
            std::string single_json = data.substr(start, end - start);
            parse_single_json(single_json);
            start = end;
        } else {
            break;
        }
    }

    if (start > 0) {
        data.erase(0, start);
    }
}

void FactorioZoneClient::parse_single_json(const std::string& json_str) {
    try {
        auto j = json::parse(json_str);
        std::string type = j.value("type", "");

        if (type == "visit") {
            std::string secret = j.value("secret", "");
            SecretCallback sec_cb;
            std::string u_token;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                visit_secret_ = secret;
                sec_cb = on_secret_;
                u_token = user_token_;
            }
            if (sec_cb) sec_cb(secret);

            // Automatically log in with the new visitSecret immediately
            if (!u_token.empty()) {
                login(u_token);
            }
        } else if (type == "running") {
            // Factorio.zone sends: {"type":"running","launchId":1201670,"socket":"15.252.100.44:25667"}
            long long lid = j.value("launchId", 0LL);
            std::string socket = j.value("socket", "");
            IpCallback ip_cb;
            LaunchIdCallback lid_cb;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (lid > 0) launch_id_ = lid;
                if (!socket.empty()) server_ip_ = socket;
                state_ = FzState::RUNNING;
                ip_cb = on_ip_;
                lid_cb = on_launch_id_;
            }
            if (lid > 0 && lid_cb) lid_cb(lid);
            if (!socket.empty() && ip_cb) ip_cb(socket);
            set_state(FzState::RUNNING, "Server running at " + socket);
        } else if (type == "starting") {
            long long lid = j.value("launchId", 0LL);
            LaunchIdCallback lid_cb;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                launch_id_ = lid;
                state_ = FzState::STARTING;
                lid_cb = on_launch_id_;
            }
            if (lid_cb) lid_cb(lid);
            set_state(FzState::STARTING, "Starting instance (Launch #" + std::to_string(lid) + ")");
        } else if (type == "idle") {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                server_ip_ = "";
                launch_id_ = 0;
                state_ = FzState::OFFLINE;
            }
            set_state(FzState::OFFLINE, "Server is offline (Ready)");
        } else if (type == "slot") {
            std::string slot = j.value("slot", "");
            std::string region = j.value("region", "");
            std::string version = j.value("version", "");
            SlotCallback slot_cb;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                slot_cb = on_slot_;
            }
            if (slot_cb) slot_cb(slot, region, version);
        } else if (type == "info" || type == "log") {
            std::string line = j.value("line", "");
            long long lid = j.value("launchId", 0LL);
            if (lid > 0) {
                LaunchIdCallback lid_cb;
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    if (launch_id_ == 0) {
                        launch_id_ = lid;
                        lid_cb = on_launch_id_;
                    }
                }
                if (lid_cb) lid_cb(lid);
            }

            if (line == "ready") {
                set_state(FzState::OFFLINE, "Ready");
            }

            // Check for IP assignment: "selecting connection 65.0.66.221:21914"
            static const std::regex ip_regex("selecting connection ([0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+:[0-9]+)");
            std::smatch match;
            if (std::regex_search(line, match, ip_regex)) {
                std::string ip = match[1].str();
                IpCallback ip_cb;
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    server_ip_ = ip;
                    state_ = FzState::RUNNING;
                    ip_cb = on_ip_;
                }
                if (ip_cb) ip_cb(ip);
                set_state(FzState::RUNNING, "Server running at " + ip);
            }

            LogCallback log_cb;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                log_cb = on_log_;
            }
            if (log_cb) log_cb(line);
        }
    } catch (const std::exception& e) {
        std::cerr << "[FZ Parse] Failed to parse JSON: " << e.what() << " | Data: " << json_str << "\n";
    }
}
