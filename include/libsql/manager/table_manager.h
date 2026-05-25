#pragma once

#include <string>
#include <vector>

struct sqlite3;

namespace libsql {

// Column definition for table creation
struct ColumnDef {
    std::string name;
    std::string type;           // TEXT, INTEGER, REAL, BLOB
    bool primary_key = false;
    bool not_null = false;
    bool unique = false;
    std::string default_value;  // Empty means no default
};

// Manages table creation, schema inspection, and migrations
class TableManager {
public:
    TableManager() = default;

    // Create a table if it doesn't exist
    bool createTable(sqlite3* db, const std::string& table_name, const std::vector<ColumnDef>& columns);

    // Drop a table
    bool dropTable(sqlite3* db, const std::string& table_name);

    // Check if a table exists
    [[nodiscard]] bool tableExists(sqlite3* db, const std::string& table_name) const;

    // Get existing column names for a table
    [[nodiscard]] std::vector<std::string> getColumnNames(sqlite3* db, const std::string& table_name) const;

    // Add a column to an existing table (SQLite ALTER TABLE ADD COLUMN)
    bool addColumn(sqlite3* db, const std::string& table_name, const ColumnDef& column);

    // Sync schema: creates table or adds missing columns automatically
    bool syncSchema(sqlite3* db, const std::string& table_name, const std::vector<ColumnDef>& columns);

private:
    std::string buildColumnSql(const ColumnDef& col) const;
};

} // namespace libsql
