#include "forge.hpp"

#include <memory>
#include <thread>

#include "../../http/api/stock_api.hpp"
#include "../../plugins/manager/plugin_manager.hpp"
#include "../../simulators/back_test/back_test_engine.hpp"
#include "../../simulators/monte_carlo/monte_carlo_engine.hpp"
#include "../../utils/thread_pool.hpp"
#include "../stores/data_store.hpp"
#include "../stores/report_store.hpp"

namespace forge {

    //
    // ForgeEngineBuilder implementation
    //

    ForgeEngineBuilder::ForgeEngineBuilder() : forge_engine_(std::make_unique<ForgeEngine>()) {}

    ForgeEngineBuilder& ForgeEngineBuilder::with_data_provider(std::unique_ptr<http::stock_api::IStockDataProvider> provider) {
        forge_engine_->set_data_provider(std::move(provider));
        return *this;
    }

    ForgeEngineBuilder& ForgeEngineBuilder::with_http_client_factory(std::function<std::unique_ptr<http::client::IHttpClient>()> http_client_factory) {
        forge_engine_->set_http_client_factory(std::move(http_client_factory));
        return *this;
    }

    ForgeEngineBuilder& ForgeEngineBuilder::with_plugin_names(const std::vector<std::string>& plugin_names) {
        plugin_names_ = plugin_names;
        return *this;
    }

    ForgeEngineBuilder& ForgeEngineBuilder::with_thread_pools(const ThreadPoolOptions& thread_pool_options) {
        forge_engine_->set_thread_pools(thread_pool_options);
        return *this;
    }

    ForgeEngineBuilder& ForgeEngineBuilder::with_renderer(std::unique_ptr<renderers::IRenderer> renderer) {
        forge_engine_->set_renderer(std::move(renderer));
        return *this;
    }

    ForgeEngineBuilder& ForgeEngineBuilder::validate() {
        if (data_provider_ == nullptr) {
            throw std::runtime_error("Data provider is required");
        }
        if (forge_engine_->get_http_client_factory() == nullptr) {
            throw std::runtime_error("HTTP client is required");
        }
        if (plugin_names_.empty()) {
            throw std::runtime_error("Plugin names are required");
        }
        if (forge_engine_->get_thread_pool_options().io_threads_ == 0) {
            throw std::runtime_error("IO threads are required");
        }
        if (forge_engine_->get_thread_pool_options().compute_threads_ == 0) {
            throw std::runtime_error("Compute threads are required");
        }
        return *this;
    }

    std::unique_ptr<ForgeEngine> ForgeEngineBuilder::build() {
        forge_engine_->set_plugin_manager(std::make_unique<plugins::manager::PluginManager>(plugin_names_));
        forge_engine_->set_data_store(std::make_unique<DataStore>());
        forge_engine_->set_report_store(std::make_unique<ReportStore>());
        return std::move(forge_engine_);
    };

    //
    // ForgeEngine implementation
    //

    void ForgeEngine::set_data_store(std::unique_ptr<DataStore> data_store) { data_store_ = std::move(data_store); }

    void ForgeEngine::set_report_store(std::unique_ptr<ReportStore> report_store) { report_store_ = std::move(report_store); }

    void ForgeEngine::set_stock_api(std::unique_ptr<http::stock_api::StockAPI> stock_api) { stock_api_ = std::move(stock_api); }

    void ForgeEngine::set_plugin_manager(std::unique_ptr<plugins::manager::PluginManager> plugin_manager) { plugin_manager_ = std::move(plugin_manager); }

    void ForgeEngine::set_thread_pools(const ThreadPoolOptions& thread_pool_options) { thread_pool_options_ = thread_pool_options; }

    void ForgeEngine::set_renderer(std::unique_ptr<renderers::IRenderer> renderer) { renderer_ = std::move(renderer); }

    const ThreadPoolOptions& ForgeEngine::get_thread_pool_options() const { return thread_pool_options_; }

    void ForgeEngine::initialize(const InitializationOptions& initialization_options) const {
        if (initialization_options.loader_ == "directory") {
            plugin_manager_->load_plugins_from_dir(initialization_options.root_path_);
        } else {
            // In the future we could have other loader types...
            throw std::runtime_error("Invalid loader type");
        }
    }

    void ForgeEngine::fetch_data() const {
        concurrency::ThreadPool pool(thread_pool_options_.io_threads_);

        plugin_manager_->with_plugins([&](auto* plugin_ptr) {
            auto host_params = plugin_ptr->get_host_params();

            auto* data_store_ptr = data_store_.get();
            auto* data_provider_ptr = data_provider_.get();

            for (const auto& symbol : host_params.symbols_) {
                pool.enqueue([data_store_ptr, plugin_ptr, data_provider_ptr, symbol, host_params, this]() {
                    auto http_client = http_client_factory_();

                    auto stock_api = std::make_unique<http::stock_api::StockAPI>(data_provider_ptr, std::move(http_client));

                    auto bars = stock_api->custom_aggregate_bars(http::stock_api::AggregateBarsArgs{
                        .symbol_ = symbol.symbol_,
                        .timespan_unit_ = symbol.timespan_unit_,
                        .timespan_ = symbol.timespan_,
                        .from_ = host_params.backtest_start_datetime_,
                        .to_ = host_params.backtest_end_datetime_,
                    });

                    data_store_ptr->store_bars_by_plugin_name(plugin_ptr->get_plugin_name(), symbol.symbol_, bars);
                });
            }
        });

        pool.wait_all();
    }

    void ForgeEngine::run() const {
        concurrency::ThreadPool pool(thread_pool_options_.compute_threads_);

        // Goal: Embarrassingly parallelize
        plugin_manager_->with_plugins([&](auto* plugin_ptr) {
            auto* data_store_ptr = data_store_.get();
            auto* report_store_ptr = report_store_.get();

            pool.enqueue([plugin_ptr, data_store_ptr, report_store_ptr]() {
                std::string plugin_name = plugin_ptr->get_plugin_name();

                if (!data_store_ptr->has_plugin_data(plugin_name)) {
                    throw std::runtime_error("Plugin data not found, exiting simulation...");
                }

                simulators::BackTestEngine back_test_engine(plugin_ptr, data_store_ptr);
                back_test_engine.run();
                report_store_ptr->store_back_test_report(plugin_name, back_test_engine.get_report());

                simulators::MonteCarloEngine monte_carlo_engine(plugin_ptr, data_store_ptr);
                monte_carlo_engine.run();
                report_store_ptr->store_monte_carlo_report(plugin_name, monte_carlo_engine.get_report());
            });
        });

        pool.wait_all();
    }

    void ForgeEngine::report() const {
        renderer_->render_back_test_report(report_store_->get_back_test_reports());
        renderer_->render_monte_carlo_report(report_store_->get_monte_carlo_reports());
    }
}  // namespace forge
