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
        bool is_exit_order_ = false;
        double quantity_;
        int64_t created_at_ns_ = std::chrono::system_clock::now().time_since_epoch().count();
        std::string symbol_;
        std::string action_;
        std::string order_type_;
        std::optional<std::string> source_fill_uuid_;  // Used for exit orders
        std::optional<Money> limit_price_;
        std::optional<Money> stop_loss_price_;
        std::optional<Money> take_profit_price_;
        std::optional<double> leverage_;

        [[nodiscard]] bool is_buy() const { return this->action_ == constants::BUY; }
        [[nodiscard]] bool is_sell() const { return this->action_ == constants::SELL; }

        [[nodiscard]] bool is_limit_order() const { return order_type_ == constants::LIMIT && limit_price_.has_value(); }
        [[nodiscard]] bool is_market_order() const { return order_type_ == constants::MARKET || !limit_price_.has_value(); }

        explicit Order(const COrder& inst)
            : quantity_(inst.quantity_),
              symbol_(inst.symbol_),
              action_(inst.action_),
              order_type_(inst.order_type_),
              limit_price_(inst.limit_price_ == NULL_MARKET_TRIGGER_PRICE ? std::nullopt : std::make_optional(Money(inst.limit_price_))),
              stop_loss_price_(inst.stop_loss_price_ == NULL_MARKET_TRIGGER_PRICE ? std::nullopt : std::make_optional(Money(inst.stop_loss_price_))),
              take_profit_price_(inst.take_profit_price_ == NULL_MARKET_TRIGGER_PRICE ? std::nullopt : std::make_optional(Money(inst.take_profit_price_))),
              leverage_(inst.leverage_ <= 0 ? std::nullopt : std::make_optional(inst.leverage_)) {}
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
        double leverage_;
        int64_t created_at_ns_;
        Money price_;
        Money margin_used_;
        std::string symbol_;
        std::string uuid_;
        std::string action_;

        // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
        Fill(std::string symbol, std::string action, double quantity, Money price, int64_t created_at_ns, double leverage = 1.0, Money margin_used = Money(0))
            : quantity_(quantity),
              leverage_(leverage),
              created_at_ns_(created_at_ns),
              price_(price),
              margin_used_(margin_used),
              symbol_(std::move(symbol)),
              uuid_(string_utils::create_uuid()),
              action_(std::move(action)) {}

        [[nodiscard]] bool is_buy() const { return action_ == constants::BUY; }
        [[nodiscard]] bool is_sell() const { return action_ == constants::SELL; }
    };

    struct StopLossExitOrder {
        bool is_triggered_ = false;
        bool is_short_position_ = false;
        double trigger_quantity_;
        Money stop_loss_price_;
        Money price_;
        int64_t created_at_ns_;
        std::string symbol_;
        std::string fill_uuid_;

        bool operator<(const StopLossExitOrder& other) const { return stop_loss_price_ < other.stop_loss_price_; }

        [[nodiscard]] Order to_close_instruction() const {
            std::string action = is_short_position_ ? constants::BUY : constants::SELL;
            Order order{trigger_quantity_, created_at_ns_, symbol_, action, "market", std::nullopt, std::nullopt, std::nullopt};
            order.is_exit_order_ = true;
            order.source_fill_uuid_ = fill_uuid_;
            return order;
        }

        // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
        StopLossExitOrder(std::string symbol, double quantity, Money stop_loss_price, Money price, int64_t created_at_ns, std::string fill_uuid,
                          bool is_short_position)
            : is_short_position_(is_short_position),
              trigger_quantity_(quantity),
              stop_loss_price_(stop_loss_price.to_abi_int64()),
              price_(price.to_abi_int64()),
              created_at_ns_(created_at_ns),
              symbol_(std::move(symbol)),
              fill_uuid_(std::move(fill_uuid)) {}
    };

    struct TakeProfitExitOrder {
        bool is_triggered_ = false;
        bool is_short_position_ = false;
        double trigger_quantity_;
        Money take_profit_price_;
        Money price_;
        int64_t created_at_ns_;
        std::string symbol_;
        std::string fill_uuid_;

        bool operator<(const TakeProfitExitOrder& other) const { return take_profit_price_ > other.take_profit_price_; }

        [[nodiscard]] Order to_close_instruction() const {
            std::string action = is_short_position_ ? constants::BUY : constants::SELL;
            Order order{trigger_quantity_, created_at_ns_, symbol_, action, "market", std::nullopt, std::nullopt, std::nullopt};
            order.is_exit_order_ = true;
            order.source_fill_uuid_ = fill_uuid_;
            return order;
        }

        // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
        TakeProfitExitOrder(std::string symbol, double quantity, Money take_profit_price, Money price, int64_t created_at_ns, std::string fill_uuid,
                            bool is_short_position)
            : is_short_position_(is_short_position),
              trigger_quantity_(quantity),
              take_profit_price_(take_profit_price.to_abi_int64()),
              price_(price.to_abi_int64()),
              created_at_ns_(created_at_ns),
              symbol_(std::move(symbol)),
              fill_uuid_(std::move(fill_uuid)) {}
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
        Money margin_used_;
        double leverage_;
        models::Position position_;
        models::Fill fill_;
        std::vector<models::ExitOrder> exit_orders_;
        std::optional<models::Order> partial_order_;

        // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
        ExecutionResultSuccess(Money cash_delta, Money margin_used, double leverage, std::optional<models::Order> partial_order, models::Position position,
                               models::Fill fill, std::vector<models::ExitOrder> exit_orders)
            : message_(partial_order.has_value() ? "Partial fill" : "Complete fill"),
              cash_delta_(cash_delta),
              margin_used_(margin_used),
              leverage_(leverage),
              position_(std::move(position)),
              fill_(std::move(fill)),
              exit_orders_(std::move(exit_orders)),
              partial_order_(std::move(partial_order)) {}

        [[nodiscard]] bool is_partial_fill() const { return partial_order_.has_value(); }
        [[nodiscard]] bool has_exit_strategy() const { return !exit_orders_.empty(); }
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

    struct LimitBuyOrder {
        Order order_;
        Money limit_price_;

        LimitBuyOrder(Order order, Money limit_price) : order_(std::move(order)), limit_price_(limit_price) {}

        bool operator<(const LimitBuyOrder& other) const { return limit_price_ < other.limit_price_; }
    };

    struct LimitSellOrder {
        Order order_;
        Money limit_price_;

        LimitSellOrder(Order order, Money limit_price) : order_(std::move(order)), limit_price_(limit_price) {}

        bool operator<(const LimitSellOrder& other) const { return limit_price_ < other.limit_price_; }
    };

    using ScheduledLimitOrder = std::variant<LimitBuyOrder, LimitSellOrder>;
}  // namespace models

#endif
