#ifndef QUANT_FORGE_PLUGINS_MANIFEST_MANIFEST_HPP
#define QUANT_FORGE_PLUGINS_MANIFEST_MANIFEST_HPP

#include <simdjson.h>

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "../abi/abi.h"

namespace plugins::manifest {

    template <typename T>
    struct ParserOptions {
        bool is_required_ = true;
        std::vector<T> allowed_values_;
        T fallback_value_;
        std::string error_message_;
    };

    // See: manifest.schema.json for more details.
    struct Symbol {
        bool primary_;
        int timespan_;
        std::string symbol_;
        std::string timespan_unit_;

        Symbol(bool primary, int timespan, std::string symbol, std::string timespan_unit)
            : primary_(primary), timespan_(timespan), symbol_(std::move(symbol)), timespan_unit_(std::move(timespan_unit)) {}
    };

    // See: manifest.schema.json for more details.
    struct HostParams {
        std::optional<bool> market_hours_only_;
        std::optional<bool> allow_fractional_shares_;
        int monte_carlo_runs_;
        int monte_carlo_seed_;
        std::optional<double> commission_;
        std::optional<double> slippage_;
        std::optional<double> tax_;
        int64_t initial_capital_;
        std::optional<std::string> commission_type_;
        std::optional<std::string> slippage_model_;
        std::optional<std::string> default_currency_;
        std::optional<std::string> timezone_;
        std::optional<std::string> optimization_mode_;
        std::string backtest_start_datetime_;
        std::string backtest_end_datetime_;
        std::vector<Symbol> symbols_;

        std::optional<std::string> position_sizing_method_;
        std::optional<double> position_size_value_;
        std::optional<double> max_position_size_;
        std::optional<int> max_concurrent_positions_;
        std::optional<bool> use_stop_loss_;
        std::optional<double> stop_loss_pct_;
        std::optional<bool> use_take_profit_;
        std::optional<double> take_profit_pct_;
        std::optional<std::string> execution_model_;
        std::optional<int> fill_delay_bars_;
    };

