//
// Created by Daniel Griffiths on 11/1/25.
//

#include "curl_easy.hpp"

#include <curl/curl.h>

#include <random>
#include <string>
#include <thread>
#include <vector>

#include "../../utils/string_utils.hpp"
#include "../cache/network_cache.hpp"
#include "../model/model.hpp"

using namespace std::chrono;

namespace http::client {

    struct CurlDefaults {
        static constexpr long FOLLOW_LOCATION = 1L;
        static constexpr long MAX_REDIRECTS = 10L;
        static constexpr long CONNECT_TIMEOUT_MS = 10'000L;
        static constexpr long TIMEOUT_MS = 30'000L;
        static constexpr const char* USER_AGENT = "cpp-libcurl-client/1.0";
        static constexpr const char* ACCEPT_ENCODING = "";
        static constexpr long HTTP_VERSION = CURL_HTTP_VERSION_2TLS;
        static constexpr long NO_PROGRESS = 1L;
        static constexpr long NO_SIGNAL = 1L;
        static constexpr long TCP_KEEPALIVE = 1L;
        static constexpr long TCP_KEEPIDLE = 120L;
        static constexpr long TCP_KEEPINTVL = 60L;
        static constexpr long POST = 0L;
        static constexpr long UPLOAD = 0L;
        static constexpr const char* CUSTOM_REQUEST = nullptr;
        static constexpr long HTTP_GET = 1L;
    };

    struct HeaderKeys {
        static constexpr const char* CONTENT_LENGTH = "content-length:";
        static constexpr const char* CONTENT_TYPE = "content-type:";
        static constexpr const char* ETAG = "etag:";
        static constexpr const char* LAST_MODIFIED = "last-modified:";
        static constexpr const char* RETRY_AFTER = "retry-after:";
        static constexpr const char* X_RATELIMIT_REMAINING = "x-ratelimit-remaining:";
        static constexpr const char* X_RATELIMIT_RESET = "x-ratelimit-reset:";
        static constexpr const char* CACHE_CONTROL = "cache-control:";
    };

    CurlEasy::CurlEasy(std::shared_ptr<http::cache::NetworkCache> cache_layer) : handle_(curl_easy_init()), cache_layer_(std::move(cache_layer)) {
        if (handle_ == nullptr) {
            throw std::runtime_error("Failed to create CURL easy handle");
        }

        error_buf_[0] = '\0';

        set_defaults_once();
    }

    CurlEasy::~CurlEasy() {
        if (headers_ != nullptr) {
            curl_slist_free_all(headers_);
        }

        if (handle_ != nullptr) {
            curl_easy_cleanup(handle_);
        }
    }

    void CurlEasy::set_url(const std::string& u) { setopt(CURLOPT_URL, u.c_str()); }

    void CurlEasy::set_headers(const std::vector<std::string>& hs) {
        if (headers_ != nullptr) {
            curl_slist_free_all(headers_);
            headers_ = nullptr;
        }
        for (const auto& h : hs) {
            headers_ = curl_slist_append(headers_, h.c_str());
        }
        if (headers_ != nullptr) {
            setopt(CURLOPT_HTTPHEADER, headers_);
        }
    }

    void CurlEasy::set_defaults_once() {
        setopt(CURLOPT_ERRORBUFFER, error_buf_.data());
        setopt(CURLOPT_FOLLOWLOCATION, CurlDefaults::FOLLOW_LOCATION);
        setopt(CURLOPT_MAXREDIRS, CurlDefaults::MAX_REDIRECTS);
        setopt(CURLOPT_CONNECTTIMEOUT_MS, CurlDefaults::CONNECT_TIMEOUT_MS);
        setopt(CURLOPT_TIMEOUT_MS, CurlDefaults::TIMEOUT_MS);
        setopt(CURLOPT_NOPROGRESS, CurlDefaults::NO_PROGRESS);
        setopt(CURLOPT_USERAGENT, CurlDefaults::USER_AGENT);
        setopt(CURLOPT_NOSIGNAL, CurlDefaults::NO_SIGNAL);  // safe in multithreaded apps
    }

