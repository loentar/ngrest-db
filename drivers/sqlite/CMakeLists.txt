project (ngrestdbsqlite CXX)

set (PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

FILE(GLOB NGRESTDBSQLITE_SOURCES ${PROJECT_SOURCE_DIR}/*.cpp)
FILE(GLOB NGRESTDBSQLITE_HEADERS ${PROJECT_SOURCE_DIR}/*.h)

file(COPY ${NGRESTDBSQLITE_HEADERS} DESTINATION ${PROJECT_INCLUDE_DIR}/ngrest/db/)

add_definitions(-DNGRESTDB_COMMON_DLL_EXPORTS)

add_library(ngrestdbsqlite SHARED ${NGRESTDBSQLITE_SOURCES})

target_link_libraries(ngrestdbsqlite ngrestutils ngrestdbcommon sqlite3)