    const ParserOptions<std::string_view> NAME_PARSER_OPTIONS = {
        .is_required_ = true, .allowed_values_ = {}, .fallback_value_ = "", .error_message_ = "Invalid name"};
    const ParserOptions<std::string_view> KIND_PARSER_OPTIONS = {
        .is_required_ = true, .allowed_values_ = {"python", "native"}, .fallback_value_ = "", .error_message_ = "Invalid kind"};
    const ParserOptions<std::string_view> ENTRY_PARSER_OPTIONS = {
        .is_required_ = true, .allowed_values_ = {}, .fallback_value_ = "", .error_message_ = "Invalid entry"};
    const ParserOptions<std::string_view> DESCRIPTION_PARSER_OPTIONS = {
        .is_required_ = false, .allowed_values_ = {}, .fallback_value_ = "", .error_message_ = "Invalid description"};
    const ParserOptions<std::string_view> AUTHOR_PARSER_OPTIONS = {
        .is_required_ = false, .allowed_values_ = {}, .fallback_value_ = "", .error_message_ = "Invalid author"};
    const ParserOptions<long long> API_VERSION_PARSER_OPTIONS = {
        .is_required_ = true, .allowed_values_ = {PLUGIN_API_VERSION}, .fallback_value_ = 0, .error_message_ = "Invalid api version"};
    const ParserOptions<std::string_view> VERSION_PARSER_OPTIONS = {
        .is_required_ = true, .allowed_values_ = {}, .fallback_value_ = "", .error_message_ = "Invalid version"};
    const ParserOptions<std::string_view> STRATEGY_PARAMS_PARSER_OPTIONS = {
        .is_required_ = true, .allowed_values_ = {}, .fallback_value_ = "", .error_message_ = "Invalid strategy params"};
    const ParserOptions<bool> PRIMARY_PARSER_OPTIONS = {
        .is_required_ = true, .allowed_values_ = {true, false}, .fallback_value_ = false, .error_message_ = "Invalid primary"};
    const ParserOptions<long long> TIMESPAN_PARSER_OPTIONS = {
        .is_required_ = true, .allowed_values_ = {}, .fallback_value_ = 0, .error_message_ = "Invalid timespan"};
    const ParserOptions<std::string_view> SYMBOL_PARSER_OPTIONS = {
        .is_required_ = true, .allowed_values_ = {}, .fallback_value_ = "", .error_message_ = "Invalid symbol"};
    const ParserOptions<std::string_view> TIMESPAN_UNIT_PARSER_OPTIONS = {.is_required_ = true,
                                                                          .allowed_values_ = {"second", "minute", "hour", "day", "week", "month", "year"},
                                                                          .fallback_value_ = "",
                                                                          .error_message_ = "Invalid timespan unit"};
    const ParserOptions<bool> MARKET_HOURS_ONLY_PARSER_OPTIONS = {
        .is_required_ = true, .allowed_values_ = {true, false}, .fallback_value_ = false, .error_message_ = "Invalid market hours only"};
    const ParserOptions<bool> ALLOW_FRACTIONAL_SHARES_PARSER_OPTIONS = {
        .is_required_ = true, .allowed_values_ = {true, false}, .fallback_value_ = false, .error_message_ = "Invalid allow fractional shares"};
    const ParserOptions<long long> MONTE_CARLO_RUNS_PARSER_OPTIONS = {
        .is_required_ = true, .allowed_values_ = {}, .fallback_value_ = 0, .error_message_ = "Invalid monte carlo runs"};
    const ParserOptions<long long> MONTE_CARLO_SEED_PARSER_OPTIONS = {
        .is_required_ = true, .allowed_values_ = {}, .fallback_value_ = 0, .error_message_ = "Invalid monte carlo seed"};
    const ParserOptions<long long> LEVERAGE_PARSER_OPTIONS = {
        .is_required_ = true, .allowed_values_ = {}, .fallback_value_ = 0, .error_message_ = "Invalid leverage"};
    const ParserOptions<double> COMMISSION_PARSER_OPTIONS = {
        .is_required_ = true, .allowed_values_ = {}, .fallback_value_ = 0.0, .error_message_ = "Invalid commission"};
    const ParserOptions<std::string_view> COMMISSION_TYPE_PARSER_OPTIONS = {
        .is_required_ = true, .allowed_values_ = {"per_share", "percentage", "flat"}, .fallback_value_ = "", .error_message_ = "Invalid commission type"};
    const ParserOptions<double> SLIPPAGE_PARSER_OPTIONS = {
        .is_required_ = true, .allowed_values_ = {}, .fallback_value_ = 0.0, .error_message_ = "Invalid slippage"};
    const ParserOptions<std::string_view> SLIPPAGE_MODEL_PARSER_OPTIONS = {.is_required_ = true,
                                                                           .allowed_values_ = {"none", "fixed", "percentage", "volume_based"},
                                                                           .fallback_value_ = "",
                                                                           .error_message_ = "Invalid slippage model"};
    const ParserOptions<double> TAX_PARSER_OPTIONS = {.is_required_ = true, .allowed_values_ = {}, .fallback_value_ = 0.0, .error_message_ = "Invalid tax"};
    const ParserOptions<std::string_view> DEFAULT_CURRENCY_PARSER_OPTIONS = {
        .is_required_ = true, .allowed_values_ = {"USD"}, .fallback_value_ = "", .error_message_ = "Invalid currency"};
    const ParserOptions<std::string_view> TIMEZONE_PARSER_OPTIONS = {
        .is_required_ = true, .allowed_values_ = {"America/New_York"}, .fallback_value_ = "", .error_message_ = "Invalid timezone"};
    const ParserOptions<std::string_view> OPTIMIZATION_MODE_PARSER_OPTIONS = {.is_required_ = true,
                                                                              .allowed_values_ = {"none", "grid_search", "bayesian", "genetic"},
                                                                              .fallback_value_ = "",
                                                                              .error_message_ = "Invalid optimization mode"};
    const ParserOptions<std::string_view> BACKTEST_START_DATETIME_PARSER_OPTIONS = {
        .is_required_ = true, .allowed_values_ = {}, .fallback_value_ = "", .error_message_ = "Invalid backtest start datetime"};
    const ParserOptions<std::string_view> BACKTEST_END_DATETIME_PARSER_OPTIONS = {
        .is_required_ = true, .allowed_values_ = {}, .fallback_value_ = "", .error_message_ = "Invalid backtest end datetime"};
    const ParserOptions<std::string_view> INITIAL_CAPITAL_PARSER_OPTIONS = {
        .is_required_ = true, .allowed_values_ = {}, .fallback_value_ = "0", .error_message_ = "Invalid initial capital, please provide a valid money string"};
    const ParserOptions<std::string_view> POSITION_SIZING_METHOD_PARSER_OPTIONS = {.is_required_ = false,
                                                                                   .allowed_values_ = {"fixed_percentage", "fixed_dollar", "equal_weight"},
                                                                                   .fallback_value_ = "fixed_percentage",
                                                                                   .error_message_ = "Invalid position sizing method"};
    const ParserOptions<double> POSITION_SIZE_VALUE_PARSER_OPTIONS = {
        .is_required_ = false, .allowed_values_ = {}, .fallback_value_ = 0.01, .error_message_ = "Invalid position size value"};
    const ParserOptions<double> MAX_POSITION_SIZE_PARSER_OPTIONS = {
        .is_required_ = false, .allowed_values_ = {}, .fallback_value_ = 0.0, .error_message_ = "Invalid max position size"};
    const ParserOptions<long long> MAX_CONCURRENT_POSITIONS_PARSER_OPTIONS = {
        .is_required_ = false, .allowed_values_ = {}, .fallback_value_ = 1, .error_message_ = "Invalid max concurrent positions"};
    const ParserOptions<bool> USE_STOP_LOSS_PARSER_OPTIONS = {
        .is_required_ = false, .allowed_values_ = {true, false}, .fallback_value_ = false, .error_message_ = "Invalid use stop loss"};
    const ParserOptions<double> STOP_LOSS_PCT_PARSER_OPTIONS = {
        .is_required_ = false, .allowed_values_ = {}, .fallback_value_ = 0.05, .error_message_ = "Invalid stop loss pct"};
    const ParserOptions<bool> USE_TAKE_PROFIT_PARSER_OPTIONS = {
        .is_required_ = false, .allowed_values_ = {true, false}, .fallback_value_ = false, .error_message_ = "Invalid use take profit"};
    const ParserOptions<double> TAKE_PROFIT_PCT_PARSER_OPTIONS = {
        .is_required_ = false, .allowed_values_ = {}, .fallback_value_ = 0.05, .error_message_ = "Invalid take profit pct"};
    const ParserOptions<std::string_view> EXECUTION_MODEL_PARSER_OPTIONS = {
        .is_required_ = false, .allowed_values_ = {"immediate", "delayed"}, .fallback_value_ = "immediate", .error_message_ = "Invalid execution model"};
    const ParserOptions<long long> FILL_DELAY_BARS_PARSER_OPTIONS = {
        .is_required_ = false, .allowed_values_ = {}, .fallback_value_ = 0, .error_message_ = "Invalid fill delay bars"};

