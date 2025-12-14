#include "./position_calculator.hpp"

#include "./equity_calculator.hpp"

namespace simulators {
    double PositionCalculator::calculate_signal_position_size(const models::Signal& signal, const plugins::manifest::HostParams& host_params,
                                                              const models::BackTestState& state) {
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
                                                                              const models::BackTestState& state) {
        if (!host_params.use_stop_loss_.value_or(false)) {
            return std::nullopt;
        }

        Money current_price = state.current_prices_.at(signal.symbol_);

        if (host_params.stop_loss_pct_ == std::nullopt) {
            return std::nullopt;
        }

        if (signal.action_ == constants::BUY) {
            return current_price * (1.0 - host_params.stop_loss_pct_.value());
        }

        if (signal.action_ == constants::SELL) {
            return current_price * (1.0 + host_params.stop_loss_pct_.value());
        }

        return std::nullopt;
    }

    std::optional<Money> PositionCalculator::calculate_signal_take_profit_price(const models::Signal& signal, const plugins::manifest::HostParams& host_params,
                                                                                const models::BackTestState& state) {
        if (!host_params.use_take_profit_.value_or(false)) {
            return std::nullopt;
        }

        Money current_price = state.current_prices_.at(signal.symbol_);

        if (host_params.take_profit_pct_ == std::nullopt) {
            return std::nullopt;
        }

        if (signal.action_ == constants::BUY) {
            return current_price * (1.0 + host_params.take_profit_pct_.value());
        }

        if (signal.action_ == constants::SELL) {
            return current_price * (1.0 - host_params.take_profit_pct_.value());
        }

        return std::nullopt;
    }

}  // namespace simulators
