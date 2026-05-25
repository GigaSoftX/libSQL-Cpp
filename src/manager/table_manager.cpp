#include "libsql/manager/table_manager.h"

#include <sqlite3.h>
#include <algorithm>
#include <sstream>

namespace libsql {

bool TableManager::createTable(sqlite3* db, const std::string& table_name, const std::vector<ColumnDef>& columns) {
    if (columns.empty()) return false;

    std::ostringstream sql;
    sql << "CREATE TABLE IF NOT EXISTS \"" << table_name << "\" (";

    for (size_t i = 0; i < columns.size(); ++i) {
        if (i > 0) sql << ", ";
        sql << buildColumnSql(columns[i]);
    }
    sql << ");";

    char* err_msg = nullptr;
    int rc = sqlite3_exec(db, sql.str().c_str(), nullptr, nullptr, &err_msg);
    if (rc != SQLITE_OK) {
        sqlite3_free(err_msg);
        return false;
    }
    return true;
}

bool TableManager::dropTable(sqlite3* db, const std::string& table_name) {
    std::string sql = "DROP TABLE IF EXISTS \"" + table_name + "\";";
    char* err_msg = nullptr;
    int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &err_msg);
    if (rc != SQLITE_OK) {
        sqlite3_free(err_msg);
        return false;
    }
    return true;
}

bool TableManager::tableExists(sqlite3* db, const std::string& table_name) const {
    std::string sql = "SELECT count(*) FROM sqlite_master WHERE type='table' AND name=?;";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, table_name.c_str(), -1, SQLITE_TRANSIENT);

    bool exists = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        exists = sqlite3_column_int(stmt, 0) > 0;
    }

    sqlite3_finalize(stmt);
    return exists;
}

std::vector<std::string> TableManager::getColumnNames(sqlite3* db, const std::string& table_name) const {
    std::vector<std::string> names;
    std::string sql = "PRAGMA table_info(\"" + table_name + "\");";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return names;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        auto name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        if (name) names.emplace_back(name);
    }

    sqlite3_finalize(stmt);
    return names;
}

bool TableManager::addColumn(sqlite3* db, const std::string& table_name, const ColumnDef& column) {
    std::string sql = "ALTER TABLE \"" + table_name + "\" ADD COLUMN " + buildColumnSql(column) + ";";
    char* err_msg = nullptr;
    int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &err_msg);
    if (rc != SQLITE_OK) {
        sqlite3_free(err_msg);
        return false;
    }
    return true;
}

bool TableManager::syncSchema(sqlite3* db, const std::string& table_name, const std::vector<ColumnDef>& columns) {
    if (!tableExists(db, table_name)) {
        return createTable(db, table_name, columns);
    }

    // Add any missing columns
    auto existing = getColumnNames(db, table_name);
    for (const auto& col : columns) {
        auto it = std::find(existing.begin(), existing.end(), col.name);
        if (it == existing.end()) {
            if (!addColumn(db, table_name, col)) {
                return false;
            }
        }
    }
    return true;
}

std::string TableManager::buildColumnSql(const ColumnDef& col) const {
    std::ostringstream sql;
    sql << "\"" << col.name << "\" " << col.type;

    if (col.primary_key) sql << " PRIMARY KEY";
    if (col.not_null) sql << " NOT NULL";
    if (col.unique) sql << " UNIQUE";
    if (!col.default_value.empty()) sql << " DEFAULT " << col.default_value;

    return sql.str();
}

} // namespace libsql
