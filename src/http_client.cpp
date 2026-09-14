#include "http_client.hpp"
#include <curl/curl.h>
#include <iostream>

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total = size * nmemb;
    std::string* s = static_cast<std::string*>(userp);
    s->append(static_cast<char*>(contents), total);
    return total;
}

void HttpClient::global_init() {
    curl_global_init(CURL_GLOBAL_ALL);
}

void HttpClient::global_cleanup() {
    curl_global_cleanup();
}

std::string HttpClient::url_encode(const std::string& value) {
    CURL* curl = curl_easy_init();
    if (!curl) return value;
    char* output = curl_easy_escape(curl, value.c_str(), static_cast<int>(value.length()));
    std::string result = output ? output : "";
    if (output) curl_free(output);
    curl_easy_cleanup(curl);
    return result;
}

HttpResponse HttpClient::get(const std::string& url,
                             const std::vector<std::string>& headers,
                             long timeout_sec) {
    HttpResponse resp;
    CURL* curl = curl_easy_init();
    if (!curl) {
        resp.error = "Failed to initialize CURL";
        return resp;
    }

    struct curl_slist* chunk = nullptr;
    for (const auto& h : headers) {
        chunk = curl_slist_append(chunk, h.c_str());
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp.body);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout_sec);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "FactoMan/1.0 (Windows NT 10.0; Win64; x64)");

    CURLcode res = curl_easy_perform(curl);
    if (res == CURLE_OK) {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resp.status_code);
        resp.success = (resp.status_code >= 200 && resp.status_code < 300);
    } else {
        resp.error = curl_easy_strerror(res);
        resp.success = false;
    }

    if (chunk) curl_slist_free_all(chunk);
    curl_easy_cleanup(curl);
    return resp;
}

HttpResponse HttpClient::post(const std::string& url,
                              const std::string& post_fields,
                              const std::vector<std::string>& headers,
                              long timeout_sec) {
    HttpResponse resp;
    CURL* curl = curl_easy_init();
    if (!curl) {
        resp.error = "Failed to initialize CURL";
        return resp;
    }

    struct curl_slist* chunk = nullptr;
    for (const auto& h : headers) {
        chunk = curl_slist_append(chunk, h.c_str());
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_fields.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp.body);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout_sec);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "FactoMan/1.0 (Windows NT 10.0; Win64; x64)");

    CURLcode res = curl_easy_perform(curl);
    if (res == CURLE_OK) {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resp.status_code);
        resp.success = (resp.status_code >= 200 && resp.status_code < 300);
    } else {
        resp.error = curl_easy_strerror(res);
        resp.success = false;
    }

    if (chunk) curl_slist_free_all(chunk);
    curl_easy_cleanup(curl);
    return resp;
}

HttpResponse HttpClient::patch(const std::string& url,
                               const std::string& json_payload,
                               const std::vector<std::string>& headers,
                               long timeout_sec) {
    HttpResponse resp;
    CURL* curl = curl_easy_init();
    if (!curl) {
        resp.error = "Failed to initialize CURL";
        return resp;
    }

    struct curl_slist* chunk = nullptr;
    for (const auto& h : headers) {
        chunk = curl_slist_append(chunk, h.c_str());
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_payload.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp.body);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout_sec);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "FactoMan/1.0 (Windows NT 10.0; Win64; x64)");

    CURLcode res = curl_easy_perform(curl);
    if (res == CURLE_OK) {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resp.status_code);
        resp.success = (resp.status_code >= 200 && resp.status_code < 300);
    } else {
        resp.error = curl_easy_strerror(res);
        resp.success = false;
    }

    if (chunk) curl_slist_free_all(chunk);
    curl_easy_cleanup(curl);
    return resp;
}
