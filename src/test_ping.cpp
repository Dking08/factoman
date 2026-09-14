#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <curl/curl.h>
#include <curl/websockets.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

void configure_ssl(CURL* curl) {
#if defined(CURLSSLOPT_NATIVE_CA)
    curl_easy_setopt(curl, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA);
#endif
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
}

int main() {
    curl_global_init(CURL_GLOBAL_ALL);
    CURL* ws = curl_easy_init();
    if (!ws) return 1;

    curl_easy_setopt(ws, CURLOPT_URL, "wss://factorio.zone/ws");
    curl_easy_setopt(ws, CURLOPT_CONNECT_ONLY, 2L);
    curl_easy_setopt(ws, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64)");
    configure_ssl(ws);

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Origin: https://factorio.zone");
    curl_easy_setopt(ws, CURLOPT_HTTPHEADER, headers);

    CURLcode res = curl_easy_perform(ws);
    if (res != CURLE_OK) {
        std::cerr << "Connect failed: " << curl_easy_strerror(res) << "\n";
        return 1;
    }

    std::cout << "Connected to WebSocket! Testing PING frame...\n";
    size_t sent = 0;
    res = curl_ws_send(ws, "ping", 4, &sent, 0, CURLWS_PING);
    std::cout << "curl_ws_send PING result: " << curl_easy_strerror(res) << ", sent: " << sent << "\n";

    char buf[4096];
    for (int i = 0; i < 20; ++i) {
        size_t rlen = 0;
        const struct curl_ws_frame* meta = nullptr;
        res = curl_ws_recv(ws, buf, sizeof(buf) - 1, &rlen, &meta);
        if (res == CURLE_OK && rlen > 0) {
            buf[rlen] = '\0';
            std::cout << "Recv (" << rlen << " bytes, flags=" << (meta ? meta->flags : 0) << "): " << buf << "\n";
        } else if (res == CURLE_AGAIN) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } else {
            std::cout << "Recv error: " << curl_easy_strerror(res) << "\n";
            break;
        }
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(ws);
    curl_global_cleanup();
    return 0;
}
