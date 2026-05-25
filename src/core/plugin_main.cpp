#include "libsql/core/plugin_main.h"

namespace libsql {

PluginMain* PluginMain::instance_ = nullptr;

void PluginMain::onLoad() {
    instance_ = this;
    getLogger().info("libSQL-Cpp loaded.");
}

void PluginMain::onEnable() {
    getLogger().info("libSQL-Cpp enabled. SQLite API is now available for other plugins.");
}

void PluginMain::onDisable() {
    connection_manager_.closeAll();
    instance_ = nullptr;
    getLogger().info("libSQL-Cpp disabled. All database connections closed.");
}

} // namespace libsql

ENDSTONE_PLUGIN("libsql_cpp", "1.0.0", libsql::PluginMain)
{
    description = "An SQLite3 API plugin for Endstone.";
    authors = {"CYooBin10"};
    load = endstone::PluginLoadOrder::Startup;
}
