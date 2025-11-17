//
// Created by Daniel Griffiths on 11/1/25.
//

#ifndef QUANT_FORGE_CURL_EASY_HPP
#define QUANT_FORGE_CURL_EASY_HPP

#include <curl/curl.h>

#include <chrono>
#include <string>
#include <vector>

#include "../cache/network_cache.hpp"
#include "../model/model.hpp"
#include "interface.hpp"

struct curl_slist;

namespace http::client {
    const size_t ERROR_BUFFER_SIZE = 256;

    enum class HttpStatusCode : long {
        TOO_MANY_REQUESTS = 429,
        BAD_GATEWAY = 502,
        SERVICE_UNAVAILABLE = 503,
        GATEWAY_TIMEOUT = 504,
        NOT_MODIFIED = 304,
        OK = 200,
    };

    class CurlEasy : public IHttpClient {
       public:
        CurlEasy(std::unique_ptr<http::cache::NetworkCache> cache_layer);

        ~CurlEasy() override;
        CurlEasy(const CurlEasy&) = delete;
        CurlEasy& operator=(const CurlEasy&) = delete;
        CurlEasy(CurlEasy&&) = delete;
        CurlEasy& operator=(CurlEasy&&) = delete;

        http::model::Response get(const http::model::Request& req) override;
        http::model::Response get_with_retries(const http::model::Request& req, const http::cache::RetryPolicy& p = {}) override;
        static std::chrono::milliseconds get_retry_delay(const http::cache::RetryPolicy& p, std::chrono::milliseconds& delay);
        void set_url(const std::string& u);
        void set_headers(const std::vector<std::string>& hs);
        void enable_keepalive();
        void enable_compression();
        void prefer_http2_tls();

       private:
        template <typename T>
        void setopt(int option, T value);  // defined in .cpp with CURLoption

        void perform_throw();
        http::model::Response make_response(std::string& incoming_body);
        void set_defaults_once();
        void prepare_for_new_request(std::string& body);
        static size_t header_cb(char* buffer, size_t size, size_t n_items, void* userdata);
        static bool is_retryable_http(long code) {
            return code == static_cast<long>(HttpStatusCode::TOO_MANY_REQUESTS) || code == static_cast<long>(HttpStatusCode::BAD_GATEWAY) ||
                   code == static_cast<long>(HttpStatusCode::SERVICE_UNAVAILABLE) || code == static_cast<long>(HttpStatusCode::GATEWAY_TIMEOUT);
        }

        long last_content_length_ = -1;
        long last_rl_remaining_ = -1;
        long last_rl_reset_ = -1;

        std::string last_etag_;
        std::string last_last_modified_;
        std::string last_retry_after_;
        std::string last_content_type_;
        std::string last_cache_control_;

        std::vector<std::string> last_response_headers_;
        std::array<char, ERROR_BUFFER_SIZE> error_buf_{};
        curl_slist* headers_{};

        CURL* handle_{};
        std::unique_ptr<http::cache::NetworkCache> cache_layer_;
    };
}  // namespace http::client

#endif
