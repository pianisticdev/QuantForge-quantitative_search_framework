#ifndef QUANT_SIMULATORS_BACK_TEST_LIMIT_ORDER_BOOK_HPP
#define QUANT_SIMULATORS_BACK_TEST_LIMIT_ORDER_BOOK_HPP

#pragma once

#include <functional>
#include <map>
#include <string>

#include "../../utils/max_heap.hpp"
#include "../../utils/min_heap.hpp"
#include "./models.hpp"
#include "./state.hpp"

namespace simulators {

    class LimitOrderBook {
       private:
        std::map<std::string, data_structures::MaxHeap<models::LimitBuyOrder>> buy_limits_;
        std::map<std::string, data_structures::MinHeap<models::LimitSellOrder>> sell_limits_;

       public:
        void add_limit_order(const models::ScheduledLimitOrder& order);

        void process_buy_limits(const simulators::State& state, const std::function<void(const models::Order&)>& callback);

        void process_sell_limits(const simulators::State& state, const std::function<void(const models::Order&)>& callback);

        void cancel_orders_for_symbol(const std::string& symbol);

        [[nodiscard]] bool empty() const;
    };

}  // namespace simulators

#endif
