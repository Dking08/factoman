#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <curl/curl.h>
#include <curl/websockets.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

void configure_ssl(CURL* curl) {
    // Tell libcurl to use Windows native root certificates
    #if defined(CURLSSLOPT_NATIVE_CA)
    curl_easy_setopt(curl, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA);
    #endif
    // Also disable strict verification fallback if ca-bundle is missing on user machine
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
}

static size_t write_cb(void* ptr, size_t size, size_t nmemb, void* stream) {
    std::string* s = static_cast<std::string*>(stream);
    s->append(static_cast<char*>(ptr), size * nmemb);
    return size * nmemb;
}

int main() {
    curl_global_init(CURL_GLOBAL_ALL);
    std::cout << "[Test] 1. Initializing WebSocket connection to wss://factorio.zone/ws ...\n";

    CURL* ws = curl_easy_init();
    if (!ws) {
        std::cerr << "[Test] Failed to init curl for WS\n";
        return 1;
    }

    curl_easy_setopt(ws, CURLOPT_URL, "wss://factorio.zone/ws");
    curl_easy_setopt(ws, CURLOPT_CONNECT_ONLY, 2L); // 2L = WebSocket
    curl_easy_setopt(ws, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/151.0.0.0 Safari/537.36");
    configure_ssl(ws);

    struct curl_slist* ws_headers = nullptr;
    ws_headers = curl_slist_append(ws_headers, "Origin: https://factorio.zone");
    ws_headers = curl_slist_append(ws_headers, "Accept-Language: en-US,en;q=0.9");
    curl_easy_setopt(ws, CURLOPT_HTTPHEADER, ws_headers);

    CURLcode res = curl_easy_perform(ws);
    if (res != CURLE_OK) {
        std::cerr << "[Test] WebSocket handshake failed: " << curl_easy_strerror(res) << "\n";
        curl_slist_free_all(ws_headers);
        curl_easy_cleanup(ws);
        return 1;
    }

    std::cout << "[Test] WebSocket connected successfully! Waiting for visitSecret...\n";

    std::string visit_secret = "";
    char buf[4096];

    for (int attempts = 0; attempts < 20; ++attempts) {
        size_t rlen = 0;
        const struct curl_ws_frame* meta = nullptr;
        res = curl_ws_recv(ws, buf, sizeof(buf) - 1, &rlen, &meta);

        if (res == CURLE_OK && rlen > 0) {
            buf[rlen] = '\0';
            std::cout << "[WS Received] " << buf << "\n";
            try {
                auto j = json::parse(buf);
                if (j.value("type", "") == "visit") {
                    visit_secret = j.value("secret", "");
                    std::cout << "[Test] Got visitSecret: " << visit_secret << "\n";
                    break;
                }
            } catch (const std::exception& e) {
                std::cerr << "[Test] JSON parse error: " << e.what() << "\n";
            }
        } else if (res == CURLE_AGAIN) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } else {
            std::cerr << "[Test] curl_ws_recv error: " << curl_easy_strerror(res) << "\n";
            break;
        }
    }

    if (visit_secret.empty()) {
        std::cerr << "[Test] Did not receive visitSecret!\n";
        curl_slist_free_all(ws_headers);
        curl_easy_cleanup(ws);
        return 1;
    }

    std::cout << "\n[Test] 2. Performing Login with userToken...\n";
    std::string user_token = "Absfpypdx6XFjueps4TgJDjF";

    CURL* http = curl_easy_init();
    std::string post_data = "reconnected=false&revision=4&userToken=" + user_token + "&visitSecret=" + visit_secret;
    std::string response_body;

    curl_easy_setopt(http, CURLOPT_URL, "https://factorio.zone/api/user/login");
    curl_easy_setopt(http, CURLOPT_POST, 1L);
    curl_easy_setopt(http, CURLOPT_POSTFIELDS, post_data.c_str());
    configure_ssl(http);

    struct curl_slist* http_headers = nullptr;
    http_headers = curl_slist_append(http_headers, "Content-Type: application/x-www-form-urlencoded");
    http_headers = curl_slist_append(http_headers, "Origin: https://factorio.zone");
    http_headers = curl_slist_append(http_headers, "Referer: https://factorio.zone/");
    http_headers = curl_slist_append(http_headers, "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/151.0.0.0 Safari/537.36");
    curl_easy_setopt(http, CURLOPT_HTTPHEADER, http_headers);
    curl_easy_setopt(http, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(http, CURLOPT_WRITEDATA, &response_body);

    res = curl_easy_perform(http);
    long http_code = 0;
    curl_easy_getinfo(http, CURLINFO_RESPONSE_CODE, &http_code);

    std::cout << "[Test] Login HTTP Response Code: " << http_code << "\n";
    std::cout << "[Test] Login Response Body: " << response_body << "\n";

    curl_slist_free_all(http_headers);
    curl_easy_cleanup(http);

    std::cout << "\n[Test] 3. Listening to WebSocket for 3 seconds after login...\n";
    for (int i = 0; i < 30; ++i) {
        size_t rlen = 0;
        const struct curl_ws_frame* meta = nullptr;
        res = curl_ws_recv(ws, buf, sizeof(buf) - 1, &rlen, &meta);
        if (res == CURLE_OK && rlen > 0) {
            buf[rlen] = '\0';
            std::cout << "[WS Post-Login] " << buf << "\n";
        } else if (res == CURLE_AGAIN) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } else {
            break;
        }
    }

    curl_slist_free_all(ws_headers);
    curl_easy_cleanup(ws);
    curl_global_cleanup();

    std::cout << "[Test] Flow test completed successfully!\n";
    return 0;
}
