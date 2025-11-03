//
// Created by Daniel Griffiths on 11/1/25.
//

#ifndef MONTE_CARLO_AND_BACKTESTER_STOCK_API_HPP
#define MONTE_CARLO_AND_BACKTESTER_STOCK_API_HPP

#include <string>
#include "../model/model.hpp"
#include "../error/http_error.hpp"
#include "../client/curl_easy.hpp"

namespace http::stock_api {
    struct AggregateBarsArgs {
        std::string symbol, from, to;
        int timespan = 1;
        std::string timespan_unit = "minute";
    };

    struct AggregateBarResult {
        double volume{}, volume_weighted_price{}, open{}, close{}, high{}, low{};
        long long unix_ts_ms{};
        int tx_count{};
        bool is_otc{};
    };

    struct AggregateBars {
        std::string ticker;
        std::size_t query_count{}, result_count{};
        bool adjusted{};
        std::vector<AggregateBarResult> results{};
    };

    class IStockDataProvider {
    public:
        virtual ~IStockDataProvider() = default;
        virtual http::model::Request build_custom_aggregate_bars(const http::stock_api::AggregateBarsArgs& a) const = 0;
        virtual http::stock_api::AggregateBars parse_custom_aggregate_bars(const http::model::Response& resp) const = 0;
    };


    class StockAPI {
    public:
        explicit StockAPI(std::shared_ptr<IStockDataProvider> p, std::shared_ptr<http::client::CurlEasy> ce);
        AggregateBars custom_aggregate_bars(const AggregateBarsArgs& args);
    private:
        std::shared_ptr<IStockDataProvider> provider_;
        std::shared_ptr<http::client::CurlEasy> http_;
    };
}

#endif //MONTE_CARLO_AND_BACKTESTER_STOCK_API_HPP
