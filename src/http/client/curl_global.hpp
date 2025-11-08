//
// Created by Daniel Griffiths on 11/1/25.
//

#ifndef QUANT_FORGE_CURL_GLOBAL_HPP
#define QUANT_FORGE_CURL_GLOBAL_HPP

namespace http::client {

    class CurlGlobal {
       public:
        CurlGlobal();

        ~CurlGlobal();
        CurlGlobal(const CurlGlobal&) = delete;
        CurlGlobal& operator=(const CurlGlobal&) = delete;
        CurlGlobal(CurlGlobal&&) = delete;
        CurlGlobal& operator=(CurlGlobal&&) = delete;
    };

}  // namespace http::client

#endif