    class PluginManifest {
       public:
        PluginManifest() = default;

        ~PluginManifest() = default;

        PluginManifest(const PluginManifest&) = delete;
        PluginManifest& operator=(const PluginManifest&) = delete;
        PluginManifest(PluginManifest&&) = delete;
        PluginManifest& operator=(PluginManifest&&) = delete;

        [[nodiscard]] bool is_python() const;
        [[nodiscard]] bool is_native() const;
        [[nodiscard]] bool is_one_of(const std::vector<std::string>& plugin_names) const;

        // Load from dir is the initial manifest loading option.
        // In the future there may be other ways to load the manifest.
        [[nodiscard]] static simdjson::ondemand::document load_from_file(const std::filesystem::path& path);
        void parse_json(simdjson::ondemand::document& doc);

        [[nodiscard]] std::string get_name() const;
        [[nodiscard]] int get_api_version() const;
        [[nodiscard]] std::string get_kind() const;
        [[nodiscard]] std::string get_entry() const;
        [[nodiscard]] PluginOptions get_options() const;
        [[nodiscard]] HostParams get_host_params() const;

       private:
        int api_version_ = 0;
        std::string name_;
        std::string kind_;
        std::string entry_;
        std::optional<std::string> description_;
        std::optional<std::string> author_;
        std::string version_;
        HostParams host_params_;
        std::string strategy_params_;

        mutable std::vector<PluginConfigKV> cached_options_;
        mutable std::vector<std::string> cached_option_strings_;
    };

}  // namespace plugins::manifest

#endif
