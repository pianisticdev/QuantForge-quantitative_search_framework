#include "./state.hpp"

#include "../plugins/manifest/manifest.hpp"
#include "./equity_calculator.hpp"
#include "./models.hpp"

namespace simulators {
    void State::update_state(const models::ExecutionResultSuccess& execution_result, const plugins::manifest::HostParams& host_params) {
        cash_ += execution_result.cash_delta_;

        positions_[execution_result.position_.symbol_] = execution_result.position_;

        fills_.emplace_back(execution_result.fill_);
        current_timestamp_ns_ = execution_result.fill_.created_at_ns_;
        current_prices_[execution_result.fill_.symbol_] = execution_result.fill_.price_;

        auto equity = EquityCalculator::calculate_equity(*this);

        // We need a configurable rolling window, and a configurable risk free rate (0.02 default)
        equity_curve_.emplace_back(models::EquitySnapshot{
            .timestamp_ns_ = execution_result.fill_.created_at_ns_,
            .equity_ = equity,
            .return_ = EquityCalculator::calculate_return(host_params, equity),
            .max_drawdown_ = EquityCalculator::calculate_max_drawdown(*this, equity),
            .sharpe_ratio_ = 0,
            .sharpe_ratio_rolling_ = 0,
            .sortino_ratio_ = 0,
            .sortino_ratio_rolling_ = 0,
            .calmar_ratio_ = 0,
            .calmar_ratio_rolling_ = 0,
            .tail_ratio_ = 0,
            .tail_ratio_rolling_ = 0,
            .value_at_risk_ = 0,
            .value_at_risk_rolling_ = 0,
            .conditional_value_at_risk_ = 0,
            .conditional_value_at_risk_rolling_ = 0,
        });
    }
}  // namespace simulators
