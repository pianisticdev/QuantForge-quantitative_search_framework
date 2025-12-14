#include "./exchange.hpp"

#include "../../utils/time_utils.hpp"

namespace simulators {

    bool Exchange::is_within_market_hour_restrictions(int64_t timestamp_ns, const plugins::manifest::HostParams& host_params) {
        if (host_params.market_hours_only_ != true) {
            return true;
        }

        return time_utils::is_within_market_hours(timestamp_ns);
    }

    Money Exchange::calculate_commision(const models::Fill& fill, const plugins::manifest::HostParams& host_params) {
        const std::string commission_type = host_params.commission_type_.value_or(std::string(""));
        const double commission_value = host_params.commission_.value_or(0.0);

        if (commission_type.empty() || commission_value == 0.0) {
            return Money(0);
        }

        if (commission_type == "per_share") {
            return Money::from_dollars(commission_value) * fill.quantity_;
        }

        if (commission_type == "percentage") {
            Money trade_value = fill.price_ * fill.quantity_;
            return trade_value * commission_value;
        }

        if (commission_type == "flat") {
            return Money::from_dollars(commission_value);
        }

        return Money(0);
    }

}  // namespace simulators
