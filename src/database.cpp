#include "libsql/database.h"

#include <sstream>
#include <stdexcept>

namespace libsql {

// --- Table implementation ---

Table::Table(sqlite3* db, QueryManager& qm, const std::string& table_name, const std::string& primary_key)
    : db_(db), query_manager_(qm), table_name_(table_name), primary_key_(primary_key) {}

void Table::set(const std::string& key, const Record& data) {
    std::vector<std::string> columns = {primary_key_};
    std::vector<Param> params = {key};

    for (const auto& [field, value] : data) {
        if (field == primary_key_) continue;
        columns.push_back(field);
        params.push_back(toParam(value));
    }

    std::ostringstream sql;
    sql << "INSERT OR REPLACE INTO \"" << table_name_ << "\" (";
    for (size_t i = 0; i < columns.size(); ++i) {
        if (i > 0) sql << ", ";
        sql << "\"" << columns[i] << "\"";
    }
    sql << ") VALUES (";
    for (size_t i = 0; i < columns.size(); ++i) {
        if (i > 0) sql << ", ";
        sql << "?";
    }
    sql << ");";

    query_manager_.execute(db_, sql.str(), params);
}

std::optional<Record> Table::get(const std::string& key) const {
    std::string sql = "SELECT * FROM \"" + table_name_ + "\" WHERE \"" + primary_key_ + "\" = ? LIMIT 1;";
    auto result = query_manager_.query(db_, sql, {key});

    if (result.empty()) return std::nullopt;

    return result.getRows()[0];
}

bool Table::has(const std::string& key) const {
    std::string sql = "SELECT 1 FROM \"" + table_name_ + "\" WHERE \"" + primary_key_ + "\" = ? LIMIT 1;";
    auto result = query_manager_.query(db_, sql, {key});
    return !result.empty();
}

bool Table::remove(const std::string& key) {
    std::string sql = "DELETE FROM \"" + table_name_ + "\" WHERE \"" + primary_key_ + "\" = ?;";
    return query_manager_.execute(db_, sql, {key}) > 0;
}

std::vector<Record> Table::all() const {
    std::string sql = "SELECT * FROM \"" + table_name_ + "\";";
    auto result = query_manager_.query(db_, sql);
    return result.getRows();
}

std::vector<Record> Table::where(const std::string& field, const FieldValue& value) const {
    std::string sql = "SELECT * FROM \"" + table_name_ + "\" WHERE \"" + field + "\" = ?;";
    auto result = query_manager_.query(db_, sql, {toParam(value)});
    return result.getRows();
}

size_t Table::count() const {
    std::string sql = "SELECT COUNT(*) FROM \"" + table_name_ + "\";";
    auto result = query_manager_.query(db_, sql);
    if (result.empty()) return 0;
    auto val = result.get<int64_t>(0, "COUNT(*)");
    return val.value_or(0);
}

void Table::clear() {
    std::string sql = "DELETE FROM \"" + table_name_ + "\";";
    query_manager_.execute(db_, sql);
}

Param Table::toParam(const FieldValue& val) {
    return std::visit([](const auto& v) -> Param { return v; }, val);
}

// --- Database implementation ---

Database::Database(const std::string& db_path, ConnectionManager& cm, QueryManager& qm, TableManager& tm)
    : connection_manager_(cm), query_manager_(qm), table_manager_(tm) {
    db_ = cm.open(db_path);
}

Table Database::table(const std::string& name, const std::vector<Column>& schema) {
    std::vector<ColumnDef> col_defs;
    std::string pk_name;

    for (const auto& col : schema) {
        ColumnDef def;
        def.name = col.name;
        def.primary_key = col.flags & Primary;
        def.not_null = col.flags & NotNull;
        def.unique = col.flags & Unique;
        def.default_value = col.default_value;

        switch (col.type) {
            case Type::TEXT:    def.type = "TEXT"; break;
            case Type::INTEGER: def.type = "INTEGER"; break;
            case Type::REAL:    def.type = "REAL"; break;
            case Type::BLOB:    def.type = "BLOB"; break;
        }

        if (def.primary_key) pk_name = def.name;
        col_defs.push_back(std::move(def));
    }

    if (pk_name.empty()) {
        throw std::runtime_error("Table '" + name + "' must have a primary key column.");
    }

    table_manager_.syncSchema(db_, name, col_defs);
    primary_keys_[name] = pk_name;

    return Table(db_, query_manager_, name, pk_name);
}

Table Database::operator[](const std::string& name) {
    auto it = primary_keys_.find(name);
    if (it == primary_keys_.end()) {
        throw std::runtime_error("Table '" + name + "' has not been defined. Call table() first.");
    }
    return Table(db_, query_manager_, name, it->second);
}

bool Database::hasTable(const std::string& name) const {
    return table_manager_.tableExists(db_, name);
}

bool Database::dropTable(const std::string& name) {
    primary_keys_.erase(name);
    return table_manager_.dropTable(db_, name);
}

QueryResult Database::rawQuery(const std::string& sql, const std::vector<Param>& params) {
    return query_manager_.query(db_, sql, params);
}

int Database::rawExecute(const std::string& sql, const std::vector<Param>& params) {
    return query_manager_.execute(db_, sql, params);
}

bool Database::transaction(const std::vector<std::pair<std::string, std::vector<Param>>>& statements) {
    return query_manager_.transaction(db_, statements);
}

} // namespace libsql
