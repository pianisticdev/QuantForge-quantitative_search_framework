//
// Created by Daniel Griffiths on 11/1/25.
//

#include "polygon.hpp"
#include <simdjson.h>
#include <string>
#include "../api/stock_api.hpp"
#include "../model/model.hpp"
using namespace simdjson;

namespace http::provider {
    PolygonProvider::PolygonProvider(std::string api_key)
            : base_url_("https://api.polygon.io"), api_key_(std::move(api_key)) {}

    http::model::Request PolygonProvider::build_custom_aggregate_bars(const http::stock_api::AggregateBarsArgs& a) const {
        http::model::Request r;
        r.url = base_url_ + "/v2/aggs/ticker/" + a.symbol + "/range/" +
                std::to_string(a.timespan) + "/" + a.timespan_unit + "/" +
                a.from + "/" + a.to;
        r.headers = {"Authorization: Bearer " + api_key_, "Accept: application/json"};
        r.method = "GET";
        return r;
    }

    http::stock_api::AggregateBars PolygonProvider::parse_custom_aggregate_bars(const http::model::Response& resp) const {
        http::stock_api::AggregateBars out{};

        try {
            ondemand::parser parser;
            padded_string json(resp.body);
            ondemand::document doc = parser.iterate(json);

            out.ticker = std::string(doc["ticker"]);
            out.adjusted = bool(doc["adjusted"]);
            out.query_count = int64_t(doc["queryCount"]);
            out.result_count = int64_t(doc["resultsCount"]);

            for (auto result : doc["results"]) {
                http::stock_api::AggregateBarResult bar{};
                bar.volume = double(result["v"]);
                bar.volume_weighted_price = double(result["vw"]);
                bar.open = double(result["o"]);
                bar.close = double(result["c"]);
                bar.high = double(result["h"]);
                bar.low = double(result["l"]);
                bar.unix_ts_ms = int64_t(result["t"]);
                bar.tx_count = int64_t(result["n"]);

                if (result["otc"].error() == simdjson::SUCCESS) {
                    bar.is_otc = bool(result["otc"]);
                } else {
                    bar.is_otc = false;
                }

                out.results.push_back(std::move(bar));
            }
        } catch (const simdjson::simdjson_error& e) {
            throw http::http_error::HttpError(
                    resp.status,
                    resp.effective_url,
                    resp.body.substr(0, 512),
                    "Failed to parse JSON response: " + std::string(e.what())
            );
        }

        return out;
    }
}