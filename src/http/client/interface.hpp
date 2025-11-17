#ifndef QUANT_FORGE_CLIENT_INTERFACE_HPP
#define QUANT_FORGE_CLIENT_INTERFACE_HPP

#include "../cache/network_cache.hpp"
#include "../model/model.hpp"

namespace http::client {
    class IHttpClient {
       public:
        IHttpClient() = default;
        virtual ~IHttpClient() = default;
        IHttpClient(const IHttpClient&) = delete;
        virtual IHttpClient& operator=(const IHttpClient&) = delete;
        IHttpClient(IHttpClient&&) = delete;
        virtual IHttpClient& operator=(IHttpClient&&) = delete;

        virtual http::model::Response get(const http::model::Request& req) = 0;
        virtual http::model::Response get_with_retries(const http::model::Request& req, const http::cache::RetryPolicy& p = {}) = 0;
    };
}  // namespace http::client

#endif
