//
// Created by Daniel Griffiths on 11/1/25.
//

#include <string>
#include <fstream>
#include <filesystem>
#include <optional>
#include <sstream>
#include <algorithm>
#include "../model/model.hpp"
#include "../../utils/string_utils.hpp"
#include "network_cache.hpp"

namespace http::cache {
    NetworkCache::NetworkCache(NetworkCachePolicy& policy) : policy_(policy) {}

    NetworkCache::~NetworkCache() {}

    std::optional<NetworkCache::Hit> NetworkCache::probe(const http::model::Request& req) const {
        if (!policy_.enable_caching) return std::nullopt;

        const auto base_path = create_base_path(req);
        const auto body_path = string_utils::append_to_path(base_path, ".body");
        const auto meta_path = string_utils::append_to_path(base_path, ".meta");

        Meta meta;
        if (!load_meta(meta_path, meta)) {
            return std::nullopt;
        }

        std::ifstream body_in(body_path, std::ios::binary);

        if (!body_in) return std::nullopt;

        return Hit{
                .in = std::move(body_in),
                .url = req.url,
                .base_path = base_path,
                .meta = meta,
        };
    }

    bool NetworkCache::fresh_enough(const NetworkCache::Meta& m) {
        const auto now = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()
        ).count();
        std::string cache_control_header = m.cache_control;

        constexpr auto max_aqe_key = "max-age";
        constexpr auto no_cache_key = "no-cache";
        constexpr auto no_store_key = "no-store";

        std::vector<std::string> split_pairs = string_utils::split_comma_delimited_string(cache_control_header);

        const long age = std::max(now - m.unix_ts_s, 0LL);

        for (const auto& pair : split_pairs) {
            std::string trimmed_pair = string_utils::trim(pair);

            if (string_utils::ieq_prefix(trimmed_pair.c_str(), trimmed_pair.size(), no_store_key)) {
                return false;
            }

            if (string_utils::ieq_prefix(trimmed_pair.c_str(), trimmed_pair.size(), no_cache_key)) {
                return age <= 0;
            }

            if (string_utils::ieq_prefix(trimmed_pair.c_str(), trimmed_pair.size(), max_aqe_key)) {
                auto pos = trimmed_pair.find('=');
                if (pos != std::string::npos) {
                    std::string value_str = string_utils::trim(trimmed_pair.substr(pos + 1));
                    char* end = nullptr;
                    long max_age = std::strtol(value_str.c_str(), &end, 10);
                    if (end && *end == '\0' && max_age >= 0) {
                        return age <= std::min(policy_.ttl_s, max_age);
                    }
                }
            }
        }

