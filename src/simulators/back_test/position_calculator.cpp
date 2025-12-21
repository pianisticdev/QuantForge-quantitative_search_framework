#include "./position_calculator.hpp"

#include "./equity_calculator.hpp"
#include "./state.hpp"

namespace simulators {
    double PositionCalculator::calculate_signal_position_size(const models::Signal& signal, const plugins::manifest::HostParams& host_params,
                                                              const simulators::State& state) {
        Money current_price = state.current_prices_.at(signal.symbol_);
        Money equity = EquityCalculator::calculate_equity(state);

        std::string sizing_method = host_params.position_sizing_method_.value_or("fixed_percentage");
        double position_size_value = host_params.position_size_value_.value_or(constants::DEFAULT_POSITION_SIZE_VALUE);

        double quantity = 0;

        if (sizing_method == "fixed_percentage") {
            Money dollar_amount = equity * position_size_value;
            quantity = dollar_amount.to_dollars() / current_price.to_dollars();
        }

        if (sizing_method == "fixed_dollar") {
            quantity = position_size_value / current_price.to_dollars();
        }

        if (sizing_method == "equal_weight") {
            size_t symbol_count = host_params.symbols_.size();
            Money dollar_per_symbol = equity / static_cast<double>(symbol_count);
            quantity = dollar_per_symbol.to_dollars() / current_price.to_dollars();
        }

        if (host_params.max_position_size_.has_value() && quantity > host_params.max_position_size_.value()) {
            quantity = host_params.max_position_size_.value();
        }

        return quantity;
    }

    std::optional<Money> PositionCalculator::calculate_signal_stop_loss_price(const models::Signal& signal, const plugins::manifest::HostParams& host_params,
                                                                              const simulators::State& state) {
        if (!host_params.use_stop_loss_.value_or(false)) {
            return std::nullopt;
        }

        Money current_price = state.current_prices_.at(signal.symbol_);

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

    std::optional<Money> PositionCalculator::calculate_signal_take_profit_price(const models::Signal& signal, const plugins::manifest::HostParams& host_params,
                                                                                const simulators::State& state) {
        if (!host_params.use_take_profit_.value_or(false)) {
            return std::nullopt;
        }

        Money current_price = state.current_prices_.at(signal.symbol_);

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

    models::Position PositionCalculator::calculate_position(const models::Order& order, double fillable_quantity, const simulators::State& state) {
        auto position = [&, state]() -> models::Position {
            auto it = state.positions_.find(order.symbol_);
            if (it != state.positions_.end()) {
                return it->second;
            }

            return {order.symbol_, 0.0, Money(0)};
        }();

        double old_quantity = position.quantity_;
        Money fill_price = state.current_prices_.at(order.symbol_);

        if (order.is_buy()) {
            position.quantity_ += fillable_quantity;
            position.average_price_ = ((position.average_price_ * old_quantity) + (fill_price * fillable_quantity)) / position.quantity_;
        } else if (order.is_sell()) {
            position.quantity_ -= fillable_quantity;
        }

        if (position.quantity_ == 0) {
            position.average_price_ = Money(0);
        }

        return position;
    }

    std::vector<std::pair<std::string, double>> PositionCalculator::find_buy_fill_uuids_closed_by_sell(const models::Fill& sell_fill,
                                                                                                       const simulators::State& state) {
        std::vector<std::pair<std::string, double>> closed_fills;

        double remaining = sell_fill.quantity_;
        for (const auto& existing_fill : state.fills_) {
            if (remaining <= 0) {
                break;
            }

            if (existing_fill.symbol_ == sell_fill.symbol_ && existing_fill.is_buy() &&
                state.active_buy_fills_.find(existing_fill.uuid_) != state.active_buy_fills_.end()) {
                double available = state.active_buy_fills_.at(existing_fill.uuid_);
                double to_close = std::min(available, remaining);
                closed_fills.emplace_back(existing_fill.uuid_, to_close);
                remaining -= to_close;
            }
        }

        return closed_fills;
    }

}  // namespace simulators
