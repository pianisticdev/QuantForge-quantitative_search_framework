#include "back_test_engine.hpp"

#include <memory>
#include <string>

#include "../../forge/stores/data_store.hpp"
#include "../../plugins/loaders/interface.hpp"
#include "./models.hpp"

namespace simulators {

    BackTestEngine::BackTestEngine(const plugins::loaders::IPluginLoader* plugin, const forge::DataStore* data_store)
        : plugin_(plugin), data_store_(data_store) {}

    void BackTestEngine::run() {
        const auto& host_params = plugin_->get_host_params();

        state_.cash_ = host_params.initial_capital_;

        auto iterable_plugin_data = data_store_->get_iterable_plugin_data(plugin_->get_plugin_name());

        std::for_each(iterable_plugin_data.begin(), iterable_plugin_data.end(), [&, host_params](const auto& bar) {
            if (!Exchange::is_market_open(bar.unix_ts_ns_, host_params, state_)) {
                return;
            }

            PluginResult result = plugin_->on_bar(bar, state_);

            if (result.code_ != 0) {
                throw std::runtime_error("Plugin on_bar failed: " + std::string(result.message_));
            }

            auto instructions = models::Instruction::to_instructions(result);

            std::for_each(instructions.begin(), instructions.end(), [&, host_params](const auto& instruction) {
                if (!Executor::is_execution_allowed(instruction, host_params, state_)) {
                    return;
                }

                auto execution_result = instruction.is_signal() ? Executor::execute_signal(instruction, host_params, state_)
                                                                : Executor::execute_order(instruction, host_params, state_);

                resolve_execution(execution_result);
            });

            std::for_each(state_.positions_.begin(), state_.positions_.end(), [&, host_params](const auto& position) {
                if (!RiskManager::is_at_stop_loss(position.second, host_params, state_) &&
                    !RiskManager::is_at_take_profit(position.second, host_params, state_)) {
                    return;
                }

                auto execution_result = Executor::execute_sell(position.second.to_sell_instruction(), host_params, state_);

                resolve_execution(execution_result);
            });
        });

        const char* json_out = nullptr;
        PluginResult result = plugin_->on_end(&json_out);

        if (result.code_ != 0) {
            throw std::runtime_error("Plugin on_end failed: " + std::string(result.message_));
        }

        plugin_->free_string(json_out);

        report_ = {
            .some_value_ = "some_value",
            .another_value_ = "another_value",
        };
    }

    const BackTestReport& BackTestEngine::get_report() { return report_; }

    void BackTestEngine::resolve_execution(const ExecutionResult& execution_result) {
        if (!execution_result.success_) {
            throw std::runtime_error("Execution failed: " + execution_result.message_);
        }
    }

    bool Exchange::is_market_open(int64_t timestamp_ns, const plugins::manifest::HostParams& host_params, const models::BackTestState& state) { return true; }

    bool Executor::is_execution_allowed(const models::Instruction& instruction, const plugins::manifest::HostParams& host_params,
                                        const models::BackTestState& state) {
        return true;
    }

    ExecutionResult Executor::execute_order(const models::Instruction& instruction, const plugins::manifest::HostParams& host_params,
                                            const models::BackTestState& state) {
        return ExecutionResult{true, "Execution allowed", 0, models::Position(), models::Trade()};
    }

    ExecutionResult Executor::execute_signal(const models::Instruction& instruction, const plugins::manifest::HostParams& host_params,
                                             const models::BackTestState& state) {
        return ExecutionResult{true, "Execution allowed", 0, models::Position(), models::Trade()};
    }

    ExecutionResult Executor::execute_sell(const models::Instruction& instruction, const plugins::manifest::HostParams& host_params,
                                           const models::BackTestState& state) {
        return ExecutionResult{true, "Execution allowed", 0, models::Position(), models::Trade()};
    }

    bool RiskManager::is_at_stop_loss(const models::Position& position, const plugins::manifest::HostParams& host_params, const models::BackTestState& state) {
        return true;
    }

    bool RiskManager::is_at_take_profit(const models::Position& position, const plugins::manifest::HostParams& host_params,
                                        const models::BackTestState& state) {
        return true;
    }

}  // namespace simulators