        return age <= policy_.ttl_s;
    }

    http::model::Response NetworkCache::get_cached_response(Hit&& hit) {
        std::string s;
        hit.in.seekg(0, std::ios::end);
        s.resize(static_cast<size_t>(hit.in.tellg()));
        hit.in.seekg(0, std::ios::beg);
        hit.in.read(s.data(), s.size());

        http::model::Response response;
        response.status = hit.meta.status;
        response.body = std::move(s);
        response.effective_url = std::move(hit.url);
        response.etag = hit.meta.etag;
        response.last_modified = hit.meta.last_modified;
        response.retry_after = hit.meta.retry_after;
        response.rate_limit_remaining = hit.meta.rate_limit_remaining;
        response.rate_limit_reset = hit.meta.rate_limit_reset;
        response.cache_control = hit.meta.cache_control;
        response.content_type = hit.meta.content_type;
        return response;
    }

    void NetworkCache::cache_response(const http::model::Request& req, const http::model::Response& resp) {
        if (!policy_.enable_caching) return;

        const auto base_path = create_base_path(req);
        const auto body_path = string_utils::append_to_path(base_path, ".body");
        const auto meta_path = string_utils::append_to_path(base_path, ".meta");

        write_atomic(body_path, resp.body);

        Meta m;
        m.status = resp.status;
        m.etag = resp.etag;
        m.last_modified = resp.last_modified;
        m.content_type = resp.content_type;
        m.cache_control = resp.cache_control;
        m.retry_after = resp.retry_after;
        m.rate_limit_remaining = resp.rate_limit_remaining;
        m.rate_limit_reset = resp.rate_limit_reset;
        m.unix_ts_s = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()
        ).count();

        save_meta_atomic(meta_path, m);
    }

    std::filesystem::path NetworkCache::create_base_path(const http::model::Request& req) const {
        std::hash<std::string> hash_maker;
        // IMPROVEMENT (out of scope): Include certain request headers in the url cache key.
        size_t hash = hash_maker(req.url);
        std::filesystem::create_directories("cache/http");
        return std::filesystem::path("cache/http/" + std::to_string(hash));
    }

    void NetworkCache::save_meta_atomic(const std::filesystem::path& p, const Meta& m) {
        std::ostringstream oss;
        oss << "status: " << m.status << "\n"
            << "etag: " << m.etag << "\n"
            << "last_modified: " << m.last_modified << "\n"
            << "content_type: " << m.content_type << "\n"
            << "cache_control: " << m.cache_control << "\n"
            << "retry_after: " << m.retry_after << "\n"
            << "rate_limit_remaining: " << m.rate_limit_remaining << "\n"
            << "rate_limit_reset: " << m.rate_limit_reset << "\n"
            << "unix_ts_s: " << m.unix_ts_s << "\n";
        write_atomic(p, oss.str());
    }

    void NetworkCache::write_atomic(const std::filesystem::path& p, std::string_view bytes) {
        auto tmp = p;
        tmp += ".tmp";
        {
            std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
            if (!out) throw std::runtime_error("open failed: " + tmp.string());
            out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
            out.flush();
            if (!out) throw std::runtime_error("write failed: " + tmp.string());
        }
        std::filesystem::rename(tmp, p); // atomic on same filesystem
    }

    bool NetworkCache::load_meta(const std::filesystem::path& p, Meta& out) const {
        std::ifstream in(p);
        if (!in) return false;
        std::string line;
        auto as_long = [](const std::string& s, long def=-1L)->long {
            char* end=nullptr; long v = std::strtol(s.c_str(), &end, 10);
            return (end && *end=='\0') ? v : def;
        };
        auto as_i64 = [](const std::string& s, std::int64_t def=0)->std::int64_t {
            char* end=nullptr; long long v = std::strtoll(s.c_str(), &end, 10);
            return (end && *end=='\0') ? static_cast<std::int64_t>(v) : def;
        };

        while (std::getline(in, line)) {
            auto pos = line.find(':');
            if (pos == std::string::npos) continue;
            std::string key = string_utils::trim(line.substr(0, pos));
            std::string val = string_utils::trim(line.substr(pos + 1));
            if (key == "status") out.status = as_long(val, 0);
            else if (key == "etag") out.etag = val;
            else if (key == "last_modified") out.last_modified = val;
            else if (key == "content_type") out.content_type = val;
            else if (key == "cache_control") out.cache_control = val;
            else if (key == "retry_after") out.retry_after = val;
            else if (key == "rate_limit_remaining") out.rate_limit_remaining = as_long(val, -1);
            else if (key == "rate_limit_reset") out.rate_limit_reset = as_long(val, -1);
            else if (key == "unix_ts_s") out.unix_ts_s = as_i64(val, 0);
        }
        return true;
    }

    void NetworkCache::refresh_meta_timestamp(const std::filesystem::path& base_path, Meta& meta) {
        meta.unix_ts_s = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()
        ).count();
        const auto meta_path = string_utils::append_to_path(base_path, ".meta");
        NetworkCache::save_meta_atomic(meta_path, meta);
    }

    bool NetworkCache::extract_header_value(const char* buffer, size_t n, const char* key, std::string& out_property) {
        size_t key_len = std::char_traits<char>::length(key);
        if (n < key_len) return false;
        // case-insensitive match at position 0
        for (size_t i = 0; i < key_len; ++i) {
            char a = buffer[i], b = key[i];
            if ((a | 0x20) != (b | 0x20)) return false; // ASCII-only fold
        }
        const char* start = buffer + key_len;
        const char* end = buffer + n;
        while (start < end && (*start == ' ' || *start == '\t')) ++start;
        while (end > start && (end[-1] == '\r' || end[-1] == '\n' || end[-1] == ' ' || end[-1] == '\t')) --end;
        out_property.assign(start, end);
        return true;
    }

    bool NetworkCache::extract_header_value(const char* buffer, size_t n, const char* key, long& out_property) {
        size_t key_len = std::char_traits<char>::length(key);
        if (n < key_len) return false;
        // case-insensitive match at position 0
        for (size_t i = 0; i < key_len; ++i) {
            char a = buffer[i], b = key[i];
            if ((a | 0x20) != (b | 0x20)) return false; // ASCII-only fold
        }
        const char* start = buffer + key_len;
        const char* end = buffer + n;
        while (start < end && (*start == ' ' || *start == '\t')) ++start;
        while (end > start && (end[-1] == '\r' || end[-1] == '\n' || end[-1] == ' ' || end[-1] == '\t')) --end;
        std::string tmp(start, end);
        char* end_ptr = nullptr;
        long val = std::strtol(tmp.c_str(), &end_ptr, 10);
        if (end_ptr == tmp.c_str()) return false;
        out_property = val;
        return true;
    }
}