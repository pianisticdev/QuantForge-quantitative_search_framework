//
// Created by Daniel Griffiths on 11/1/25.
//

#include "network_cache.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>

#include "../../utils/constants.hpp"
#include "../../utils/string_utils.hpp"
#include "../model/model.hpp"

static constexpr const char *BODY_FILE_EXT = ".body";
static constexpr const char *META_FILE_EXT = ".meta";

namespace http::cache {

    NetworkCache::NetworkCache(std::unique_ptr<NetworkCachePolicy> policy) : policy_(std::move(policy)) {}

    std::optional<NetworkCache::Hit> NetworkCache::probe(const http::model::Request &req) const {
        if (!policy_->enable_caching_) {
            return std::nullopt;
        }

        const auto base_path = create_base_path(req);
        const auto body_path = string_utils::append_to_path(base_path, BODY_FILE_EXT);
        const auto meta_path = string_utils::append_to_path(base_path, META_FILE_EXT);

        Meta meta;
        if (!load_meta(meta_path, meta)) {
            return std::nullopt;
        }

        std::ifstream body_in(body_path, std::ios::binary);

        if (!body_in) {
            return std::nullopt;
        }

        return Hit{
            .in_ = std::move(body_in),
            .url_ = req.url_,
            .base_path_ = base_path,
            .meta_ = meta,
        };
    }

    bool NetworkCache::fresh_enough(const NetworkCache::Meta &m) const {
        const auto now = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        std::string cache_control_header = m.cache_control_;

        std::vector<std::string> split_pairs = string_utils::split_comma_delimited_string(cache_control_header);

        const long age = std::max(now - m.unix_ts_s_, 0LL);

        for (const auto &pair : split_pairs) {
            std::string trimmed_pair = string_utils::trim(pair);

            if (string_utils::ieq_prefix(trimmed_pair.c_str(), trimmed_pair.size(), CacheControlKeys::NO_STORE_KEY)) {
                return false;
            }

            if (string_utils::ieq_prefix(trimmed_pair.c_str(), trimmed_pair.size(), CacheControlKeys::NO_CACHE_KEY)) {
                return age <= 0;
            }

            if (string_utils::ieq_prefix(trimmed_pair.c_str(), trimmed_pair.size(), CacheControlKeys::MAX_AGE_KEY)) {
                auto pos = trimmed_pair.find('=');
                if (pos != std::string::npos) {
                    std::string value_str = string_utils::trim(trimmed_pair.substr(pos + 1));
                    char *end = nullptr;
                    long max_age = std::strtol(value_str.c_str(), &end, constants::BASE_10);
                    if (*end == '\0' && max_age >= 0) {
                        return age <= std::min(policy_->ttl_s_, max_age);
                    }
                }
            }
        }

        return age <= policy_->ttl_s_;
    }

    http::model::Response NetworkCache::get_cached_response(Hit &&hit) {
        std::string s;
        hit.in_.seekg(0, std::ios::end);
        s.resize(static_cast<size_t>(hit.in_.tellg()));
        hit.in_.seekg(0, std::ios::beg);
        hit.in_.read(s.data(), static_cast<std::streamsize>(s.size()));

        http::model::Response response;
        response.status_ = hit.meta_.status_;
        response.body_ = std::move(s);
        response.effective_url_ = std::move(hit.url_);
        response.etag_ = hit.meta_.etag_;
        response.last_modified_ = hit.meta_.last_modified_;
        response.retry_after_ = hit.meta_.retry_after_;
        response.rate_limit_remaining_ = hit.meta_.rate_limit_remaining_;
        response.rate_limit_reset_ = hit.meta_.rate_limit_reset_;
        response.cache_control_ = hit.meta_.cache_control_;
        response.content_type_ = hit.meta_.content_type_;
        return response;
    }

    void NetworkCache::cache_response(const http::model::Request &req, const http::model::Response &resp) const {
        if (!policy_->enable_caching_) {
            return;
        }

        const auto base_path = create_base_path(req);
        const auto body_path = string_utils::append_to_path(base_path, BODY_FILE_EXT);
        const auto meta_path = string_utils::append_to_path(base_path, META_FILE_EXT);

        write_atomic(body_path, resp.body_);

        Meta m;
        m.status_ = resp.status_;
        m.etag_ = resp.etag_;
        m.last_modified_ = resp.last_modified_;
        m.content_type_ = resp.content_type_;
        m.cache_control_ = resp.cache_control_;
        m.retry_after_ = resp.retry_after_;
        m.rate_limit_remaining_ = resp.rate_limit_remaining_;
        m.rate_limit_reset_ = resp.rate_limit_reset_;
        m.unix_ts_s_ = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();

        save_meta_atomic(meta_path, m);
    }

    std::filesystem::path NetworkCache::create_base_path(const http::model::Request &req) {
        std::hash<std::string> hash_maker;
        // IMPROVEMENT (out of scope): Include certain request headers in the url
        // cache key.
        size_t hash = hash_maker(req.url_);
        std::filesystem::create_directories("cache/http");
        return {"cache/http/" + std::to_string(hash)};
    }

