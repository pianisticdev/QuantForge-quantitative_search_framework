#include "src/http/model/model.hpp"
#include "src/http/cache/network_cache.hpp"
#include "src/http/client/curl_easy.hpp"
#include "src/http/client/curl_global.hpp"
#include "src/http/provider/polygon.hpp"
#include "src/http/api/stock_api.hpp"
#include <iostream>

int main() {
    try {
        //
        // Collect
        //
        std::string api_key = "REDACTED";

        http::cache::NetworkCachePolicy cache_policy{
                .enable_caching = true,
                .ttl_s = 60*60*24
        };

        http::stock_api::AggregateBarsArgs args{
                .symbol = "AAPL",
                .from = "2023-01-01",
                .to = "2025-10-01",
                .timespan = 1,
                .timespan_unit = "hour"
        };

        static http::client::CurlGlobal curl_global;

        static std::shared_ptr<http::cache::NetworkCache> cache_layer =
                std::make_shared<http::cache::NetworkCache>(cache_policy);

        static std::shared_ptr<http::client::CurlEasy> curl_easy =
                std::make_shared<http::client::CurlEasy>(cache_layer);
        curl_easy->enable_keepalive();
        curl_easy->enable_compression();
        curl_easy->prefer_http2_tls();

        auto provider = std::make_shared<http::provider::PolygonProvider>(api_key);
        http::stock_api::StockAPI stock_api(provider, curl_easy);
        auto bars = stock_api.custom_aggregate_bars(args);

        //
        // Log
        //

        std::cout << "\n--- Aggregate Bars ---\n";
        std::cout << "Ticker: " << bars.ticker << "\n";
        std::cout << "Adjusted: " << (bars.adjusted ? "true" : "false") << "\n";
        std::cout << "Query count: " << bars.query_count
                  << "  Result count: " << bars.result_count << "\n";

        std::cout << "\nFirst 5 bars:\n";
        for (size_t i = 0; i < std::min<size_t>(5, bars.results.size()); ++i) {
            const auto& b = bars.results[i];
            std::cout << "#" << i
                      << " t=" << b.unix_ts_ms
                      << " o=" << b.open
                      << " h=" << b.high
                      << " l=" << b.low
                      << " c=" << b.close
                      << " v=" << b.volume
                      << " vw=" << b.volume_weighted_price
                      << " n=" << b.tx_count
                      << " otc=" << (b.is_otc ? "true" : "false")
                      << "\n";
        }
    } catch (const http::http_error::HttpError& ex) {
        std::cerr << "HTTP Error " << ex.status << " at " << ex.url << "\n"
                  << ex.what() << "\n"
                  << "Body (preview): " << ex.body_preview << "\n";
        return 1;
    }
    return 0;
}
