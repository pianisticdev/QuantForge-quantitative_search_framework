#ifndef QUANT_UTILS_MONEY_UTILS_HPP
#define QUANT_UTILS_MONEY_UTILS_HPP

#include <cstdint>
#include <string>

namespace money_utils {
    int64_t parse_money_micro(const std::string& money_string);
}  // namespace money_utils

#endif
