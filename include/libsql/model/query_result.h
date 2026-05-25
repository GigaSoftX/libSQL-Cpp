#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <variant>

namespace libsql {

// A single cell value in a query result
using Value = std::variant<std::nullptr_t, int64_t, double, std::string, std::vector<uint8_t>>;

// A single row: column name -> value
using Row = std::unordered_map<std::string, Value>;

// Represents the result of a SQL query
class QueryResult {
public:
    QueryResult() = default;
    QueryResult(std::vector<std::string> columns, std::vector<Row> rows, int affected_rows = 0);

    // Check if the result has any rows
    [[nodiscard]] bool empty() const;

    // Number of rows returned
    [[nodiscard]] size_t size() const;

    // Number of rows affected by INSERT/UPDATE/DELETE
    [[nodiscard]] int getAffectedRows() const;

    // Get column names
    [[nodiscard]] const std::vector<std::string>& getColumns() const;

    // Get all rows
    [[nodiscard]] const std::vector<Row>& getRows() const;

    // Get a specific row by index
    [[nodiscard]] std::optional<Row> getRow(size_t index) const;

    // Get a typed value from a specific row and column
    template <typename T>
    [[nodiscard]] std::optional<T> get(size_t row, const std::string& column) const {
        if (row >= rows_.size()) return std::nullopt;
        auto it = rows_[row].find(column);
        if (it == rows_[row].end()) return std::nullopt;
        if (auto* val = std::get_if<T>(&it->second)) {
            return *val;
        }
        return std::nullopt;
    }

    // Iterator support
    [[nodiscard]] auto begin() const { return rows_.begin(); }
    [[nodiscard]] auto end() const { return rows_.end(); }

private:
    std::vector<std::string> columns_;
    std::vector<Row> rows_;
    int affected_rows_ = 0;
};

} // namespace libsql