    void CurlEasy::enable_keepalive() {
        setopt(CURLOPT_TCP_KEEPALIVE, CurlDefaults::TCP_KEEPALIVE);
        setopt(CURLOPT_TCP_KEEPIDLE, CurlDefaults::TCP_KEEPIDLE);
        setopt(CURLOPT_TCP_KEEPINTVL, CurlDefaults::TCP_KEEPINTVL);
    }

    void CurlEasy::enable_compression() {
        // Empty string => accept all supported encodings (gzip/deflate/br)
        setopt(CURLOPT_ACCEPT_ENCODING, CurlDefaults::ACCEPT_ENCODING);
    }

    void CurlEasy::prefer_http2_tls() { setopt(CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2TLS); }

    void CurlEasy::prepare_for_new_request(std::string& body) {
        // Clear per-request scratch
        last_response_headers_.clear();
        last_content_length_ = -1;
        body.clear();

        // Always set these per request (don’t rely on old values)
        setopt(CURLOPT_HTTPGET, CurlDefaults::HTTP_GET);
        setopt(CURLOPT_WRITEFUNCTION, &::string_utils::write_to_string);
        setopt(CURLOPT_WRITEDATA, &body);
        setopt(CURLOPT_HEADERFUNCTION, &CurlEasy::header_cb);
        setopt(CURLOPT_HEADERDATA, this);

        // If you later add POST/PUT elsewhere, ensure they’re disabled here:
        setopt(CURLOPT_POST, CurlDefaults::POST);
        setopt(CURLOPT_UPLOAD, CurlDefaults::UPLOAD);
        setopt(CURLOPT_CUSTOMREQUEST, CurlDefaults::CUSTOM_REQUEST);  // clears any previous custom verb
    }

    size_t CurlEasy::header_cb(char* buffer, size_t size, size_t n_items, void* userdata) {
        auto* self = static_cast<CurlEasy*>(userdata);
        const size_t bytes = size * n_items;

        self->last_response_headers_.emplace_back(buffer, bytes);

        http::cache::NetworkCache::extract_header_value(buffer, bytes, HeaderKeys::CONTENT_LENGTH, self->last_content_length_);
        http::cache::NetworkCache::extract_header_value(buffer, bytes, HeaderKeys::CONTENT_TYPE, self->last_content_type_);
        http::cache::NetworkCache::extract_header_value(buffer, bytes, HeaderKeys::ETAG, self->last_etag_);
        http::cache::NetworkCache::extract_header_value(buffer, bytes, HeaderKeys::LAST_MODIFIED, self->last_last_modified_);
        http::cache::NetworkCache::extract_header_value(buffer, bytes, HeaderKeys::RETRY_AFTER, self->last_retry_after_);
        http::cache::NetworkCache::extract_header_value(buffer, bytes, HeaderKeys::X_RATELIMIT_REMAINING, self->last_rl_remaining_);
        http::cache::NetworkCache::extract_header_value(buffer, bytes, HeaderKeys::X_RATELIMIT_RESET, self->last_rl_reset_);
        http::cache::NetworkCache::extract_header_value(buffer, bytes, HeaderKeys::CACHE_CONTROL, self->last_cache_control_);

        return bytes;
    }

    http::model::Response CurlEasy::get(const http::model::Request& req) {
        std::optional<http::cache::NetworkCache::Hit> hit = cache_layer_->probe(req);

        if (hit && cache_layer_->fresh_enough(hit->meta_)) {
            return cache_layer_->get_cached_response(std::move(*hit));
        }

        // add conditional headers if revalidating
        std::vector<std::string> hdrs = req.headers_;
        if (hit) {
            if (!hit->meta_.etag_.empty()) {
                hdrs.push_back("If-None-Match: " + hit->meta_.etag_);
            }
            if (!hit->meta_.last_modified_.empty()) {
                hdrs.push_back("If-Modified-Since: " + hit->meta_.last_modified_);
            }
        }

        set_url(req.url_);
        set_headers(hdrs);

        std::string body;
        prepare_for_new_request(body);

        // IMPROVEMENT [out of scope]: Not sure if the juice is worth the squeeze here, but reserve body size if known:
        if (last_content_length_ > 0) {
            body.reserve(static_cast<size_t>(last_content_length_));
        }

        perform_throw();
        http::model::Response r = make_response(body);

        if (r.status_ == static_cast<long>(HttpStatusCode::NOT_MODIFIED) && hit) {
            // Optionally refresh stored_at in meta:
            cache_layer_->refresh_meta_timestamp(hit->base_path_, hit->meta_);
            return cache_layer_->get_cached_response(std::move(*hit));
        }

        if (cache_layer_) {
            cache_layer_->cache_response(req, r);
        }

        return r;
    }

