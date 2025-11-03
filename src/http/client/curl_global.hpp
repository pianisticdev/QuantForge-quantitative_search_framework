//
// Created by Daniel Griffiths on 11/1/25.
//

#ifndef MONTE_CARLO_AND_BACKTESTER_CURL_GLOBAL_HPP
#define MONTE_CARLO_AND_BACKTESTER_CURL_GLOBAL_HPP

namespace http::client {
    class CurlGlobal {
    public:
        CurlGlobal();
        ~CurlGlobal();
        CurlGlobal(const CurlGlobal&) = delete;
        CurlGlobal& operator=(const CurlGlobal&) = delete;
    };
}

#endif //MONTE_CARLO_AND_BACKTESTER_CURL_GLOBAL_HPP
