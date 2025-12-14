#ifndef QUANT_SIMULATORS_BACK_TEST_ABI_CONVERTER_HPP
#define QUANT_SIMULATORS_BACK_TEST_ABI_CONVERTER_HPP

#pragma once

#include "../plugins/abi/abi.h"
#include "./models.hpp"

namespace simulators {

    class ABIConverter {
       public:
        [[nodiscard]] CState transform(const models::BackTestState& state);
        static void iterate_c_instructions(const PluginResult& result, const std::function<void(const CInstruction&)>& callback);
        [[nodiscard]] static models::Instruction to_instruction(const CInstruction& c_instruction);

       private:
        std::unordered_set<std::string> visited_fills_;
        std::unordered_set<std::string> visited_exit_orders_;

        mutable std::vector<CPosition> c_positions_cache_;
        mutable std::vector<CFill> c_fills_cache_;
        mutable std::vector<CEquitySnapshot> c_equity_cache_;
        mutable std::vector<CExitOrder> c_exit_orders_cache_;

        [[nodiscard]] static std::vector<CFill> to_c_fills(const std::vector<models::Fill>& fills);
        [[nodiscard]] static std::vector<CPosition> to_c_positions(const std::map<std::string, models::Position>& positions);
        [[nodiscard]] static std::vector<CEquitySnapshot> to_c_equity_snapshots(const std::vector<models::EquitySnapshot>& equity_snapshots);
        [[nodiscard]] static std::vector<CExitOrder> to_c_exit_orders(const std::vector<models::ExitOrder>& exit_orders);
        [[nodiscard]] static CStopLossExitOrder to_c_stop_loss_exit_order(const models::StopLossExitOrder& stop_loss);
        [[nodiscard]] static CTakeProfitExitOrder to_c_take_profit_exit_order(const models::TakeProfitExitOrder& take_profit);
    };

}  // namespace simulators

#endif
