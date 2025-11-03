//
// Created by Daniel Griffiths on 11/1/25.
//

#include <string>
#include <vector>
#include <random>
#include <curl/curl.h>
#include <thread>
#include "../model/model.hpp"
#include "../cache/network_cache.hpp"
#include "../../utils/string_utils.hpp"
#include "curl_easy.hpp"


namespace http::client {
    CurlEasy::CurlEasy(std::shared_ptr<http::cache::NetworkCache> cache_layer) : handle_(curl_easy_init()), cache_layer_(cache_layer) {
        if (!handle_) throw std::runtime_error("Failed to create CURL easy handle");

        error_buf_[0] = '\0';
        set_defaults_once();
    }

    CurlEasy::~CurlEasy() {
        if (headers_) curl_slist_free_all(headers_);
        if (handle_) curl_easy_cleanup(handle_);
    }

    void CurlEasy::set_url(const std::string& u) {
        setopt(CURLOPT_URL, u.c_str());
    }

    void CurlEasy::set_headers(const std::vector<std::string>& hs) {
        if (headers_) {
            curl_slist_free_all(headers_);
            headers_ = nullptr;
        }
        for (auto& h : hs) {
            headers_ = curl_slist_append(headers_, h.c_str());
        }
        if (headers_) {
            setopt(CURLOPT_HTTPHEADER, headers_);
        }
    }

