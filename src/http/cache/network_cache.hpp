//
// Created by Daniel Griffiths on 11/1/25.
//

#ifndef MONTE_CARLO_AND_BACKTESTER_NETWORK_CACHE_HPP
#define MONTE_CARLO_AND_BACKTESTER_NETWORK_CACHE_HPP

#include "../model/model.hpp"
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>

namespace http::cache {
struct RetryPolicy {
  size_t max_tries = 3;
  std::chrono::milliseconds base_delay{300};
  std::chrono::milliseconds max_delay{1500};
};

struct NetworkCachePolicy {
  bool enable_caching = true;
  long ttl_s = 60 * 60 * 24;
};

class NetworkCache {
public:
  struct Meta {
    long unix_ts_s;
    long status = 0;
    std::string etag;
    std::string last_modified;
    std::string content_type;
    std::string cache_control;
    std::string retry_after;
    long rate_limit_remaining = -1;
    long rate_limit_reset = -1;
  };

  struct Hit {
    std::ifstream in;
    std::string url;
    std::filesystem::path base_path;
    Meta meta;
    explicit operator bool() const { return in.is_open(); };
  };

  NetworkCache(NetworkCachePolicy &policy);

  ~NetworkCache();
  NetworkCache(const NetworkCache &) = delete;
  NetworkCache &operator=(const NetworkCache &) = delete;

  std::optional<NetworkCache::Hit> probe(const http::model::Request &req) const;
  http::model::Response get_cached_response(Hit &&hit);
  void cache_response(const http::model::Request &req,
                      const http::model::Response &resp);
  bool fresh_enough(const Meta &meta);
  void refresh_meta_timestamp(const std::filesystem::path &base_path,
                              Meta &meta);
  static bool extract_header_value(const char *buffer, size_t bytes,
                                   const char *key, std::string &out_property);
  static bool extract_header_value(const char *buffer, size_t bytes,
                                   const char *key, long &out_property);

private:
  NetworkCachePolicy policy_;

  std::filesystem::path create_base_path(const http::model::Request &req) const;

  bool load_meta(const std::filesystem::path &p, Meta &out) const;
  void save_meta_atomic(const std::filesystem::path &p, const Meta &m);
  void write_atomic(const std::filesystem::path &p, std::string_view bytes);
};
} // namespace http::cache

#endif // MONTE_CARLO_AND_BACKTESTER_NETWORK_CACHE_HPP
