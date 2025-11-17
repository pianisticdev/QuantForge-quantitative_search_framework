// NOLINTBEGIN(modernize-deprecated-headers, cppcoreguidelines-macro-usage, modernize-use-using)

#pragma once
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PLUGIN_API_VERSION 1

typedef struct Instruction {
    const char* symbol_;
    const char* action_;
    double quantity_;

    const char* order_type_;
    int64_t limit_price_;
    int64_t stop_loss_price_;
} Instruction;

typedef struct Bar {
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
    const Instruction* instructions_;
    size_t instructions_count_;
} PluginResult;

typedef struct PluginVTable {
    void (*destroy)(void* self);

    PluginResult (*on_init)(void* self, const PluginOptions* opts);

    PluginResult (*on_start)(void* self);

    PluginResult (*on_bar)(void* self, const Bar* bar);

    // Provides a nullptr to the plugin
    // The plugin is responsible for building the JSON string
    PluginResult (*on_end)(void* self, const char** json_out);
    // The plugin needs to free the string, which can be called after on_end by the host
    void (*free_string)(void* self, const char* p);
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
