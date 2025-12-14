add_library(
  simulators_back_test
  STATIC
    abi_converter.cpp
    back_test_engine.cpp
    equity_calculator.cpp
    exchange.cpp
    executor.cpp
    slippage_calculator.cpp
    exit_order_book.cpp
)
target_include_directories(simulators_back_test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
