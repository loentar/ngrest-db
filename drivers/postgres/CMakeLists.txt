project (ngrestdbpostgres CXX)

set (PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

FILE(GLOB NGRESTDBPOSTGRES_SOURCES ${PROJECT_SOURCE_DIR}/*.cpp)
FILE(GLOB NGRESTDBPOSTGRES_HEADERS ${PROJECT_SOURCE_DIR}/*.h)

file(COPY ${NGRESTDBPOSTGRES_HEADERS} DESTINATION ${PROJECT_INCLUDE_DIR}/ngrest/db/)

add_definitions(-DNGRESTDB_COMMON_DLL_EXPORTS)

add_library(ngrestdbpostgres SHARED ${NGRESTDBPOSTGRES_SOURCES})

target_link_libraries(ngrestdbpostgres ngrestutils ngrestdbcommon pq)
