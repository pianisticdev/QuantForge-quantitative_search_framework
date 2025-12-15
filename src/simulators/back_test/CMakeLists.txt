add_library(
  simulators_back_test
  STATIC
    abi_converter.cpp
    back_test_engine.cpp
    state.cpp
    equity_calculator.cpp
    exchange.cpp
    executor.cpp
    exit_order_book.cpp
    position_calculator.cpp
    slippage_calculator.cpp
)
target_include_directories(simulators_back_test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
