#include <iostream>
#include <thread>

#include "src/forge/forge_engine/forge.hpp"
#include "src/http/cache/network_cache.hpp"
#include "src/http/client/curl_easy.hpp"
#include "src/http/client/curl_global.hpp"
#include "src/http/error/http_error.hpp"
#include "src/http/provider/polygon.hpp"
#include "src/renderers/console_renderer.hpp"
#include "src/utils/constants.hpp"

int main() {
    try {
        //
        // Collect
        //

        const char* plugin_loader = "directory";
        const char* plugin_root_path = "plugins";
        const unsigned int max_threads = std::thread::hardware_concurrency();
        const std::string polygon_api_key = std::getenv("POLYGON_API_KEY");
        const std::vector<std::string> enabled_plugin_names = {"sma_native", "sma_python"};
        const bool is_cache_enabled = true;
        const int cache_ttl_s = constants::ONE_DAY_S;

        if (polygon_api_key.empty()) {
            std::cout << "POLYGON_API_KEY not set" << std::endl;
            return 1;
        }

        http::client::CurlGlobal curl_global;

        auto data_provider = std::make_unique<http::provider::PolygonProvider>(polygon_api_key);

        auto engine = forge::ForgeEngineBuilder()
                          .with_http_client_factory([]() {
                              auto network_cache_policy = std::make_unique<http::cache::NetworkCachePolicy>(
                                  http::cache::NetworkCachePolicy{.enable_caching_ = is_cache_enabled, .ttl_s_ = cache_ttl_s});
                              auto network_cache = std::make_unique<http::cache::NetworkCache>(std::move(network_cache_policy));
                              return std::make_unique<http::client::CurlEasy>(std::move(network_cache));
                          })
                          .with_data_provider(std::move(data_provider))
                          .with_thread_pools(forge::ThreadPoolOptions{.io_threads_ = max_threads / 2, .compute_threads_ = max_threads})
                          .with_plugin_names(enabled_plugin_names)
                          .with_renderer(std::make_unique<renderers::ConsoleRenderer>())
                          .validate()
                          .build();

        engine->initialize(forge::InitializationOptions{.loader_ = plugin_loader, .root_path_ = plugin_root_path});
        engine->fetch_data();
        engine->run();
        engine->report();
    } catch (const http::http_error::HttpError& e) {
        std::cerr << "HTTP Error: " << e.what() << " (URL: " << e.url_ << ")\n";
        return 2;
    } catch (const std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
};
