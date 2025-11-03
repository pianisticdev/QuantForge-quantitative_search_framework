//
// Created by Daniel Griffiths on 11/1/25.
//

#include "http_error.hpp"
#include <string>
#include <stdexcept>

namespace http::http_error {
    HttpError::HttpError(long s, std::string u, std::string preview, std::string msg)
            : std::runtime_error(std::move(msg)), status(s),
              url(std::move(u)), body_preview(std::move(preview)) {}
};
