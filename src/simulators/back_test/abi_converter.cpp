
#include "./abi_converter.hpp"

#include "./models.hpp"
#include "./state.hpp"

namespace simulators {
    CState ABIConverter::transform(const simulators::State& state) {
        c_positions_cache_ = to_c_positions(state.positions_);
        c_fills_cache_ = to_c_fills(state.new_fills_);
        c_equity_cache_ = to_c_equity_snapshots(state.equity_curve_);
        c_exit_orders_cache_ = to_c_exit_orders(state.new_exit_orders_);

        return CState{.cash_ = state.cash_.to_abi_int64(),
                      .positions_ = c_positions_cache_.data(),
                      .positions_count_ = c_positions_cache_.size(),
                      .new_exit_orders_ = c_exit_orders_cache_.data(),
                      .new_exit_orders_count_ = c_exit_orders_cache_.size(),
                      .new_fills_ = c_fills_cache_.data(),
                      .new_fills_count_ = c_fills_cache_.size(),
                      .equity_curve_ = c_equity_cache_.data(),
                      .equity_curve_count_ = c_equity_cache_.size()};
    }

    std::vector<CFill> ABIConverter::to_c_fills(const std::vector<models::Fill>& fills) {
        std::vector<CFill> c_fills;
        c_fills.reserve(fills.size());
        for (const auto& fill : fills) {
            c_fills.emplace_back(CFill{
                .symbol_ = fill.symbol_.c_str(),
                .quantity_ = fill.quantity_,
                .price_ = fill.price_.to_abi_int64(),
                .created_at_ns_ = fill.created_at_ns_,
                .uuid_ = fill.uuid_.c_str(),
                .action_ = fill.action_.c_str(),
            });
        }
        return c_fills;
    }

    std::vector<CPosition> ABIConverter::to_c_positions(const std::map<std::string, models::Position>& positions) {
        std::vector<CPosition> c_positions;
        c_positions.reserve(positions.size());
        for (const auto& [symbol, position] : positions) {
            c_positions.emplace_back(CPosition{
                .symbol_ = symbol.c_str(),
                .quantity_ = position.quantity_,
                .average_price_ = position.average_price_.to_abi_double(),
            });
        }
        return c_positions;
    }

    std::vector<CEquitySnapshot> ABIConverter::to_c_equity_snapshots(const std::vector<models::EquitySnapshot>& equity_snapshots) {
        std::vector<CEquitySnapshot> c_equity_snapshots;
        c_equity_snapshots.reserve(equity_snapshots.size());
        for (const auto& equity_snapshot : equity_snapshots) {
            c_equity_snapshots.emplace_back(CEquitySnapshot{
                .timestamp_ns_ = equity_snapshot.timestamp_ns_,
                .equity_ = equity_snapshot.equity_.to_abi_int64(),
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

    CStopLossExitOrder ABIConverter::to_c_stop_loss_exit_order(const models::StopLossExitOrder& stop_loss) {
        return CStopLossExitOrder{
            .is_triggered_ = stop_loss.is_triggered_,
            .symbol_ = stop_loss.symbol_.c_str(),
            .trigger_quantity_ = stop_loss.trigger_quantity_,
            .stop_loss_price_ = stop_loss.stop_loss_price_.to_abi_int64(),
            .price_ = stop_loss.price_.to_abi_int64(),
            .created_at_ns_ = stop_loss.created_at_ns_,
            .fill_uuid_ = stop_loss.fill_uuid_.c_str(),
        };
    }

    CTakeProfitExitOrder ABIConverter::to_c_take_profit_exit_order(const models::TakeProfitExitOrder& take_profit) {
        return CTakeProfitExitOrder{
            .is_triggered_ = take_profit.is_triggered_,
            .symbol_ = take_profit.symbol_.c_str(),
            .trigger_quantity_ = take_profit.trigger_quantity_,
            .take_profit_price_ = take_profit.take_profit_price_.to_abi_int64(),
            .price_ = take_profit.price_.to_abi_int64(),
            .created_at_ns_ = take_profit.created_at_ns_,
            .fill_uuid_ = take_profit.fill_uuid_.c_str(),
        };
    }

    std::vector<CExitOrder> ABIConverter::to_c_exit_orders(const std::vector<models::ExitOrder>& exit_orders) {
        std::vector<CExitOrder> c_exit_orders;
        c_exit_orders.reserve(exit_orders.size());
        for (const auto& exit_order : exit_orders) {
            switch (exit_order.index()) {
                case 0:
                    CExitOrder c_stop_loss_exit_order;
                    c_stop_loss_exit_order.type_ = EXIT_ORDER_STOP_LOSS;
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
                    c_stop_loss_exit_order.data_.stop_loss_ = to_c_stop_loss_exit_order(std::get<models::StopLossExitOrder>(exit_order));
                    c_exit_orders.emplace_back(c_stop_loss_exit_order);
                    break;
                case 1:
                    CExitOrder c_take_profit_exit_order;
                    c_take_profit_exit_order.type_ = EXIT_ORDER_TAKE_PROFIT;
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
                    c_take_profit_exit_order.data_.take_profit_ = to_c_take_profit_exit_order(std::get<models::TakeProfitExitOrder>(exit_order));
                    c_exit_orders.emplace_back(c_take_profit_exit_order);
                    break;
            }
        }
        return c_exit_orders;
    }

    void ABIConverter::iterate_c_instructions(const PluginResult& result, const std::function<void(const CInstruction&)>& callback) {
        for (size_t i = 0; i < result.instructions_count_; ++i) {
            const CInstruction& c_instruction = result.instructions_[i];
            callback(c_instruction);
        }
    }

    models::Instruction ABIConverter::to_instruction(const CInstruction& c_instruction) {
        if (c_instruction.type_ == INSTRUCTION_TYPE_SIGNAL) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
            return models::Signal(c_instruction.data_.signal_);
        }

        if (c_instruction.type_ == INSTRUCTION_TYPE_ORDER) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
            return models::Order(c_instruction.data_.order_);
        }

        throw std::runtime_error("Invalid instruction type");
    }
}  // namespace simulators
