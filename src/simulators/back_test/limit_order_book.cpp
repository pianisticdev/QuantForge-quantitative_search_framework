#include "limit_order_book.hpp"

#include <ranges>
#include <string>

#include "./models.hpp"
#include "./state.hpp"

namespace simulators {

    void LimitOrderBook::add_limit_order(const models::ScheduledLimitOrder& order) {
        std::visit(
            [&](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, models::LimitBuyOrder>) {
                    buy_limits_[arg.order_.symbol_].push(arg);
                } else if constexpr (std::is_same_v<T, models::LimitSellOrder>) {
                    sell_limits_[arg.order_.symbol_].push(arg);
                }
            },
            order);
    }

    void LimitOrderBook::process_buy_limits(const simulators::State& state, const std::function<void(const models::Order&)>& callback) {
        for (auto& [symbol, heap] : buy_limits_) {
            auto price_it = state.current_prices_.find(symbol);
            if (price_it == state.current_prices_.end()) {
                continue;
            }

            Money current_price = price_it->second;

            while (!heap.empty()) {
                auto top_opt = heap.top();
                if (!top_opt.has_value()) {
                    break;
                }

                const auto& limit_order = top_opt.value();

                if (current_price > limit_order.limit_price_) {
                    break;
                }

                heap.pop();
                callback(limit_order.order_);
            }
        }
    }

    void LimitOrderBook::process_sell_limits(const simulators::State& state, const std::function<void(const models::Order&)>& callback) {
        for (auto& [symbol, heap] : sell_limits_) {
            auto price_it = state.current_prices_.find(symbol);
            if (price_it == state.current_prices_.end()) {
                continue;
            }

            Money current_price = price_it->second;

            while (!heap.empty()) {
                auto top_opt = heap.top();
                if (!top_opt.has_value()) {
                    break;
                }

                const auto& limit_order = top_opt.value();

                if (current_price < limit_order.limit_price_) {
                    break;
                }

                heap.pop();
                callback(limit_order.order_);
            }
        }
    }

    void LimitOrderBook::cancel_orders_for_symbol(const std::string& symbol) {
        buy_limits_.erase(symbol);
        sell_limits_.erase(symbol);
    }

    bool LimitOrderBook::empty() const {
        // Adding no-lint because we are in-fact not discarding the return value of empty.
        // NOLINTNEXTLINE(bugprone-standalone-empty)
        return std::ranges::all_of(buy_limits_, [](const auto& pair) { return pair.second.empty(); }) &&
               // NOLINTNEXTLINE(bugprone-standalone-empty)
               std::ranges::all_of(sell_limits_, [](const auto& pair) { return pair.second.empty(); });
    }

}  // namespace simulators
