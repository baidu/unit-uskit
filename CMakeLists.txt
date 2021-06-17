cmake_minimum_required(VERSION 2.8.10)
project(uskit C CXX)

execute_process(
    COMMAND bash -c "find ${CMAKE_SOURCE_DIR}/third_party/brpc -type d -regex \".*output/include$\" | xargs dirname | tr -d '\n'"
    OUTPUT_VARIABLE BRPC_PATH
)

execute_process(
    COMMAND bash -c "find ${CMAKE_SOURCE_DIR}/third_party/boost -type d -regex \".*output/include$\" | xargs dirname | tr -d '\n'"
    OUTPUT_VARIABLE BOOST_PATH
)

set(CMAKE_PREFIX_PATH ${BRPC_PATH} ${BOOST_PATH})

include(FindThreads)
include(FindProtobuf)
include(FindBISON 3.0.5)
include(FindFLEX 2.6.4)

protobuf_generate_cpp(PROTO_SRC PROTO_HEADER proto/us.proto proto/config.proto)
# include PROTO_HEADER
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(
    src
    third_party/rapidjson/include
    )
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    include_directories(/usr/local/opt/flex/include) # Homebrew flex
endif()


BISON_TARGET(EXPR_PARSER src/expression/parser.yy ${CMAKE_CURRENT_BINARY_DIR}/parser.cpp
             COMPILE_FLAGS "--defines=${CMAKE_CURRENT_BINARY_DIR}/parser.h")
FLEX_TARGET(EXPR_LEXER src/expression/lexer.ll ${CMAKE_CURRENT_BINARY_DIR}/lexer.cpp)

find_path(BRPC_INCLUDE_PATH NAMES brpc/server.h)
find_library(BRPC_LIB NAMES libbrpc.a brpc)
if((NOT BRPC_INCLUDE_PATH) OR (NOT BRPC_LIB))
    message(FATAL_ERROR "Fail to find brpc")
endif()
include_directories(${BRPC_INCLUDE_PATH})

find_path(BOOST_INCLUDE_PATH NAMES boost/regex.h)
find_library(BOOST_LIB NAMES libboost_regex.a)
if((NOT BOOST_INCLUDE_PATH) OR (NOT BOOST_LIB))
    message(FATAL_ERROR "Fail to find boost")
endif()
include_directories(${BOOST_INCLUDE_PATH})

find_path(GFLAGS_INCLUDE_PATH gflags/gflags.h)
find_library(GFLAGS_LIBRARY NAMES gflags libgflags)
if((NOT GFLAGS_INCLUDE_PATH) OR (NOT GFLAGS_LIBRARY))
    message(FATAL_ERROR "Fail to find gflags")
endif()
include_directories(${GFLAGS_INCLUDE_PATH})

execute_process(
    COMMAND bash -c "grep \"namespace [_A-Za-z0-9]\\+ {\" ${GFLAGS_INCLUDE_PATH}/gflags/gflags_declare.h | head -1 | awk '{print $2}' | tr -d '\n'"
    OUTPUT_VARIABLE GFLAGS_NS
)
if(${GFLAGS_NS} STREQUAL "GFLAGS_NAMESPACE")
    execute_process(
        COMMAND bash -c "grep \"#define GFLAGS_NAMESPACE [_A-Za-z0-9]\\+\" ${GFLAGS_INCLUDE_PATH}/gflags/gflags_declare.h | head -1 | awk '{print $3}' | tr -d '\n'"
        OUTPUT_VARIABLE GFLAGS_NS
    )
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    include(CheckFunctionExists)
    CHECK_FUNCTION_EXISTS(clock_gettime HAVE_CLOCK_GETTIME)
    if(NOT HAVE_CLOCK_GETTIME)
        set(DEFINE_CLOCK_GETTIME "-DNO_CLOCK_GETTIME_IN_MAC")
    endif()
endif()

if (NOT CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE "Release")
endif()
set(CMAKE_CPP_FLAGS "${DEFINE_CLOCK_GETTIME} -DGFLAGS_NS=${GFLAGS_NS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CPP_FLAGS} -D__const__= -pipe -W -Wall -Wno-unused-parameter -fPIC -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS_DEBUG "-DDEBUG -O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -O2")

if(CMAKE_VERSION VERSION_LESS "3.1.3")
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
    endif()
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
    endif()
else()
    set(CMAKE_CXX_STANDARD 14)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

find_path(LEVELDB_INCLUDE_PATH NAMES leveldb/db.h)
find_library(LEVELDB_LIB NAMES leveldb)
if ((NOT LEVELDB_INCLUDE_PATH) OR (NOT LEVELDB_LIB))
    message(FATAL_ERROR "Fail to find leveldb")
endif()
include_directories(${LEVELDB_INCLUDE_PATH})

find_library(SSL_LIB NAMES ssl)
if (NOT SSL_LIB)
    message(FATAL_ERROR "Fail to find ssl")
endif()

find_library(CURL_LIB NAMES curl)
if (NOT CURL_LIB)
    message(FATAL_ERROR "Fail to find curl")
endif()

find_library(CRYPTO_LIB NAMES crypto)
if (NOT CRYPTO_LIB)
    message(FATAL_ERROR "Fail to find crypto")
endif()

file(GLOB USKIT_SRC
    "src/*.cpp"
    "src/*/*.cpp"
    "src/*/*/*.cpp"
    )

add_executable(uskit ${USKIT_SRC} ${PROTO_SRC} ${PROTO_HEADER}
                     ${BISON_EXPR_PARSER_OUTPUTS} ${FLEX_EXPR_LEXER_OUTPUTS})

set(DYNAMIC_LIB
    ${CMAKE_THREAD_LIBS_INIT}
    ${GFLAGS_LIBRARY}
    ${PROTOBUF_LIBRARIES}
    ${LEVELDB_LIB}
    ${SSL_LIB}
    ${CURL_LIB}
    ${CRYPTO_LIB}
    dl
    )

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(DYNAMIC_LIB ${DYNAMIC_LIB}
        pthread
        "-framework CoreFoundation"
        "-framework CoreGraphics"
        "-framework CoreData"
        "-framework CoreText"
        "-framework Security"
        "-framework Foundation"
        "-Wl,-U,_MallocExtension_ReleaseFreeMemory"
        "-Wl,-U,_ProfilerStart"
        "-Wl,-U,_ProfilerStop"
        "-Wl,-U,_RegisterThriftProtocol")
endif()

target_link_libraries(uskit ${BRPC_LIB} ${BOOST_LIB} ${DYNAMIC_LIB})

add_custom_command(
    TARGET uskit POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/conf
    ${CMAKE_BINARY_DIR}/conf
    )
