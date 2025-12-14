#ifndef QUANT_SIMULATORS_BACK_TEST_EXIT_ORDER_BOOK_HPP
#define QUANT_SIMULATORS_BACK_TEST_EXIT_ORDER_BOOK_HPP

#pragma once

#include "../../utils/max_heap.hpp"
#include "../../utils/min_heap.hpp"
#include "./models.hpp"

namespace simulators {

    class ExitOrderBook {
       private:
        data_structures::MinHeap<models::StopLossExitOrder> stop_loss_heap_;
        data_structures::MaxHeap<models::TakeProfitExitOrder> take_profit_heap_;

       public:
        void add_exit_order(const models::ExitOrder& order);
        void process_stop_loss_heap(const models::BackTestState& state, const std::function<void(const models::StopLossExitOrder&)>& callback);
        void process_take_profit_heap(const models::BackTestState& state, const std::function<void(const models::TakeProfitExitOrder&)>& callback);
        // Fill out additional methods here
    };

}  // namespace simulators

#endif
