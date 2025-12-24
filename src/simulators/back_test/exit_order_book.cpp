#include "exit_order_book.hpp"

#include <string>

#include "./models.hpp"
#include "./state.hpp"

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

    void ExitOrderBook::process_stop_loss_heap(const simulators::State& state, const std::function<void(const models::StopLossExitOrder&)>& callback) {
        std::vector<models::StopLossExitOrder> not_triggered;

        while (!stop_loss_heap_.empty()) {
            auto top_trade_opt = stop_loss_heap_.top();

            if (!top_trade_opt.has_value()) {
                break;
            }

            auto pos_it = state.positions_.find(top_trade_opt.value().symbol_);
            if (pos_it == state.positions_.end()) {
                stop_loss_heap_.pop();
                continue;
            }

            models::StopLossExitOrder exit_order = top_trade_opt.value();
            Money current_price = state.current_prices_.at(exit_order.symbol_);

            bool should_trigger = false;
            if (exit_order.is_short_position_) {
                should_trigger = current_price >= exit_order.stop_loss_price_;
            } else {
                should_trigger = current_price <= exit_order.stop_loss_price_;
            }

            stop_loss_heap_.pop();

            if (should_trigger) {
                callback(exit_order);
            } else {
                not_triggered.push_back(exit_order);
            }
        }

        for (const auto& order : not_triggered) {
            stop_loss_heap_.push(order);
        }
    }

    void ExitOrderBook::process_take_profit_heap(const simulators::State& state, const std::function<void(const models::TakeProfitExitOrder&)>& callback) {
        std::vector<models::TakeProfitExitOrder> not_triggered;

        while (!take_profit_heap_.empty()) {
            auto top_trade_opt = take_profit_heap_.top();

            if (!top_trade_opt.has_value()) {
                break;
            }

            auto pos_it = state.positions_.find(top_trade_opt.value().symbol_);
            if (pos_it == state.positions_.end()) {
                take_profit_heap_.pop();
                continue;
            }

            models::TakeProfitExitOrder exit_order = top_trade_opt.value();
            Money current_price = state.current_prices_.at(exit_order.symbol_);

            bool should_trigger = false;
            if (exit_order.is_short_position_) {
                should_trigger = current_price <= exit_order.take_profit_price_;
            } else {
                should_trigger = current_price >= exit_order.take_profit_price_;
            }

            take_profit_heap_.pop();

            if (should_trigger) {
                callback(exit_order);
            } else {
                not_triggered.push_back(exit_order);
            }
        }

        for (const auto& order : not_triggered) {
            take_profit_heap_.push(order);
        }
    }

    void ExitOrderBook::reduce_exit_orders_by_fills(const std::vector<std::pair<std::string, double>>& closed_fills) {
        for (const auto& [fill_uuid, quantity] : closed_fills) {
            reduce_exit_orders_by_fill_uuid(fill_uuid, quantity);
        }
    }

    void ExitOrderBook::reduce_exit_orders_by_fill_uuid(const std::string& fill_uuid, double quantity_sold) {
        std::vector<models::StopLossExitOrder> temp_stop_loss;

        while (!stop_loss_heap_.empty()) {
            auto exit_order_opt = stop_loss_heap_.top();
            stop_loss_heap_.pop();

            if (!exit_order_opt.has_value()) {
                continue;
            }

            auto exit_order = exit_order_opt.value();

            if (exit_order.fill_uuid_ == fill_uuid) {
                if (exit_order.trigger_quantity_ > quantity_sold) {
                    exit_order.trigger_quantity_ -= quantity_sold;
                    temp_stop_loss.push_back(exit_order);
                }
            } else {
                temp_stop_loss.push_back(exit_order);
            }
        }

        for (const auto& order : temp_stop_loss) {
            stop_loss_heap_.push(order);
        }

        std::vector<models::TakeProfitExitOrder> temp_take_profit;

        while (!take_profit_heap_.empty()) {
            auto exit_order_opt = take_profit_heap_.top();
            take_profit_heap_.pop();

            if (!exit_order_opt.has_value()) {
                continue;
            }

            auto exit_order = exit_order_opt.value();

            if (exit_order.fill_uuid_ == fill_uuid) {
                if (exit_order.trigger_quantity_ > quantity_sold) {
                    exit_order.trigger_quantity_ -= quantity_sold;
                    temp_take_profit.push_back(exit_order);
                }
            } else {
                temp_take_profit.push_back(exit_order);
            }
        }

        for (const auto& order : temp_take_profit) {
            take_profit_heap_.push(order);
        }
    }

}  // namespace simulators