    void NetworkCache::save_meta_atomic(const std::filesystem::path &p, const Meta &m) {
        std::ostringstream oss;
        oss << "status: " << m.status_ << "\n"
            << "etag: " << m.etag_ << "\n"
            << "last_modified: " << m.last_modified_ << "\n"
            << "content_type: " << m.content_type_ << "\n"
            << "cache_control: " << m.cache_control_ << "\n"
            << "retry_after: " << m.retry_after_ << "\n"
            << "rate_limit_remaining: " << m.rate_limit_remaining_ << "\n"
            << "rate_limit_reset: " << m.rate_limit_reset_ << "\n"
            << "unix_ts_s: " << m.unix_ts_s_ << "\n";
        write_atomic(p, oss.str());
    }

    void NetworkCache::write_atomic(const std::filesystem::path &p, std::string_view bytes) {
        auto tmp = p;
        tmp += ".tmp";
        {
            std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
            if (!out) {
                throw std::runtime_error("open failed: " + tmp.string());
            }
            out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
            out.flush();
            if (!out) {
                throw std::runtime_error("write failed: " + tmp.string());
            }
        }
        std::filesystem::rename(tmp, p);  // atomic on same filesystem
    }

    bool NetworkCache::load_meta(const std::filesystem::path &p, Meta &out) {
        std::ifstream in(p);
        if (!in) {
            return false;
        }
        std::string line;
        auto as_long = [](const std::string &s, long def = -1L) -> long {
            char *end = nullptr;
            long v = std::strtol(s.c_str(), &end, constants::BASE_10);
            return (end != nullptr && *end == '\0') ? v : def;
        };
        auto as_i64 = [](const std::string &s, std::int64_t def = 0) -> std::int64_t {
            char *end = nullptr;
            long long v = std::strtoll(s.c_str(), &end, constants::BASE_10);
            return (end != nullptr && *end == '\0') ? static_cast<std::int64_t>(v) : def;
        };

        while (std::getline(in, line)) {
            auto pos = line.find(':');
            if (pos == std::string::npos) {
                continue;
            }
            std::string key = string_utils::trim(line.substr(0, pos));
            std::string val = string_utils::trim(line.substr(pos + 1));
            if (key == "status") {
                out.status_ = as_long(val, 0);
            } else if (key == "etag") {
                out.etag_ = val;
            } else if (key == "last_modified") {
                out.last_modified_ = val;
            } else if (key == "content_type") {
                out.content_type_ = val;
            } else if (key == "cache_control") {
                out.cache_control_ = val;
            } else if (key == "retry_after") {
                out.retry_after_ = val;
            } else if (key == "rate_limit_remaining") {
                out.rate_limit_remaining_ = as_long(val, -1);
            } else if (key == "rate_limit_reset") {
                out.rate_limit_reset_ = as_long(val, -1);
            } else if (key == "unix_ts_s") {
                out.unix_ts_s_ = as_i64(val, 0);
            }
        }
        return true;
    }

    void NetworkCache::refresh_meta_timestamp(const std::filesystem::path &base_path, Meta &meta) {
        meta.unix_ts_s_ = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        const auto meta_path = string_utils::append_to_path(base_path, META_FILE_EXT);
        NetworkCache::save_meta_atomic(meta_path, meta);
    }

    bool NetworkCache::extract_header_value(const char *buffer, size_t bytes, const char *key, std::string &out_property) {
        size_t key_len = std::char_traits<char>::length(key);
        if (bytes < key_len) {
            return false;
        }
        for (size_t i = 0; i < key_len; ++i) {
            const char a = buffer[i];
            const char b = key[i];
            if ((a | constants::ASCII_LOWERCASE_BIT) != (b | constants::ASCII_LOWERCASE_BIT)) {
                return false;
            }  // ASCII-only fold
        }
        const char *start = buffer + key_len;
        const char *end = buffer + bytes;
        while (start < end && (*start == ' ' || *start == '\t')) {
            ++start;
        }
        while (end > start && (end[-1] == '\r' || end[-1] == '\n' || end[-1] == ' ' || end[-1] == '\t')) {
            --end;
        }
        out_property.assign(start, end);
        return true;
    }

    bool NetworkCache::extract_header_value(const char *buffer, size_t bytes, const char *key, long &out_property) {
        size_t key_len = std::char_traits<char>::length(key);
        if (bytes < key_len) {
            return false;
        }
        for (size_t i = 0; i < key_len; ++i) {
            const char a = buffer[i];
            const char b = key[i];
            if ((a | constants::ASCII_LOWERCASE_BIT) != (b | constants::ASCII_LOWERCASE_BIT)) {
                return false;
            }  // ASCII-only fold
        }
        const char *start = buffer + key_len;
        const char *end = buffer + bytes;
        while (start < end && (*start == ' ' || *start == '\t')) {
            ++start;
        }
        while (end > start && (end[-1] == '\r' || end[-1] == '\n' || end[-1] == ' ' || end[-1] == '\t')) {
            --end;
        }
        std::string tmp(start, end);
        char *end_ptr = nullptr;
        long val = std::strtol(tmp.c_str(), &end_ptr, constants::BASE_10);
        if (end_ptr == tmp.c_str()) {
            return false;
        }
        out_property = val;
        return true;
    }
}  // namespace http::cache
