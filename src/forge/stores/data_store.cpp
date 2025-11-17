#include "data_store.hpp"

#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "../../http/api/stock_api.hpp"

namespace forge {
    void DataStore::store_bars_by_plugin_name(const std::string& plugin_name, const std::string& symbol, const http::stock_api::AggregateBars& bars) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (bars_.find(plugin_name) == bars_.end()) {
            bars_[plugin_name] = std::unordered_map<std::string, http::stock_api::AggregateBars>();
        }

        bars_[plugin_name][symbol] = bars;
    }

    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    std::optional<http::stock_api::AggregateBars> DataStore::get_bars(const std::string& plugin_name, const std::string& symbol) const {
        std::lock_guard<std::mutex> lock(mutex_);

        auto plugin_it = bars_.find(plugin_name);
        if (plugin_it == bars_.end()) {
            return std::nullopt;
        }

        auto symbol_it = plugin_it->second.find(symbol);
        if (symbol_it == plugin_it->second.end()) {
            return std::nullopt;
        }

        return symbol_it->second;
    }

    std::optional<std::unordered_map<std::string, http::stock_api::AggregateBars>> DataStore::get_all_bars_for_plugin(const std::string& plugin_name) const {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = bars_.find(plugin_name);
        if (it == bars_.end()) {
            return std::nullopt;
        }

        return it->second;
    }

    std::vector<std::string> DataStore::get_symbols_for_plugin(const std::string& plugin_name) const {
        std::lock_guard<std::mutex> lock(mutex_);

        std::vector<std::string> symbols;
        auto it = bars_.find(plugin_name);
        if (it != bars_.end()) {
            symbols.reserve(it->second.size());
            for (const auto& [symbol, _] : it->second) {
                symbols.push_back(symbol);
            }
        }

        return symbols;
    }

    bool DataStore::has_plugin_data(const std::string& plugin_name) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return bars_.find(plugin_name) != bars_.end();
    }

    void DataStore::clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        bars_.clear();
        iterable_plugin_data_.clear();
    }

    bool DataStore::has_iterable_plugin_data(const std::string& plugin_name) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return iterable_plugin_data_.find(plugin_name) != iterable_plugin_data_.end();
    }

    void DataStore::create_iterable_plugin_data(const std::string& plugin_name) const {
        std::lock_guard<std::mutex> lock(mutex_);

        if (has_iterable_plugin_data(plugin_name)) {
            return;
        }

        auto bars = get_all_bars_for_plugin(plugin_name);

        if (!bars) {
            return;
        }

        std::vector<http::stock_api::AggregateBarResult> iterable_bars;

        for (const auto& [symbol, bars] : *bars) {
            iterable_bars.insert(iterable_bars.end(), bars.results_.begin(), bars.results_.end());
        }

        std::sort(iterable_bars.begin(), iterable_bars.end(), [](const auto& a, const auto& b) { return a.unix_ts_ns_ < b.unix_ts_ns_; });

        iterable_plugin_data_[plugin_name] = iterable_bars;
    }

    std::vector<http::stock_api::AggregateBarResult> DataStore::get_iterable_plugin_data(const std::string& plugin_name) const {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!has_iterable_plugin_data(plugin_name)) {
            create_iterable_plugin_data(plugin_name);
        }

        return iterable_plugin_data_.at(plugin_name);
    }
}  // namespace forge
