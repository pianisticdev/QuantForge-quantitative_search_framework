#ifndef QUANT_SIMULATORS_BACK_TEST_EQUITY_CALCULATOR_HPP
#define QUANT_SIMULATORS_BACK_TEST_EQUITY_CALCULATOR_HPP

#pragma once

#include "../../plugins/manifest/manifest.hpp"
#include "../../utils/money_utils.hpp"
#include "./state.hpp"

using namespace money_utils;

namespace simulators::equity_calc {

    [[nodiscard]] Money calculate_equity(const simulators::State& state);
    [[nodiscard]] double calculate_return(const plugins::manifest::HostParams& host_params, Money equity);
    [[nodiscard]] double calculate_max_drawdown(const simulators::State& state, Money equity);
    [[nodiscard]] Money calculate_available_margin(const simulators::State& state);

}  // namespace simulators::equity_calc
#endif
