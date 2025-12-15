#ifndef QUANT_FORGE_MODELS_MODELS_HPP
#define QUANT_FORGE_MODELS_MODELS_HPP

#pragma once

#include <chrono>
#include <map>
#include <string>
#include <unordered_set>
#include <vector>

#include "../../utils/money_utils.hpp"
#include "../../utils/string_utils.hpp"
#include "../plugins/abi/abi.h"

using namespace money_utils;

namespace models {
    constexpr int64_t NULL_MARKET_TRIGGER_PRICE = INT64_MIN;

    struct Signal {
        int64_t created_at_ns_ = std::chrono::system_clock::now().time_since_epoch().count();
        std::string symbol_;
        std::string action_;

        [[nodiscard]] bool is_buy() const { return this->action_ == constants::BUY; }
        [[nodiscard]] bool is_sell() const { return this->action_ == constants::SELL; }

        explicit Signal(const CSignal& inst) : symbol_(inst.symbol_), action_(inst.action_) {}
        Signal(std::string symbol, std::string action) : symbol_(std::move(symbol)), action_(std::move(action)) {}
    };

    struct Order {
        double quantity_;
        int64_t created_at_ns_ = std::chrono::system_clock::now().time_since_epoch().count();
        std::string symbol_;
        std::string action_;
        std::string order_type_;
        std::optional<Money> limit_price_;
        std::optional<Money> stop_loss_price_;
        std::optional<Money> take_profit_price_;

        [[nodiscard]] bool is_buy() const { return this->action_ == constants::BUY; }
        [[nodiscard]] bool is_sell() const { return this->action_ == constants::SELL; }

        explicit Order(const COrder& inst)
            : quantity_(inst.quantity_),
              symbol_(inst.symbol_),
              action_(inst.action_),
              order_type_(inst.order_type_),
              limit_price_(Money(inst.limit_price_)),
              stop_loss_price_(Money(inst.stop_loss_price_)),
              take_profit_price_(Money(inst.take_profit_price_)) {}
        // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
        Order(double quantity, int64_t created_at_ns, std::string symbol, std::string action, std::string order_type, std::optional<Money> limit_price,
              std::optional<Money> stop_loss_price, std::optional<Money> take_profit_price)
            : quantity_(quantity),
              created_at_ns_(created_at_ns),
              symbol_(std::move(symbol)),
              action_(std::move(action)),
              order_type_(std::move(order_type)),
              limit_price_(limit_price),
              stop_loss_price_(stop_loss_price),
              take_profit_price_(take_profit_price) {}
    };

    using Instruction = std::variant<Signal, Order>;

    struct Fill {
        double quantity_;
        int64_t created_at_ns_;
        Money price_;
        std::string symbol_;
        std::string uuid_;
        std::string action_;

        // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
        Fill(std::string symbol, std::string action, double quantity, Money price, int64_t created_at_ns)
            : quantity_(quantity),
              created_at_ns_(created_at_ns),
              price_(price),
              symbol_(std::move(symbol)),
              uuid_(string_utils::create_uuid()),
              action_(std::move(action)) {}
    };

    struct StopLossExitOrder {
        bool is_triggered_ = false;
        double trigger_quantity_;
        Money stop_loss_price_;
        Money price_;
        int64_t created_at_ns_;
        std::string symbol_;
        std::string fill_uuid_;

        bool operator<(const StopLossExitOrder& other) const { return stop_loss_price_ < other.stop_loss_price_; }

        [[nodiscard]] Order to_sell_instruction() const {
            return {trigger_quantity_, created_at_ns_, symbol_, constants::SELL, "market", std::nullopt, std::nullopt, std::nullopt};
        }

        // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
        StopLossExitOrder(std::string symbol, double quantity, Money stop_loss_price, Money price, int64_t created_at_ns)
            : trigger_quantity_(quantity),
              stop_loss_price_(stop_loss_price.to_abi_int64()),
              price_(price.to_abi_int64()),
              created_at_ns_(created_at_ns),
              symbol_(std::move(symbol)),
              fill_uuid_(string_utils::create_uuid()) {}
    };

    struct TakeProfitExitOrder {
        bool is_triggered_ = false;
        double trigger_quantity_;
        Money take_profit_price_;
        Money price_;
        int64_t created_at_ns_;
        std::string symbol_;
        std::string fill_uuid_;

        bool operator<(const TakeProfitExitOrder& other) const { return take_profit_price_ > other.take_profit_price_; }

        [[nodiscard]] Order to_sell_instruction() const {
            return {trigger_quantity_, created_at_ns_, symbol_, constants::SELL, "market", std::nullopt, std::nullopt, std::nullopt};
        }

        // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
        TakeProfitExitOrder(std::string symbol, double quantity, Money take_profit_price, Money price, int64_t created_at_ns)
            : trigger_quantity_(quantity),
              take_profit_price_(take_profit_price.to_abi_int64()),
              price_(price.to_abi_int64()),
              created_at_ns_(created_at_ns),
              symbol_(std::move(symbol)),
              fill_uuid_(string_utils::create_uuid()) {}
    };

    using ExitOrder = std::variant<StopLossExitOrder, TakeProfitExitOrder>;

    struct Position {
        std::string symbol_;
        double quantity_;
        Money average_price_;
    };

    // We calculate the rolling windows once per day
    // We need to track peak equity in memory
    // We should track average return across rolling windows in memory
    struct EquitySnapshot {
        int64_t timestamp_ns_;
        Money equity_;
        double return_;
        double max_drawdown_;
        double sharpe_ratio_;
        double sharpe_ratio_rolling_;
        double sortino_ratio_;
        double sortino_ratio_rolling_;
        double calmar_ratio_;
        double calmar_ratio_rolling_;
        double tail_ratio_;
        double tail_ratio_rolling_;
        double value_at_risk_;
        double value_at_risk_rolling_;
        double conditional_value_at_risk_;
        double conditional_value_at_risk_rolling_;
    };

    struct ExecutionResultSuccess {
        std::string message_;
        Money cash_delta_;
        models::Position position_;
        models::Fill fill_;
        std::optional<models::ExitOrder> exit_order_;
        std::optional<models::Order> partial_order_;

        // Success Case Constructor
        ExecutionResultSuccess(Money cash_delta, std::optional<models::Order> partial_order, models::Position position, models::Fill fill,
                               std::optional<models::ExitOrder> exit_order)
            : message_(partial_order.has_value() ? "Partial fill" : "Complete fill"),
              cash_delta_(cash_delta),
              position_(std::move(position)),
              fill_(std::move(fill)),
              exit_order_(std::move(exit_order)),
              partial_order_(std::move(partial_order)) {}

        [[nodiscard]] bool is_partial_fill() const { return partial_order_.has_value(); }
        [[nodiscard]] bool has_exit_strategy() const { return exit_order_.has_value(); }
    };

    struct ExecutionResultError {
        std::string message_;

        ExecutionResultError(std::string message) : message_(std::move(message)) {}
    };

    using ExecutionResult = std::variant<ExecutionResultSuccess, ExecutionResultError>;

    struct ScheduledOrder {
        Order order_;
        int64_t scheduled_fill_at_ns_ = 0;

        ScheduledOrder(Order order, int64_t scheduled_fill_at_ns) : order_(std::move(order)), scheduled_fill_at_ns_(scheduled_fill_at_ns) {}

        bool operator<(const ScheduledOrder& other) const { return scheduled_fill_at_ns_ < other.scheduled_fill_at_ns_; }
    };
}  // namespace models

#endif
