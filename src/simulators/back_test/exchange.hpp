#ifndef QUANT_SIMULATORS_BACK_TEST_EXCHANGE_HPP
#define QUANT_SIMULATORS_BACK_TEST_EXCHANGE_HPP

#pragma once

#include "../../plugins/manifest/manifest.hpp"
#include "../../utils/money_utils.hpp"
#include "./models.hpp"

using namespace money_utils;

namespace simulators::exchange {

    [[nodiscard]] bool is_within_market_hour_restrictions(int64_t timestamp_ns, const plugins::manifest::HostParams& host_params);
    [[nodiscard]] Money calculate_commision(const models::Fill& fill, const plugins::manifest::HostParams& host_params);

}  // namespace simulators::exchange

#endif
