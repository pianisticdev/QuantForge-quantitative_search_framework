#include "./equity_calculator.hpp"

#include <numeric>

#include "./state.hpp"

namespace simulators {
    Money EquityCalculator::calculate_equity(const simulators::State& state) {
        Money unrealized_pnl(0);

        for (const auto& [symbol, position] : state.positions_) {
            if (!state.has_symbol_prices(symbol)) {
                continue;
            }

            const Money current_price = state.get_symbol_close(symbol);
            const Money position_pnl = (current_price - position.average_price_) * position.quantity_;
            unrealized_pnl += position_pnl;
        }

        return state.cash_ + state.margin_in_use_ + unrealized_pnl;
    }

    double EquityCalculator::calculate_return(const plugins::manifest::HostParams& host_params, Money equity) {
        const Money initial_capital_money = Money(host_params.initial_capital_);
        const Money net_return = equity - initial_capital_money;
        return net_return.to_dollars() / initial_capital_money.to_dollars();
    }

    double EquityCalculator::calculate_max_drawdown(const simulators::State& state, Money current_equity) {
        double current_drawdown = 0.0;

        if (state.peak_equity_.to_dollars() > constants::EPSILON) {
            current_drawdown = (state.peak_equity_ - current_equity).to_dollars() / state.peak_equity_.to_dollars();
        }

        return std::max(current_drawdown, state.max_drawdown_);
    }

    Money EquityCalculator::calculate_available_margin(const simulators::State& state) { return calculate_equity(state) - state.margin_in_use_; }
}  // namespace simulators
