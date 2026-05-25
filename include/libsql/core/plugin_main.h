#pragma once

#include <endstone/endstone.hpp>
#include <memory>

#include "libsql/manager/connection_manager.h"
#include "libsql/manager/query_manager.h"
#include "libsql/manager/table_manager.h"
#include "libsql/model/query_result.h"
#include "libsql/database.h"

namespace libsql {

// Main plugin class that provides SQLite API to other Endstone plugins
class PluginMain : public endstone::Plugin {
public:
    void onLoad() override;
    void onEnable() override;
    void onDisable() override;

    // Config-like API: open a database and use it like YAML/JSON
    [[nodiscard]] Database openDatabase(const std::string& path) {
        return Database(path, connection_manager_, query_manager_, table_manager_);
    }

    // Access the shared managers (advanced usage)
    [[nodiscard]] ConnectionManager& getConnectionManager() { return connection_manager_; }
    [[nodiscard]] QueryManager& getQueryManager() { return query_manager_; }
    [[nodiscard]] TableManager& getTableManager() { return table_manager_; }

    // Convenience: get the singleton instance
    static PluginMain* getInstance() { return instance_; }

private:
    static PluginMain* instance_;

    ConnectionManager connection_manager_;
    QueryManager query_manager_;
    TableManager table_manager_;
};

} // namespace libsql
