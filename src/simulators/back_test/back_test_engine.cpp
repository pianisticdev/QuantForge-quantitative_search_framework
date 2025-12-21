#include "back_test_engine.hpp"

#include <memory>
#include <string>

#include "../../forge/stores/data_store.hpp"
#include "../../plugins/loaders/interface.hpp"
#include "../../utils/money_utils.hpp"
#include "./abi_converter.hpp"
#include "./exchange.hpp"
#include "./executor.hpp"
#include "./models.hpp"
#include "./position_calculator.hpp"
#include "./state.hpp"

using namespace money_utils;

namespace simulators {

    BackTestEngine::BackTestEngine(const plugins::loaders::IPluginLoader* plugin, const forge::DataStore* data_store)
        : plugin_(plugin), data_store_(data_store) {}

    void BackTestEngine::run() {
        const auto& host_params = plugin_->get_host_params();

        state_.cash_ = Money(host_params.initial_capital_);

        auto iterable_plugin_data = data_store_->get_iterable_plugin_data(plugin_->get_plugin_name());

        std::for_each(iterable_plugin_data.begin(), iterable_plugin_data.end(), [&, host_params](const auto& bar) {
            if (!Exchange::is_within_market_hour_restrictions(bar.unix_ts_ns_, host_params)) {
                return;
            }

            state_.prepare_next_bar_state(bar);

            execute_order_book(bar, host_params);
            schedule_exit_orders(host_params);

            PluginResult result = plugin_->on_bar(bar, state_);

            if (result.code_ != 0) {
                throw std::runtime_error("Plugin on_bar failed: " + std::string(result.message_));
            }

            schedule_plugin_instructions(result, host_params);

            state_.clear_previous_bar_state();
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

    void BackTestEngine::execute_order_book(const http::stock_api::AggregateBarResult& bar, const plugins::manifest::HostParams& host_params) {
        while (!order_book_.empty()) {
            auto top_order_optional = order_book_.top();

            if (!top_order_optional.has_value()) {
                break;
            }

            auto scheduled_order = top_order_optional.value();

            if (scheduled_order.scheduled_fill_at_ns_ > bar.unix_ts_ns_) {
                break;
            }

            order_book_.pop();

            auto execution_result = Executor::execute_order(scheduled_order.order_, host_params, state_);

            handle_execution_result(execution_result, host_params);
        }
    }

    void BackTestEngine::handle_execution_result(const models::ExecutionResult& execution_result, const plugins::manifest::HostParams& host_params) {
        std::visit(
            [&](auto arg) {
                using T = std::decay_t<decltype(arg)>;

                if constexpr (std::is_same_v<T, models::ExecutionResultError>) {
                    throw std::runtime_error("Execution failed: " + arg.message_);
                }

                if constexpr (std::is_same_v<T, models::ExecutionResultSuccess>) {
                    if (arg.fill_.is_sell()) {
                        auto closed_fills = PositionCalculator::find_buy_fill_uuids_closed_by_sell(arg.fill_, state_);
                        for (const auto& [fill_uuid, quantity] : closed_fills) {
                            exit_order_book_.reduce_exit_orders_by_fill_uuid(fill_uuid, quantity);
                        }
                    }

                    if (arg.has_exit_strategy()) {
                        models::ExitOrder exit_order = arg.exit_order_.value();
                        exit_order_book_.add_exit_order(exit_order);
                    }

                    if (arg.is_partial_fill()) {
                        order_book_.push(simulators::create_scheduled_order(arg.partial_order_.value(), host_params, state_));
                    }
                }

                resolve_execution(arg, host_params);
            },
            execution_result);
    }

    // Instead of "executing" a signal, we need to convert it to order earlier, before we add a scheduled instruction to the heap.
    // We will need to create some new methods to do this, and remove some existing business logic.
    // But, this will allow us to have a much clearer resolution instruction_heap (scheduled order heap).
    void BackTestEngine::schedule_plugin_instructions(const PluginResult& result, const plugins::manifest::HostParams& host_params) {
        ABIConverter::iterate_c_instructions(result, [&, host_params](const auto& c_intruction) {
            auto instruction = ABIConverter::to_instruction(c_intruction);

            models::Order order = std::visit(
                [&](auto&& arg) {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, models::Signal>) {
                        return Executor::signal_to_order(arg, host_params, state_);
                    }
                    if constexpr (std::is_same_v<T, models::Order>) {
                        return arg;
                    }
                    throw std::runtime_error("Invalid instruction type");
                },
                instruction);

            auto scheduled_order = create_scheduled_order(order, host_params, state_);

            order_book_.push(scheduled_order);
        });
    }

    void BackTestEngine::schedule_exit_orders(const plugins::manifest::HostParams& host_params) {
        exit_order_book_.process_stop_loss_heap(state_, [&, host_params](const models::StopLossExitOrder& exit_order) {
            auto sell_order = exit_order.to_sell_instruction();
            auto scheduled_order = create_scheduled_order(sell_order, host_params, state_);
            order_book_.push(scheduled_order);
        });

        exit_order_book_.process_take_profit_heap(state_, [&, host_params](const models::TakeProfitExitOrder& exit_order) {
            auto sell_order = exit_order.to_sell_instruction();
            auto scheduled_order = create_scheduled_order(sell_order, host_params, state_);
            order_book_.push(scheduled_order);
        });
    }

    const BackTestReport& BackTestEngine::get_report() { return report_; }

    void BackTestEngine::resolve_execution(const models::ExecutionResult& execution_result, const plugins::manifest::HostParams& host_params) {
        std::visit(
            [&](auto arg) {
                using T = std::decay_t<decltype(arg)>;

                if constexpr (std::is_same_v<T, models::ExecutionResultError>) {
                    throw std::runtime_error("Execution failed: " + arg.message_);
                }

                if constexpr (std::is_same_v<T, models::ExecutionResultSuccess>) {
                    state_.update_state(arg, host_params);
                }
            },
            execution_result);
    }

}  // namespace simulators
