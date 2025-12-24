#ifndef QUANT_FORGE_PLUGINS_LOADERS_NATIVE_LOADER_HPP
#define QUANT_FORGE_PLUGINS_LOADERS_NATIVE_LOADER_HPP

#include <string>

#include "../../http/api/stock_api.hpp"
#include "../../simulators/back_test/state.hpp"
#include "../abi/abi.h"
#include "../abi/lib_handler.hpp"
#include "../manifest/manifest.hpp"
#include "interface.hpp"

namespace plugins::loaders {

    class NativeLoader : public IPluginLoader {
       public:
        NativeLoader(std::unique_ptr<plugins::manifest::PluginManifest> plugin_manifest);

        void load_plugin(const SimulatorContext& ctx) override;
        void on_init() const override;
        [[nodiscard]] PluginResult on_start() const override;
        [[nodiscard]] PluginResult on_bar(const http::stock_api::AggregateBarResult& bar, simulators::State& state) const override;
        [[nodiscard]] PluginResult on_end(const char** json_out) const override;
        void free_string(const char* str) const override;
        [[nodiscard]] std::string get_plugin_name() const override;
        void unload_plugin() override;
        [[nodiscard]] PluginExport* get_plugin_export() const override;
        [[nodiscard]] plugins::manifest::HostParams get_host_params() const override;

       private:
        std::unique_ptr<plugins::manifest::PluginManifest> plugin_manifest_;
        mutable PluginExport exp_{};
        mutable LibHandler lib_;
    };
}  // namespace plugins::loaders

#endif
