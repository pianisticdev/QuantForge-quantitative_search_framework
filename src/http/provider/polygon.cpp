//
// Created by Daniel Griffiths on 11/1/25.
//

#include "polygon.hpp"

#include <simdjson.h>

#include <string>

#include "../api/stock_api.hpp"
#include "../error/http_error.hpp"
#include "../model/model.hpp"
using namespace simdjson;

namespace http::provider {
    struct PolygonOptions {
        static constexpr const char* BASE_URL = "https://api.polygon.io";
    };

    PolygonProvider::PolygonProvider(std::string api_key) : base_url_(PolygonOptions::BASE_URL), api_key_(std::move(api_key)) {}

    http::model::Request PolygonProvider::build_custom_aggregate_bars(const http::stock_api::AggregateBarsArgs& a) const {
        http::model::Request r;
        r.url_ = base_url_ + "/v2/aggs/ticker/" + a.symbol_ + "/range/" + std::to_string(a.timespan_) + "/" + a.timespan_unit_ + "/" + a.from_ + "/" + a.to_;
        r.headers_ = {"Authorization: Bearer " + api_key_, "Accept: application/json"};
        r.method_ = "GET";
        return r;
    }

    http::stock_api::AggregateBars PolygonProvider::parse_custom_aggregate_bars(const http::model::Response& resp) const {
        http::stock_api::AggregateBars out{};

        try {
            ondemand::parser parser;
            padded_string json(resp.body_);
            ondemand::document doc = parser.iterate(json);

            out.ticker_ = std::string(doc["ticker"]);
            out.adjusted_ = bool(doc["adjusted"]);
            out.query_count_ = int64_t(doc["queryCount"]);
            out.result_count_ = int64_t(doc["resultsCount"]);

            for (auto result : doc["results"]) {
                http::stock_api::AggregateBarResult bar{};
                bar.volume_ = double(result["v"]);
                bar.volume_weighted_price_ = double(result["vw"]);
                bar.open_ = double(result["o"]);
                bar.close_ = double(result["c"]);
                bar.high_ = double(result["h"]);
                bar.low_ = double(result["l"]);
                bar.unix_ts_ms_ = (long long)(result["t"]);
                bar.tx_count_ = int(result["n"]);

                if (result["otc"].error() == simdjson::SUCCESS) {
                    bar.is_otc_ = bool(result["otc"]);
                } else {
                    bar.is_otc_ = false;
                }

                out.results_.emplace_back(bar);
            }
        } catch (const simdjson::simdjson_error& e) {
            throw http::http_error::HttpError(resp.status_, resp.effective_url_, resp.body_.substr(0, http::http_error::ERROR_MESSAGE_LENGTH),
                                              "Failed to parse JSON response: " + std::string(e.what()));
        }

        return out;
    }
}  // namespace http::provider
