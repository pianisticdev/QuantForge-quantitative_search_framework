#ifndef QUANT_SIMULATORS_BACK_TEST_ENGINE_HPP
#define QUANT_SIMULATORS_BACK_TEST_ENGINE_HPP

#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "../../forge/stores/data_store.hpp"
#include "../../plugins/loaders/interface.hpp"
#include "./models.hpp"

namespace simulators {

    struct BackTestReport {
        std::string some_value_;
        std::string another_value_;
    };

    struct ExecutionResult {
        bool success_;
        std::string message_;
        int64_t cash_delta_;
        models::Position position_;
        models::Trade trade_;

        ExecutionResult(std::string message) : success_(false), message_(std::move(message)), cash_delta_(0), position_(), trade_() {}
        ExecutionResult(bool success, std::string message, int64_t cash_delta, models::Position position, models::Trade trade)
            : success_(success), message_(std::move(message)), cash_delta_(cash_delta), position_(std::move(position)), trade_(std::move(trade)) {}
    };

    class Exchange {
       public:
        [[nodiscard]] static bool is_market_open(int64_t timestamp_ns, const plugins::manifest::HostParams& host_params, const models::BackTestState& state);
    };

    class Executor {
       public:
        [[nodiscard]] static bool is_execution_allowed(const models::Instruction& instruction, const plugins::manifest::HostParams& host_params,
                                                       const models::BackTestState& state);
        [[nodiscard]] static ExecutionResult execute_order(const models::Instruction& instruction, const plugins::manifest::HostParams& host_params,
                                                           const models::BackTestState& state);
        [[nodiscard]] static ExecutionResult execute_signal(const models::Instruction& instruction, const plugins::manifest::HostParams& host_params,
                                                            const models::BackTestState& state);
        [[nodiscard]] static ExecutionResult execute_sell(const models::Instruction& instruction, const plugins::manifest::HostParams& host_params,
                                                          const models::BackTestState& state);
    };

    class RiskManager {
       public:
        [[nodiscard]] static bool is_at_stop_loss(const models::Position& position, const plugins::manifest::HostParams& host_params,
                                                  const models::BackTestState& state);
        [[nodiscard]] static bool is_at_take_profit(const models::Position& position, const plugins::manifest::HostParams& host_params,
                                                    const models::BackTestState& state);
    };

    class BackTestEngine {
       public:
        BackTestEngine(const plugins::loaders::IPluginLoader* plugin, const forge::DataStore* data_store);
        void run();
        [[nodiscard]] const BackTestReport& get_report();
        void resolve_execution(const ExecutionResult& execution_result);

       private:
        const plugins::loaders::IPluginLoader* plugin_;
        const forge::DataStore* data_store_;
        BackTestReport report_;
        models::BackTestState state_ = {
            .cash_ = 0,
            .positions_ = {},
            .current_prices_ = {},
            .current_timestamp_ns_ = 0,
            .trade_history_ = {},
            .equity_curve_ = {},
        };
        Exchange exchange_{};
        Executor executor_{};
        RiskManager risk_manager_{};
    };
}  // namespace simulators

#endif
