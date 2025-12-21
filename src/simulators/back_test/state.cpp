#include "./state.hpp"

#include "../../utils/constants.hpp"
#include "../plugins/manifest/manifest.hpp"
#include "./equity_calculator.hpp"
#include "./models.hpp"

namespace simulators {
    void State::update_state(const models::ExecutionResultSuccess& execution_result, const plugins::manifest::HostParams& host_params) {
        cash_ += execution_result.cash_delta_;

        fills_.emplace_back(execution_result.fill_);
        new_fills_.emplace_back(execution_result.fill_);

        if (execution_result.fill_.is_buy()) {
            active_buy_fills_[execution_result.fill_.uuid_] = execution_result.fill_.quantity_;
        } else if (execution_result.fill_.is_sell()) {
            reduce_active_fills_fifo(execution_result.fill_.symbol_, execution_result.fill_.quantity_);
        }

        if (execution_result.exit_order_.has_value()) {
            exit_orders_.emplace_back(execution_result.exit_order_.value());
            new_exit_orders_.emplace_back(execution_result.exit_order_.value());
        }

        if (execution_result.position_.quantity_ == 0) {
            positions_.erase(execution_result.position_.symbol_);
        } else {
            positions_[execution_result.position_.symbol_] = execution_result.position_;
        }

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

    void State::clear_previous_bar_state() {
        new_fills_.clear();
        new_exit_orders_.clear();
    }

    void State::prepare_next_bar_state(const http::stock_api::AggregateBarResult& bar) {
        current_timestamp_ns_ = bar.unix_ts_ns_;
        current_prices_[bar.symbol_] = Money::from_dollars(bar.close_);
        current_volumes_[bar.symbol_] = static_cast<int64_t>(bar.volume_);
    }

    void State::reduce_active_fills_fifo(const std::string& symbol, double quantity_sold) {
        double remaining = quantity_sold;

        for (auto& fill : fills_) {
            if (remaining <= 0) {
                break;
            }

            if (fill.symbol_ == symbol && fill.is_buy() && active_buy_fills_.find(fill.uuid_) != active_buy_fills_.end()) {
                double available = active_buy_fills_[fill.uuid_];
                double to_close = std::min(available, remaining);

                active_buy_fills_[fill.uuid_] -= to_close;

                if (active_buy_fills_[fill.uuid_] <= constants::EPSILON) {
                    active_buy_fills_.erase(fill.uuid_);
                }

                remaining -= to_close;
            }
        }

        if (remaining > constants::EPSILON) {
            throw std::runtime_error("FIFO matching error: tried to sell more than available");
        }
    }
}  // namespace simulators
