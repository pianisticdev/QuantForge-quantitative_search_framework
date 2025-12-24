#ifndef QUANT_SIMULATORS_BACK_TEST_ENGINE_HPP
#define QUANT_SIMULATORS_BACK_TEST_ENGINE_HPP

#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "../../forge/stores/data_store.hpp"
#include "../../plugins/loaders/interface.hpp"
#include "../../utils/min_heap.hpp"
#include "./exit_order_book.hpp"
#include "./limit_order_book.hpp"
#include "./models.hpp"
#include "./slippage_calculator.hpp"
#include "./state.hpp"

using namespace money_utils;

namespace simulators {

    struct BackTestReport {
        std::string some_value_;
        std::string another_value_;
    };

    class BackTestEngine {
       public:
        BackTestEngine(const plugins::loaders::IPluginLoader* plugin, const forge::DataStore* data_store);
        void run();
        void execute_order_book(const http::stock_api::AggregateBarResult& bar, const plugins::manifest::HostParams& host_params);
        void execute_limit_orders(const plugins::manifest::HostParams& host_params);
        void handle_execution_result(const models::ExecutionResult& execution_result, const plugins::manifest::HostParams& host_params);
        void schedule_plugin_instructions(const PluginResult& result, const plugins::manifest::HostParams& host_params);
        void execute_exit_orders(const plugins::manifest::HostParams& host_params);
        [[nodiscard]] const BackTestReport& get_report();
        void resolve_execution(const models::ExecutionResult& execution_result, const plugins::manifest::HostParams& host_params);

       private:
        const plugins::loaders::IPluginLoader* plugin_;
        const forge::DataStore* data_store_;
        BackTestReport report_;
        simulators::State state_ = {
            .cash_ = Money(0),
            .positions_ = {},
            .current_prices_ = {},
            .current_volumes_ = {},
            .current_timestamp_ns_ = 0,
            .fills_ = {},
            .exit_orders_ = {},
            .equity_curve_ = {},
            .peak_equity_ = Money(0),
            .max_drawdown_ = 0.0,
            .active_buy_fills_ = {},
            .active_sell_fills_ = {},
            .new_fills_ = {},
            .new_exit_orders_ = {},
        };
        data_structures::MinHeap<models::ScheduledOrder> order_book_;
        ExitOrderBook exit_order_book_;
        LimitOrderBook limit_order_book_;
    };

    [[nodiscard]] inline models::ScheduledOrder create_scheduled_order(const models::Order& order, const plugins::manifest::HostParams& host_params,
                                                                       const simulators::State& state) {
        return {order, SlippageCalculator::calculate_slippage_time_ns(order, host_params, state)};
    }

    [[nodiscard]] inline models::ScheduledLimitOrder create_scheduled_limit_order(const models::Order& order) {
        if (!order.limit_price_.has_value()) {
            throw std::runtime_error("Cannot create limit order without limit price");
        }

        if (order.is_buy()) {
            return models::LimitBuyOrder{order, order.limit_price_.value()};
        }

        return models::LimitSellOrder{order, order.limit_price_.value()};
    }

}  // namespace simulators

#endif
