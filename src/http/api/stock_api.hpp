//
// Created by Daniel Griffiths on 11/1/25.
//

#ifndef QUANT_FORGE_STOCK_API_HPP
#define QUANT_FORGE_STOCK_API_HPP

#include <memory>
#include <string>

#include "../client/interface.hpp"
#include "../model/model.hpp"

namespace http::stock_api {

    struct AggregateBarsArgs {
        int timespan_ = 1;
        std::string symbol_;
        std::string from_;
        std::string to_;
        std::string timespan_unit_ = "minute";
    };

    struct AggregateBarResult {
        std::string symbol_;
        bool is_otc_{};
        int tx_count_{};
        double volume_{};
        double volume_weighted_price_{};
        double open_{};
        double close_{};
        double high_{};
        double low_{};
        long long unix_ts_ns_{};
    };

    struct AggregateBars {
        bool adjusted_{};
        std::size_t query_count_{};
        std::size_t result_count_{};
        std::string ticker_{};
        std::vector<AggregateBarResult> results_{};
    };

    class IStockDataProvider {
       public:
        IStockDataProvider() = default;
        virtual ~IStockDataProvider() = default;
        IStockDataProvider(const IStockDataProvider&) = delete;
        IStockDataProvider& operator=(const IStockDataProvider&) = delete;
        IStockDataProvider(IStockDataProvider&&) = delete;
        IStockDataProvider& operator=(IStockDataProvider&&) = delete;

        [[nodiscard]] virtual http::model::Request build_custom_aggregate_bars(const http::stock_api::AggregateBarsArgs& a) const = 0;
        [[nodiscard]] virtual http::stock_api::AggregateBars parse_custom_aggregate_bars(const http::model::Response& resp) const = 0;
    };

    class StockAPI {
       public:
        explicit StockAPI(std::unique_ptr<IStockDataProvider> p, std::unique_ptr<http::client::IHttpClient> ce);
        explicit StockAPI(const IStockDataProvider* p, std::unique_ptr<http::client::IHttpClient> ce);
        AggregateBars custom_aggregate_bars(const AggregateBarsArgs& args);

       private:
        const IStockDataProvider* provider_;
        std::shared_ptr<http::client::IHttpClient> http_;
    };

}  // namespace http::stock_api

#endif
