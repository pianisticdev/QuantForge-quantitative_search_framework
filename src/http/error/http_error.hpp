//
// Created by Daniel Griffiths on 11/1/25.
//

#ifndef QUANT_FORGE_HTTP_ERROR_HPP
#define QUANT_FORGE_HTTP_ERROR_HPP

#include <stdexcept>
#include <string>

namespace http::http_error {
    const long ERROR_MESSAGE_LENGTH = 512;

    struct HttpError : public std::runtime_error {
        long status_;
        std::string url_;
        std::string body_preview_;
        explicit HttpError(long s, std::string u, std::string preview, const std::string &msg);
    };
}  // namespace http::http_error

#endif
