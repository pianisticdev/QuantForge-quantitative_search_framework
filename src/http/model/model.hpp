//
// Created by Daniel Griffiths on 11/1/25.
//

#ifndef MONTE_CARLO_AND_BACKTESTER_MODEL_HPP
#define MONTE_CARLO_AND_BACKTESTER_MODEL_HPP

#include <string>
#include <vector>

namespace http::model {
    struct Request {
        std::string url;
        std::vector<std::string> headers;
        std::string method = "GET";
        std::string body;
    };

    struct Response {
        long status = 0;
        std::string body;
        std::string effective_url;

        std::string etag;
        std::string last_modified;
        std::string retry_after;
        std::string cache_control;
        std::string content_type;

        long rate_limit_remaining = -1;
        long rate_limit_reset = -1;
    };
}

#endif //MONTE_CARLO_AND_BACKTESTER_MODEL_HPP