    http::model::Response CurlEasy::get_with_retries(const http::model::Request& req, const http::cache::RetryPolicy& p) {
        milliseconds delay = p.base_delay_;

        for (size_t attempt = 1; attempt <= p.max_tries_; ++attempt) {
            try {
                auto resp = get(req);

                if (is_retryable_http(resp.status_) && attempt < p.max_tries_) {
                    delay = CurlEasy::get_retry_delay(p, delay);
                    continue;
                }

                if (resp.status_ != static_cast<long>(HttpStatusCode::TOO_MANY_REQUESTS)) {
                    return resp;
                }

                if (!resp.retry_after_.empty()) {
                    char* end = nullptr;
                    const long s = std::strtol(resp.retry_after_.c_str(), &end, 10);
                    if (s > 0) {
                        std::this_thread::sleep_for(seconds{s});
                    }
                } else if (resp.rate_limit_remaining_ == 0 && resp.rate_limit_reset_ > 0) {
                    const auto now = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
                    const long wait = resp.rate_limit_reset_ > now ? (resp.rate_limit_reset_ - now) : 1;
                    std::this_thread::sleep_for(seconds{wait});
                } else if (attempt < p.max_tries_) {
                    delay = CurlEasy::get_retry_delay(p, delay);
                } else {
                    return resp;
                }
            } catch (const std::exception&) {
                if (attempt < p.max_tries_) {
                    delay = CurlEasy::get_retry_delay(p, delay);
                    continue;
                }
                throw;
            }
        }

        return {};
    }

    milliseconds CurlEasy::get_retry_delay(const http::cache::RetryPolicy& p, milliseconds& delay) {
        std::minstd_rand rng{std::random_device{}()};

        auto jitter = [&](milliseconds base) {
            std::uniform_int_distribution<int> d(0, static_cast<int>(base.count()));
            return milliseconds{d(rng)};
        };

        std::this_thread::sleep_for(delay + jitter(p.base_delay_));
        return std::min(delay * 2, p.max_delay_);
    }

    template <typename T>
    void CurlEasy::setopt(int option, T value) {
        const auto rc = curl_easy_setopt(handle_, static_cast<CURLoption>(option), value);

        if (rc != CURLE_OK) {
            throw std::runtime_error(std::string("curl_easy_setopt failed: ") + curl_easy_strerror(rc));
        }
    }
    // explicit instantiations for used types (optional but can help some compilers)
    template void CurlEasy::setopt<long>(int, long);
    template void CurlEasy::setopt<const char*>(int, const char*);
    template void CurlEasy::setopt<void*>(int, void*);

    void CurlEasy::perform_throw() {
        const auto rc = curl_easy_perform(handle_);

        if (rc == CURLE_OK) {
            return;
        }

        std::string err = "curl_easy_perform failed: ";

        if (error_buf_[0] != '\0') {
            err += error_buf_.data();
        } else {
            err += curl_easy_strerror(rc);
        }

        throw std::runtime_error(err);
    }

    http::model::Response CurlEasy::make_response(std::string& incoming_body) {
        long code = 0;
        char* eff = nullptr;
        curl_easy_getinfo(handle_, CURLINFO_RESPONSE_CODE, &code);
        curl_easy_getinfo(handle_, CURLINFO_EFFECTIVE_URL, &eff);

        http::model::Response r;
        r.status_ = code;
        r.body_ = std::move(incoming_body);
        r.effective_url_ = eff != nullptr ? eff : std::string{};
        r.etag_ = std::move(last_etag_);
        r.last_modified_ = std::move(last_last_modified_);
        r.retry_after_ = std::move(last_retry_after_);
        r.rate_limit_remaining_ = last_rl_remaining_;
        r.rate_limit_reset_ = last_rl_reset_;
        r.content_type_ = std::move(last_content_type_);
        r.cache_control_ = std::move(last_cache_control_);
        return r;
    }

}  // namespace http::client
