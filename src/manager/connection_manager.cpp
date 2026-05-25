#include "libsql/manager/connection_manager.h"

#include <sqlite3.h>
#include <stdexcept>

namespace libsql {

ConnectionManager::~ConnectionManager() {
    closeAll();
}

sqlite3* ConnectionManager::open(const std::string& db_path) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = connections_.find(db_path);
    if (it != connections_.end()) {
        return it->second;
    }

    sqlite3* db = nullptr;
    int rc = sqlite3_open(db_path.c_str(), &db);
    if (rc != SQLITE_OK) {
        std::string err = sqlite3_errmsg(db);
        sqlite3_close(db);
        throw std::runtime_error("Failed to open database '" + db_path + "': " + err);
    }

    configureConnection(db);
    connections_[db_path] = db;
    return db;
}

void ConnectionManager::close(const std::string& db_path) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = connections_.find(db_path);
    if (it != connections_.end()) {
        sqlite3_close(it->second);
        connections_.erase(it);
    }
}

void ConnectionManager::closeAll() {
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto& [path, db] : connections_) {
        sqlite3_close(db);
    }
    connections_.clear();
}

bool ConnectionManager::isOpen(const std::string& db_path) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return connections_.find(db_path) != connections_.end();
}

sqlite3* ConnectionManager::getHandle(const std::string& db_path) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = connections_.find(db_path);
    if (it != connections_.end()) {
        return it->second;
    }
    return nullptr;
}

void ConnectionManager::configureConnection(sqlite3* db) {
    // Enable WAL mode for better concurrent read performance
    sqlite3_exec(db, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
    // Enable foreign keys
    sqlite3_exec(db, "PRAGMA foreign_keys=ON;", nullptr, nullptr, nullptr);
    // Set busy timeout to 5 seconds
    sqlite3_busy_timeout(db, 5000);
}

} // namespace libsql
