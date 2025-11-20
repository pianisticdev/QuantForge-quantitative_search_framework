#ifndef QUANT_FORGE_MODELS_MODELS_HPP
#define QUANT_FORGE_MODELS_MODELS_HPP

#pragma once

#include <map>
#include <string>
#include <vector>

#include "../plugins/abi/abi.h"

namespace models {
    struct Instruction {
        std::string symbol_;
        std::string action_;
        double quantity_;
        std::optional<std::string> order_type_;
        std::optional<int64_t> limit_price_;
        std::optional<int64_t> stop_loss_price_;

        explicit Instruction(const CInstruction& inst)
            : symbol_(inst.symbol_ != nullptr ? inst.symbol_ : ""),
              action_(inst.action_ != nullptr ? inst.action_ : ""),
              quantity_(inst.quantity_),
              order_type_(inst.order_type_ != nullptr ? inst.order_type_ : ""),
              limit_price_(inst.limit_price_),
              stop_loss_price_(inst.stop_loss_price_) {}
        Instruction(std::string symbol, double quantity, std::string action) : symbol_(std::move(symbol)), action_(std::move(action)), quantity_(quantity) {}

        [[nodiscard]] static std::vector<Instruction> to_instructions(const PluginResult& result) {
            std::vector<Instruction> vec;
            vec.reserve(result.instructions_count_);
            for (size_t i = 0; i < result.instructions_count_; ++i) {
                vec.emplace_back(result.instructions_[i]);
            }
            return vec;
        }

        [[nodiscard]] bool is_signal() const { return !this->order_type_.has_value(); }
        [[nodiscard]] bool is_order() const { return this->order_type_.has_value(); }
        [[nodiscard]] bool is_limit_order() const { return this->order_type_.has_value() && this->order_type_.value() == "limit"; }
        [[nodiscard]] bool is_stop_loss_order() const { return this->order_type_.has_value() && this->order_type_.value() == "stop_loss"; }
        [[nodiscard]] bool is_buy() const { return this->action_ == "buy"; }
        [[nodiscard]] bool is_sell() const { return this->action_ == "sell"; }
    };

    struct Position {
        std::string symbol_;
        double quantity_;
        double average_price_;

        [[nodiscard]] static std::vector<CPosition> to_c_positions(const std::map<std::string, Position>& positions) {
            std::vector<CPosition> c_positions;
            c_positions.reserve(positions.size());
            for (const auto& [symbol, position] : positions) {
                c_positions.emplace_back(CPosition{
                    .symbol_ = symbol.c_str(),
                    .quantity_ = position.quantity_,
                    .average_price_ = position.average_price_,
                });
            }
            return c_positions;
        }

        [[nodiscard]] Instruction to_sell_instruction() const { return {this->symbol_, this->quantity_, "SELL"}; }
    };

    struct Trade {
        std::string symbol_;
        double quantity_;
        double price_;
        int64_t timestamp_ns_;

        [[nodiscard]] static std::vector<CTrade> to_c_trades(const std::vector<Trade>& trades) {
            std::vector<CTrade> c_trades;
            c_trades.reserve(trades.size());
            for (const auto& trade : trades) {
                c_trades.emplace_back(CTrade{
                    .symbol_ = trade.symbol_.c_str(),
                    .quantity_ = trade.quantity_,
                    .price_ = trade.price_,
                    .timestamp_ns_ = trade.timestamp_ns_,
                });
            }
            return c_trades;
        }
    };

    struct EquitySnapshot {
        int64_t timestamp_ns_;
        double equity_;
        double return_;
        double max_drawdown_;
        double sharpe_ratio_;
        double sortino_ratio_;
        double calmar_ratio_;
        double tail_ratio_;
        double value_at_risk_;
        double conditional_value_at_risk_;

        [[nodiscard]] static std::vector<CEquitySnapshot> to_c_equity_snapshots(const std::vector<EquitySnapshot>& equity_snapshots) {
            std::vector<CEquitySnapshot> c_equity_snapshots;
            c_equity_snapshots.reserve(equity_snapshots.size());
            for (const auto& equity_snapshot : equity_snapshots) {
                c_equity_snapshots.emplace_back(CEquitySnapshot{
                    .timestamp_ns_ = equity_snapshot.timestamp_ns_,
                    .equity_ = equity_snapshot.equity_,
                    .return_ = equity_snapshot.return_,
                    .max_drawdown_ = equity_snapshot.max_drawdown_,
                    .sharpe_ratio_ = equity_snapshot.sharpe_ratio_,
                    .sortino_ratio_ = equity_snapshot.sortino_ratio_,
                    .calmar_ratio_ = equity_snapshot.calmar_ratio_,
                    .tail_ratio_ = equity_snapshot.tail_ratio_,
                    .value_at_risk_ = equity_snapshot.value_at_risk_,
                    .conditional_value_at_risk_ = equity_snapshot.conditional_value_at_risk_,
                });
            }
            return c_equity_snapshots;
        }
    };

    struct BackTestState {
        int64_t cash_;
        std::map<std::string, Position> positions_;
        std::map<std::string, int64_t> current_prices_;
        int64_t current_timestamp_ns_;
        std::vector<Trade> trade_history_;
        std::vector<EquitySnapshot> equity_curve_;

        mutable std::vector<CPosition> c_positions_cache_;
        mutable std::vector<CTrade> c_trades_cache_;
        mutable std::vector<CEquitySnapshot> c_equity_cache_;

        [[nodiscard]] static CState to_c_state(BackTestState& state) {
            state.c_positions_cache_ = Position::to_c_positions(state.positions_);
            state.c_trades_cache_ = Trade::to_c_trades(state.trade_history_);
            state.c_equity_cache_ = EquitySnapshot::to_c_equity_snapshots(state.equity_curve_);

            return CState{.cash_ = state.cash_,
                          .positions_ = state.c_positions_cache_.data(),
                          .positions_count_ = state.c_positions_cache_.size(),
                          .trade_history_ = state.c_trades_cache_.data(),
                          .trade_history_count_ = state.c_trades_cache_.size(),
                          .equity_curve_ = state.c_equity_cache_.data(),
                          .equity_curve_count_ = state.c_equity_cache_.size()};
        }
    };
}  // namespace models

#endif
