#include "./executor.hpp"

#include "../../utils/constants.hpp"
#include "./exchange.hpp"
#include "./position_calculator.hpp"
#include "./state.hpp"

namespace simulators {
    std::pair<double, double> Executor::get_fillable_and_remaining_quantities(const models::Order& order, const plugins::manifest::HostParams& host_params,
                                                                              const simulators::State& state) {
        if (host_params.fill_max_pct_of_volume_.has_value()) {
            auto bar_volume = static_cast<double>(state.current_volumes_.at(order.symbol_));
            auto max_fill_quantity = bar_volume * host_params.fill_max_pct_of_volume_.value();

            if (order.quantity_ > max_fill_quantity) {
                auto remaining_quantity = order.quantity_ - max_fill_quantity;
                return {max_fill_quantity, remaining_quantity};
            }
        }

        return {order.quantity_, 0};
    }

    models::ExecutionResult Executor::execute_order(const models::Order& order, const plugins::manifest::HostParams& host_params,
                                                    const simulators::State& state) {
        if (order.quantity_ <= 0) {
            return models::ExecutionResultError("Order quantity must be positive");
        }

        if (state.current_prices_.find(order.symbol_) == state.current_prices_.end()) {
            return models::ExecutionResultError("No price data for symbol: " + order.symbol_);
        }

        if (state.current_volumes_.find(order.symbol_) == state.current_volumes_.end()) {
            return models::ExecutionResultError("No volume data for symbol: " + order.symbol_);
        }

        auto [fillable_quantity, remaining_quantity] = Executor::get_fillable_and_remaining_quantities(order, host_params, state);

        if (!host_params.allow_fractional_shares_.value_or(false)) {
            fillable_quantity = std::floor(fillable_quantity);
            if (fillable_quantity <= 0) {
                return models::ExecutionResultError("Order quantity is too small to execute");
            }
        }

        auto pos_it = state.positions_.find(order.symbol_);
        double current_position = (pos_it != state.positions_.end()) ? pos_it->second.quantity_ : 0.0;

        if (order.is_buy() && current_position < 0) {
            // Buying to cover short - no special restriction, but validate we don't over-cover beyond our intent
            // (this is usually fine, results in going long)
        }
        // Selling with no position or short position is now ALLOWED (opens/adds to short)
        // Selling with long position reduces the long (and may flip to short)

        models::Fill fill(order.symbol_, order.action_, fillable_quantity, state.current_prices_.at(order.symbol_), state.current_timestamp_ns_);

        bool is_opening_short = order.is_sell() && current_position <= 0;

        auto exit_orders = [&, state, is_opening_short]() -> std::vector<models::ExitOrder> {
            std::vector<models::ExitOrder> exit_orders;
            if (order.stop_loss_price_.has_value()) {
                exit_orders.emplace_back(std::in_place_type<models::StopLossExitOrder>, order.symbol_, fillable_quantity, order.stop_loss_price_.value(),
                                         fill.price_, state.current_timestamp_ns_, fill.uuid_, is_opening_short);  // NEW param
            }
            if (order.take_profit_price_.has_value()) {
                exit_orders.emplace_back(std::in_place_type<models::TakeProfitExitOrder>, order.symbol_, fillable_quantity, order.take_profit_price_.value(),
                                         fill.price_, state.current_timestamp_ns_, fill.uuid_, is_opening_short);  // NEW param
            }
            return exit_orders;
        }();

        auto commission = Exchange::calculate_commision(fill, host_params);

        auto fill_value = fill.price_ * fillable_quantity;
        auto cash_delta = [&]() {
            if (order.is_buy()) {
                return (fill_value + commission) * -1;
            }

            if (order.is_sell()) {
                return fill_value - commission;
            }

            return Money(0);
        }();

        if ((state.cash_ + cash_delta).to_dollars() < 0) {
            return models::ExecutionResultError("Insufficient funds for trade and commission");
        }

        auto position = PositionCalculator::calculate_position(order, fillable_quantity, state);

        if (remaining_quantity > 0) {
            models::Order partial_order = order;
            partial_order.quantity_ = remaining_quantity;
            partial_order.created_at_ns_ = state.current_timestamp_ns_;
            return models::ExecutionResultSuccess(cash_delta, std::make_optional(partial_order), position, fill, exit_orders);
        }

        return models::ExecutionResultSuccess(cash_delta, std::nullopt, position, fill, exit_orders);
    }

    models::Order Executor::signal_to_order(const models::Signal& signal, const plugins::manifest::HostParams& host_params, const simulators::State& state) {
        std::optional<Money> stop_loss_price = PositionCalculator::calculate_signal_stop_loss_price(signal, host_params, state);
        std::optional<Money> take_profit_price = PositionCalculator::calculate_signal_take_profit_price(signal, host_params, state);
        double quantity = PositionCalculator::calculate_signal_position_size(signal, host_params, state);

        return {quantity, state.current_timestamp_ns_, signal.symbol_, signal.action_, constants::MARKET, std::nullopt, stop_loss_price, take_profit_price};
    }
}  // namespace simulators
