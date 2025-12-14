#ifndef QUANT_SIMULATORS_BACK_TEST_POSITION_CALCULATOR_HPP
#define QUANT_SIMULATORS_BACK_TEST_POSITION_CALCULATOR_HPP

#pragma once

#include <optional>

#include "../../plugins/manifest/manifest.hpp"
#include "../../utils/money_utils.hpp"
#include "./models.hpp"

using namespace money_utils;

namespace simulators {

    class PositionCalculator {
       public:
        [[nodiscard]] static double calculate_signal_position_size(const models::Signal& signal, const plugins::manifest::HostParams& host_params,
                                                                   const models::BackTestState& state);
        [[nodiscard]] static std::optional<Money> calculate_signal_stop_loss_price(const models::Signal& signal,
                                                                                   const plugins::manifest::HostParams& host_params,
                                                                                   const models::BackTestState& state);
        [[nodiscard]] static std::optional<Money> calculate_signal_take_profit_price(const models::Signal& signal,
                                                                                     const plugins::manifest::HostParams& host_params,
                                                                                     const models::BackTestState& state);
    };

}  // namespace simulators

#endif
