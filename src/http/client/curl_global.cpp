//
// Created by Daniel Griffiths on 11/1/25.
//

#include <stdexcept>
#include <curl/curl.h>
#include "curl_global.hpp"

namespace http::client {
    CurlGlobal::CurlGlobal() {
        const auto rc = curl_global_init(CURL_GLOBAL_ALL);
        if (rc != CURLE_OK) throw std::runtime_error("Failed to initialize libcurl");
    }
    CurlGlobal::~CurlGlobal() { curl_global_cleanup(); }
}