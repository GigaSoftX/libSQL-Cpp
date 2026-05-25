#include "libsql/model/query_result.h"

namespace libsql {

QueryResult::QueryResult(std::vector<std::string> columns, std::vector<Row> rows, int affected_rows)
    : columns_(std::move(columns)), rows_(std::move(rows)), affected_rows_(affected_rows) {}

bool QueryResult::empty() const {
    return rows_.empty();
}

size_t QueryResult::size() const {
    return rows_.size();
}

int QueryResult::getAffectedRows() const {
    return affected_rows_;
}

const std::vector<std::string>& QueryResult::getColumns() const {
    return columns_;
}

const std::vector<Row>& QueryResult::getRows() const {
    return rows_;
}

std::optional<Row> QueryResult::getRow(size_t index) const {
    if (index >= rows_.size()) return std::nullopt;
    return rows_[index];
}

} // namespace libsql
