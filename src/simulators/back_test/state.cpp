#include "./state.hpp"

#include "../../utils/constants.hpp"
#include "../plugins/manifest/manifest.hpp"
#include "./equity_calculator.hpp"
#include "./models.hpp"

namespace simulators {
    void State::prepare_initial_state(const plugins::manifest::HostParams& host_params) {
        cash_ = Money(host_params.initial_capital_);
        peak_equity_ = Money(host_params.initial_capital_);
        max_drawdown_ = 0.0;
    }

    std::optional<std::pair<std::string, double>> State::populate_active_fills(const models::Fill& fill, double current_qty, bool comparison) {
        if (!comparison) {
            return std::make_optional(std::pair<std::string, double>{fill.uuid_, fill.quantity_});
        }

        double diff_qty = std::min(fill.quantity_, std::abs(current_qty));
        if (diff_qty > 0) {
            if (fill.is_buy()) {
                reduce_active_sell_fills_fifo(fill.symbol_, diff_qty);
            } else {
                reduce_active_buy_fills_fifo(fill.symbol_, diff_qty);
            }
        }

        double open_qty = fill.quantity_ - diff_qty;
        if (open_qty > constants::EPSILON) {
            return std::make_optional(std::pair<std::string, double>{fill.uuid_, open_qty});
        }

        return std::nullopt;
    }

    void State::update_state(const models::ExecutionResultSuccess& execution_result, const plugins::manifest::HostParams& host_params) {
        cash_ += execution_result.cash_delta_;

        fills_.emplace_back(execution_result.fill_);
        new_fills_.emplace_back(execution_result.fill_);

        auto pos_it = positions_.find(execution_result.fill_.symbol_);
        double current_qty = (pos_it != positions_.end()) ? pos_it->second.quantity_ : 0.0;
        auto comparison = execution_result.fill_.is_buy() ? current_qty < 0 : current_qty > 0;
        auto active_fills_optional = populate_active_fills(execution_result.fill_, current_qty, comparison);
        if (active_fills_optional.has_value()) {
            auto [fill_uuid, quantity] = active_fills_optional.value();
            if (execution_result.fill_.is_buy()) {
                active_buy_fills_[fill_uuid] = quantity;
            } else if (execution_result.fill_.is_sell()) {
                active_sell_fills_[fill_uuid] = quantity;
            }
        }

        if (!execution_result.exit_orders_.empty()) {
            for (const auto& exit_order : execution_result.exit_orders_) {
                exit_orders_.emplace_back(exit_order);
                new_exit_orders_.emplace_back(exit_order);
            }
        }

        if (execution_result.position_.quantity_ == 0) {
            positions_.erase(execution_result.position_.symbol_);
        } else {
            positions_[execution_result.position_.symbol_] = execution_result.position_;
        }

        current_timestamp_ns_ = execution_result.fill_.created_at_ns_;
        current_prices_[execution_result.fill_.symbol_] = execution_result.fill_.price_;
    }

    void State::record_bar_equity_snapshot(const plugins::manifest::HostParams& host_params) {
        auto equity = EquityCalculator::calculate_equity(*this);

        if (equity > peak_equity_) {
            peak_equity_ = equity;
        }

        double current_drawdown = EquityCalculator::calculate_max_drawdown(*this, equity);
        if (current_drawdown > max_drawdown_) {
            max_drawdown_ = current_drawdown;
        }

        if (equity_curve_.empty() || equity_curve_.back().timestamp_ns_ != current_timestamp_ns_) {
            equity_curve_.emplace_back(models::EquitySnapshot{
                .timestamp_ns_ = current_timestamp_ns_,
                .equity_ = equity,
                .return_ = EquityCalculator::calculate_return(host_params, equity),
                .max_drawdown_ = max_drawdown_,
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
        } else {
            equity_curve_.back().equity_ = equity;
            equity_curve_.back().return_ = EquityCalculator::calculate_return(host_params, equity);
            equity_curve_.back().max_drawdown_ = max_drawdown_;
        }
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

    void State::reduce_active_buy_fills_fifo(const std::string& symbol, double quantity) {
        double remaining = quantity;

        for (auto& fill : fills_) {
            if (remaining <= constants::EPSILON) {
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
    }

    void State::reduce_active_sell_fills_fifo(const std::string& symbol, double quantity) {
        double remaining = quantity;

        for (auto& fill : fills_) {
            if (remaining <= constants::EPSILON) {
                break;
            }

            if (fill.symbol_ == symbol && fill.is_sell() && active_sell_fills_.find(fill.uuid_) != active_sell_fills_.end()) {
                double available = active_sell_fills_[fill.uuid_];
                double to_close = std::min(available, remaining);

                active_sell_fills_[fill.uuid_] -= to_close;

                if (active_sell_fills_[fill.uuid_] <= constants::EPSILON) {
                    active_sell_fills_.erase(fill.uuid_);
                }

                remaining -= to_close;
            }
        }
    }
}  // namespace simulators
