//
// Created by Daniel Griffiths on 11/1/25.
//

#ifndef QUANT_FORGE_MODEL_HPP
#define QUANT_FORGE_MODEL_HPP

#include <string>
#include <vector>

namespace http::model {
    struct Request {
        std::string url_;
        std::string method_ = "GET";
        std::string body_;

        std::vector<std::string> headers_;
    };

    struct Response {
        long rate_limit_remaining_ = -1;
        long rate_limit_reset_ = -1;
        long status_ = 0;

        std::string body_;
        std::string effective_url_;

        std::string etag_;
        std::string last_modified_;
        std::string retry_after_;
        std::string cache_control_;
        std::string content_type_;
    };
}  // namespace http::model

#endif
