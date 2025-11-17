
#include <cmath>
#include <cstdint>
#include <string>
#include <string_view>

#include "constants.hpp"
#include "money_utils.hpp"

namespace money_utils {
    int64_t parse_money_micro(const std::string& money_string) {
        const auto scaled_base = static_cast<int64_t>(std::pow(constants::MONEY_BASE, constants::MONEY_SCALE));

        auto decimal_position = money_string.find('.');

        if (decimal_position == std::string_view::npos) {
            return std::stoll(money_string) * scaled_base;
        }

        auto whole_part = money_string.substr(0, decimal_position);
        auto fractional_part = money_string.substr(decimal_position + 1);

        if (fractional_part.length() < constants::MONEY_SCALE) {
            fractional_part.append(fractional_part.length() - constants::MONEY_SCALE, '0');
        } else if (fractional_part.length() > constants::MONEY_SCALE) {
            fractional_part = fractional_part.substr(0, constants::MONEY_SCALE);
        }

        return std::stoll(whole_part) * scaled_base + std::stoll(fractional_part);
    };
}  // namespace money_utils
