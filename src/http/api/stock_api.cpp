//
// Created by Daniel Griffiths on 11/1/25.
//

#include <string>
#include "../model/model.hpp"
#include "../error/http_error.hpp"
#include "../client/curl_easy.hpp"
#include "stock_api.hpp"

namespace http::stock_api {
    StockAPI::StockAPI(std::shared_ptr<IStockDataProvider> p, std::shared_ptr<http::client::CurlEasy> ce) :
            provider_(std::move(p)),
            http_(std::move(ce)) {}

    AggregateBars StockAPI::custom_aggregate_bars(const AggregateBarsArgs& args) {
        const http::model::Request req = provider_->build_custom_aggregate_bars(args);
        const http::model::Response resp = http_->get_with_retries(req);
        if (resp.status < 200 || resp.status >= 300) {
            throw http::http_error::HttpError(
                    resp.status,
                    resp.effective_url,
                    resp.body.substr(0, 512),
                    "HTTP request failed with status " + std::to_string(resp.status)
            );
        }
        return provider_->parse_custom_aggregate_bars(resp);
    }
}