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

        if (order.is_exit_order_ && order.source_fill_uuid_.has_value()) {
            const auto& uuid = order.source_fill_uuid_.value();
            bool fill_still_active = state.active_buy_fills_.contains(uuid) || state.active_sell_fills_.contains(uuid);

            if (!fill_still_active) {
                return models::ExecutionResultError("Exit order source fill no longer active - skipping");
            }
        }

        auto [fillable_quantity, remaining_quantity] = Executor::get_fillable_and_remaining_quantities(order, host_params, state);

        if (!host_params.allow_fractional_shares_.value_or(false)) {
            fillable_quantity = std::floor(fillable_quantity);
            if (fillable_quantity <= 0) {
                return models::ExecutionResultError("Order quantity is too small to execute");
            }
        }

        Money fill_price = calculate_fill_price(order, state);

        models::Fill fill(order.symbol_, order.action_, fillable_quantity, fill_price, state.current_timestamp_ns_);

        auto exit_orders = create_exit_orders(order, fill, state, fillable_quantity);

        auto cash_delta = calculate_cash_delta(order, fill, host_params, fillable_quantity);

        if ((state.cash_ + cash_delta).to_dollars() < 0) {
            return models::ExecutionResultError("Insufficient funds for trade and commission");
        }

        auto position = PositionCalculator::calculate_position(order, fillable_quantity, fill_price, state);

        if (remaining_quantity > 0) {
            models::Order partial_order = order;
            partial_order.quantity_ = remaining_quantity;
            partial_order.created_at_ns_ = state.current_timestamp_ns_;
            return models::ExecutionResultSuccess(cash_delta, std::make_optional(partial_order), position, fill, exit_orders);
        }

        return models::ExecutionResultSuccess(cash_delta, std::nullopt, position, fill, exit_orders);
    }

    std::vector<models::ExitOrder> Executor::create_exit_orders(const models::Order& order, const models::Fill& fill, const simulators::State& state,
                                                                double fillable_quantity) {
        auto pos_it = state.positions_.find(order.symbol_);
        double current_position = (pos_it != state.positions_.end()) ? pos_it->second.quantity_ : 0.0;

        bool is_short_position_fill = order.is_sell() && current_position <= 0;

        std::vector<models::ExitOrder> exit_orders;
        if (order.stop_loss_price_.has_value()) {
            exit_orders.emplace_back(std::in_place_type<models::StopLossExitOrder>, order.symbol_, fillable_quantity, order.stop_loss_price_.value(),
                                     fill.price_, state.current_timestamp_ns_, fill.uuid_, is_short_position_fill);
        }
        if (order.take_profit_price_.has_value()) {
            exit_orders.emplace_back(std::in_place_type<models::TakeProfitExitOrder>, order.symbol_, fillable_quantity, order.take_profit_price_.value(),
                                     fill.price_, state.current_timestamp_ns_, fill.uuid_, is_short_position_fill);
        }
        return exit_orders;
    }

    Money Executor::calculate_cash_delta(const models::Order& order, const models::Fill& fill, const plugins::manifest::HostParams& host_params,
                                         double fillable_quantity) {
        auto commission = Exchange::calculate_commision(fill, host_params);

        auto fill_value = fill.price_ * fillable_quantity;

        if (order.is_buy()) {
            return (fill_value + commission) * -1;
        }

        if (order.is_sell()) {
            return fill_value - commission;
        }

        return Money(0);
    }

    Money Executor::calculate_fill_price(const models::Order& order, const simulators::State& state) {
        if (order.is_limit_order() && order.limit_price_.has_value()) {
            Money current_price = state.current_prices_.at(order.symbol_);
            if (order.is_buy()) {
                return std::min(order.limit_price_.value(), current_price);
            }
            return std::max(order.limit_price_.value(), current_price);
        }
        return state.current_prices_.at(order.symbol_);
    }

    models::Order Executor::signal_to_order(const models::Signal& signal, const plugins::manifest::HostParams& host_params, const simulators::State& state) {
        std::optional<Money> stop_loss_price = PositionCalculator::calculate_signal_stop_loss_price(signal, host_params, state);
        std::optional<Money> take_profit_price = PositionCalculator::calculate_signal_take_profit_price(signal, host_params, state);
        double quantity = PositionCalculator::calculate_signal_position_size(signal, host_params, state);

        return {quantity, state.current_timestamp_ns_, signal.symbol_, signal.action_, constants::MARKET, std::nullopt, stop_loss_price, take_profit_price};
    }
}  // namespace simulators
