#ifndef QUANT_FORGE_DATA_STORE_HPP
#define QUANT_FORGE_DATA_STORE_HPP

#pragma once

#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "../../http/api/stock_api.hpp"

namespace forge {

    class DataStore {
       public:
        void store_bars_by_plugin_name(const std::string& plugin_name, const std::string& symbol, const http::stock_api::AggregateBars& bars);
        [[nodiscard]] std::optional<http::stock_api::AggregateBars> get_bars(const std::string& plugin_name, const std::string& symbol) const;
        [[nodiscard]] std::optional<std::unordered_map<std::string, http::stock_api::AggregateBars>> get_all_bars_for_plugin(
            const std::string& plugin_name) const;
        [[nodiscard]] std::vector<std::string> get_symbols_for_plugin(const std::string& plugin_name) const;
        [[nodiscard]] bool has_plugin_data(const std::string& plugin_name) const;
        void create_iterable_plugin_data(const std::string& plugin_name) const;
        [[nodiscard]] std::vector<http::stock_api::AggregateBarResult> get_iterable_plugin_data(const std::string& plugin_name) const;
        [[nodiscard]] bool has_iterable_plugin_data(const std::string& plugin_name) const;
        void clear();

       private:
        mutable std::mutex mutex_;
        std::unordered_map<std::string, std::unordered_map<std::string, http::stock_api::AggregateBars>> bars_;
        mutable std::unordered_map<std::string, std::vector<http::stock_api::AggregateBarResult>> iterable_plugin_data_;
    };
}  // namespace forge

#endif
