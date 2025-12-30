#include "./executor.hpp"

#include "../../utils/constants.hpp"
#include "./equity_calculator.hpp"
#include "./exchange.hpp"
#include "./position_calculator.hpp"
#include "./state.hpp"

namespace simulators {
    std::pair<double, double> Executor::get_fillable_and_remaining_quantities(const models::Order& order, const plugins::manifest::HostParams& host_params,
                                                                              const simulators::State& state) {
        if (host_params.fill_max_pct_of_volume_.has_value()) {
            const auto bar_volume = static_cast<double>(state.get_symbol_volume(order.symbol_));
            const double max_fill_quantity = bar_volume * host_params.fill_max_pct_of_volume_.value();

            if (order.quantity_ > max_fill_quantity) {
                const double remaining_quantity = order.quantity_ - max_fill_quantity;
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

        if (!state.has_symbol_prices(order.symbol_)) {
            return models::ExecutionResultError("No price data for symbol: " + order.symbol_);
        }

        if (!state.has_symbol_volume(order.symbol_)) {
            return models::ExecutionResultError("No volume data for symbol: " + order.symbol_);
        }

        if (order.is_exit_order_ && order.source_fill_uuid_.has_value()) {
            const auto& uuid = order.source_fill_uuid_.value();
            const bool fill_still_active = state.active_buy_fills_.contains(uuid) || state.active_sell_fills_.contains(uuid);

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

        const Money fill_price = calculate_fill_price(order, state);

        const models::Position symbol_position = state.get_symbol_position(order.symbol_);
        const double current_position_quantity = state.has_symbol_position(order.symbol_) ? symbol_position.quantity_ : 0.0;
        const double new_position_quantity = current_position_quantity + (order.is_buy() ? fillable_quantity : -fillable_quantity);

        const double position_opening_quantity =
            calculate_position_opening_quantity(order, fillable_quantity, current_position_quantity, new_position_quantity);

        const Money commission =
            Exchange::calculate_commision(models::Fill(order.symbol_, order.action_, fillable_quantity, fill_price, state.current_timestamp_ns_), host_params);

        const double leverage = order.leverage_.value_or(1.0);

        const Money margin_required =
            position_opening_quantity > constants::EPSILON ? calculate_margin_required(host_params, fill_price, position_opening_quantity, leverage) : Money(0);

        const auto validation_error =
            validate_margin(order, fill_price, commission, host_params, state, position_opening_quantity, new_position_quantity, margin_required);
        if (validation_error.has_value()) {
            return models::ExecutionResultError(validation_error.value());
        }

        const Money cash_delta = calculate_cash_delta(order, fill_price, fillable_quantity, commission, position_opening_quantity, margin_required, state);

        const models::Fill fill(order.symbol_, order.action_, fillable_quantity, fill_price, state.current_timestamp_ns_, leverage, margin_required);

        const auto exit_orders = create_exit_orders(order, fill, state, position_opening_quantity, new_position_quantity);

        const models::Position position = PositionCalculator::calculate_position(order, fillable_quantity, fill_price, state);

        if (remaining_quantity > 0) {
            models::Order partial_order = order;
            partial_order.quantity_ = remaining_quantity;
            partial_order.created_at_ns_ = state.current_timestamp_ns_;
            return models::ExecutionResultSuccess(cash_delta, margin_required, leverage, std::make_optional(partial_order), position, fill, exit_orders);
        }

        return models::ExecutionResultSuccess(cash_delta, margin_required, leverage, std::nullopt, position, fill, exit_orders);
    }

    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    double Executor::calculate_position_opening_quantity(const models::Order& order, double fillable_quantity, double current_position_quantity,
                                                         double new_position_quantity) {
        if (order.is_buy()) {
            if (current_position_quantity >= 0) {
                return fillable_quantity;
            }
            return std::max(0.0, new_position_quantity);
        }

        if (current_position_quantity <= 0) {
            return fillable_quantity;
        }

        return std::max(0.0, -new_position_quantity);
    }

    std::vector<models::ExitOrder> Executor::create_exit_orders(const models::Order& order, const models::Fill& fill, const simulators::State& state,
                                                                // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
                                                                double position_opening_quantity, double new_position_quantity) {
        if (position_opening_quantity <= constants::EPSILON) {
            return {};
        }

        const bool is_short_position_fill = order.is_sell() && new_position_quantity <= 0;

        std::vector<models::ExitOrder> exit_orders;
        if (order.stop_loss_price_.has_value()) {
            exit_orders.emplace_back(std::in_place_type<models::StopLossExitOrder>, order.symbol_, position_opening_quantity, order.stop_loss_price_.value(),
                                     fill.price_, state.current_timestamp_ns_, fill.uuid_, is_short_position_fill);
        }
        if (order.take_profit_price_.has_value()) {
            exit_orders.emplace_back(std::in_place_type<models::TakeProfitExitOrder>, order.symbol_, position_opening_quantity,
                                     order.take_profit_price_.value(), fill.price_, state.current_timestamp_ns_, fill.uuid_, is_short_position_fill);
        }
        return exit_orders;
    }

    Money Executor::calculate_cash_delta(const models::Order& order, Money fill_price, double fillable_quantity, Money commission,
                                         double position_opening_quantity, Money margin_required, const simulators::State& state) {
        const double position_closing_quantity = fillable_quantity - position_opening_quantity;

        const simulators::ClosingMarginInfo closing_info = calculate_closing_margin_info(order, fill_price, position_closing_quantity, state);

        if (order.is_buy()) {
            const Money opening_cost = margin_required;
            const Money closing_return = closing_info.margin_to_release_ + closing_info.realized_pnl_;
            return closing_return - opening_cost - commission;
        }

        if (order.is_sell()) {
            const Money opening_cost = margin_required;
            const Money closing_return = closing_info.margin_to_release_ + closing_info.realized_pnl_;
            return closing_return - opening_cost - commission;
        }

        return Money(0);
    }

    Money Executor::calculate_fill_price(const models::Order& order, const simulators::State& state) {
        const Money current_bar_close = state.get_symbol_close(order.symbol_);
        if (order.is_limit_order() && order.limit_price_.has_value()) {
            if (order.is_buy()) {
                return std::min(order.limit_price_.value(), current_bar_close);
            }
            return std::max(order.limit_price_.value(), current_bar_close);
        }
        return current_bar_close;
    }

    models::Order Executor::signal_to_order(const models::Signal& signal, const plugins::manifest::HostParams& host_params, const simulators::State& state) {
        const std::optional<Money> stop_loss_price = PositionCalculator::calculate_signal_stop_loss_price(signal, host_params, state);
        const std::optional<Money> take_profit_price = PositionCalculator::calculate_signal_take_profit_price(signal, host_params, state);
        const double quantity = PositionCalculator::calculate_signal_position_size(signal, host_params, state);

        return {quantity, state.current_timestamp_ns_, signal.symbol_, signal.action_, constants::MARKET, std::nullopt, stop_loss_price, take_profit_price};
    }

    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    Money Executor::calculate_margin_required(const plugins::manifest::HostParams& host_params, Money fill_price, double position_opening_quantity,
                                              double leverage) {
        const double initial_margin_pct = host_params.initial_margin_pct_.value_or(1.0);
        const double opening_value_dollars = fill_price.to_dollars() * position_opening_quantity;
        const double margin_from_leverage = opening_value_dollars / leverage;
        const double margin_from_initial = opening_value_dollars * initial_margin_pct;

        return Money::from_dollars(std::max(margin_from_leverage, margin_from_initial));
    }

    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    std::optional<std::string> Executor::validate_margin(const models::Order& order, Money fill_price, Money commission,
                                                         const plugins::manifest::HostParams& host_params, const simulators::State& state,
                                                         // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
                                                         double position_opening_quantity, double new_position_quantity, Money margin_required) {
        if (order.is_sell() && !host_params.allow_short_selling_.value_or(true)) {
            if (new_position_quantity < 0) {
                return "Short selling is not allowed";
            }
        }

        const double leverage = order.leverage_.value_or(1.0);
        const double max_leverage = host_params.max_leverage_.value_or(1.0);

        if (leverage < 1.0) {
            return "Leverage must be >= 1.0";
        }

        if (leverage > max_leverage) {
            return "Order leverage exceeds maximum allowed";
        }

        if (position_opening_quantity <= constants::EPSILON) {
            const Money total_cost = order.is_buy() ? (fill_price * order.quantity_) + commission : commission;

            if (total_cost > state.cash_) {
                return "Insufficient cash to close position.";
            }
            return std::nullopt;
        }

        const Money total_required = margin_required + commission;
        const Money available_margin = EquityCalculator::calculate_available_margin(state);

        if (total_required > available_margin) {
            return "Insufficient margin.";
        }

        return std::nullopt;
    }

    simulators::ClosingMarginInfo Executor::calculate_closing_margin_info(const models::Order& order, Money fill_price, double position_closing_quantity,
                                                                          const simulators::State& state) {
        if (position_closing_quantity <= constants::EPSILON) {
            return {Money(0), Money(0)};
        }

        Money margin_to_release(0);
        Money realized_pnl(0);
        double remaining = position_closing_quantity;

        const auto& active_fills = order.is_sell() ? state.active_buy_fills_ : state.active_sell_fills_;

        for (const auto& fill : state.fills_) {
            if (remaining <= constants::EPSILON) {
                break;
            }

            bool is_matching_fill = (order.is_sell() && fill.is_buy()) || (order.is_buy() && fill.is_sell());

            if (fill.symbol_ == order.symbol_ && is_matching_fill && active_fills.find(fill.uuid_) != active_fills.end()) {
                const double available = active_fills.at(fill.uuid_);
                const double to_close = std::min(available, remaining);

                const auto margin_it = state.active_margin_for_fills_.find(fill.uuid_);
                if (margin_it != state.active_margin_for_fills_.end()) {
                    const double proportion = to_close / available;
                    margin_to_release += margin_it->second * proportion;
                }

                if (order.is_sell()) {
                    realized_pnl += (fill_price - fill.price_) * to_close;
                } else {
                    realized_pnl += (fill.price_ - fill_price) * to_close;
                }

                remaining -= to_close;
            }
        }

        return {margin_to_release, realized_pnl};
    }
}  // namespace simulators
