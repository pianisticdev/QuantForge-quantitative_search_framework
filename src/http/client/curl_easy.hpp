//
// Created by Daniel Griffiths on 11/1/25.
//

#ifndef MONTE_CARLO_AND_BACKTESTER_CURL_EASY_HPP
#define MONTE_CARLO_AND_BACKTESTER_CURL_EASY_HPP

#include <string>
#include <vector>
#include <curl/curl.h>
#include "../model/model.hpp"
#include "../cache/network_cache.hpp"

struct curl_slist;

namespace http::client {
    class CurlEasy {
    public:
        CurlEasy(std::shared_ptr<http::cache::NetworkCache> cache_layer);
        ~CurlEasy();
        CurlEasy(const CurlEasy&) = delete;
        CurlEasy& operator=(const CurlEasy&) = delete;

        http::model::Response get(const http::model::Request& req);
        http::model::Response get_with_retries(const http::model::Request& req, const http::cache::RetryPolicy& p = {});

        void set_url(const std::string& u);
        void set_headers(const std::vector<std::string>& hs);

        void enable_keepalive();
        void enable_compression();
        void prefer_http2_tls();

    private:
        template <typename T>
        void setopt(int option, T value); // defined in .cpp with CURLoption

        void perform_throw();
        http::model::Response make_response(std::string& incoming_body);

        CURL* handle_{};
        char error_buf_[256]{};
        curl_slist* headers_{};

        void set_defaults_once();
        void prepare_for_new_request(std::string& body);
        static size_t header_cb(char* buffer, size_t size, size_t n_items, void* userdata);
        std::vector<std::string> last_response_headers_;
        long last_content_length_ = -1;

        static bool is_retryable_http(long code) {
            return code == 429 || code == 502 || code == 503 || code == 504;
        }

        std::string last_etag_, last_last_modified_, last_retry_after_;
        long last_rl_remaining_ = -1;
        long last_rl_reset_ = -1;
        std::string last_content_type_;
        std::string last_cache_control_;

        std::shared_ptr<http::cache::NetworkCache> cache_layer_;
    };
}

#endif //MONTE_CARLO_AND_BACKTESTER_CURL_EASY_HPP
