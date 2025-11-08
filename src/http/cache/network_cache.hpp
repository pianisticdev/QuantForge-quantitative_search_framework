//
// Created by Daniel Griffiths on 11/1/25.
//

#ifndef QUANT_FORGE_NETWORK_CACHE_HPP
#define QUANT_FORGE_NETWORK_CACHE_HPP

#include <filesystem>
#include <fstream>
#include <optional>
#include <string>

#include "../../utils/constants.hpp"
#include "../model/model.hpp"

namespace http::cache {
    const long BASE_DELAY_MS = 300;
    const long MAX_DELAY_MS = 1500;

    struct CacheControlKeys {
        static constexpr const char *MAX_AGE_KEY = "max-age";
        static constexpr const char *NO_CACHE_KEY = "no-cache";
        static constexpr const char *NO_STORE_KEY = "no-store";
    };

    struct RetryPolicy {
        size_t max_tries_ = 3;
        std::chrono::milliseconds base_delay_{BASE_DELAY_MS};
        std::chrono::milliseconds max_delay_{MAX_DELAY_MS};
    };

    struct NetworkCachePolicy {
        bool enable_caching_ = true;
        long ttl_s_ = constants::ONE_DAY_S;
    };

    class NetworkCache {
       public:
        struct Meta {
            long unix_ts_s_;
            long status_ = 0;
            std::string etag_;
            std::string last_modified_;
            std::string content_type_;
            std::string cache_control_;
            std::string retry_after_;
            long rate_limit_remaining_ = -1;
            long rate_limit_reset_ = -1;
        };

        struct Hit {
            std::ifstream in_;
            std::string url_;
            std::filesystem::path base_path_;
            Meta meta_;
            explicit operator bool() const { return in_.is_open(); };
        };

        NetworkCache(std::unique_ptr<NetworkCachePolicy> policy);

        ~NetworkCache() = default;
        NetworkCache(const NetworkCache &) = delete;
        NetworkCache &operator=(const NetworkCache &) = delete;
        NetworkCache(NetworkCache &&) = delete;
        NetworkCache &operator=(NetworkCache &&) = delete;

        [[nodiscard]] std::optional<NetworkCache::Hit> probe(const http::model::Request &req) const;
        static http::model::Response get_cached_response(Hit &&hit);
        void cache_response(const http::model::Request &req, const http::model::Response &resp) const;
        [[nodiscard]] bool fresh_enough(const Meta &meta) const;
        static void refresh_meta_timestamp(const std::filesystem::path &base_path, Meta &meta);
        static bool extract_header_value(const char *buffer, size_t bytes, const char *key, std::string &out_property);
        static bool extract_header_value(const char *buffer, size_t bytes, const char *key, long &out_property);

       private:
        std::unique_ptr<NetworkCachePolicy> policy_;

        [[nodiscard]] static std::filesystem::path create_base_path(const http::model::Request &req);

        static bool load_meta(const std::filesystem::path &p, Meta &out);
        static void save_meta_atomic(const std::filesystem::path &p, const Meta &m);
        static void write_atomic(const std::filesystem::path &p, std::string_view bytes);
    };
}  // namespace http::cache

#endif
