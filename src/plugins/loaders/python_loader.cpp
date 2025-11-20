#include "python_loader.hpp"

#include <pybind11/embed.h>
#include <pybind11/pybind11.h>

#include "../../http/api/stock_api.hpp"
#include "../../simulators/back_test/models.hpp"
#include "../abi/abi.h"
#include "../manifest/manifest.hpp"

namespace py = pybind11;

namespace plugins::loaders {

    PythonLoader::PythonLoader(std::unique_ptr<plugins::manifest::PluginManifest> plugin_manifest) : plugin_manifest_(std::move(plugin_manifest)) {}

    void PythonLoader::load_plugin(const SimulatorContext& ctx) {
        static bool py_up = false;

        if (!py_up) {
            py::initialize_interpreter();
            py_up = true;
        }

        try {
            py::gil_scoped_acquire gil;
            py::module python_module = py::module::import(plugin_manifest_->get_entry().c_str());
            py::object create_plugin_fn = python_module.attr("create_plugin");
            py::capsule ctx_capsule((void*)&ctx, "SimulatorContext", [](void*) {});
            py::object plugin_instance = create_plugin_fn(ctx_capsule);

            auto* pp = new PyPlugin{plugin_instance, {}, {}, {}};  // NOLINT(cppcoreguidelines-owning-memory)

            pp->vtable_.destroy = [](void* self) {
                delete static_cast<PyPlugin*>(self);  // NOLINT(cppcoreguidelines-owning-memory)
            };

            pp->vtable_.on_init = [](void* self, const PluginOptions* opts) -> PluginResult {
                auto& python_plugin = *static_cast<PyPlugin*>(self);
                py::dict plugin_options_dict = py::dict{};
                if (opts != nullptr) {
                    for (size_t i = 0; i < opts->count_; ++i) {
                        plugin_options_dict[opts->items_[i].key_] = opts->items_[i].value_;
                    }
                }
                auto py_result = python_plugin.obj_.attr("on_init")(plugin_options_dict);

                return PythonLoader::to_plugin_result(python_plugin, py_result);
            };

            pp->vtable_.on_start = [](void* self) -> PluginResult {
                auto& python_plugin = *static_cast<PyPlugin*>(self);
                auto py_result = python_plugin.obj_.attr("on_start")();
                return PythonLoader::to_plugin_result(python_plugin, py_result);
            };

            pp->vtable_.on_bar = [](void* self, const Bar* bar, const CState* state) -> PluginResult {
                auto& python_plugin = *static_cast<PyPlugin*>(self);

                py::dict state_dict;
                state_dict["cash"] = state->cash_;

                py::list positions_list;
                for (size_t i = 0; i < state->positions_count_; ++i) {
                    py::dict pos;
                    pos["symbol"] = state->positions_[i].symbol_;
                    pos["quantity"] = state->positions_[i].quantity_;
                    pos["average_price"] = state->positions_[i].average_price_;
                    positions_list.append(pos);
                }
                state_dict["positions"] = positions_list;

                py::list trade_history_list;
                for (size_t i = 0; i < state->trade_history_count_; ++i) {
                    py::dict trade;
                    trade["symbol"] = state->trade_history_[i].symbol_;
                    trade["quantity"] = state->trade_history_[i].quantity_;
                    trade["price"] = state->trade_history_[i].price_;
                    trade["timestamp_ns"] = state->trade_history_[i].timestamp_ns_;
                    trade_history_list.append(trade);
                }
                state_dict["trade_history"] = trade_history_list;

                py::list equity_curve_list;
                for (size_t i = 0; i < state->equity_curve_count_; ++i) {
                    py::dict equity;
                    equity["timestamp_ns"] = state->equity_curve_[i].timestamp_ns_;
                    equity["equity"] = state->equity_curve_[i].equity_;
                    equity["return"] = state->equity_curve_[i].return_;
                    equity["max_drawdown"] = state->equity_curve_[i].max_drawdown_;
                    equity["sharpe_ratio"] = state->equity_curve_[i].sharpe_ratio_;
                    equity["sortino_ratio"] = state->equity_curve_[i].sortino_ratio_;
                    equity["calmar_ratio"] = state->equity_curve_[i].calmar_ratio_;
                    equity["tail_ratio"] = state->equity_curve_[i].tail_ratio_;
                    equity["value_at_risk"] = state->equity_curve_[i].value_at_risk_;
                    equity["conditional_value_at_risk"] = state->equity_curve_[i].conditional_value_at_risk_;
                    equity_curve_list.append(equity);
                }
                state_dict["equity_curve"] = equity_curve_list;

                auto py_result = python_plugin.obj_.attr("on_bar")(bar->unix_ts_ns_, bar->open_, bar->high_, bar->low_, bar->close_, bar->volume_, state_dict);

                return PythonLoader::to_plugin_result(python_plugin, py_result);
            };

            pp->vtable_.on_end = [](void* self, const char** json_out) -> PluginResult {
                auto& python_plugin = *static_cast<PyPlugin*>(self);
                auto out = python_plugin.obj_.attr("on_end")().cast<std::string>();
                char* heap = new char[out.size() + 1];  // NOLINT(cppcoreguidelines-owning-memory)
                memcpy(heap, out.c_str(), out.size() + 1);
                *json_out = heap;
                return PluginResult{0, nullptr, .instructions_count_ = 0, .instructions_ = nullptr};
            };

            pp->vtable_.free_string = [](void*, const char* p) {
                delete[] p;  // NOLINT(cppcoreguidelines-owning-memory)
            };

            exp_ = PluginExport{PLUGIN_API_VERSION, pp, pp->vtable_};
        } catch (const py::error_already_set& e) {
            exp_ = PluginExport{0, nullptr, {}};
        } catch (...) {
            exp_ = PluginExport{0, nullptr, {}};
        }
    }