    void CurlEasy::set_defaults_once() {
        setopt(CURLOPT_ERRORBUFFER,      error_buf_);
        setopt(CURLOPT_FOLLOWLOCATION,   1L);
        setopt(CURLOPT_MAXREDIRS,        10L);
        setopt(CURLOPT_CONNECTTIMEOUT_MS,10'000L);
        setopt(CURLOPT_TIMEOUT_MS,       30'000L);
        setopt(CURLOPT_NOPROGRESS,       1L);
        setopt(CURLOPT_USERAGENT,        "cpp-libcurl-client/1.0");
        setopt(CURLOPT_NOSIGNAL,         1L);     // safe in multithreaded apps
    }

    void CurlEasy::enable_keepalive() {
        setopt(CURLOPT_TCP_KEEPALIVE, 1L);
        setopt(CURLOPT_TCP_KEEPIDLE,  120L);
        setopt(CURLOPT_TCP_KEEPINTVL, 60L);
    }

    void CurlEasy::enable_compression() {
        // Empty string => accept all supported encodings (gzip/deflate/br)
        setopt(CURLOPT_ACCEPT_ENCODING, "");
    }

    void CurlEasy::prefer_http2_tls() {
        setopt(CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2TLS);
    }

    void CurlEasy::prepare_for_new_request(std::string& body) {
        // Clear per-request scratch
        last_response_headers_.clear();
        last_content_length_ = -1;
        body.clear();

        // Always set these per request (don’t rely on old values)
        setopt(CURLOPT_HTTPGET, 1L);
        setopt(CURLOPT_WRITEFUNCTION, &::string_utils::write_to_string);
        setopt(CURLOPT_WRITEDATA, &body);
        setopt(CURLOPT_HEADERFUNCTION, &CurlEasy::header_cb);
        setopt(CURLOPT_HEADERDATA, this);

        // If you later add POST/PUT elsewhere, ensure they’re disabled here:
        setopt(CURLOPT_POST, 0L);
        setopt(CURLOPT_UPLOAD, 0L);
        setopt(CURLOPT_CUSTOMREQUEST, nullptr); // clears any previous custom verb
    }

    size_t CurlEasy::header_cb(char* buffer, size_t size, size_t n_items, void* userdata) {
        auto self = static_cast<CurlEasy*>(userdata);
        const size_t bytes = size * n_items;

        self->last_response_headers_.emplace_back(buffer, bytes);

        http::cache::NetworkCache::extract_header_value(buffer, bytes, "content-length:", self->last_content_length_);
        http::cache::NetworkCache::extract_header_value(buffer, bytes, "content-type:", self->last_content_type_);
        http::cache::NetworkCache::extract_header_value(buffer, bytes, "etag:", self->last_etag_);
        http::cache::NetworkCache::extract_header_value(buffer, bytes, "last-modified:", self->last_last_modified_);
        http::cache::NetworkCache::extract_header_value(buffer, bytes, "retry-after:", self->last_retry_after_);
        http::cache::NetworkCache::extract_header_value(buffer, bytes, "x-ratelimit-remaining:", self->last_rl_remaining_);
        http::cache::NetworkCache::extract_header_value(buffer, bytes, "x-ratelimit-reset:", self->last_rl_reset_);
        http::cache::NetworkCache::extract_header_value(buffer, bytes, "cache-control:", self->last_cache_control_);

        return bytes;
    }

    http::model::Response CurlEasy::get(const http::model::Request& req) {
        std::optional<http::cache::NetworkCache::Hit> hit = cache_layer_->probe(req);

        if (hit && cache_layer_->fresh_enough(hit->meta)) {
            return cache_layer_->get_cached_response(std::move(*hit));
        }

        // add conditional headers if revalidating
        std::vector<std::string> hdrs = req.headers;
        if (hit) {
            if (!hit->meta.etag.empty()) {
                hdrs.push_back("If-None-Match: " + hit->meta.etag);
            }
            if (!hit->meta.last_modified.empty()) {
                hdrs.push_back("If-Modified-Since: " + hit->meta.last_modified);
            }
        }

        set_url(req.url);
        set_headers(hdrs);

        std::string body;
        prepare_for_new_request(body);

        // IMPROVEMENT [out of scope]: Not sure if the juice is worth the squeeze here, but reserve body size if known:
        if (last_content_length_ > 0) {
            body.reserve(static_cast<size_t>(last_content_length_));
        }

        perform_throw();
        http::model::Response r = make_response(body);

        if (r.status == 304 && hit) {
            // Optionally refresh stored_at in meta:
            cache_layer_->refresh_meta_timestamp(hit->base_path, hit->meta);
            return cache_layer_->get_cached_response(std::move(*hit));
        }

        if (cache_layer_) {
            cache_layer_->cache_response(req, r);
        }

        return r;
    }



    http::model::Response CurlEasy::get_with_retries(const http::model::Request& req, const http::cache::RetryPolicy& p) {
        using namespace std::chrono;
        std::minstd_rand rng{std::random_device{}()};
        auto jitter = [&](milliseconds base) {
            std::uniform_int_distribution<int> d(0, static_cast<int>(base.count()));
            return milliseconds{d(rng)};
        };

        milliseconds delay = p.base_delay;
        for (size_t attempt = 1; attempt <= p.max_tries; ++attempt) {
            try {
                auto resp = get(req);

                if (resp.status == 429) {
                    if (!resp.retry_after.empty()) {
                        char* end = nullptr;
                        const long s = std::strtol(resp.retry_after.c_str(), &end, 10);
                        if (s > 0) std::this_thread::sleep_for(seconds{s});
                    } else if (resp.rate_limit_remaining == 0 && resp.rate_limit_reset > 0) {
                        const auto now = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
                        const long wait = resp.rate_limit_reset > now ? (resp.rate_limit_reset - now) : 1;
                        std::this_thread::sleep_for(seconds{wait});
                    } else if (attempt < p.max_tries) {
                        std::this_thread::sleep_for(delay + jitter(p.base_delay));
                        delay = std::min(delay * 2, p.max_delay);
                    } else {
                        return resp;
                    }
                    continue;
                }

                if (is_retryable_http(resp.status) && attempt < p.max_tries) {
                    std::this_thread::sleep_for(delay + jitter(p.base_delay));
                    delay = std::min(delay * 2, p.max_delay);
                    continue;
                }

                return resp;
            } catch (const std::exception&) {
                if (attempt < p.max_tries) {
                    std::this_thread::sleep_for(delay + jitter(p.base_delay));
                    delay = std::min(delay * 2, p.max_delay);
                    continue;
                }
                throw;
            }
        }

        return {};
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
        if (rc != CURLE_OK) {
            std::string err = "curl_easy_perform failed: ";
            if (error_buf_[0] != '\0') {
                err += error_buf_;
            } else {
                err += curl_easy_strerror(rc);
            }
            throw std::runtime_error(err);
        }
    }

    http::model::Response CurlEasy::make_response(std::string& incoming_body) {
        long code = 0;
        char* eff = nullptr;
        curl_easy_getinfo(handle_, CURLINFO_RESPONSE_CODE, &code);
        curl_easy_getinfo(handle_, CURLINFO_EFFECTIVE_URL, &eff);

        http::model::Response r;
        r.status = code;
        r.body = std::move(incoming_body);
        r.effective_url = eff ? std::string(eff) : std::string{};
        r.etag = std::move(last_etag_);
        r.last_modified = std::move(last_last_modified_);
        r.retry_after = std::move(last_retry_after_);
        r.rate_limit_remaining = last_rl_remaining_;
        r.rate_limit_reset = last_rl_reset_;
        r.content_type = std::move(last_content_type_);
        r.cache_control = std::move(last_cache_control_);
        return r;
    }
}