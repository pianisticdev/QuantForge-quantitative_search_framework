#include "./slippage_calculator.hpp"

#include "../../utils/constants.hpp"
#include "../plugins/manifest/manifest.hpp"
#include "./models.hpp"

namespace simulators {

    int64_t SlippageCalculator::calculate_slippage_time_ns(const models::Order& order, const plugins::manifest::HostParams& host_params,
                                                           const models::BackTestState& state) {
        if (!host_params.slippage_model_.has_value() || host_params.slippage_model_.value() == "none") {
            return 0;
        }

        if (host_params.slippage_model_.value() == "time_based") {
            int64_t base_delay_ns = static_cast<int64_t>(host_params.slippage_.value_or(0.0) * constants::MONEY_SCALED_BASE);
            return base_delay_ns;
        }

        if (host_params.slippage_model_.value() == "time_volume_based") {
            const auto order_value = state.current_prices_.at(order.symbol_) * order.quantity_;
            const auto volume_value = state.current_volumes_.at(order.symbol_);
            const double size_ratio = static_cast<double>(volume_value) / order_value.to_dollars();

            const double delay_seconds = host_params.slippage_.value_or(1.0) * size_ratio;
            return static_cast<int64_t>(delay_seconds * constants::MONEY_SCALED_BASE);
        }

        return 0;
    }

}  // namespace simulators
