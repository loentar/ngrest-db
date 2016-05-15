project (ngrest_db_drivers)

include(CheckLibraryExists)
include(CheckIncludeFileCXX)


# SQLite3
if (HAS_SQLITE)
    add_subdirectory(sqlite)
endif()

