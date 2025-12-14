#ifndef QUANT_FORGE_PLUGINS_LOADERS_INTERFACE_HPP
#define QUANT_FORGE_PLUGINS_LOADERS_INTERFACE_HPP

#include <string>

#include "../../http/api/stock_api.hpp"
#include "../../simulators/back_test/models.hpp"
#include "../abi/abi.h"
#include "../manifest/manifest.hpp"

namespace plugins::loaders {

    inline CBar to_plugin_bar(const http::stock_api::AggregateBarResult& bar) {
        return CBar{
            .unix_ts_ns_ = bar.unix_ts_ns_,
            .open_ = bar.open_,
            .high_ = bar.high_,
            .low_ = bar.low_,
            .close_ = bar.close_,
            .volume_ = bar.volume_,
            .symbol_ = bar.symbol_.c_str(),
        };
    }

    inline http::stock_api::AggregateBarResult to_http_bar(const CBar& bar) {
        return http::stock_api::AggregateBarResult{
            .unix_ts_ns_ = bar.unix_ts_ns_,
            .open_ = bar.open_,
            .high_ = bar.high_,
            .low_ = bar.low_,
            .close_ = bar.close_,
            .volume_ = bar.volume_,
            .symbol_ = bar.symbol_,
        };
    }

    class IPluginLoader {
       public:
        IPluginLoader() = default;
        virtual ~IPluginLoader() = default;

        virtual void load_plugin(const SimulatorContext& ctx);
        virtual void on_init() const = 0;
        [[nodiscard]] virtual PluginResult on_start() const = 0;
        [[nodiscard]] virtual PluginResult on_bar(const http::stock_api::AggregateBarResult& bar, models::BackTestState& state) const = 0;
        [[nodiscard]] virtual PluginResult on_end(const char** json_out) const = 0;
        virtual void free_string(const char* str) const = 0;
        [[nodiscard]] virtual std::string get_plugin_name() const = 0;
        virtual void unload_plugin();
        [[nodiscard]] virtual PluginExport* get_plugin_export() const;
        [[nodiscard]] virtual plugins::manifest::HostParams get_host_params() const = 0;
    };

}  // namespace plugins::loaders

#endif
