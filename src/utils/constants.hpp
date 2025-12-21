
#ifndef QUANT_FORGE_CONSTANTS_HPP
#define QUANT_FORGE_CONSTANTS_HPP

#include <array>
#include <cmath>

namespace constants {
    inline constexpr int BASE_10 = 10;
    inline constexpr int ASCII_LOWERCASE_BIT = 0x20;
    inline constexpr long ONE_DAY_S = 60L * 60L * 24L;
    inline constexpr int MONEY_SCALE = 6;
    inline constexpr int MONEY_BASE = 10;
    inline constexpr int64_t MONEY_SCALED_BASE = 1'000'000.0;
    inline constexpr int NANOSECONDS_PER_MILLISECOND = 1000000;
    inline constexpr const char* STOP_LOSS = "stop_loss";
    inline constexpr const char* TAKE_PROFIT = "take_profit";
    inline constexpr const char* LIMIT = "limit";
    inline constexpr const char* MARKET = "market";
    inline constexpr const char* BUY = "buy";
    inline constexpr const char* SELL = "sell";
    inline constexpr std::array<int, 6> ROLLING_EQUITY_WINDOWS = {1, 7, 30, 90, 180, 365};
    inline constexpr double DEFAULT_POSITION_SIZE_VALUE = 0.01;
    inline constexpr double EPSILON = 0.0001;

}  // namespace constants

#endif
