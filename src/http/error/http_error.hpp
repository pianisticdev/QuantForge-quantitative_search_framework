//
// Created by Daniel Griffiths on 11/1/25.
//

#ifndef MONTE_CARLO_AND_BACKTESTER_HTTP_ERROR_HPP
#define MONTE_CARLO_AND_BACKTESTER_HTTP_ERROR_HPP

#include <stdexcept>
#include <string>

namespace http::http_error {
    struct HttpError : public std::runtime_error {
        long status;
        std::string url;
        std::string body_preview;
        explicit HttpError(long s, std::string u, std::string preview, std::string msg);
    };
}

#endif //MONTE_CARLO_AND_BACKTESTER_HTTP_ERROR_HPP
