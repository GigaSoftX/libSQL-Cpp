#pragma once

#include <string>
#include <memory>
#include <mutex>
#include <unordered_map>

struct sqlite3;

namespace libsql {

// Manages SQLite database connections (open, close, WAL mode)
class ConnectionManager {
public:
    ConnectionManager() = default;
    ~ConnectionManager();

    ConnectionManager(const ConnectionManager&) = delete;
    ConnectionManager& operator=(const ConnectionManager&) = delete;

    // Open or retrieve an existing connection to a database file
    // Path is relative to the plugin's data folder
    sqlite3* open(const std::string& db_path);

    // Close a specific database connection
    void close(const std::string& db_path);

    // Close all open connections
    void closeAll();

    // Check if a database is currently open
    [[nodiscard]] bool isOpen(const std::string& db_path) const;

    // Get the raw sqlite3 handle for advanced usage
    [[nodiscard]] sqlite3* getHandle(const std::string& db_path) const;

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, sqlite3*> connections_;

    void configureConnection(sqlite3* db);
};

} // namespace libsql
