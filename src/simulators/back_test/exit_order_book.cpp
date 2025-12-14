#include "exit_order_book.hpp"

#include <string>

#include "./models.hpp"

namespace simulators {

    void ExitOrderBook::add_exit_order(const models::ExitOrder& order) {
        switch (order.index()) {
            case 0:
                stop_loss_heap_.push(std::get<models::StopLossExitOrder>(order));
                break;
            case 1:
                take_profit_heap_.push(std::get<models::TakeProfitExitOrder>(order));
                break;
            default:
                throw std::runtime_error("Invalid exit order type");
                break;
        }
    }

    void ExitOrderBook::process_stop_loss_heap(const models::BackTestState& state, const std::function<void(const models::StopLossExitOrder&)>& callback) {
        while (!stop_loss_heap_.empty()) {
            auto top_trade_opt = stop_loss_heap_.top();

            if (!top_trade_opt.has_value()) {
                break;
            }

            models::StopLossExitOrder exit_order = top_trade_opt.value();

            if (exit_order.stop_loss_price_ < state.current_prices_.at(exit_order.symbol_)) {
                break;
            }

            stop_loss_heap_.pop();

            callback(exit_order);
        }
    }

    void ExitOrderBook::process_take_profit_heap(const models::BackTestState& state, const std::function<void(const models::TakeProfitExitOrder&)>& callback) {
        while (!take_profit_heap_.empty()) {
            auto top_trade_opt = take_profit_heap_.top();

            if (!top_trade_opt.has_value()) {
                break;
            }

            auto exit_order = top_trade_opt.value();

            if (exit_order.take_profit_price_ > state.current_prices_.at(exit_order.symbol_)) {
                break;
            }

            take_profit_heap_.pop();

            callback(exit_order);
        }
    }

}  // namespace simulators
