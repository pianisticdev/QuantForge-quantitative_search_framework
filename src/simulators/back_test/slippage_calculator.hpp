#ifndef QUANT_SIMULATORS_BACK_TEST_SLIPPAGE_CALCULATOR_HPP
#define QUANT_SIMULATORS_BACK_TEST_SLIPPAGE_CALCULATOR_HPP

#pragma once

#include "../plugins/manifest/manifest.hpp"
#include "./models.hpp"

namespace simulators {

    class SlippageCalculator {
       public:
        [[nodiscard]] static int64_t calculate_slippage_time_ns(const models::Order& order, const plugins::manifest::HostParams& host_params,
                                                                const models::BackTestState& state);
    };

}  // namespace simulators

#endif
