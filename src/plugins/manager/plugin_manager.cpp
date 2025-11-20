#include "plugin_manager.hpp"

#include <filesystem>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include "../loaders/interface.hpp"
#include "../loaders/native_loader.hpp"
#include "../loaders/python_loader.hpp"
#include "../manifest/manifest.hpp"

namespace plugins::manager {
    PluginManager::PluginManager(const std::vector<std::string>& plugin_names) : plugin_names_(plugin_names) { ctx_.api_version_ = PLUGIN_API_VERSION; }

    void PluginManager::load_plugins_from_dir(const std::filesystem::path& root) {
        if (plugin_names_.empty()) {
            throw std::runtime_error("No plugin names provided to load");
        }

        for (const auto& dir : std::filesystem::directory_iterator{root}) {
            if (!dir.is_directory()) {
                continue;
            }

            std::unique_ptr<plugins::manifest::PluginManifest> plugin_manifest = std::make_unique<plugins::manifest::PluginManifest>();

            auto document = plugin_manifest->load_from_file(dir.path());

            plugin_manifest->parse_json(document);

            if (!plugin_manifest->is_one_of(plugin_names_)) {
                continue;
            }

            auto loader = [&]() -> std::unique_ptr<plugins::loaders::IPluginLoader> {
                if (plugin_manifest->is_python()) {
                    return std::make_unique<plugins::loaders::PythonLoader>(std::move(plugin_manifest));
                }

                if (plugin_manifest->is_native()) {
                    return std::make_unique<plugins::loaders::NativeLoader>(std::move(plugin_manifest));
                }

                throw std::runtime_error("Unknown plugin kind in manifest");
            }();

            loader->load_plugin(ctx_);

            loader->on_init();

            plugin_map_by_name_.emplace(loader->get_plugin_name(), std::move(loader));
        }
    }

    PluginManager::~PluginManager() {
        for (auto& [plugin_name, loader] : plugin_map_by_name_) {
            loader->unload_plugin();
        }
    }
}  // namespace plugins::manager
