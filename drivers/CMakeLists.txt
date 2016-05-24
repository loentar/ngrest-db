project (ngrest_db_drivers)

# SQLite3
if (HAS_SQLITE)
    add_subdirectory(sqlite)
endif()

# MySQL
if (HAS_MYSQL)
    add_subdirectory(mysql)
endif()

# Postgres
if (HAS_POSTGRES)
    add_subdirectory(postgres)
endif()

