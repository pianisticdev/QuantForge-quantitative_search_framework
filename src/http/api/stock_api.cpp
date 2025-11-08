//
// Created by Daniel Griffiths on 11/1/25.
//

#include "stock_api.hpp"

#include <string>

#include "../client/curl_easy.hpp"
#include "../error/http_error.hpp"
#include "../model/model.hpp"

const long HTTP_SUCCESS_UPPER_BOUNDARY = 300;

namespace http::stock_api {
    StockAPI::StockAPI(std::shared_ptr<IStockDataProvider> p, std::shared_ptr<http::client::CurlEasy> ce) : provider_(std::move(p)), http_(std::move(ce)) {}

    AggregateBars StockAPI::custom_aggregate_bars(const AggregateBarsArgs &args) {
        const http::model::Request req = provider_->build_custom_aggregate_bars(args);
        const http::model::Response resp = http_->get_with_retries(req);

        if (resp.status_ < static_cast<long>(http::client::HttpStatusCode::OK) || resp.status_ >= HTTP_SUCCESS_UPPER_BOUNDARY) {
            throw http::http_error::HttpError(resp.status_, resp.effective_url_, resp.body_.substr(0, http::http_error::ERROR_MESSAGE_LENGTH),
                                              "HTTP request failed with status " + std::to_string(resp.status_));
        }

        return provider_->parse_custom_aggregate_bars(resp);
    }
}  // namespace http::stock_api
