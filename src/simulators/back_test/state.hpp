#ifndef QUANT_SIMULATORS_BACK_TEST_STATE_HPP
#define QUANT_SIMULATORS_BACK_TEST_STATE_HPP

#pragma once

#include "../plugins/manifest/manifest.hpp"
#include "./models.hpp"

namespace simulators {

    class State {
       public:
        Money cash_;
        int64_t current_timestamp_ns_;
        std::map<std::string, models::Position> positions_;
        std::map<std::string, Money> current_prices_;
        std::map<std::string, int64_t> current_volumes_;
        std::vector<models::Fill> fills_;
        std::vector<models::ExitOrder> exit_orders_;
        std::vector<models::EquitySnapshot> equity_curve_;

        void update_state(const models::ExecutionResultSuccess& execution_result, const plugins::manifest::HostParams& host_params);
    };

}  // namespace simulators

#endif
