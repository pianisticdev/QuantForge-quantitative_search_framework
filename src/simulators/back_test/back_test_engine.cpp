#include "back_test_engine.hpp"

#include <memory>
#include <string>

#include "../../forge/stores/data_store.hpp"
#include "../../plugins/loaders/interface.hpp"

namespace simulators {

    BackTestEngine::BackTestEngine(const plugins::loaders::IPluginLoader* plugin, const forge::DataStore* data_store)
        : plugin_(plugin),
          data_store_(data_store),
          position_sizer_(plugin_->get_host_params()),
          risk_manager_(plugin_->get_host_params()),
          execution_simulator_(plugin_->get_host_params()),
          portfolio_tracker_(plugin_->get_host_params()) {}

    void BackTestEngine::run() {
        state_.cash_ = plugin_->get_host_params().initial_capital_;

        auto iterable_plugin_data = data_store_->get_iterable_plugin_data(plugin_->get_plugin_name());

        for (const auto& bar : iterable_plugin_data) {
            PluginResult result = plugin_->on_bar(bar);

            if (result.code_ != 0) {
                throw std::runtime_error("Plugin on_bar failed: " + std::string(result.message_));
            }
        }

        report_ = {
            .some_value_ = "some_value",
            .another_value_ = "another_value",
        };
    }

    const BackTestReport& BackTestEngine::get_report() { return report_; }

}  // namespace simulators
