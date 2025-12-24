#include "./slippage_calculator.hpp"

#include "../../utils/constants.hpp"
#include "../plugins/manifest/manifest.hpp"
#include "./models.hpp"
#include "./state.hpp"

namespace simulators {

    int64_t SlippageCalculator::calculate_slippage_time_ns(const models::Order& order, const plugins::manifest::HostParams& host_params,
                                                           const simulators::State& state) {
        if (!host_params.slippage_model_.has_value() || host_params.slippage_model_.value() == "none") {
            return state.current_timestamp_ns_;
        }

        if (host_params.slippage_model_.value() == "time_based") {
            int64_t base_delay_ns = static_cast<int64_t>(host_params.slippage_.value_or(0.0) * constants::NANOSECONDS_PER_MILLISECOND);
            return state.current_timestamp_ns_ + base_delay_ns;
        }

        if (host_params.slippage_model_.value() == "time_volume_based") {
            const auto volume = static_cast<double>(state.current_volumes_.at(order.symbol_));

            const double size_ratio = order.quantity_ / volume;

            const double delay_seconds = host_params.slippage_.value_or(1.0) * size_ratio;
            auto delay_ns = static_cast<int64_t>(delay_seconds * constants::NANOSECONDS_PER_SECOND);
            return state.current_timestamp_ns_ + delay_ns;
        }

        return state.current_timestamp_ns_;
    }

}  // namespace simulators
