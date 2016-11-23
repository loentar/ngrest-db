cmake_minimum_required (VERSION 2.8)
project (ngrestdb)

set(NGREST_VERSION "0.1")

include(CheckCXXCompilerFlag)
include(CheckLibraryExists)
include(CheckIncludeFileCXX)

if (NOT "$ENV{NGREST_HOME}" STREQUAL "")
    include_directories("$ENV{NGREST_HOME}/include")
    link_directories("$ENV{NGREST_HOME}/lib")
endif()

check_cxx_compiler_flag(-std=gnu++11 HAS_CXX11)
if (HAS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")
endif()

if (NOT APPLE)
    check_cxx_compiler_flag(-Wl,--no-undefined HAS_NO_UNDEFINED)
    if (HAS_NO_UNDEFINED)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--no-undefined")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined")
        set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined")
    endif()
endif()

check_cxx_compiler_flag(-pedantic HAS_PEDANTIC)
if (HAS_PEDANTIC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic")
endif()

check_cxx_compiler_flag(-Wall HAS_WALL)
if (HAS_WALL)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
endif()

if (MINGW)
    add_definitions(-D_WIN32_WINNT=0x0501)
endif()

if (CMAKE_BUILD_TYPE MATCHES "DEBUG")
    add_definitions(-DDEBUG)
endif()

if (NOT "$ENV{NGREST_HOME}" STREQUAL "")
    # take from NGREST_HOME dir
    set(PROJECT_DEPLOY_DIR "$ENV{NGREST_HOME}")
    set(NGRESTCG_BIN "$ENV{NGREST_HOME}/bin/ngrestcg")
else()
    # somewhere in path..
    set(PROJECT_DEPLOY_DIR ${PROJECT_BINARY_DIR}/deploy)
    set(NGRESTCG_BIN "ngrestcg")
endif()

set(PROJECT_INCLUDE_DIR ${PROJECT_DEPLOY_DIR}/include)
set(PROJECT_SHARE_DIR ${PROJECT_DEPLOY_DIR}/share/ngrest)
set(PROJECT_SERVICES_DIR ${PROJECT_SHARE_DIR}/services)
set(PROJECT_CODEGEN_DIR ${PROJECT_SHARE_DIR}/codegen)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_DEPLOY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_DEPLOY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_DEPLOY_DIR}/bin)
set(TESTS_OUTPUT_DIRECTORY ${PROJECT_DEPLOY_DIR}/tests)
set(EXAMPLES_OUTPUT_DIRECTORY ${PROJECT_DEPLOY_DIR}/examples)

FUNCTION(PREPEND var prefix)
   SET(listVar "")
   FOREACH(f ${ARGN})
      LIST(APPEND listVar "${prefix}/${f}")
   ENDFOREACH(f)
   SET(${var} "${listVar}" PARENT_SCOPE)
ENDFUNCTION(PREPEND)

FUNCTION(CODEGEN_FILES outVar prefix)
   SET(listVar "${prefix}/tableEntities.h")
   FOREACH(file ${ARGN})
      get_filename_component(fileBase "${file}" NAME_WE)
      LIST(APPEND listVar "${prefix}/${fileBase}Entities.h")
      LIST(APPEND listVar "${prefix}/${fileBase}Entities.cpp")
   ENDFOREACH(file)
   SET(${outVar} "${listVar}" PARENT_SCOPE)
ENDFUNCTION(CODEGEN_FILES)

include_directories("${PROJECT_INCLUDE_DIR}")

# detect DB drivers

# SQLite 3
check_include_file_cxx(sqlite3.h HAS_SQLITE)
if (HAS_SQLITE)
    add_definitions(-DHAS_SQLITE)
else()
    message("No sqlite3.h file found. SQLite driver disabled")
endif()

# MySQL
check_include_file_cxx(mysql/mysql.h HAS_MYSQL)
if (HAS_MYSQL)
    add_definitions(-DHAS_MYSQL)
else()
    message("No mysql.h file found. MySQL driver disabled")
endif()

# Postgres
check_include_file_cxx(postgresql/libpq-fe.h HAS_POSTGRES)
if (HAS_POSTGRES)
    add_definitions(-DHAS_POSTGRES)
else()
    message("No libpq-fe.h file found. Postgres driver disabled")
endif()


##
add_subdirectory(common)
add_subdirectory(tools)
add_subdirectory(drivers)

if ("$ENV{WITH_TESTS}" STREQUAL "1")
    add_subdirectory(tests)
endif()

if ("$ENV{WITH_EXAMPLES}" STREQUAL "1")
    add_subdirectory(examples)
endif()
