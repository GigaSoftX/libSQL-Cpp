#include "libsql/manager/query_manager.h"

#include <sqlite3.h>
#include <stdexcept>

namespace libsql {

QueryResult QueryManager::query(sqlite3* db, const std::string& sql, const std::vector<Param>& params) {
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        throw std::runtime_error("SQL prepare failed: " + std::string(sqlite3_errmsg(db)));
    }

    bindParams(stmt, params);

    std::vector<std::string> columns;
    int col_count = sqlite3_column_count(stmt);
    columns.reserve(col_count);
    for (int i = 0; i < col_count; ++i) {
        columns.emplace_back(sqlite3_column_name(stmt, i));
    }

    std::vector<Row> rows;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        Row row;
        for (int i = 0; i < col_count; ++i) {
            int type = sqlite3_column_type(stmt, i);
            Value val;

            switch (type) {
                case SQLITE_NULL:
                    val = nullptr;
                    break;
                case SQLITE_INTEGER:
                    val = static_cast<int64_t>(sqlite3_column_int64(stmt, i));
                    break;
                case SQLITE_FLOAT:
                    val = sqlite3_column_double(stmt, i);
                    break;
                case SQLITE_TEXT: {
                    auto text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i));
                    val = std::string(text ? text : "");
                    break;
                }
                case SQLITE_BLOB: {
                    auto blob = static_cast<const uint8_t*>(sqlite3_column_blob(stmt, i));
                    int size = sqlite3_column_bytes(stmt, i);
                    val = std::vector<uint8_t>(blob, blob + size);
                    break;
                }
                default:
                    val = nullptr;
                    break;
            }

            row[columns[i]] = std::move(val);
        }
        rows.push_back(std::move(row));
    }

    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        throw std::runtime_error("SQL step failed: " + std::string(sqlite3_errmsg(db)));
    }

    return QueryResult(std::move(columns), std::move(rows));
}

int QueryManager::execute(sqlite3* db, const std::string& sql, const std::vector<Param>& params) {
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        throw std::runtime_error("SQL prepare failed: " + std::string(sqlite3_errmsg(db)));
    }

    bindParams(stmt, params);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE && rc != SQLITE_ROW) {
        throw std::runtime_error("SQL execute failed: " + std::string(sqlite3_errmsg(db)));
    }

    return sqlite3_changes(db);
}

bool QueryManager::transaction(sqlite3* db, const std::vector<std::pair<std::string, std::vector<Param>>>& statements) {
    char* err_msg = nullptr;

    int rc = sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, &err_msg);
    if (rc != SQLITE_OK) {
        sqlite3_free(err_msg);
        return false;
    }

    for (const auto& [sql, params] : statements) {
        try {
            execute(db, sql, params);
        } catch (...) {
            sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
            return false;
        }
    }

    rc = sqlite3_exec(db, "COMMIT;", nullptr, nullptr, &err_msg);
    if (rc != SQLITE_OK) {
        sqlite3_free(err_msg);
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return false;
    }

    return true;
}

int64_t QueryManager::lastInsertId(sqlite3* db) const {
    return sqlite3_last_insert_rowid(db);
}

void QueryManager::bindParams(sqlite3_stmt* stmt, const std::vector<Param>& params) {
    for (size_t i = 0; i < params.size(); ++i) {
        int idx = static_cast<int>(i + 1); // SQLite params are 1-indexed

        std::visit([&](const auto& val) {
            using T = std::decay_t<decltype(val)>;

            if constexpr (std::is_same_v<T, std::nullptr_t>) {
                sqlite3_bind_null(stmt, idx);
            } else if constexpr (std::is_same_v<T, int64_t>) {
                sqlite3_bind_int64(stmt, idx, val);
            } else if constexpr (std::is_same_v<T, double>) {
                sqlite3_bind_double(stmt, idx, val);
            } else if constexpr (std::is_same_v<T, std::string>) {
                sqlite3_bind_text(stmt, idx, val.c_str(), static_cast<int>(val.size()), SQLITE_TRANSIENT);
            } else if constexpr (std::is_same_v<T, std::vector<uint8_t>>) {
                sqlite3_bind_blob(stmt, idx, val.data(), static_cast<int>(val.size()), SQLITE_TRANSIENT);
            }
        }, params[i]);
    }
}

} // namespace libsql
