#include "./equity_calculator.hpp"

namespace simulators {
    Money EquityCalculator::calculate_equity(const models::BackTestState& state) {
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

    double EquityCalculator::calculate_max_drawdown(const models::BackTestState& state, Money equity) {
        Money peak_equity = equity;
        for (const auto& equity_snapshot : state.equity_curve_) {
            if (equity_snapshot.equity_ > peak_equity) {
                peak_equity = equity_snapshot.equity_;
            }
        }
        return (peak_equity - equity).to_dollars() / peak_equity.to_dollars();
    }
}  // namespace simulators
