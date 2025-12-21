#include "./equity_calculator.hpp"

#include "./state.hpp"

namespace simulators {
    Money EquityCalculator::calculate_equity(const simulators::State& state) {
        auto total_assets = Money(0);
        for (const auto& [_, position] : state.positions_) {
            total_assets += state.current_prices_.at(position.symbol_) * position.quantity_;
        }
        return total_assets + state.cash_;
    }

    double EquityCalculator::calculate_return(const plugins::manifest::HostParams& host_params, Money equity) {
        auto initial_capital_money = Money(host_params.initial_capital_);
        auto net_return = equity - initial_capital_money;
        return net_return.to_dollars() / initial_capital_money.to_dollars();
    }

    double EquityCalculator::calculate_max_drawdown(const simulators::State& state, Money current_equity) {
        double current_drawdown = 0.0;

        if (state.peak_equity_.to_dollars() > 0) {
            current_drawdown = (state.peak_equity_ - current_equity).to_dollars() / state.peak_equity_.to_dollars();
        }

        return std::max(current_drawdown, state.max_drawdown_);
    }
}  // namespace simulators
