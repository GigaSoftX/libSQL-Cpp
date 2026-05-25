# libSQL-Cpp

A lightweight SQLite3 API plugin for [Endstone](https://github.com/EndstoneMC/endstone) servers.  
Use databases as easily as config files — define schema, get/set records, no SQL required.

---

## Features

- **Config-Like API** — Read and write database records like YAML/JSON: `table.set(key, data)` / `table.get(key)`.
- **Auto Schema Sync** — Declare your columns once. Tables are created or updated automatically.
- **Type-Safe** — Strongly typed values (int64, double, string, blob) with `std::optional` returns.
- **Transactions** — Atomic multi-statement execution with automatic rollback.
- **WAL Mode** — Pre-configured for concurrent read performance out of the box.
- **Thread-Safe** — Mutex-protected connections safe for multi-threaded plugins.
- **Raw SQL Escape Hatch** — Full query/execute access when you need it.

---

## Quick Start

### 1. Link against libSQL-Cpp

```cmake
target_link_libraries(${PROJECT_NAME} PRIVATE libsql_cpp)
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/plugins/libSQL-Cpp/include
)
```

Add `libsql_cpp` to your plugin's `depend` list so it loads first.

### 2. Open a database

```cpp
#include <libsql/core/plugin_main.h>

auto* api = libsql::PluginMain::getInstance();
auto db = api->openDatabase("plugins/my_plugin/data.sqlite");
```

### 3. Define your schema

```cpp
using namespace libsql;

auto players = db.table("players", {
    {"uuid",  Type::TEXT,    Primary | NotNull},
    {"name",  Type::TEXT,    NotNull},
    {"coins", Type::INTEGER, None, "0"},
    {"rank",  Type::TEXT,    None, "'default'"}
});
```

That's it. The table is created if it doesn't exist, or missing columns are added automatically.

### 4. Write data

```cpp
players.set("550e8400-...", {
    {"name",  std::string("Steve")},
    {"coins", int64_t(100)},
    {"rank",  std::string("vip")}
});
```

### 5. Read data

```cpp
// Get full record
auto record = players.get("550e8400-...");
if (record) {
    auto name = std::get<std::string>((*record)["name"]);  // "Steve"
}

// Get a single field directly
auto coins = players.get<int64_t>("550e8400-...", "coins");  // -> std::optional<int64_t>(100)
```

### 6. Query and filter

```cpp
// Check existence
if (players.has("550e8400-...")) { /* ... */ }

// Find by field value
auto vips = players.where("rank", std::string("vip"));

// Get all records
auto everyone = players.all();

// Count
size_t total = players.count();
```

### 7. Remove data

```cpp
players.remove("550e8400-...");  // Delete one record
players.clear();                  // Delete all records
```

### 8. Transactions (atomic operations)

```cpp
db.transaction({
    {"UPDATE players SET coins = coins - ? WHERE uuid = ?", {int64_t(50), std::string(sender)}},
    {"UPDATE players SET coins = coins + ? WHERE uuid = ?", {int64_t(50), std::string(receiver)}}
});
// Both succeed or both rollback
```

### 9. Raw SQL (escape hatch)

```cpp
auto result = db.rawQuery("SELECT * FROM players WHERE coins > ?", {int64_t(500)});
db.rawExecute("DELETE FROM players WHERE coins < ?", {int64_t(0)});
```

---

## API Reference

### Database

| Method | Description |
|--------|-------------|
| `table(name, schema)` | Define/sync a table, returns `Table` handle |
| `operator[](name)` | Get a previously defined table |
| `hasTable(name)` | Check if table exists |
| `dropTable(name)` | Drop a table |
| `rawQuery(sql, params)` | Execute SELECT, returns `QueryResult` |
| `rawExecute(sql, params)` | Execute INSERT/UPDATE/DELETE |
| `transaction(statements)` | Run statements atomically |
| `handle()` | Get raw `sqlite3*` pointer |

### Table

| Method | Description |
|--------|-------------|
| `set(key, record)` | Insert or replace a record |
| `get(key)` | Get full record by primary key |
| `get<T>(key, field)` | Get a single typed field value |
| `has(key)` | Check if record exists |
| `remove(key)` | Delete a record |
| `all()` | Get all records |
| `where(field, value)` | Find records matching a condition |
| `count()` | Count total records |
| `clear()` | Delete all records |
| `name()` | Get table name |

### Column Definition

```cpp
Column(name, type, flags, default_value)
```

| Type | Flags | Example |
|------|-------|---------|
| `Type::TEXT` | `Primary` | `{"id", Type::TEXT, Primary \| NotNull}` |
| `Type::INTEGER` | `NotNull` | `{"age", Type::INTEGER, NotNull}` |
| `Type::REAL` | `Unique` | `{"score", Type::REAL, Unique}` |
| `Type::BLOB` | `None` | `{"data", Type::BLOB}` |

Flags can be combined: `Primary | NotNull | Unique`

### Value Types

| C++ Type | SQLite Type |
|----------|-------------|
| `std::nullptr_t` | NULL |
| `int64_t` | INTEGER |
| `double` | REAL |
| `std::string` | TEXT |
| `std::vector<uint8_t>` | BLOB |

---

## Full Example Plugin

```cpp
#include <endstone/endstone.hpp>
#include <libsql/core/plugin_main.h>

class EconomyPlugin : public endstone::Plugin {
    libsql::Table players_;

public:
    void onEnable() override {
        auto* api = libsql::PluginMain::getInstance();
        auto db = api->openDatabase("plugins/economy/economy.sqlite");

        players_ = db.table("players", {
            {"uuid",    libsql::Type::TEXT,    libsql::Primary | libsql::NotNull},
            {"name",    libsql::Type::TEXT,    libsql::NotNull},
            {"balance", libsql::Type::INTEGER, libsql::None, "1000"}
        });

        getLogger().info("Economy plugin enabled!");
    }

    void addCoins(const std::string& uuid, int64_t amount) {
        auto current = players_.get<int64_t>(uuid, "balance").value_or(0);
        players_.set(uuid, {
            {"balance", current + amount}
        });
    }

    int64_t getBalance(const std::string& uuid) {
        return players_.get<int64_t>(uuid, "balance").value_or(0);
    }
};
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
