#include "native_loader.hpp"

#include <pybind11/embed.h>
#include <pybind11/pybind11.h>

#include "../../http/api/stock_api.hpp"
#include "../../simulators/back_test/state.hpp"
#include "../abi/abi.h"
#include "../abi/lib_handler.hpp"
#include "../manifest/manifest.hpp"
#include "simulators/back_test/abi_converter.hpp"

namespace py = pybind11;

namespace plugins::loaders {
    struct NativePlugin {
        LibHandler lib_;
        PluginExport exp_{};
    };

    NativeLoader::NativeLoader(std::unique_ptr<plugins::manifest::PluginManifest> plugin_manifest) : plugin_manifest_(std::move(plugin_manifest)) {}

    void NativeLoader::load_plugin(const SimulatorContext& ctx) {
        std::string err;

        lib_ = LibHandler::open(plugin_manifest_->get_entry(), err);

        if (lib_.handle_ == nullptr) {
            throw std::runtime_error(err);
        }

        auto create = lib_.sym<CreatePluginFn>(PLUGIN_CREATE_SYMBOL, err);
        if (create == nullptr) {
            lib_.close();
            throw std::runtime_error("Failed to load plugin");
            return;
        }

        exp_ = create(&ctx);

        if (exp_.api_version_ != PLUGIN_API_VERSION || exp_.instance_ == nullptr) {
            err = "API mismatch or null instance";
            lib_.close();
            exp_ = {};
            throw std::runtime_error(err);
        }

        if (exp_.vtable_.destroy == nullptr || exp_.vtable_.on_end == nullptr) {
            err = "Required vtable methods missing";
            lib_.close();
            exp_ = {};
            throw std::runtime_error(err);
        }
    }

    void NativeLoader::on_init() const {
        if (exp_.api_version_ != PLUGIN_API_VERSION) {
            return;
        }

        if (exp_.vtable_.on_init != nullptr) {
            auto options = plugin_manifest_->get_options();
            exp_.vtable_.on_init(exp_.instance_, &options);
        }
    }

    PluginResult NativeLoader::on_start() const {
        if (exp_.api_version_ != PLUGIN_API_VERSION) {
            return PluginResult{1, "Invalid API Version", .instructions_count_ = 0, .instructions_ = nullptr};
        }

        if (exp_.vtable_.on_start == nullptr) {
            return PluginResult{1, "Undefined Method on_start", .instructions_count_ = 0, .instructions_ = nullptr};
        }

        return exp_.vtable_.on_start(exp_.instance_);
    }

    PluginResult NativeLoader::on_bar(const http::stock_api::AggregateBarResult& bar, simulators::State& state) const {
        if (exp_.api_version_ != PLUGIN_API_VERSION) {
            return PluginResult{1, "Invalid API Version", .instructions_count_ = 0, .instructions_ = nullptr};
        }

        if (exp_.vtable_.on_bar == nullptr) {
            return PluginResult{1, "Undefined Method on_bar", .instructions_count_ = 0, .instructions_ = nullptr};
        }

        CBar plugin_bar = plugins::loaders::to_plugin_bar(bar);
        CState c_state = simulators::ABIConverter().transform(state);  // TODO This should be a static method
        return exp_.vtable_.on_bar(exp_.instance_, &plugin_bar, &c_state);
    }

    PluginResult NativeLoader::on_end(const char** json_out) const {
        if (exp_.api_version_ != PLUGIN_API_VERSION) {
            return PluginResult{1, "Invalid API Version", .instructions_count_ = 0, .instructions_ = nullptr};
        }

        if (exp_.vtable_.on_end == nullptr) {
            return PluginResult{1, "Undefined Method on_end", .instructions_count_ = 0, .instructions_ = nullptr};
        }

        return exp_.vtable_.on_end(exp_.instance_, json_out);
    }

    void NativeLoader::free_string(const char* str) const {
        if (exp_.api_version_ != PLUGIN_API_VERSION) {
            return;
        }

        if (exp_.vtable_.free_string == nullptr) {
            return;
        }

        return exp_.vtable_.free_string(exp_.instance_, str);
    }

    std::string NativeLoader::get_plugin_name() const { return plugin_manifest_->get_name(); }

    void NativeLoader::unload_plugin() {
        if (exp_.api_version_ != PLUGIN_API_VERSION) {
            return;
        }

        if (exp_.instance_ != nullptr && exp_.vtable_.destroy != nullptr) {
            exp_.vtable_.destroy(exp_.instance_);
            exp_.instance_ = nullptr;
        }
        lib_.close();
        exp_ = {};
    }

    PluginExport* NativeLoader::get_plugin_export() const { return &exp_; }

    plugins::manifest::HostParams NativeLoader::get_host_params() const { return plugin_manifest_->get_host_params(); }

}  // namespace plugins::loaders