    void PythonLoader::on_init() const {
        if (exp_.api_version_ != PLUGIN_API_VERSION) {
            return;
        }

        if (exp_.vtable_.on_init == nullptr) {
            return;
        }

        auto options = plugin_manifest_->get_options();
        exp_.vtable_.on_init(exp_.instance_, &options);
    }

    PluginResult PythonLoader::on_start() const {
        if (exp_.api_version_ != PLUGIN_API_VERSION) {
            return PluginResult{1, "Invalid API Version", .instructions_count_ = 0, .instructions_ = nullptr};
        }

        if (exp_.vtable_.on_start == nullptr) {
            return PluginResult{1, "Undefined Method on_start", .instructions_count_ = 0, .instructions_ = nullptr};
        }

        return exp_.vtable_.on_start(exp_.instance_);
    }

    PluginResult PythonLoader::on_bar(const http::stock_api::AggregateBarResult& bar, models::BackTestState& state) const {
        if (exp_.api_version_ != PLUGIN_API_VERSION) {
            return PluginResult{1, "Invalid API Version", .instructions_count_ = 0, .instructions_ = nullptr};
        }

        if (exp_.vtable_.on_bar == nullptr) {
            return PluginResult{1, "Undefined Method on_bar", .instructions_count_ = 0, .instructions_ = nullptr};
        }

        Bar plugin_bar = plugins::loaders::to_plugin_bar(bar);
        CState c_state = models::BackTestState::to_c_state(state);
        return exp_.vtable_.on_bar(exp_.instance_, &plugin_bar, &c_state);
    }

    PluginResult PythonLoader::on_end(const char** json_out) const {
        if (exp_.api_version_ != PLUGIN_API_VERSION) {
            return PluginResult{1, "Invalid API Version", .instructions_count_ = 0, .instructions_ = nullptr};
        }

        if (exp_.vtable_.on_end == nullptr) {
            return PluginResult{1, "Undefined Method on_end", .instructions_count_ = 0, .instructions_ = nullptr};
        }

        return exp_.vtable_.on_end(exp_.instance_, json_out);
    }

    void PythonLoader::free_string(const char* str) const {
        if (exp_.api_version_ != PLUGIN_API_VERSION) {
            return;
        }

        if (exp_.vtable_.free_string == nullptr) {
            return;
        }

        return exp_.vtable_.free_string(exp_.instance_, str);
    }

    void PythonLoader::unload_plugin() {
        if (exp_.api_version_ != PLUGIN_API_VERSION) {
            return;
        }

        if (exp_.vtable_.destroy != nullptr && exp_.instance_ != nullptr) {
            exp_.vtable_.destroy(exp_.instance_);
            exp_.instance_ = nullptr;
        }
    }

    PluginExport* PythonLoader::get_plugin_export() const { return &exp_; }

    std::string PythonLoader::get_plugin_name() const { return plugin_manifest_->get_name(); }

    plugins::manifest::HostParams PythonLoader::get_host_params() const { return plugin_manifest_->get_host_params(); }

    PluginResult PythonLoader::to_plugin_result(PyPlugin& python_plugin, py::object& py_result) {
        python_plugin.current_instructions_.clear();
        python_plugin.instruction_strings_.clear();

        int status_code = 0;
        py::list instructions_list;

        try {
            auto result_tuple = py_result.cast<py::tuple>();
            status_code = result_tuple[0].cast<int>();
            instructions_list = result_tuple[1].cast<py::list>();
        } catch (...) {
            status_code = py_result.cast<int>();
        }

        for (auto item : instructions_list) {
            auto inst_dict = item.cast<py::dict>();

            python_plugin.instruction_strings_.push_back(inst_dict["symbol"].cast<std::string>());
            const char* symbol = python_plugin.instruction_strings_.back().c_str();

            python_plugin.instruction_strings_.push_back(inst_dict["action"].cast<std::string>());
            const char* action = python_plugin.instruction_strings_.back().c_str();

            python_plugin.instruction_strings_.push_back(inst_dict.contains("order_type") ? inst_dict["order_type"].cast<std::string>() : "");
            const char* order_type = python_plugin.instruction_strings_.back().c_str();

            CInstruction c_inst;
            c_inst.symbol_ = symbol;
            c_inst.action_ = action;
            c_inst.quantity_ = inst_dict["quantity"].cast<double>();
            c_inst.order_type_ = order_type;
            c_inst.limit_price_ = inst_dict.contains("limit_price") ? inst_dict["limit_price"].cast<int64_t>() : 0;
            c_inst.stop_loss_price_ = inst_dict.contains("stop_loss_price") ? inst_dict["stop_loss_price"].cast<int64_t>() : 0;

            python_plugin.current_instructions_.push_back(c_inst);
        }

        return PluginResult{status_code, status_code == 0 ? nullptr : "on_bar failed",
                            python_plugin.current_instructions_.empty() ? nullptr : python_plugin.current_instructions_.data(),
                            python_plugin.current_instructions_.size()};
    }

}  // namespace plugins::loaders
