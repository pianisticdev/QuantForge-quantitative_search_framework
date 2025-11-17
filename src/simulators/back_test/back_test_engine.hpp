#ifndef QUANT_SIMULATORS_BACK_TEST_ENGINE_HPP
#define QUANT_SIMULATORS_BACK_TEST_ENGINE_HPP

#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "../../forge/stores/data_store.hpp"
#include "../../plugins/loaders/interface.hpp"
#include "../../plugins/manifest/manifest.hpp"

namespace simulators {
    struct Position {
        std::string symbol_;
        double quantity_;
        double average_price_;
    };

    struct Trade {
        std::string symbol_;
        double quantity_;
        double price_;
        int64_t timestamp_ns_;
    };

    struct EquitySnapshot {
        int64_t timestamp_ns_;
        double equity_;
        double return_;
        double max_drawdown_;
        double sharpe_ratio_;
        double sortino_ratio_;
        double calmar_ratio_;
        double tail_ratio_;
        double value_at_risk_;
        double conditional_value_at_risk_;
    };

    struct BackTestState {
        int64_t cash_;
        std::map<std::string, Position> positions_;

        std::map<std::string, int64_t> current_prices_;
        int64_t current_timestamp_ns_;

        std::vector<Trade> trade_history_;
        std::vector<EquitySnapshot> equity_curve_;
    };

    struct BackTestReport {
        std::string some_value_;
        std::string another_value_;
    };

    class PositionSizer {
       public:
        PositionSizer(const plugins::manifest::HostParams& host_params);

        [[nodiscard]] double calculate_position_size(const BackTestState& state) const;

       private:
        plugins::manifest::HostParams host_params_;
    };

    class RiskManager {
       public:
        RiskManager(const plugins::manifest::HostParams& host_params);

        [[nodiscard]] double calculate_stop_loss(const BackTestState& state) const;
        [[nodiscard]] double calculate_take_profit(const BackTestState& state) const;

       private:
        plugins::manifest::HostParams host_params_;
    };

    class ExecutionSimulator {
       public:
        ExecutionSimulator(const plugins::manifest::HostParams& host_params);

        void simulate_execution(const BackTestState& state) const;

       private:
        plugins::manifest::HostParams host_params_;
    };

    class PortfolioTracker {
       public:
        PortfolioTracker(const plugins::manifest::HostParams& host_params);

        void apply_trade(const Trade& trade) const;

       private:
        plugins::manifest::HostParams host_params_;
    };

    class BackTestEngine {
       public:
        BackTestEngine(const plugins::loaders::IPluginLoader* plugin, const forge::DataStore* data_store);
        void run();
        [[nodiscard]] const BackTestReport& get_report();

       private:
        const plugins::loaders::IPluginLoader* plugin_;
        const forge::DataStore* data_store_;
        BackTestReport report_;
        BackTestState state_ = {
            .cash_ = 0,
            .positions_ = {},
            .current_prices_ = {},
            .current_timestamp_ns_ = 0,
            .trade_history_ = {},
            .equity_curve_ = {},
        };

        PositionSizer position_sizer_;
        RiskManager risk_manager_;
        ExecutionSimulator execution_simulator_;
        PortfolioTracker portfolio_tracker_;
    };
}  // namespace simulators

#endif
