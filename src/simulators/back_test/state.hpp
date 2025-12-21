#ifndef QUANT_SIMULATORS_BACK_TEST_STATE_HPP
#define QUANT_SIMULATORS_BACK_TEST_STATE_HPP

#pragma once

#include "../../http/api/stock_api.hpp"
#include "../plugins/manifest/manifest.hpp"
#include "./models.hpp"

namespace simulators {

    class State {
       public:
        Money cash_;
        int64_t current_timestamp_ns_;
        std::map<std::string, models::Position> positions_;
        std::map<std::string, Money> current_prices_;
        std::map<std::string, int64_t> current_volumes_;
        std::vector<models::Fill> fills_;
        std::vector<models::ExitOrder> exit_orders_;
        std::vector<models::EquitySnapshot> equity_curve_;
        std::vector<models::Fill> new_fills_;
        std::vector<models::ExitOrder> new_exit_orders_;
        std::map<std::string, double> active_buy_fills_;

        void update_state(const models::ExecutionResultSuccess& execution_result, const plugins::manifest::HostParams& host_params);

        void clear_previous_bar_state();

        void prepare_next_bar_state(const http::stock_api::AggregateBarResult& bar);

        void reduce_active_fills_fifo(const std::string& symbol, double quantity_sold);
    };

}  // namespace simulators

#endif
