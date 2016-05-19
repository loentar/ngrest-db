project (ngrestdbmysql CXX)

set (PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

FILE(GLOB NGRESTDBMYSQL_SOURCES ${PROJECT_SOURCE_DIR}/*.cpp)
FILE(GLOB NGRESTDBMYSQL_HEADERS ${PROJECT_SOURCE_DIR}/*.h)

file(COPY ${NGRESTDBMYSQL_HEADERS} DESTINATION ${PROJECT_INCLUDE_DIR}/ngrest/db/)

add_definitions(-DNGRESTDB_COMMON_DLL_EXPORTS)

add_library(ngrestdbmysql SHARED ${NGRESTDBMYSQL_SOURCES})

target_link_libraries(ngrestdbmysql ngrestutils ngrestdbcommon mysqlclient)
