#pragma once

#include <string>
#include <vector>
#include <map>

struct HttpResponse {
    long status_code = 0;
    std::string body;
    std::string error;
    bool success = false;
};

class HttpClient {
public:
    static void global_init();
    static void global_cleanup();

    static std::string url_encode(const std::string& value);

    static HttpResponse get(const std::string& url,
                            const std::vector<std::string>& headers = {},
                            long timeout_sec = 10);

    static HttpResponse post(const std::string& url,
                             const std::string& post_fields,
                             const std::vector<std::string>& headers = {},
                             long timeout_sec = 15);

    static HttpResponse patch(const std::string& url,
                              const std::string& json_payload,
                              const std::vector<std::string>& headers = {},
                              long timeout_sec = 10);
};
