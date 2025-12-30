#ifndef QUANT_SIMULATORS_BACK_TEST_STATE_HPP
#define QUANT_SIMULATORS_BACK_TEST_STATE_HPP

#pragma once

#include "../../http/api/stock_api.hpp"
#include "../plugins/manifest/manifest.hpp"
#include "./models.hpp"

namespace simulators {

    struct CurrentBarPrices {
        Money close_;
        Money open_;
        Money high_;
        Money low_;
    };

    class State {
       public:
        Money cash_;
        Money margin_in_use_;
        int64_t current_timestamp_ns_;
        std::map<std::string, models::Position> positions_;
        std::map<std::string, CurrentBarPrices> current_bar_prices_;
        std::map<std::string, int64_t> current_bar_volumes_;
        std::vector<models::Fill> fills_;
        std::vector<models::ExitOrder> exit_orders_;
        std::vector<models::EquitySnapshot> equity_curve_;
        std::vector<models::Fill> new_fills_;
        std::vector<models::ExitOrder> new_exit_orders_;
        std::map<std::string, double> active_buy_fills_;
        std::map<std::string, double> active_sell_fills_;
        std::map<std::string, Money> active_margin_for_fills_;
        std::map<std::string, double> active_leverage_for_fills_;
        Money peak_equity_;
        double max_drawdown_;

        [[nodiscard]] Money get_symbol_close(const std::string& symbol) const { return current_bar_prices_.at(symbol).close_; }

        [[nodiscard]] Money get_symbol_open(const std::string& symbol) const { return current_bar_prices_.at(symbol).open_; }

        [[nodiscard]] Money get_symbol_high(const std::string& symbol) const { return current_bar_prices_.at(symbol).high_; }

        [[nodiscard]] Money get_symbol_low(const std::string& symbol) const { return current_bar_prices_.at(symbol).low_; }

        [[nodiscard]] bool has_symbol_prices(const std::string& symbol) const { return current_bar_prices_.find(symbol) != current_bar_prices_.end(); }

        [[nodiscard]] int64_t get_symbol_volume(const std::string& symbol) const { return current_bar_volumes_.at(symbol); }

        [[nodiscard]] bool has_symbol_volume(const std::string& symbol) const { return current_bar_volumes_.find(symbol) != current_bar_volumes_.end(); }

        [[nodiscard]] models::Position get_symbol_position(const std::string& symbol) const { return positions_.at(symbol); }

        [[nodiscard]] bool has_symbol_position(const std::string& symbol) const { return positions_.find(symbol) != positions_.end(); }

        void prepare_initial_state(const plugins::manifest::HostParams& host_params);

        void update_state(const models::ExecutionResultSuccess& execution_result);

        void clear_previous_bar_state();

        void prepare_next_bar_state(const http::stock_api::AggregateBarResult& bar);

        void reduce_active_buy_fills_fifo(const std::string& symbol, double quantity);
        void reduce_active_sell_fills_fifo(const std::string& symbol, double quantity);
        [[nodiscard]] std::optional<std::pair<std::string, double>> populate_active_fills(const models::Fill& fill, double current_qty, bool comparison);
        void record_bar_equity_snapshot(const plugins::manifest::HostParams& host_params);

        [[nodiscard]] Money recalculate_margin_in_use() const;
    };

}  // namespace simulators

#endif
