project (ngrestdb_test1 CXX)

set(NGRESTDB_TEST1_HEADERS test1.h)

set(PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(NGRESTDB_TEST1_CODEGEN_DIR "${PROJECT_BINARY_DIR}/codegen")

PREPEND(NGRESTDB_TEST1_HEADERS_PATHS ${PROJECT_SOURCE_DIR} ${NGRESTDB_TEST1_HEADERS})

CODEGEN_FILES(NGRESTDB_TEST1_CODEGEN_SOURCES ${NGRESTDB_TEST1_CODEGEN_DIR} ${NGRESTDB_TEST1_HEADERS})

add_custom_command(OUTPUT ${NGRESTDB_TEST1_CODEGEN_SOURCES}
    COMMAND ${NGRESTCG_BIN} -e -i "${PROJECT_SOURCE_DIR}" -o ${NGRESTDB_TEST1_CODEGEN_DIR} -t dbentity ${NGRESTDB_TEST1_HEADERS} -x
    DEPENDS ${NGRESTDB_TEST1_HEADERS_PATHS}
)

file(GLOB NGRESTDB_TEST1_SOURCES ${PROJECT_SOURCE_DIR}/*.cpp)

list(APPEND NGRESTDB_TEST1_SOURCES ${NGRESTDB_TEST1_CODEGEN_SOURCES})

include_directories(${PROJECT_SOURCE_DIR} ${NGRESTDB_TEST1_CODEGEN_DIR})

add_executable(ngrestdb_test1 ${NGRESTDB_TEST1_SOURCES})

set_target_properties(ngrestdb_test1 PROPERTIES PREFIX "")
set_target_properties(ngrestdb_test1 PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SERVICES_DIR}"
)

target_link_libraries(ngrestdb_test1 ngrestutils ngrestdbcommon)

if (HAS_SQLITE)
    target_link_libraries(ngrestdb_test1 ngrestdbsqlite)
endif()
if (HAS_MYSQL)
    target_link_libraries(ngrestdb_test1 ngrestdbmysql)
endif()
if (HAS_POSTGRES)
    target_link_libraries(ngrestdb_test1 ngrestdbpostgres)
endif()
