#ifndef QUANT_SIMULATORS_BACK_TEST_EXECUTOR_HPP
#define QUANT_SIMULATORS_BACK_TEST_EXECUTOR_HPP

#pragma once

#include "../../plugins/manifest/manifest.hpp"
#include "./models.hpp"
#include "./state.hpp"

namespace simulators::executor {

    struct ClosingMarginInfo {
        Money margin_to_release_;
        Money realized_pnl_;
    };

    [[nodiscard]] models::ExecutionResult execute_order(const models::Order& order, const plugins::manifest::HostParams& host_params,
                                                        const simulators::State& state);
    [[nodiscard]] models::Order signal_to_order(const models::Signal& signal, const plugins::manifest::HostParams& host_params, const simulators::State& state);
    [[nodiscard]] models::ExecutionResult execute_sell(const models::Order& order, const plugins::manifest::HostParams& host_params,
                                                       const simulators::State& state);
    [[nodiscard]] std::pair<double, double> get_fillable_and_remaining_quantities(const models::Order& order, const plugins::manifest::HostParams& host_params,
                                                                                  const simulators::State& state);

    [[nodiscard]] std::vector<models::ExitOrder> create_exit_orders(const models::Order& order, const models::Fill& fill, const simulators::State& state,
                                                                    double position_opening_quantity, double new_position);
    [[nodiscard]] Money calculate_cash_delta(const models::Order& order, Money fill_price, double fillable_quantity, Money commission,
                                             double position_opening_quantity, Money margin_required, const simulators::State& state);

    [[nodiscard]] Money calculate_fill_price(const models::Order& order, const simulators::State& state);

    [[nodiscard]] std::optional<std::string> validate_margin(const models::Order& order, Money fill_price, Money commission,
                                                             const plugins::manifest::HostParams& host_params, const simulators::State& state,
                                                             double position_opening_quantity, double new_position_quantity, Money margin_required,
                                                             double fillable_quantity);

    [[nodiscard]] double calculate_position_opening_quantity(const models::Order& order, double fillable_quantity, double current_position_quantity,
                                                             double new_position_quantity);

    [[nodiscard]] Money calculate_margin_required(const plugins::manifest::HostParams& host_params, Money fill_price, double position_opening_quantity,
                                                  double leverage);

    [[nodiscard]] ClosingMarginInfo calculate_closing_margin_info(const models::Order& order, Money fill_price, double position_closing_quantity,
                                                                  const simulators::State& state);

}  // namespace simulators::executor

#endif
