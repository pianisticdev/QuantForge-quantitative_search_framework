// NOLINTBEGIN(modernize-deprecated-headers, cppcoreguidelines-macro-usage, modernize-use-using)

#pragma once
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PLUGIN_API_VERSION 1

typedef enum CExitOrderType {
    EXIT_ORDER_STOP_LOSS = 0,
    EXIT_ORDER_TAKE_PROFIT = 1,
} CExitOrderType;

typedef struct CFill {
    double quantity_;
    int64_t price_;
    int64_t created_at_ns_;
    const char* symbol_;
    const char* uuid_;
    const char* action_;
} CFill;

typedef struct CStopLossExitOrder {
    bool is_triggered_;
    double trigger_quantity_;
    int64_t stop_loss_price_;
    int64_t price_;
    int64_t created_at_ns_;
    const char* symbol_;
    const char* fill_uuid_;
} CStopLossExitOrder;

typedef struct CTakeProfitExitOrder {
    bool is_triggered_;
    double trigger_quantity_;
    int64_t take_profit_price_;
    int64_t price_;
    int64_t created_at_ns_;
    const char* symbol_;
    const char* fill_uuid_;
} CTakeProfitExitOrder;

typedef struct CExitOrder {
    CExitOrderType type_;

    union {
        CStopLossExitOrder stop_loss_;
        CTakeProfitExitOrder take_profit_;
    } data_;
} CExitOrder;

typedef struct CPosition {
    const char* symbol_;
    double quantity_;
    double average_price_;
} CPosition;

typedef enum CInstructionType {
    INSTRUCTION_TYPE_SIGNAL = 0,
    INSTRUCTION_TYPE_ORDER = 1,
} CInstructionType;

typedef struct CSignal {
    const char* symbol_;
    const char* action_;
} CSignal;

typedef struct COrder {
    const char* symbol_;
    const char* action_;
    double quantity_;

    int64_t limit_price_;
    int64_t stop_loss_price_;
    int64_t take_profit_price_;

    const char* order_type_;
} CLimitOrder;

typedef struct CInstruction {
    CInstructionType type_;

    union {
        CSignal signal_;
        COrder order_;
    } data_;
} CInstruction;

typedef struct CEquitySnapshot {
    int64_t timestamp_ns_;
    int64_t equity_;
    double return_;
    double max_drawdown_;
    double sharpe_ratio_;
    double sortino_ratio_;
    double calmar_ratio_;
    double tail_ratio_;
    double value_at_risk_;
    double conditional_value_at_risk_;
} CEquitySnapshot;

typedef struct CState {
    int64_t cash_;
    const CPosition* positions_;
    size_t positions_count_;
    const CExitOrder* new_exit_orders_;
    size_t new_exit_orders_count_;
    const CFill* new_fills_;
    size_t new_fills_count_;
    const CEquitySnapshot* equity_curve_;
    size_t equity_curve_count_;
} CState;

typedef struct CBar {
    const char* symbol_;
    int64_t unix_ts_ns_;
    double open_;
    double high_;
    double low_;
    double close_;
    double volume_;
} Bar;

typedef struct PluginConfigKV {
    const char* key_;
    const char* value_;
} PluginConfigKV;

typedef struct PluginOptions {
    const PluginConfigKV* items_;
    size_t count_;
} PluginOptions;

typedef struct SimulatorContext {
    int api_version_;
} SimulatorContext;

// NOLINTBEGIN(readability-identifier-naming)
// ---- Strategy-oriented vtable
// V1: Signal Only Strategy Implementation
typedef struct PluginResult {
    int32_t code_;         // 0 == OK
    const char* message_;  // optional (owned by plugin, valid until next call)
    const CInstruction* instructions_;
    size_t instructions_count_;
} PluginResult;

typedef struct PluginVTable {
    void (*destroy)(void* self);

    PluginResult (*on_init)(void* self, const PluginOptions* opts);

    PluginResult (*on_start)(void* self);

    PluginResult (*on_bar)(void* self, const Bar* bar, const CState* state);

    // Provides a nullptr to the plugin
    // The plugin is responsible for building the JSON string
    PluginResult (*on_end)(void* self, const char** json_out);
    // The plugin needs to free the string, which can be called after on_end by the host
    void (*free_string)(void* self, const char* json_out_str);
} PluginVTable;
// NOLINTEND(readability-identifier-naming)

// ---- Factory export every plugin must provide
typedef struct PluginExport {
    int api_version_;      // must equal PLUGIN_API_VERSION
    void* instance_;       // opaque pointer to plugin state
    PluginVTable vtable_;  // function pointers
} PluginExport;

#define PLUGIN_CREATE_SYMBOL "create_plugin"
typedef PluginExport (*CreatePluginFn)(const SimulatorContext* ctx);

#ifdef __cplusplus
}
#endif

// NOLINTEND(modernize-deprecated-headers, cppcoreguidelines-macro-usage, modernize-use-using)
