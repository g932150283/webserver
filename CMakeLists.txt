cmake_minimum_required(VERSION 3.0)
project(webserver)


set(CMAKE_VERBOSE_MAKEFILE ON)
# 指定编译器的行为
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -fPIC -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")
# set(CMAKE_C_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -fPIC -ggdb -std=c11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")



set(LIB_SRC
    src/log.cc
    sylar/util.cc
    )

add_library(sylar SHARED ${LIB_SRC})


set(LIBS
        sylar
        dl
        pthread
        yaml-cpp
        jsoncpp
        ${ZLIB_LIBRARIES}
        ${OPENSSL_LIBRARIES}
        ${PROTOBUF_LIBRARIES}
        event
        hiredis_vip
        mysqlclient_r
        zookeeper_mt
        sqlite3
        tinyxml2
        jemalloc
        )

add_executable(test tests/test.cc)
add_dependencies(test sylar)
target_link_libraries(test sylar)


SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
