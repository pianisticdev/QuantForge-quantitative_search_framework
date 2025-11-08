#include <iostream>

#include "src/http/api/stock_api.hpp"
#include "src/http/cache/network_cache.hpp"
#include "src/http/client/curl_easy.hpp"
#include "src/http/client/curl_global.hpp"
#include "src/http/error/http_error.hpp"
#include "src/http/provider/polygon.hpp"
#include "src/utils/constants.hpp"

int main() {
    try {
        //
        // Collect
        //

        const char* api_key = std::getenv("POLYGON_API_KEY");
        if (api_key == nullptr) {
            std::cout << "POLYGON_API_KEY not set" << std::endl;
            return 1;
        }

        auto cache_policy =
            std::make_unique<http::cache::NetworkCachePolicy>(http::cache::NetworkCachePolicy{.enable_caching_ = true, .ttl_s_ = constants::ONE_DAY_S});

        http::stock_api::AggregateBarsArgs args{.symbol_ = "AAPL", .from_ = "2023-01-01", .to_ = "2025-10-01", .timespan_ = 1, .timespan_unit_ = "hour"};

        static http::client::CurlGlobal curl_global;

        static std::shared_ptr<http::cache::NetworkCache> cache_layer = std::make_shared<http::cache::NetworkCache>(std::move(cache_policy));

        static std::shared_ptr<http::client::CurlEasy> curl_easy = std::make_shared<http::client::CurlEasy>(cache_layer);
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
        std::cout << "Ticker: " << bars.ticker_ << "\n";
        std::cout << "Adjusted: " << (bars.adjusted_ ? "true" : "false") << "\n";
        std::cout << "Query count: " << bars.query_count_ << "  Result count: " << bars.result_count_ << "\n";

        std::cout << "\nFirst 5 bars:\n";
        constexpr int MIN_BARS_TO_DISPLAY = 5;
        for (size_t i = 0; i < std::min<size_t>(MIN_BARS_TO_DISPLAY, bars.results_.size()); ++i) {
            const auto& b = bars.results_[i];
            std::cout << "#" << i << " t=" << b.unix_ts_ms_ << " o=" << b.open_ << " h=" << b.high_ << " l=" << b.low_ << " c=" << b.close_
                      << " v=" << b.volume_ << " vw=" << b.volume_weighted_price_ << " n=" << b.tx_count_ << " otc=" << (b.is_otc_ ? "true" : "false") << "\n";
        }
    } catch (const http::http_error::HttpError& ex) {
        std::cerr << "HTTP Error " << ex.status_ << " at " << ex.url_ << "\n"
                  << ex.what() << "\n"
                  << "Body (preview): " << ex.body_preview_ << "\n";
        return 1;
    }
    return 0;
}
