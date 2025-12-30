#ifndef QUANT_SIMULATORS_BACK_TEST_POSITION_CALCULATOR_HPP
#define QUANT_SIMULATORS_BACK_TEST_POSITION_CALCULATOR_HPP

#pragma once

#include <optional>

#include "../../plugins/manifest/manifest.hpp"
#include "../../utils/money_utils.hpp"
#include "./models.hpp"
#include "./state.hpp"

using namespace money_utils;

namespace simulators::position_calc {

    [[nodiscard]] double calculate_signal_position_size(const models::Signal& signal, const plugins::manifest::HostParams& host_params,
                                                        const simulators::State& state);
    [[nodiscard]] std::optional<Money> calculate_signal_stop_loss_price(const models::Signal& signal, const plugins::manifest::HostParams& host_params,
                                                                        const simulators::State& state);
    [[nodiscard]] std::optional<Money> calculate_signal_take_profit_price(const models::Signal& signal, const plugins::manifest::HostParams& host_params,
                                                                          const simulators::State& state);

    [[nodiscard]] models::Position calculate_position(const models::Order& order, double fillable_quantity, Money fill_price, const simulators::State& state);

    [[nodiscard]] std::vector<std::pair<std::string, double>> find_buy_fill_uuids_closed_by_sell(const models::Fill& sell_fill, const simulators::State& state);
    [[nodiscard]] std::vector<std::pair<std::string, double>> find_sell_fill_uuids_closed_by_buy(const models::Fill& buy_fill, const simulators::State& state);

}  // namespace simulators::position_calc

#endif
