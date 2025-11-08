//
// Created by Daniel Griffiths on 11/1/25.
//

#include "http_error.hpp"

#include <stdexcept>
#include <string>

namespace http::http_error {
    HttpError::HttpError(long s, std::string u,
                         std::string preview,     // NOLINT(bugprone-easily-swappable-parameters)
                         const std::string &msg)  // NOLINT(bugprone-easily-swappable-parameters)
        : std::runtime_error(msg), status_(s), url_(std::move(u)), body_preview_(std::move(preview)) {}
};  // namespace http::http_error
