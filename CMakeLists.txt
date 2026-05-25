cmake_minimum_required(VERSION 3.15)

project(libsql_cpp CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    endstone
    GIT_REPOSITORY https://github.com/EndstoneMC/endstone.git
    GIT_TAG HEAD
)
FetchContent_MakeAvailable(endstone)

find_package(unofficial-sqlite3 CONFIG REQUIRED)

endstone_add_plugin(${PROJECT_NAME}
    src/core/plugin_main.cpp
    src/manager/connection_manager.cpp
    src/manager/query_manager.cpp
    src/manager/table_manager.cpp
    src/model/query_result.cpp
    src/database.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    unofficial::sqlite3::sqlite3
)
