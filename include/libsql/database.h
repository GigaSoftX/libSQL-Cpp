#pragma once

#include <string>
#include <vector>
#include <variant>
#include <optional>
#include <unordered_map>
#include <initializer_list>

#include "libsql/manager/connection_manager.h"
#include "libsql/manager/query_manager.h"
#include "libsql/manager/table_manager.h"
#include "libsql/model/query_result.h"

namespace libsql {

// Value type for the config-like API
using FieldValue = std::variant<std::nullptr_t, int64_t, double, std::string, std::vector<uint8_t>>;

// A record is a map of field name -> value
using Record = std::unordered_map<std::string, FieldValue>;

// Column constraint flags
enum ColumnFlag : uint8_t {
    None     = 0,
    Primary  = 1 << 0,
    NotNull  = 1 << 1,
    Unique   = 1 << 2
};

inline ColumnFlag operator|(ColumnFlag a, ColumnFlag b) {
    return static_cast<ColumnFlag>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline bool operator&(ColumnFlag a, ColumnFlag b) {
    return (static_cast<uint8_t>(a) & static_cast<uint8_t>(b)) != 0;
}

// Column type enum for clean declaration
enum class Type { TEXT, INTEGER, REAL, BLOB };

// Schema column definition (config-style)
struct Column {
    std::string name;
    Type type;
    ColumnFlag flags = None;
    std::string default_value;

    Column(std::string name, Type type, ColumnFlag flags = None, std::string default_value = "")
        : name(std::move(name)), type(type), flags(flags), default_value(std::move(default_value)) {}
};

// Helper to set default value inline
inline Column Default(std::string name, Type type, ColumnFlag flags, std::string value) {
    return Column(std::move(name), type, flags, std::move(value));
}

// Represents a single table with config-like get/set/remove operations
class Table {
public:
    Table(sqlite3* db, QueryManager& qm, const std::string& table_name, const std::string& primary_key);

    // Set a record by primary key (insert or replace)
    void set(const std::string& key, const Record& data);

    // Get a record by primary key
    [[nodiscard]] std::optional<Record> get(const std::string& key) const;

    // Get a specific field from a record
    template <typename T>
    [[nodiscard]] std::optional<T> get(const std::string& key, const std::string& field) const {
        auto record = get(key);
        if (!record) return std::nullopt;
        auto it = record->find(field);
        if (it == record->end()) return std::nullopt;
        if (auto* val = std::get_if<T>(&it->second)) return *val;
        return std::nullopt;
    }

    // Check if a record exists
    [[nodiscard]] bool has(const std::string& key) const;

    // Remove a record by primary key
    bool remove(const std::string& key);

    // Get all records
    [[nodiscard]] std::vector<Record> all() const;

    // Get all records matching a condition (field = value)
    [[nodiscard]] std::vector<Record> where(const std::string& field, const FieldValue& value) const;

    // Count total records
    [[nodiscard]] size_t count() const;

    // Clear all records
    void clear();

    // Get table name
    [[nodiscard]] const std::string& name() const { return table_name_; }

private:
    sqlite3* db_;
    QueryManager& query_manager_;
    std::string table_name_;
    std::string primary_key_;

    static Param toParam(const FieldValue& val);
};

// High-level database handle with config-like API
class Database {
public:
    Database(const std::string& db_path, ConnectionManager& cm, QueryManager& qm, TableManager& tm);
    ~Database() = default;

    // Define and sync a table schema, returns a Table handle for get/set operations
    Table table(const std::string& name, const std::vector<Column>& schema);

    // Get a previously defined table handle
    [[nodiscard]] Table operator[](const std::string& name);

    // Check if a table exists
    [[nodiscard]] bool hasTable(const std::string& name) const;

    // Drop a table
    bool dropTable(const std::string& name);

    // Run a raw query (escape hatch)
    [[nodiscard]] QueryResult rawQuery(const std::string& sql, const std::vector<Param>& params = {});

    // Run a raw execute (escape hatch)
    int rawExecute(const std::string& sql, const std::vector<Param>& params = {});

    // Run a transaction (escape hatch)
    bool transaction(const std::vector<std::pair<std::string, std::vector<Param>>>& statements);

    // Get the underlying sqlite3 handle
    [[nodiscard]] sqlite3* handle() const { return db_; }

private:
    sqlite3* db_;
    ConnectionManager& connection_manager_;
    QueryManager& query_manager_;
    TableManager& table_manager_;
    std::unordered_map<std::string, std::string> primary_keys_; // table -> primary key name
};

} // namespace libsql
