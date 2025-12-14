#include "./executor.hpp"

#include "../../utils/constants.hpp"
#include "./exchange.hpp"
#include "./position_calculator.hpp"

namespace simulators {
    std::pair<double, double> Executor::get_fillable_and_remaining_quantities(const models::Order& order, const plugins::manifest::HostParams& host_params,
                                                                              const models::BackTestState& state) {
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
                                                    const models::BackTestState& state) {
        auto [fillable_quantity, remaining_quantity] = Executor::get_fillable_and_remaining_quantities(order, host_params, state);

        if (!host_params.allow_fractional_shares_.has_value()) {
            fillable_quantity = std::floor(fillable_quantity);
            if (fillable_quantity <= 0) {
                return {"Order quantity is too small to execute"};
            }
        }

        models::Fill fill(order.symbol_, order.action_, fillable_quantity, state.current_prices_.at(order.symbol_), state.current_timestamp_ns_);

        std::optional<models::ExitOrder> exit_order = std::nullopt;
        if (order.stop_loss_price_.has_value()) {
            exit_order = models::StopLossExitOrder(order.symbol_, fillable_quantity, order.stop_loss_price_.value(), fill.price_, state.current_timestamp_ns_);
        }
        if (order.take_profit_price_.has_value()) {
            exit_order =
                models::TakeProfitExitOrder(order.symbol_, fillable_quantity, order.take_profit_price_.value(), fill.price_, state.current_timestamp_ns_);
        }

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

        auto post_trade_cash = state.cash_ + cash_delta;

        if (post_trade_cash.to_dollars() < 0) {
            return {"Insufficient funds for trade and commission"};
        }

        models::Position position = state.positions_.at(order.symbol_);
        position.symbol_ = order.symbol_;
        if (order.action_ == constants::BUY) {
            position.quantity_ += fillable_quantity;
        } else if (order.action_ == constants::SELL) {
            position.quantity_ -= fillable_quantity;
        }
        std::vector<models::Fill> fills_for_symbol;
        for (const auto& existing_fill : state.fills_) {
            if (existing_fill.symbol_ == order.symbol_) {
                fills_for_symbol.emplace_back(existing_fill);
            }
        }
        position.average_price_ =
            (fill.price_ + (position.average_price_ * static_cast<double>(fills_for_symbol.size()))) / (static_cast<double>(fills_for_symbol.size()) + 1);

        if (remaining_quantity > 0) {
            models::Order partial_order = order;
            partial_order.quantity_ = remaining_quantity;
            partial_order.created_at_ns_ = state.current_timestamp_ns_;
            return {true, "Partial fill", cash_delta, std::make_optional(partial_order), position, fill, exit_order};
        }

        return {true, "Execution allowed", cash_delta, std::nullopt, position, fill, exit_order};
    }

    models::Order Executor::signal_to_order(const models::Signal& signal, const plugins::manifest::HostParams& host_params,
                                            const models::BackTestState& state) {
        std::optional<Money> stop_loss_price = PositionCalculator::calculate_signal_stop_loss_price(signal, host_params, state);
        std::optional<Money> take_profit_price = PositionCalculator::calculate_signal_take_profit_price(signal, host_params, state);
        double quantity = PositionCalculator::calculate_signal_position_size(signal, host_params, state);

        return {quantity, state.current_timestamp_ns_, signal.symbol_, signal.action_, constants::MARKET, std::nullopt, stop_loss_price, take_profit_price};
    }
}  // namespace simulators
