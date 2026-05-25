# libSQL-Cpp

A lightweight SQLite3 API plugin for [Endstone](https://github.com/EndstoneMC/endstone) servers.  
Provides a clean, ready-to-use database layer so other C++ plugins don't have to manage SQLite themselves.

---

## Features

- **Connection Management** — Open/close databases with WAL mode, foreign keys, and busy timeout pre-configured.
- **Query Execution** — Parameterized queries with type-safe binding (int64, double, string, blob, null).
- **Table Management** — Auto-create tables, sync schema changes, inspect columns at runtime.
- **Transactions** — Execute multiple statements atomically with automatic rollback on failure.
- **Thread-Safe** — Mutex-protected connection pool safe for multi-threaded access.

---

## Quick Start

### 1. Link against libSQL-Cpp

In your plugin's `CMakeLists.txt`:

```cmake
target_link_libraries(${PROJECT_NAME} PRIVATE libsql_cpp)
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/plugins/libSQL-Cpp/include
)
```

Add `libsql_cpp` to your plugin's `depend` list so it loads first.

### 2. Get the API instance

```cpp
#include <libsql/core/plugin_main.h>

auto* api = libsql::PluginMain::getInstance();
```

### 3. Open a database

```cpp
auto* db = api->getConnectionManager().open("plugins/my_plugin/data.sqlite");
```

### 4. Define and sync your schema

```cpp
using libsql::ColumnDef;

std::vector<ColumnDef> schema = {
    {"uuid",  "TEXT",    .primary_key = true, .not_null = true},
    {"name",  "TEXT",    .not_null = true},
    {"coins", "INTEGER", .default_value = "0"},
    {"rank",  "TEXT",    .default_value = "'default'"}
};

api->getTableManager().syncSchema(db, "players", schema);
```

`syncSchema` creates the table if missing, or adds new columns if your schema evolves.

### 5. Insert data

```cpp
api->getQueryManager().execute(db,
    "INSERT OR REPLACE INTO players (uuid, name, coins) VALUES (?, ?, ?)",
    {std::string("550e8400-..."), std::string("Steve"), int64_t(100)}
);
```

### 6. Query data

```cpp
auto result = api->getQueryManager().query(db,
    "SELECT name, coins FROM players WHERE uuid = ?",
    {std::string("550e8400-...")}
);

if (!result.empty()) {
    auto name  = result.get<std::string>(0, "name");   // -> "Steve"
    auto coins = result.get<int64_t>(0, "coins");      // -> 100
}
```

### 7. Transactions

```cpp
bool ok = api->getQueryManager().transaction(db, {
    {"UPDATE players SET coins = coins - ? WHERE uuid = ?", {int64_t(50), std::string(sender_uuid)}},
    {"UPDATE players SET coins = coins + ? WHERE uuid = ?", {int64_t(50), std::string(receiver_uuid)}}
});
// Both succeed or both rollback
```

---

## API Reference

| Manager | Method | Description |
|---------|--------|-------------|
| `ConnectionManager` | `open(path)` | Open or reuse a database connection |
| | `close(path)` | Close a specific connection |
| | `closeAll()` | Close all connections |
| | `isOpen(path)` | Check if a database is open |
| | `getHandle(path)` | Get raw `sqlite3*` for advanced use |
| `QueryManager` | `query(db, sql, params)` | Execute SELECT, returns `QueryResult` |
| | `execute(db, sql, params)` | Execute INSERT/UPDATE/DELETE, returns affected rows |
| | `transaction(db, statements)` | Run multiple statements atomically |
| | `lastInsertId(db)` | Get last auto-increment ID |
| `TableManager` | `createTable(db, name, columns)` | Create table if not exists |
| | `dropTable(db, name)` | Drop a table |
| | `tableExists(db, name)` | Check table existence |
| | `getColumnNames(db, name)` | List existing columns |
| | `addColumn(db, name, column)` | Add a single column |
| | `syncSchema(db, name, columns)` | Create or update table to match schema |

### Parameter Types

Bind values using `libsql::Param`:

| C++ Type | SQLite Type |
|----------|-------------|
| `std::nullptr_t` | NULL |
| `int64_t` | INTEGER |
| `double` | REAL |
| `std::string` | TEXT |
| `std::vector<uint8_t>` | BLOB |

### QueryResult

```cpp
result.empty()                        // No rows?
result.size()                         // Row count
result.getColumns()                   // Column names
result.getRow(index)                  // Get a row as map
result.get<std::string>(row, "col")   // Typed value access
result.get<int64_t>(row, "col")       // Returns std::optional<T>

for (const auto& row : result) { }   // Range-based iteration
```

---

## Build

Requires **C++20**, **CMake 3.15+**, and **vcpkg** with `sqlite3` installed.

```bash
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[vcpkg-root]/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
```

Output: `endstone_libsql_cpp.dll` (Windows) or `endstone_libsql_cpp.so` (Linux).  
Place it in your server's `plugins/` folder.

---

## Info

| | |
|---|---|
| **Author** | CYooBin10 |
| **Version** | 1.0.0 |
| **Load Order** | Startup (available before other plugins enable) |
| **License** | MIT |
