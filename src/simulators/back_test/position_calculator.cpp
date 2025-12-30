#include "./position_calculator.hpp"

#include "./equity_calculator.hpp"
#include "./state.hpp"

namespace simulators::position_calc {
    double calculate_signal_position_size(const models::Signal& signal, const plugins::manifest::HostParams& host_params, const simulators::State& state) {
        const Money current_price = state.get_symbol_close(signal.symbol_);
        const Money equity = equity_calc::calculate_equity(state);

        const std::string sizing_method = host_params.position_sizing_method_.value_or("fixed_percentage");
        const double position_size_value = host_params.position_size_value_.value_or(constants::DEFAULT_POSITION_SIZE_VALUE);

        double quantity = 0;

        if (sizing_method == "fixed_percentage") {
            const Money dollar_amount = equity * position_size_value;
            quantity = dollar_amount.to_dollars() / current_price.to_dollars();
        }

        if (sizing_method == "fixed_dollar") {
            quantity = position_size_value / current_price.to_dollars();
        }

        if (sizing_method == "equal_weight") {
            const size_t symbol_count = host_params.symbols_.size();
            if (symbol_count == 0) {
                return 0.0;
            }
            const Money dollar_per_symbol = equity / static_cast<double>(symbol_count);
            quantity = dollar_per_symbol.to_dollars() / current_price.to_dollars();
        }

        if (host_params.max_position_size_.has_value() && quantity > host_params.max_position_size_.value()) {
            quantity = host_params.max_position_size_.value();
        }

        return quantity;
    }

    std::optional<Money> calculate_signal_stop_loss_price(const models::Signal& signal, const plugins::manifest::HostParams& host_params,
                                                          const simulators::State& state) {
        if (!host_params.use_stop_loss_.value_or(false)) {
            return std::nullopt;
        }

        const Money current_price = state.get_symbol_close(signal.symbol_);

        if (host_params.stop_loss_pct_ == std::nullopt) {
            return std::nullopt;
        }

        if (signal.is_buy()) {
            return current_price * (1.0 - host_params.stop_loss_pct_.value());
        }

        if (signal.is_sell()) {
            return current_price * (1.0 + host_params.stop_loss_pct_.value());
        }

        return std::nullopt;
    }

    std::optional<Money> calculate_signal_take_profit_price(const models::Signal& signal, const plugins::manifest::HostParams& host_params,
                                                            const simulators::State& state) {
        if (!host_params.use_take_profit_.value_or(false)) {
            return std::nullopt;
        }

        const Money current_price = state.get_symbol_close(signal.symbol_);

        if (host_params.take_profit_pct_ == std::nullopt) {
            return std::nullopt;
        }

        if (signal.is_buy()) {
            return current_price * (1.0 + host_params.take_profit_pct_.value());
        }

        if (signal.is_sell()) {
            return current_price * (1.0 - host_params.take_profit_pct_.value());
        }

        return std::nullopt;
    }

    models::Position calculate_position(const models::Order& order, double fillable_quantity, Money fill_price, const simulators::State& state) {
        models::Position position = [&, state]() -> models::Position {
            const auto it = state.positions_.find(order.symbol_);
            if (it != state.positions_.end()) {
                return it->second;
            }

            return {order.symbol_, 0.0, Money(0)};
        }();

        const double old_quantity = position.quantity_;

        if (order.is_buy()) {
            const double new_quantity = old_quantity + fillable_quantity;

            if (old_quantity < 0 && new_quantity > 0) {
                position.average_price_ = fill_price;
            } else if (old_quantity >= 0) {
                position.average_price_ = ((position.average_price_ * old_quantity) + (fill_price * fillable_quantity)) / new_quantity;
            }

            position.quantity_ = new_quantity;
        } else if (order.is_sell()) {
            const double new_quantity = old_quantity - fillable_quantity;

            if (old_quantity > 0 && new_quantity < 0) {
                position.average_price_ = fill_price;
            } else if (old_quantity <= 0) {
                const double abs_old = std::abs(old_quantity);
                const double abs_new = std::abs(new_quantity);
                position.average_price_ = ((position.average_price_ * abs_old) + (fill_price * fillable_quantity)) / abs_new;
            }

            position.quantity_ = new_quantity;
        }

        if (std::abs(position.quantity_) < constants::EPSILON) {
            position.quantity_ = 0;
            position.average_price_ = Money(0);
        }

        return position;
    }

    std::vector<std::pair<std::string, double>> find_buy_fill_uuids_closed_by_sell(const models::Fill& sell_fill, const simulators::State& state) {
        std::vector<std::pair<std::string, double>> closed_fills;

        const auto pos_it = state.positions_.find(sell_fill.symbol_);
        if (pos_it == state.positions_.end() || pos_it->second.quantity_ <= 0) {
            return closed_fills;
        }

        const double closeable = std::min(sell_fill.quantity_, pos_it->second.quantity_);
        double remaining = closeable;

        for (const auto& existing_fill : state.fills_) {
            if (remaining <= constants::EPSILON) {
                break;
            }

            if (existing_fill.symbol_ == sell_fill.symbol_ && existing_fill.is_buy() &&
                state.active_buy_fills_.find(existing_fill.uuid_) != state.active_buy_fills_.end()) {
                const double available = state.active_buy_fills_.at(existing_fill.uuid_);
                const double to_close = std::min(available, remaining);
                closed_fills.emplace_back(existing_fill.uuid_, to_close);
                remaining -= to_close;
            }
        }

        return closed_fills;
    }

    std::vector<std::pair<std::string, double>> find_sell_fill_uuids_closed_by_buy(const models::Fill& buy_fill, const simulators::State& state) {
        std::vector<std::pair<std::string, double>> closed_fills;

        const auto pos_it = state.positions_.find(buy_fill.symbol_);
        if (pos_it == state.positions_.end() || pos_it->second.quantity_ >= 0) {
            return closed_fills;
        }

        const double closeable = std::min(buy_fill.quantity_, std::abs(pos_it->second.quantity_));
        double remaining = closeable;

        for (const auto& existing_fill : state.fills_) {
            if (remaining <= constants::EPSILON) {
                break;
            }

            if (existing_fill.symbol_ == buy_fill.symbol_ && existing_fill.is_sell() &&
                state.active_sell_fills_.find(existing_fill.uuid_) != state.active_sell_fills_.end()) {
                const double available = state.active_sell_fills_.at(existing_fill.uuid_);
                const double to_close = std::min(available, remaining);
                closed_fills.emplace_back(existing_fill.uuid_, to_close);
                remaining -= to_close;
            }
        }

        return closed_fills;
    }

}  // namespace simulators::position_calc
