#pragma once

#include <string>
#include <vector>
#include <variant>

#include "libsql/model/query_result.h"

struct sqlite3;

namespace libsql {

// Parameter type for binding values to prepared statements
using Param = std::variant<std::nullptr_t, int64_t, double, std::string, std::vector<uint8_t>>;

// Executes SQL queries with parameter binding and result parsing
class QueryManager {
public:
    QueryManager() = default;

    // Execute a query that returns rows (SELECT)
    [[nodiscard]] QueryResult query(sqlite3* db, const std::string& sql, const std::vector<Param>& params = {});

    // Execute a statement that modifies data (INSERT, UPDATE, DELETE)
    [[nodiscard]] int execute(sqlite3* db, const std::string& sql, const std::vector<Param>& params = {});

    // Execute multiple statements in a transaction
    [[nodiscard]] bool transaction(sqlite3* db, const std::vector<std::pair<std::string, std::vector<Param>>>& statements);

    // Get the last inserted row ID
    [[nodiscard]] int64_t lastInsertId(sqlite3* db) const;

private:
    void bindParams(struct sqlite3_stmt* stmt, const std::vector<Param>& params);
};

} // namespace libsql
