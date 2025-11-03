//
// Created by Daniel Griffiths on 11/1/25.
//

#ifndef MONTE_CARLO_AND_BACKTESTER_POLYGON_HPP
#define MONTE_CARLO_AND_BACKTESTER_POLYGON_HPP

#include <string>
#include "../model/model.hpp"
#include "../api/stock_api.hpp"

namespace http::provider {
    class PolygonProvider : public http::stock_api::IStockDataProvider {
    public:
        explicit PolygonProvider(std::string api_key);
        [[nodiscard]] http::model::Request build_custom_aggregate_bars(const http::stock_api::AggregateBarsArgs& a) const override;
        [[nodiscard]] http::stock_api::AggregateBars parse_custom_aggregate_bars(const http::model::Response& resp) const override;
    private:
        std::string base_url_;
        std::string api_key_;
    };
}

#endif //MONTE_CARLO_AND_BACKTESTER_POLYGON_HPP
