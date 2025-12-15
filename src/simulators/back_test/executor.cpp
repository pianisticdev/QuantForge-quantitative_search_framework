#include "./executor.hpp"

#include "../../utils/constants.hpp"
#include "./exchange.hpp"
#include "./position_calculator.hpp"
#include "./state.hpp"

namespace simulators {
    std::pair<double, double> Executor::get_fillable_and_remaining_quantities(const models::Order& order, const plugins::manifest::HostParams& host_params,
                                                                              const simulators::State& state) {
        if (host_params.fill_max_pct_of_volume_ != std::nullopt) {
            auto symbol_cash = state.current_prices_.at(order.symbol_) * order.quantity_;
            auto fill_pct = symbol_cash.to_dollars() / static_cast<double>(state.current_volumes_.at(order.symbol_));

            if (host_params.fill_max_pct_of_volume_.has_value() && fill_pct > host_params.fill_max_pct_of_volume_.value()) {
                auto max_fill_quantity = static_cast<double>(state.current_volumes_.at(order.symbol_)) * host_params.fill_max_pct_of_volume_.value();
                auto remaining_quantity = order.quantity_ - max_fill_quantity;
                return {max_fill_quantity, remaining_quantity};
            }
        }

        return {order.quantity_, 0};
    }

    models::ExecutionResult Executor::execute_order(const models::Order& order, const plugins::manifest::HostParams& host_params,
                                                    const simulators::State& state) {
        auto [fillable_quantity, remaining_quantity] = Executor::get_fillable_and_remaining_quantities(order, host_params, state);

        if (!host_params.allow_fractional_shares_.has_value()) {
            fillable_quantity = std::floor(fillable_quantity);
            if (fillable_quantity <= 0) {
                return models::ExecutionResultError("Order quantity is too small to execute");
            }
        }

        models::Fill fill(order.symbol_, order.action_, fillable_quantity, state.current_prices_.at(order.symbol_), state.current_timestamp_ns_);

        auto exit_order = [&, state]() -> std::optional<models::ExitOrder> {
            if (order.stop_loss_price_.has_value()) {
                return models::StopLossExitOrder(order.symbol_, fillable_quantity, order.stop_loss_price_.value(), fill.price_, state.current_timestamp_ns_);
            }
            if (order.take_profit_price_.has_value()) {
                return models::TakeProfitExitOrder(order.symbol_, fillable_quantity, order.take_profit_price_.value(), fill.price_,
                                                   state.current_timestamp_ns_);
            }
            return std::nullopt;
        }();

        auto commission = Exchange::calculate_commision(fill, host_params);

        auto fill_value = fill.price_ * fillable_quantity;
        auto cash_delta = [&]() {
            if (order.action_ == constants::BUY) {
                return (fill_value + commission) * -1;
            }

            if (order.action_ == constants::SELL) {
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
            return models::ExecutionResultSuccess(cash_delta, std::make_optional(partial_order), position, fill, exit_order);
        }

        return models::ExecutionResultSuccess(cash_delta, std::nullopt, position, fill, exit_order);
    }

    models::Order Executor::signal_to_order(const models::Signal& signal, const plugins::manifest::HostParams& host_params, const simulators::State& state) {
        std::optional<Money> stop_loss_price = PositionCalculator::calculate_signal_stop_loss_price(signal, host_params, state);
        std::optional<Money> take_profit_price = PositionCalculator::calculate_signal_take_profit_price(signal, host_params, state);
        double quantity = PositionCalculator::calculate_signal_position_size(signal, host_params, state);

        return {quantity, state.current_timestamp_ns_, signal.symbol_, signal.action_, constants::MARKET, std::nullopt, stop_loss_price, take_profit_price};
    }
}  // namespace simulators
